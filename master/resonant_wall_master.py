#!/usr/bin/env python3
"""Resonant Wall master control UI.

PC-side master for the three ResonantWallMotorBoard peripherals. Each board is
reached either over ESP-NOW (through a USB-connected ResonantWallGateway ESP32)
or directly over its USB-serial bench console.

    pip install pyserial
    python3 resonant_wall_master.py [--config resonant_wall_master.json]

Protocol and control decisions: docs/master-ui-and-gateway.md
Wall topology and actuation model:  docs/resonant-wall-master-spec.md
"""

import argparse
import copy
import json
import math
import os
import queue
import threading
import time

try:
    import serial
    import serial.tools.list_ports
except ImportError:  # the UI reports this when a port is opened
    serial = None

# ---------------------------------------------------------------------------
# Protocol (must match firmware/ResonantWallMotorBoard/Protocol.h)
# ---------------------------------------------------------------------------

PROTOCOL_VERSION = 2
DEFAULT_PROTOCOL = {
    "version": PROTOCOL_VERSION,
    "cmd_magic": 0xA5,
    "status_magic": 0xA6,
    "all_motors": 0xFF,
    "pos_unknown": 255,
    "cmds": {"FWD": 0, "REV": 1, "STOP": 2, "HOME": 3, "CAL": 4, "GOTO": 5},
    "states": ["UNINIT", "HOMING_CLEAR", "HOMING_SEEK", "IDLE",
               "MOVING_FWD", "MOVING_REV", "CALIBRATING", "FAULT"],
    "faults": ["NONE", "HOMING_TIMEOUT", "RUNAWAY_TIMEOUT"],
}
BROADCAST_MAC = "FF:FF:FF:FF:FF:FF"
CMDS_WITH_VALUE = ("FWD", "REV", "GOTO")


class Protocol:
    """Encodes commands and decodes status using the config's protocol section."""

    def __init__(self, spec):
        self.spec = spec
        self.cmds = spec["cmds"]
        self.cmd_names = {v: k for k, v in self.cmds.items()}
        self.all_motors = spec["all_motors"]

    def encode(self, motor_idx, cmd, value=0):
        """4-byte command packet: [magic, motorIdx, cmd, value]."""
        if cmd not in self.cmds:
            raise ValueError("unknown command %r" % cmd)
        limit = 100 if cmd == "GOTO" else 255
        value = int(value)
        if not 0 <= value <= limit:
            raise ValueError("%s value must be 0-%d" % (cmd, limit))
        return bytes([self.spec["cmd_magic"], motor_idx & 0xFF, self.cmds[cmd], value])

    def console_line(self, motor_idx, cmd, value=0):
        """Bench-console form of the same command: <motor>,<CMD>[,<value>]."""
        self.encode(motor_idx, cmd, value)  # validates
        m = "*" if motor_idx == self.all_motors else str(motor_idx)
        if cmd in CMDS_WITH_VALUE:
            return "%s,%s,%d" % (m, cmd, int(value))
        return "%s,%s" % (m, cmd)

    def decode_status(self, data):
        """6-byte status packet → dict, or None if it is not one."""
        if len(data) != 6 or data[0] != self.spec["status_magic"]:
            return None
        _, board_id, motor_idx, state, pos, fault = data
        return {
            "board": board_id,
            "motor": motor_idx,
            "state": self.state_name(state),
            "pos": None if pos == self.spec["pos_unknown"] else pos,
            "fault": self.fault_name(fault),
        }

    def state_name(self, code):
        states = self.spec["states"]
        return states[code] if 0 <= code < len(states) else "?%d" % code

    def fault_name(self, code):
        faults = self.spec["faults"]
        return faults[code] if 0 <= code < len(faults) else "?%d" % code


def normalize_mac(text):
    """'24:0a:c4:00:00:01' / '240AC4000001' / '24-0A-...' → '24:0A:C4:00:00:01', else None."""
    h = "".join(c for c in text.strip() if c not in ":-. ").upper()
    if len(h) != 12 or any(c not in "0123456789ABCDEF" for c in h):
        return None
    return ":".join(h[i:i + 2] for i in range(0, 12, 2))


def parse_gateway_line(line):
    """Gateway → PC line → (kind, fields) or None."""
    parts = line.strip().split()
    if not parts:
        return None
    kind = parts[0]
    if kind == "MAC" and len(parts) == 2:
        return "MAC", {"mac": normalize_mac(parts[1])}
    if kind == "READY":
        chan = None
        for p in parts[1:]:
            if p.startswith("chan="):
                try:
                    chan = int(p[5:])
                except ValueError:
                    pass
        return "READY", {"channel": chan}
    if kind == "ACK" and len(parts) == 3:
        return "ACK", {"mac": normalize_mac(parts[1]), "ok": parts[2] == "OK"}
    if kind == "RX" and len(parts) == 3:
        try:
            data = bytes.fromhex(parts[2])
        except ValueError:
            return None
        return "RX", {"mac": normalize_mac(parts[1]), "data": data}
    if kind == "PONG":
        return "PONG", {}
    if kind == "ERR":
        return "ERR", {"reason": line.strip()[4:]}
    return "TEXT", {"text": line.strip()}


