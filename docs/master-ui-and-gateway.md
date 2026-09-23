# Master Control UI + ESP-NOW Gateway

> Companion to `resonant-wall-master-spec.md` and `motor-board-peripheral-firmware.md`.
> Covers the PC-side master app [`master/resonant_wall_master.py`](../master/resonant_wall_master.py) (Python/Tkinter + pyserial) and the
> USB↔ESP-NOW gateway sketch [`firmware/ResonantWallGateway/ResonantWallGateway.ino`](../firmware/ResonantWallGateway/ResonantWallGateway.ino).
> The protocol values below match the `ResonantWallMotorBoard` firmware source (Protocol.h, CommsEspNow.cpp, CommsSerial.cpp).

## Architecture
- A PC has no ESP-NOW radio, so **one extra ESP32 (the gateway)** stays plugged into the PC over USB.
- Each board's link is chosen separately in the UI:
  - **ESP-NOW (MAC)**: PC → gateway (USB serial) → ESP-NOW unicast to the board's MAC.
  - **USB serial**: PC → the board's bench console. This is for bench use only, because L0/L1 share UART0 on the new board.
- The gateway MAC is the board's **`MASTER_MAC`**. Until that is set, the boards receive commands but send no status. The UI flags this case as "Gets commands, sends no status".
- The gateway WiFi channel must equal `ESPNOW_WIFI_CHANNEL`, which defaults to 1.

## Protocol (from the firmware)
| Item | Value |
|---|---|
| Command packet (4 B) | `[0xA5, motorIdx, cmd, value]`. `motorIdx 0xFF` = all motors on that board |
| cmd codes | FWD 0 · REV 1 · STOP 2 · HOME 3 · CAL 4 · GOTO 5 |
| value | FWD/REV = PWM 0-255. GOTO = 0-100 % (0 = retracted, 100 = extended). Ignored for the others |
| Status packet (6 B) | `[0xA6, boardId, motorIdx, state, posPct, fault]`, sent every 250 ms for every motor |
| state (MotorState) | 0 UNINIT · 1 HOMING_CLEAR · 2 HOMING_SEEK · 3 IDLE · 4 MOVING_FWD · 5 MOVING_REV · 6 CALIBRATING · 7 FAULT |
| fault (FaultCode) | 0 NONE · 1 HOMING_TIMEOUT · 2 RUNAWAY_TIMEOUT. posPct 255 = unknown |
| Bench console | in: `<motor>,<CMD>[,<value>]` (`*` = all). out: `OK,<i>` · `ERR,<reason>` · `STAT,<i>,<STATE>,<pos>,<FAULT>` |
| E-STOP | There is no ESTOP command. The master sends `A5 FF 02 00` (STOP, all motors) to the broadcast MAC, then unicast to each board. USB-serial boards get `*,STOP` |

## Gateway line protocol (115200 8N1, `\n`)
| Direction | Line | Meaning |
|---|---|---|
| PC → GW | `SEND <mac> <hex>` | transmit bytes. `FF:FF:FF:FF:FF:FF` = broadcast |
| PC → GW | `CHAN <1-13>` / `MAC?` / `PING` | set channel / query MAC / liveness check |
| GW → PC | `MAC <mac>`, `READY chan=<n>` | sent at boot |
| GW → PC | `ACK <mac> OK\|FAIL` | ESP-NOW delivery result for each SEND |
| GW → PC | `RX <mac> <hex>` | packet from a board (status) |

## Master logic decisions (these settle open questions from the spec's §5)
- **Threshold mode** (spec §4): REV when ext ≤ 0.12, FWD when ext ≥ 0.85. REV PWM = round(retract·255) and FWD PWM = round(ext·255), both floored at "min drive PWM" (default 120). FWD/REV run until a limit switch trips or the runaway timeout (1.5× travel) is reached.
- **GOTO streaming**: pct = ext·100. It is rate-limited per motor with a deadband of 3 % and a minimum interval of 100 ms. The firmware refuses a GOTO while the motor's position is unknown and runs HOME instead. So on the bench board (no limit switches) GOTO ends in HOMING_TIMEOUT → FAULT, and threshold mode should be used there.
- Motor index = col − 1. The "All motors" buttons send one command with idx 0xFF (or `*` over USB serial).
- Status handling: the tables update on every status packet. The logs only record a change of state or fault, to avoid 52 lines per second.
- Delivery ✓ comes from the gateway `ACK` for ESP-NOW commands, and from the board's `OK,<i>` for USB-serial commands.
- Closing the app sends STOP-all to every board.
- A saved config written by the first version (which used placeholder codes) has its protocol section replaced automatically. This is tracked with `protocol.version`, currently 2.

## Status
- The master app was tested end to end against fake boards and a fake gateway on virtual serial ports. The fakes reproduce the firmware's packets, console replies and 250 ms status stream. The tests covered MAC detection, ACK/FAIL, OK matching for `*` commands, status decoding, threshold and GOTO modes, E-STOP, STOP-all on close, and migration of an old config. The Tk UI was also run under Xvfb.
- Neither sketch has been **compiled against a real ESP32 core**, because no toolchain was available. Both were compiled with a host compiler against stub Arduino/ESP-IDF headers. The motor board state machine (HOME, CAL, GOTO, FWD/REV to limit, runaway and homing timeouts, bench console) was run against a simulated actuator. The gateway's send callback was checked against both the pre-5.5 and the ESP-IDF 5.5 callback signatures.
- The board firmware requires Arduino-ESP32 core 3.x (`esp_now_recv_info_t` callback, `ledcAttach`). The gateway builds on core 2.x or 3.x.
