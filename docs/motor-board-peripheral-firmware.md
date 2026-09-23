# Motor Driver Board — Peripheral Firmware Notes

> Companion doc for `ResonantWallMotorBoard` (Arduino sketch, source in
> [`firmware/ResonantWallMotorBoard/`](../firmware/ResonantWallMotorBoard/)). Captures the pinout as read off the
> schematic, the protocol decisions, and open items, so this stays in sync
> with `resonant-wall-master-spec.md` and `vl53l0x-arduino-uno-test.md`.

## What it is

Firmware for the ESP32-WROOM-32 on `MotorDriver Board.kicad_sch`: drives up
to 5 un-encoded BTS7960-controlled linear actuators, reads their (shared-pin)
limit switches, and takes motion commands over **ESP-NOW** (MAC-address
addressed — corrected from an earlier serial-based draft, see "Communication
transport" below). One image flashed to all three boards, with
`BOARD_ID`/`MOTOR_COUNT` set per board (0/5, 1/3, 2/5 for row A/B/C, matching
the addressing in the master spec).

The sketch also carries pinout + control code for an **older prototype
board** currently on the bench (RPWM/LPWM only, no limit switches, no
`DRV_EN`) behind a one-line toggle (`USE_OLD_BENCH_PINOUT` in `config.h`),
so it's usable for bench testing before the real MotorDriver Board is built.

## Communication transport: ESP-NOW, not serial

⚠️ **Note for anyone reading `resonant-wall-master-spec.md` §5 or this
project's description field** ("ESP32 will receive communication through
serial"): that was superseded. The firmware now uses **ESP-NOW**, addressed
by each ESP32's WiFi MAC address — no wiring or shared bus between the
master and the peripherals, no `board` field needed in commands (the MAC
*is* the address). If the project description gets updated to match, this
is the reason why.

- Command packet (master → board), 4 bytes: `magic, motorIdx, cmd, value`.
- Status packet (board → master), 6 bytes: `magic, boardId, motorIdx, state, posPct, fault`.
- Wall-wide commands (e-stop) go to the ESP-NOW broadcast MAC
  (`FF:FF:FF:FF:FF:FF`); every powered board receives those automatically.
- Each board needs the master's MAC in `config.h` (`MASTER_MAC`) to send
  status back; the master needs each board's MAC (printed over USB serial
  at boot) to send it commands.
- A USB-serial bench console (same command verbs, ASCII) still exists
  purely for typing commands from the Arduino Serial Monitor without a
  master present — that's local debug only, not the real link.
  `SERIAL_CONSOLE_ENABLED` in `config.h` switches it on (default: on for the
  old bench board, off for the MotorDriver Board). With it off, the board
  prints its boot banner and MAC, then hands GPIO1/GPIO3 to L1/L0. With it
  on, M0/M1 on the MotorDriver Board run without limit switches.

## Pinout — read from the schematic, not yet verified against real hardware

The board hasn't been ordered, so this is transcribed from a rendered PDF
export of the KiCad schematic. **Treat it as a first draft** — verify against
`MotorDriver Board.kicad_pcb` / the physical board before wiring.

| Signal | GPIO | Note |
|---|---|---|
| DRV_EN (shared R_EN+L_EN, all 5 BTS7960) | 2 | pulled down 10k → motors OFF by default at boot |
| M0 RPWM / LPWM | 12 / 13 | |
| M1 RPWM / LPWM | 26 / 25 | |
| M2 RPWM / LPWM | 32 / 27 | |
| M3 RPWM / LPWM | 18 / 19 | |
| M4 RPWM / LPWM | 22 / 23 | |
| L0 limit (M0, shared ext+retract) | 3 (RX0) | ⚠ shares USB-serial UART0 pin, see below |
| L1 limit (M1) | 1 (TX0) | ⚠ shares USB-serial UART0 pin, see below |
| L2 limit (M2) | 21 | |
| L3 limit (M3) | 14 | |
| L4 limit (M4) | 15 | |
| IR0–IR4 (phased out, unused) | 36, 39, 34, 35, 5 | input-only-capable pins (VP/VN/D34/D35), sensible choice since IR is input-only |
| LED strip data (not used yet) | 4 | |

**Flag:** L0/L1 appear wired to GPIO3/GPIO1 (RX0/TX0), the same pins the USB
serial chip uses for flashing and the bench console. Since the real master
link is ESP-NOW (no UART involved), this only matters for bench testing:
don't type console commands, or leave heavy debug printing running, while
motors are live on this board. Worth moving L0/L1 to different GPIOs on a
future board revision to remove even that restriction.

`R_IS`/`L_IS` (BTS7960 current-sense outputs) are not wired to the ESP32 —
no electrical stall/overcurrent feedback is possible on this board revision.

### Older prototype board (bench, in hand now)

No `DRV_EN` net (R_EN/L_EN assumed hardwired to 3.3V) and no limit switches.
Motor numbering is M1–M5 (silkscreen) rather than M0–M4:

| Signal | GPIO |
|---|---|
| M1 RPWM / LPWM | 13 / 27 |
| M2 RPWM / LPWM | 26 / 25 |
| M3 RPWM / LPWM | 33 / 32 |
| M4 RPWM / LPWM | 18 / 19 |
| M5 RPWM / LPWM | 22 / 23 |

Active in the sketch today via `USE_OLD_BENCH_PINOUT 1` in `config.h`.

### Limit switches

Each actuator has one switch that closes at both ends of travel. The firmware
reads it as switch → GND with the ESP32's internal pull-up (active LOW, 15 ms
debounce), following the node controller design sheet. A move that starts on a
pressed switch ignores it until it releases. HOME backs off a pressed switch
(extend first, then retract if it stays pressed), then retracts until the
switch trips, which sets position 0 %.

## Position without encoders

No position feedback exists, so position is time-based: elapsed drive time
÷ that motor's measured full-travel time. `CAL` measures it per motor (home
→ drive to extend limit → record elapsed ms) and persists it to flash, since
travel time is expected to vary motor to motor and wasn't known up front.
Every drive has a runaway timeout (1.5× travel time) that stops the motor
and raises a fault if no limit switch trips in time. (On the old bench board,
`HOME`/`CAL`/`GOTO` will always time out into `FAULT` since there's no limit
switch to find — expected there, not a bug; use `FWD`/`REV`/`STOP` directly.)

## Open items

- Pin table for the new board needs verification once it exists.
- L0/L1 on GPIO1/3 — fine for ESP-NOW, only affects the bench console; worth
  fixing on a respin.
- IR sensors and LED strip: pins reserved, no driver code yet.
- No current-sense / stall detection (R_IS, L_IS unused).
- M0 RPWM is on GPIO12 (MTDI strapping pin). If anything pulls it high at
  reset, the ESP32 selects 1.8 V flash and fails to boot (see the node
  controller design sheet). The firmware drives it low as soon as it starts,
  but check the board's pull-downs before fabrication.
- No firmware has been compiled against a real ESP32 core yet. It has only
  been compiled on a host with stub headers and run against a simulated
  actuator.
- `MASTER_MAC` / `ESPNOW_WIFI_CHANNEL` need real values once a master device
  exists.
- Camera-based hand tracking and its mapping to per-motor targets remain
  entirely on the master side; this board only ever receives the resulting
  `FWD`/`REV`/`GOTO` commands.