def parse_console_line(line):
    """Board bench console → PC line → (kind, fields) or None."""
    line = line.strip()
    if not line:
        return None
    if line.startswith("#"):
        fields = {"text": line}
        parts = line.lstrip("# ").split()
        if len(parts) >= 2 and parts[0] == "MAC":
            fields["mac"] = normalize_mac(parts[1])
        return "INFO", fields
    parts = [p.strip() for p in line.split(",")]
    if parts[0] == "OK" and len(parts) == 2:
        return "OK", {"motor": parts[1]}
    if parts[0] == "ERR":
        return "ERR", {"reason": ",".join(parts[1:])}
    if parts[0] == "STAT" and len(parts) == 5:
        try:
            motor, pos = int(parts[1]), int(parts[3])
        except ValueError:
            return None
        return "STAT", {"motor": motor, "state": parts[2],
                        "pos": None if pos == 255 else pos, "fault": parts[4]}
    return "TEXT", {"text": line}


# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

DEFAULT_CONFIG = {
    "gateway": {"port": "", "channel": 1},
    "boards": [
        {"id": 0, "name": "Board A", "motors": 5, "link": "espnow", "mac": "", "port": ""},
        {"id": 1, "name": "Board B", "motors": 3, "link": "espnow", "mac": "", "port": ""},
        {"id": 2, "name": "Board C", "motors": 5, "link": "espnow", "mac": "", "port": ""},
    ],
    "control": {
        "mode": "manual",              # manual | threshold | goto
        "sigma": 135,                  # reactivity radius (sim units)
        "smoothing": 0.15,             # response speed, first-order slew per tick
        "tick_ms": 33,
        "retract_threshold": 0.12,
        "extend_threshold": 0.85,
        "min_drive_pwm": 120,
        "goto_deadband_pct": 3,
        "goto_min_interval_ms": 100,
    },
    "protocol": DEFAULT_PROTOCOL,
}


def merge_defaults(cfg, defaults):
    """Fill keys missing from cfg with defaults (recursively for dicts)."""
    out = copy.deepcopy(defaults)
    for k, v in cfg.items():
        if isinstance(v, dict) and isinstance(out.get(k), dict):
            out[k] = merge_defaults(v, out[k])
        else:
            out[k] = v
    return out


def load_config(path):
    """Load the JSON config. Returns (config, notes). Missing file → defaults.

    A config saved by the first version of this app carries placeholder
    protocol codes; any protocol section whose version is not current is
    replaced with the firmware's values.
    """
    notes = []
    if not os.path.exists(path):
        return copy.deepcopy(DEFAULT_CONFIG), ["No config at %s, using defaults" % path]
    with open(path) as f:
        raw = json.load(f)
    old_version = raw.get("protocol", {}).get("version")
    cfg = merge_defaults(raw, DEFAULT_CONFIG)
    if old_version != PROTOCOL_VERSION:
        cfg["protocol"] = copy.deepcopy(DEFAULT_PROTOCOL)
        notes.append("Protocol section in config was version %s; replaced with firmware protocol v%d"
                     % (old_version, PROTOCOL_VERSION))
    return cfg, notes


def save_config(path, cfg):
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        json.dump(cfg, f, indent=2)
    os.replace(tmp, path)


# ---------------------------------------------------------------------------
# Wall geometry and control (docs/resonant-wall-master-spec.md §2, §4)
# ---------------------------------------------------------------------------

R = 44
COL = 1.5 * R
HALF = math.sqrt(3) * R / 2
FULL = math.sqrt(3) * R


def build_motors():
    """13 motors in the flat-top honeycomb, sim coordinates, origin at M1,1."""
    motors = []
    for i in range(5):  # Board A: cols 1,3,5 high, 2,4 dropped half a hex
        motors.append({"board": 0, "row": 1, "col": i + 1, "x": i * COL, "y": 0 if i % 2 == 0 else HALF})
    for j, c in enumerate((0, 2, 4)):  # Board B: under A's high hexes
        motors.append({"board": 1, "row": 2, "col": j + 1, "x": c * COL, "y": FULL})
    for k in range(5):  # Board C: mirrors A
        motors.append({"board": 2, "row": 3, "col": k + 1, "x": k * COL,
                       "y": 2 * FULL if k % 2 == 0 else 1.5 * FULL})
    for m in motors:
        m["id"] = "M%d,%d" % (m["row"], m["col"])
        m["idx"] = m["col"] - 1  # motor index on its board
    return motors


class WallController:
    """Turns a tracked point into per-motor ext targets and commands.

    step() returns a list of (board_id, motor_idx, cmd, value) to send.
    """

    def __init__(self, control):
        self.control = control
        self.motors = build_motors()
        for m in self.motors:
            m["ext"] = 1.0
            m["latched"] = "EXTENDED"   # threshold-mode hysteresis state
            m["goto_sent"] = None       # last GOTO pct sent
            m["goto_time"] = 0.0

    def reset_streams(self):
        """Forget what was sent, so the next step re-sends from scratch."""
        for m in self.motors:
            m["goto_sent"] = None
            m["goto_time"] = 0.0
            m["latched"] = "EXTENDED" if m["ext"] > self.control["retract_threshold"] else "RETRACTED"

    def step(self, point, occupied, now=None):
        now = time.monotonic() if now is None else now
        c = self.control
        sigma = float(c["sigma"])
        smoothing = float(c["smoothing"])
        mode = c["mode"]
        out = []
        for m in self.motors:
            target = 1.0
            if occupied and point is not None:
                d2 = (point[0] - m["x"]) ** 2 + (point[1] - m["y"]) ** 2
                target = 1.0 - math.exp(-d2 / (2 * sigma * sigma))
            m["ext"] += (target - m["ext"]) * smoothing
            m["ext"] = min(1.0, max(0.0, m["ext"]))
            ext = m["ext"]
            retract = 1.0 - ext

            if mode == "threshold":
                floor = int(c["min_drive_pwm"])
                if ext <= c["retract_threshold"] and m["latched"] != "RETRACTED":
                    m["latched"] = "RETRACTED"
                    out.append((m["board"], m["idx"], "REV", max(floor, round(retract * 255))))
                elif ext >= c["extend_threshold"] and m["latched"] == "RETRACTED":
                    m["latched"] = "EXTENDED"
                    out.append((m["board"], m["idx"], "FWD", max(floor, round(ext * 255))))
            elif mode == "goto":
                pct = int(round(ext * 100))
                last = m["goto_sent"]
                due = now - m["goto_time"] >= c["goto_min_interval_ms"] / 1000.0
                if due and (last is None or abs(pct - last) >= c["goto_deadband_pct"]):
                    m["goto_sent"] = pct
                    m["goto_time"] = now
                    out.append((m["board"], m["idx"], "GOTO", pct))
        return out


# ---------------------------------------------------------------------------
# Serial links (reader threads push events into one queue for the UI)
# ---------------------------------------------------------------------------

class SerialLink:
    """Line-oriented serial port with a reader thread.

    Events put on `events`: (source, "line", text) and (source, "closed", reason).
    """

    def __init__(self, source, port, events, baud=115200):
        if serial is None:
            raise RuntimeError("pyserial is not installed (pip install pyserial)")
        self.source = source
        self.events = events
        self.ser = serial.Serial(port, baud, timeout=0.1)
        self.lock = threading.Lock()
        self.alive = True
        self.thread = threading.Thread(target=self._reader, daemon=True)
        self.thread.start()

    def _reader(self):
        buf = b""
        while self.alive:
            try:
                chunk = self.ser.read(256)
            except Exception as e:  # port unplugged
                self.alive = False
                self.events.put((self.source, "closed", str(e)))
                return
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                self.events.put((self.source, "line", raw.decode("utf-8", "replace").strip()))

    def write_line(self, text):
        with self.lock:
            self.ser.write((text + "\n").encode())

    def close(self):
        self.alive = False
        try:
            self.ser.flush()
        except Exception:
            pass
        self.thread.join(timeout=0.5)
        try:
            self.ser.close()
        except Exception:
            pass


class Master:
    """Everything except the Tk widgets: links, sending, delivery tracking, status.

    UI callbacks (set by the app, all optional):
      on_command(entry)           a command was sent; entry["delivery"] is "…"
      on_delivery(entry)          entry["delivery"] became "✓" or "✗"
      on_status(board_id, motor)  a status row changed
      on_event(text)              something worth a line in the event log
      on_gateway()                gateway connection/MAC/channel changed
    """

    STATUS_STALE_S = 2.0

    def __init__(self, cfg):
        self.cfg = cfg
        self.proto = Protocol(cfg["protocol"])
        self.events = queue.Queue()
        self.gateway = None
        self.gateway_mac = None
        self.gateway_channel = None
        self.board_links = {}          # board id → SerialLink (USB-serial boards)
        self.pending = {}              # delivery key → list of command entries (FIFO)
        self.status = {b["id"]: {} for b in cfg["boards"]}
        self.last_rx = {b["id"]: 0.0 for b in cfg["boards"]}
        self.last_ack_ok = {b["id"]: 0.0 for b in cfg["boards"]}
        self.seq = 0
        self.on_command = self.on_delivery = self.on_status = self.on_event = self.on_gateway = None

    # --- helpers -----------------------------------------------------------
    def board(self, board_id):
        for b in self.cfg["boards"]:
            if b["id"] == board_id:
                return b
        return None

    def board_by_mac(self, mac):
        for b in self.cfg["boards"]:
            if b.get("mac") and normalize_mac(b["mac"]) == mac:
                return b
        return None

    def _emit(self, name, *args):
        cb = getattr(self, name)
        if cb:
            cb(*args)

    def event(self, text):
        self._emit("on_event", text)

    # --- connections -------------------------------------------------------
    def open_gateway(self, port):
        self.close_gateway()
        self.gateway = SerialLink("gateway", port, self.events)
        self.cfg["gateway"]["port"] = port
        self.gateway.write_line("MAC?")
        self.set_channel(self.cfg["gateway"]["channel"])
        self.event("Gateway opened on %s" % port)
        self._emit("on_gateway")

    def close_gateway(self):
        if self.gateway:
            self.gateway.close()
            self.gateway = None
            self.gateway_mac = None
            self._emit("on_gateway")

    def set_channel(self, chan):
        self.cfg["gateway"]["channel"] = int(chan)
        if self.gateway:
            self.gateway.write_line("CHAN %d" % int(chan))

    def open_board_serial(self, board_id, port):
        self.close_board_serial(board_id)
        self.board_links[board_id] = SerialLink(("board", board_id), port, self.events)
        self.board(board_id)["port"] = port
        self.event("%s: USB serial opened on %s" % (self.board(board_id)["name"], port))

    def close_board_serial(self, board_id):
        link = self.board_links.pop(board_id, None)
        if link:
            link.close()

    def close_all(self):
        for bid in list(self.board_links):
            self.close_board_serial(bid)
        self.close_gateway()

    # --- sending -----------------------------------------------------------
    def reachable(self, board_id):
        """True if the board's selected link is up (and it has a MAC, for ESP-NOW)."""
        b = self.board(board_id)
        if b["link"] == "serial":
            return board_id in self.board_links
        return bool(self.gateway and normalize_mac(b.get("mac", "")))

    def send(self, board_id, motor_idx, cmd, value=0):
        """Send one command to one board over its selected link. Returns the entry or None."""
        b = self.board(board_id)
        self.seq += 1
        entry = {"seq": self.seq, "time": time.time(), "board": b["name"], "board_id": board_id,
                 "motor": "*" if motor_idx == self.proto.all_motors else motor_idx,
                 "cmd": cmd, "value": value if cmd in CMDS_WITH_VALUE else None,
                 "via": b["link"], "delivery": "…"}
        try:
            if b["link"] == "serial":
                link = self.board_links.get(board_id)
                if not link:
                    raise RuntimeError("USB serial not open")
                line = self.proto.console_line(motor_idx, cmd, value)
                key = ("serial", board_id, str(entry["motor"]))
                link.write_line(line)
            else:
                mac = normalize_mac(b.get("mac", ""))
                if not mac:
                    raise RuntimeError("no MAC set")
                if not self.gateway:
                    raise RuntimeError("gateway not connected")
                pkt = self.proto.encode(motor_idx, cmd, value)
                key = ("mac", mac)
                self.gateway.write_line("SEND %s %s" % (mac, pkt.hex().upper()))
        except Exception as e:
            entry["delivery"] = "✗"
            entry["error"] = str(e)
            self._emit("on_command", entry)
            return None
        self.pending.setdefault(key, []).append(entry)
        self._emit("on_command", entry)
        return entry

    def send_broadcast(self, motor_idx, cmd, value=0):
        if not self.gateway:
            return None
        self.seq += 1
        entry = {"seq": self.seq, "time": time.time(), "board": "broadcast", "board_id": None,
                 "motor": "*", "cmd": cmd, "value": None, "via": "espnow", "delivery": "…"}
        pkt = self.proto.encode(motor_idx, cmd, value)
        self.gateway.write_line("SEND %s %s" % (BROADCAST_MAC, pkt.hex().upper()))
        self.pending.setdefault(("mac", BROADCAST_MAC), []).append(entry)
        self._emit("on_command", entry)
        return entry

    def estop(self):
        """STOP all motors on every board: broadcast first, then each board on its own link."""
        allm = self.proto.all_motors
        self.send_broadcast(allm, "STOP")
        for b in self.cfg["boards"]:
            if self.reachable(b["id"]):
                self.send(b["id"], allm, "STOP")
        self.event("E-STOP sent")

    # --- receiving ---------------------------------------------------------
    def _resolve(self, key, ok):
        q = self.pending.get(key)
        if not q:
            return
        entry = q.pop(0)
        entry["delivery"] = "✓" if ok else "✗"
        if ok and entry["board_id"] is not None:
            self.last_ack_ok[entry["board_id"]] = time.monotonic()
        self._emit("on_delivery", entry)

    def _update_status(self, board_id, motor, state, pos, fault):
        rows = self.status.setdefault(board_id, {})
        prev = rows.get(motor)
        rows[motor] = {"state": state, "pos": pos, "fault": fault, "time": time.monotonic()}
        self.last_rx[board_id] = time.monotonic()
        # Log only transitions: the stream is 4 packets/s per motor.
        if prev is None or prev["state"] != state or prev["fault"] != fault:
            name = self.board(board_id)["name"] if self.board(board_id) else "board %d" % board_id
            text = "%s M%d: %s" % (name, motor, state)
            if fault != "NONE":
                text += " (fault %s)" % fault
            if prev is not None:
                self.event(text)
        self._emit("on_status", board_id, motor)

    def process_events(self, limit=500):
        """Drain reader-thread events. Call from the UI thread."""
        for _ in range(limit):
            try:
                source, kind, payload = self.events.get_nowait()
            except queue.Empty:
                return
            if kind == "closed":
                self.event("%s closed: %s" % (source, payload))
                if source == "gateway":
                    self.gateway = None
                    self._emit("on_gateway")
                else:
                    self.board_links.pop(source[1], None)
                continue
            if source == "gateway":
                self._gateway_line(payload)
            else:
                self._console_line(source[1], payload)

    def _gateway_line(self, line):
        parsed = parse_gateway_line(line)
        if not parsed:
            return
        kind, f = parsed
        if kind == "MAC":
            self.gateway_mac = f["mac"]
            self.event("Gateway MAC %s (set as MASTER_MAC in each board's config.h)" % f["mac"])
            self._emit("on_gateway")
        elif kind == "READY":
            self.gateway_channel = f["channel"]
            self._emit("on_gateway")
        elif kind == "ACK":
            self._resolve(("mac", f["mac"]), f["ok"])
        elif kind == "RX":
            st = self.proto.decode_status(f["data"])
            if not st:
                return
            b = self.board_by_mac(f["mac"])
            if b is None:
                b = self.board(st["board"])
                if b is None:
                    return
                if not normalize_mac(b.get("mac", "")):
                    b["mac"] = f["mac"]
                    self.event("%s MAC learned from status: %s" % (b["name"], f["mac"]))
                elif b["link"] == "espnow":
                    self.event("Status from unknown MAC %s claims board %d" % (f["mac"], st["board"]))
                    return
            self._update_status(b["id"], st["motor"], st["state"], st["pos"], st["fault"])
        elif kind == "ERR":
            self.event("Gateway error: %s" % f["reason"])

    def _console_line(self, board_id, line):
        parsed = parse_console_line(line)
        if not parsed:
            return
        kind, f = parsed
        b = self.board(board_id)
        if kind == "OK":
            self._resolve(("serial", board_id, f["motor"]), True)
        elif kind == "ERR":
            # The console does not echo the motor on ERR; fail the oldest pending command.
            keys = [k for k, q in self.pending.items() if k[0] == "serial" and k[1] == board_id and q]
            if keys:
                self._resolve(min(keys, key=lambda k: self.pending[k][0]["seq"]), False)
            self.event("%s: ERR %s" % (b["name"], f["reason"]))
        elif kind == "STAT":
            self._update_status(board_id, f["motor"], f["state"], f["pos"], f["fault"])
        elif kind == "INFO":
            if f.get("mac") and not normalize_mac(b.get("mac", "")):
                b["mac"] = f["mac"]
                self.event("%s MAC detected over USB: %s" % (b["name"], f["mac"]))
            self.event("%s: %s" % (b["name"], f["text"]))

    def link_health(self, board_id):
        """Short text for the board's link indicator."""
        b = self.board(board_id)
        now = time.monotonic()
        fresh = now - self.last_rx[board_id] < self.STATUS_STALE_S
        if b["link"] == "serial":
            if board_id not in self.board_links:
                return "USB serial not open"
            return "Status OK" if fresh else "No status"
        if not normalize_mac(b.get("mac", "")):
            return "No MAC set"
        if not self.gateway:
            return "Gateway not connected"
        if fresh:
            return "Status OK"
        if now - self.last_ack_ok[board_id] < 10.0:
            # Commands are delivered but no status comes back: MASTER_MAC is not set
            # (or wrong) on the board.
            return "Gets commands, sends no status"
        return "No status"

    def shutdown(self):
        """STOP everything, then close ports."""
        try:
            self.estop()
            time.sleep(0.2)
        finally:
            self.close_all()


# ---------------------------------------------------------------------------
# Tk UI
# ---------------------------------------------------------------------------

def list_ports():
    if serial is None:
        return []
    return [p.device for p in serial.tools.list_ports.comports()]


def ext_color(ext):
    """White (extended) → red (retracted), as in the simulator."""
    t = 1.0 - ext
    white, red = (246, 244, 239), (255, 53, 53)
    return "#%02x%02x%02x" % tuple(int(round(w + (r - w) * t)) for w, r in zip(white, red))


def run_app(config_path):
    import tkinter as tk
    from tkinter import messagebox, ttk

    cfg, notes = load_config(config_path)
    master = Master(cfg)
    control = cfg["control"]
    wall = WallController(control)

    root = tk.Tk()
    root.title("Resonant Wall — Master")
    root.geometry("1400x940")

    # ---- top bar ------------------------------------------------------------
    top = ttk.Frame(root, padding=6)
    top.pack(side="top", fill="x")

    ttk.Label(top, text="Gateway port").pack(side="left")
    gw_port = tk.StringVar(value=cfg["gateway"]["port"])
    gw_combo = ttk.Combobox(top, textvariable=gw_port, width=16, values=list_ports(),
                            postcommand=lambda: gw_combo.configure(values=list_ports()))
    gw_combo.pack(side="left", padx=4)

    def connect_gateway():
        try:
            master.open_gateway(gw_port.get())
        except Exception as e:
            messagebox.showerror("Gateway", str(e))

    ttk.Button(top, text="Connect", command=connect_gateway).pack(side="left")
    ttk.Label(top, text="  Channel").pack(side="left")
    chan_var = tk.IntVar(value=cfg["gateway"]["channel"])
    ttk.Spinbox(top, from_=1, to=13, width=4, textvariable=chan_var).pack(side="left", padx=4)
    ttk.Button(top, text="Set", command=lambda: master.set_channel(chan_var.get())).pack(side="left")
    gw_label = ttk.Label(top, text="Gateway: not connected", width=44)
    gw_label.pack(side="left", padx=10)

    estop_btn = tk.Button(top, text="E-STOP", bg="#c62828", fg="white", activebackground="#8e0000",
                          font=("TkDefaultFont", 12, "bold"), padx=16)
    estop_btn.pack(side="right")

    # ---- control bar --------------------------------------------------------
    ctl = ttk.Frame(root, padding=(6, 0))
    ctl.pack(side="top", fill="x")
    mode_var = tk.StringVar(value=control["mode"])
    ttk.Label(ctl, text="Mode").pack(side="left")
    for label, val in (("Manual", "manual"), ("Threshold (FWD/REV)", "threshold"), ("GOTO stream", "goto")):
        ttk.Radiobutton(ctl, text=label, value=val, variable=mode_var).pack(side="left", padx=3)
    occupied_var = tk.BooleanVar(value=True)
    ttk.Checkbutton(ctl, text="Hallway occupied", variable=occupied_var).pack(side="left", padx=12)

    def slider(label, key, lo, hi, res, fmt):
        ttk.Label(ctl, text=label).pack(side="left")
        var = tk.DoubleVar(value=control[key])
        val = ttk.Label(ctl, width=5, text=fmt % control[key])

        def changed(_=None):
            v = round(var.get() / res) * res
            control[key] = int(v) if res >= 1 else v
            val.configure(text=fmt % control[key])

        ttk.Scale(ctl, from_=lo, to=hi, variable=var, command=changed, length=110).pack(side="left")
        val.pack(side="left", padx=(0, 10))

    slider("Radius", "sigma", 60, 220, 1, "%d")
    slider("Speed", "smoothing", 0.05, 0.40, 0.01, "%.2f")
    slider("Min PWM", "min_drive_pwm", 0, 255, 1, "%d")

    def mode_changed(*_):
        control["mode"] = mode_var.get()
        wall.reset_streams()
        master.event("Mode: %s" % control["mode"])

    mode_var.trace_add("write", mode_changed)

    # ---- main area ----------------------------------------------------------
    body = ttk.PanedWindow(root, orient="horizontal")
    body.pack(fill="both", expand=True, padx=6, pady=6)

    # wall canvas
    wall_frame = ttk.LabelFrame(body, text="Wall (move the cursor = tracked hand)", padding=4)
    body.add(wall_frame, weight=1)
    canvas = tk.Canvas(wall_frame, width=520, height=440, bg="#1b1f23", highlightthickness=0)
    canvas.pack(fill="both", expand=True)
    view = {"scale": 1.3, "ox": 80.0, "oy": 70.0}  # sim units → canvas pixels
    pointer = {"pt": None}
    hex_items = {}
    for m in wall.motors:
        poly = canvas.create_polygon(0, 0, 0, 0, fill=ext_color(1), outline="#3a4045", width=1.5)
        label = canvas.create_text(0, 0, text=m["id"], fill="#1a1a1a", font=("TkDefaultFont", 9, "bold"))
        stat = canvas.create_text(0, 0, text="—", fill="#333333", font=("TkDefaultFont", 8))
        hex_items[(m["board"], m["idx"])] = (poly, label, stat)

    def layout(e):
        # Fit the honeycomb (x -R..264+R, y -HALF..2*FULL+HALF) into the canvas.
        span_x, span_y = 4 * COL + 2 * R, 2 * FULL + 2 * HALF
        scale = min((e.width - 20) / span_x, (e.height - 20) / span_y)
        view.update(scale=scale, ox=(e.width - span_x * scale) / 2 + R * scale,
                    oy=(e.height - span_y * scale) / 2 + HALF * scale)
        for m in wall.motors:
            poly, label, stat = hex_items[(m["board"], m["idx"])]
            cx, cy = view["ox"] + m["x"] * scale, view["oy"] + m["y"] * scale
            r = (R - 3) * scale
            pts = []
            for i in range(6):
                a = math.radians(60 * i)
                pts += [cx + r * math.cos(a), cy + r * math.sin(a)]
            canvas.coords(poly, *pts)
            canvas.coords(label, cx, cy - 8)
            canvas.coords(stat, cx, cy + 10)

    canvas.bind("<Configure>", layout)

    def on_move(e):
        pointer["pt"] = ((e.x - view["ox"]) / view["scale"], (e.y - view["oy"]) / view["scale"])

    canvas.bind("<Motion>", on_move)
    canvas.bind("<Leave>", lambda e: pointer.update(pt=None))

    # boards
    right = ttk.Frame(body)
    body.add(right, weight=1)
    board_ui = {}

    def board_panel(b):
        bid = b["id"]
        lf = ttk.LabelFrame(right, text="%s  (BOARD_ID %d, %d motors)" % (b["name"], bid, b["motors"]), padding=4)
        lf.pack(fill="x", pady=3)

        row = ttk.Frame(lf)
        row.pack(fill="x")
        link_var = tk.StringVar(value=b["link"])

        def link_changed(*_):
            b["link"] = link_var.get()

        link_var.trace_add("write", link_changed)
        ttk.Radiobutton(row, text="ESP-NOW (MAC)", value="espnow", variable=link_var).pack(side="left")
        mac_var = tk.StringVar(value=b.get("mac", ""))

        def mac_changed(*_):
            b["mac"] = mac_var.get().strip()

        mac_var.trace_add("write", mac_changed)
        ttk.Entry(row, textvariable=mac_var, width=19).pack(side="left", padx=(2, 10))
        ttk.Radiobutton(row, text="USB serial", value="serial", variable=link_var).pack(side="left")
        port_var = tk.StringVar(value=b.get("port", ""))
        pc = ttk.Combobox(row, textvariable=port_var, width=12, values=list_ports())
        pc.configure(postcommand=lambda: pc.configure(values=list_ports()))
        pc.pack(side="left", padx=2)

        def open_port():
            try:
                master.open_board_serial(bid, port_var.get())
            except Exception as e:
                messagebox.showerror(b["name"], str(e))

        ttk.Button(row, text="Open", command=open_port, width=5).pack(side="left")
        health = ttk.Label(row, text="", foreground="#b26a00")
        health.pack(side="left", padx=8)

        tree = ttk.Treeview(lf, columns=("motor", "state", "pos", "fault", "target"), show="headings",
                            height=b["motors"], selectmode="browse")
        for col, w in (("motor", 70), ("state", 120), ("pos", 60), ("fault", 130), ("target", 70)):
            tree.heading(col, text=col.capitalize())
            tree.column(col, width=w, anchor="center")
        for i in range(b["motors"]):
            tree.insert("", "end", iid=str(i), values=("M%d,%d" % (bid + 1, i + 1), "—", "—", "—", "—"))
        tree.pack(fill="x", pady=3)

        btns = ttk.Frame(lf)
        btns.pack(fill="x")
        value_var = tk.IntVar(value=200)

        def do(cmd, all_motors):
            if all_motors:
                idx = master.proto.all_motors
            else:
                sel = tree.selection()
                if not sel:
                    messagebox.showinfo(b["name"], "Select a motor row first")
                    return
                idx = int(sel[0])
            v = value_var.get() if cmd in CMDS_WITH_VALUE else 0
            if cmd == "GOTO":
                v = max(0, min(100, v))
            master.send(bid, idx, cmd, v)

        ttk.Label(btns, text="Value").grid(row=0, column=0, rowspan=2)
        ttk.Spinbox(btns, from_=0, to=255, width=5, textvariable=value_var).grid(row=0, column=1, rowspan=2,
                                                                              padx=(2, 10))
        for r, (scope, all_m) in enumerate((("Selected motor", False), ("All motors", True))):
            ttk.Label(btns, text=scope, width=14).grid(row=r, column=2, sticky="w")
            for c, cmd in enumerate(("FWD", "REV", "STOP", "HOME", "CAL", "GOTO")):
                ttk.Button(btns, text=cmd, width=6,
                           command=lambda c=cmd, a=all_m: do(c, a)).grid(row=r, column=3 + c)
        board_ui[bid] = {"tree": tree, "health": health, "mac_var": mac_var}

    for b in cfg["boards"]:
        board_panel(b)

    # ---- logs ---------------------------------------------------------------
    logs = ttk.PanedWindow(root, orient="horizontal")
    logs.pack(fill="both", expand=False, padx=6, pady=(0, 6))
    cmd_frame = ttk.LabelFrame(logs, text="Commands", padding=4)
    logs.add(cmd_frame, weight=1)
    cmd_tree = ttk.Treeview(cmd_frame, columns=("time", "board", "motor", "cmd", "value", "via", "ok"),
                            show="headings", height=7)
    for col, w in (("time", 80), ("board", 80), ("motor", 50), ("cmd", 60), ("value", 50),
                   ("via", 60), ("ok", 40)):
        cmd_tree.heading(col, text=col.capitalize() if col != "ok" else "✓")
        cmd_tree.column(col, width=w, anchor="center")
    cmd_tree.pack(fill="both", expand=True)
    ev_frame = ttk.LabelFrame(logs, text="Events", padding=4)
    logs.add(ev_frame, weight=1)
    ev_text = tk.Text(ev_frame, height=8, width=60, state="disabled", bg="#101316", fg="#d6d6d6")
    ev_text.pack(fill="both", expand=True)

    MAX_ROWS = 400

    def fmt_time(t):
        return time.strftime("%H:%M:%S", time.localtime(t)) + ".%d" % int((t % 1) * 10)

    def on_command(entry):
        motor = entry["motor"]
        cmd_tree.insert("", "end", iid=str(entry["seq"]), values=(
            fmt_time(entry["time"]), entry["board"], motor, entry["cmd"],
            "" if entry["value"] is None else entry["value"], entry["via"], entry["delivery"]))
        kids = cmd_tree.get_children()
        if len(kids) > MAX_ROWS:
            cmd_tree.delete(*kids[:len(kids) - MAX_ROWS])
        cmd_tree.see(str(entry["seq"]))
        if entry.get("error"):
            on_event("%s %s failed: %s" % (entry["board"], entry["cmd"], entry["error"]))

    def on_delivery(entry):
        if cmd_tree.exists(str(entry["seq"])):
            cmd_tree.set(str(entry["seq"]), "ok", entry["delivery"])

    def on_event(text):
        ev_text.configure(state="normal")
        ev_text.insert("end", "[%s] %s\n" % (fmt_time(time.time()), text))
        lines = int(ev_text.index("end-1c").split(".")[0])
        if lines > MAX_ROWS:
            ev_text.delete("1.0", "%d.0" % (lines - MAX_ROWS))
        ev_text.see("end")
        ev_text.configure(state="disabled")

    def on_status(board_id, motor):
        ui = board_ui.get(board_id)
        st = master.status[board_id].get(motor)
        if not ui or not st or not ui["tree"].exists(str(motor)):
            return
        pos = "?" if st["pos"] is None else "%d%%" % st["pos"]
        vals = list(ui["tree"].item(str(motor), "values"))
        vals[1:4] = [st["state"], pos, st["fault"]]
        ui["tree"].item(str(motor), values=vals)
        items = hex_items.get((board_id, motor))
        if items:
            canvas.itemconfigure(items[2], text="%s %s" % (st["state"][:9], pos))

    def on_gateway():
        if not master.gateway:
            gw_label.configure(text="Gateway: not connected")
        else:
            gw_label.configure(text="Gateway: %s  chan %s" % (master.gateway_mac or "?", master.gateway_channel or "?"))

    master.on_command, master.on_delivery = on_command, on_delivery
    master.on_event, master.on_status, master.on_gateway = on_event, on_status, on_gateway

    def estop():
        mode_var.set("manual")
        master.estop()

    estop_btn.configure(command=estop)
    root.bind("<Escape>", lambda e: estop())

    for n in notes:
        on_event(n)
    on_event("Esc = E-STOP. Set each board's MASTER_MAC to the gateway MAC shown above.")

    # ---- loops --------------------------------------------------------------
    def poll():
        master.process_events()
        root.after(20, poll)

    def tick():
        cmds = wall.step(pointer["pt"], occupied_var.get())
        for bid, idx, cmd, value in cmds:
            b = master.board(bid)
            # Unreachable boards are skipped here, or streaming would flood the log with failures.
            if b and idx < b["motors"] and master.reachable(bid):
                master.send(bid, idx, cmd, value)
        for m in wall.motors:
            poly, label, _ = hex_items[(m["board"], m["idx"])]
            canvas.itemconfigure(poly, fill=ext_color(m["ext"]))
            canvas.itemconfigure(label, fill="#1a1a1a" if m["ext"] > 0.55 else "#fff2f2")
            ui = board_ui.get(m["board"])
            if ui and ui["tree"].exists(str(m["idx"])):
                ui["tree"].set(str(m["idx"]), "target", "%d%%" % round(m["ext"] * 100))
        root.after(int(control["tick_ms"]), tick)

    def health():
        for b in cfg["boards"]:
            ui = board_ui[b["id"]]
            text = master.link_health(b["id"])
            ui["health"].configure(text=text, foreground="#2e7d32" if text == "Status OK" else "#b26a00")
            if ui["mac_var"].get().strip() != b.get("mac", ""):
                ui["mac_var"].set(b.get("mac", ""))  # MAC learned from a status packet / USB banner
        root.after(500, health)

    def on_close():
        try:
            master.shutdown()
        finally:
            try:
                save_config(config_path, cfg)
            except OSError as e:
                print("could not save config:", e)
            root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    if cfg["gateway"]["port"] and cfg["gateway"]["port"] in list_ports():
        root.after(100, connect_gateway)
    poll()
    tick()
    health()
    root.mainloop()


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--config", default=os.path.join(here, "resonant_wall_master.json"),
                    help="JSON config file (created on exit if missing)")
    args = ap.parse_args()
    run_app(args.config)


if __name__ == "__main__":
    main()
