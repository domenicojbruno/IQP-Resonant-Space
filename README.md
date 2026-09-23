# IQP-Resonant-Space

An interactive "resonant wall" for a hallway. VL53L0X time-of-flight sensors detect when people cross a threshold, and a honeycomb of 13 hexagonal panels, each moved by a linear actuator, pulls back from and pushes out toward them. Each actuator is driven by a BTS7960 H-bridge, and three ESP32-WROOM-32 boards each drive one row of the wall.

## System at a glance

- **Wall:** 13 motors in a flat-top hex honeycomb: 5 on top (Board A), 3 in the middle (Board B) and 5 on the bottom (Board C).
- **Motor boards:** ESP32-WROOM-32 + up to 5 BTS7960 H-bridges each, with limit switches per actuator. Position is time-based, since the actuators have no encoders.
- **Sensing:** 4× VL53L0X ToF sensors for hallway occupancy. Two overhead cameras for hand tracking are planned.
- **Master:** a PC app (Python/Tkinter) talks to the boards over **ESP-NOW** through a USB-connected ESP32 gateway. This replaced the original serial-link plan.

## Repository layout

| Path | Contents |
|---|---|
| [`firmware/ResonantWallMotorBoard/`](firmware/ResonantWallMotorBoard/) | Motor board sketch (one image for all three boards): BTS7960 drive, limit switches, time-based position, HOME/CAL/GOTO, ESP-NOW link and USB bench console |
| [`firmware/ResonantWallGateway/`](firmware/ResonantWallGateway/) | USB-serial ↔ ESP-NOW gateway sketch for the ESP32 that stays plugged into the PC |
| [`master/`](master/) | PC master app `resonant_wall_master.py` (Python/Tkinter + pyserial): wall view, threshold and GOTO-stream control, per-board links, E-STOP |
| [`docs/resonant-wall-master-spec.md`](docs/resonant-wall-master-spec.md) | Wall topology, motor addressing, actuation model and command semantics |
| [`docs/motor-board-peripheral-firmware.md`](docs/motor-board-peripheral-firmware.md) | Motor board firmware notes: ESP-NOW transport, pinout, open items |
| [`docs/master-ui-and-gateway.md`](docs/master-ui-and-gateway.md) | PC master app and ESP-NOW gateway: packet formats, line protocol, control logic |
| [`docs/vl53l0x-arduino-uno-test.md`](docs/vl53l0x-arduino-uno-test.md) | VL53L0X range test on an Arduino Uno: wiring, findings, ESP32 porting notes |
| [`docs/hexagon-node-controller.html`](docs/hexagon-node-controller.html) | Earlier node controller design sheet (Rev B): power, pin map, H-bridge wiring, sensors, BOM, layout rules |
| [`simulator/index.html`](simulator/index.html) | Resonant Wall master-node simulator. Open it in a browser and move the cursor over the hex field to see per-motor commands |
| [`docs/images/TOFpins.png`](docs/images/TOFpins.png) | VL53L0X breakout pinout |
| [`hardware/motor-driver-board/`](hardware/motor-driver-board/) | KiCad PCB (`.kicad_pcb`) and schematic PDF for the motor driver board |
| [`datasheets/`](datasheets/) | ESP32-WROOM-32, VL53L0X, BTS7960 and Arduino Uno (A000066) datasheets |

## Getting started

1. **Gateway.** Flash `firmware/ResonantWallGateway/` to a spare ESP32 and plug it into the PC. It prints `MAC <mac>` at boot.
2. **Motor boards.** In `firmware/ResonantWallMotorBoard/config.h`, set `BOARD_ID` / `MOTOR_COUNT` (0/5, 1/3, 2/5 for rows A/B/C), `USE_OLD_BENCH_PINOUT`, and `MASTER_MAC` (the gateway's MAC). Then flash each board. Each board prints its own MAC over USB at boot. The board needs Arduino-ESP32 core 3.x; the gateway builds on core 2.x or 3.x.
3. **Master.** `pip install -r master/requirements.txt`, then `python3 master/resonant_wall_master.py`. Choose the gateway port and enter each board's MAC (the app also learns a MAC from the board's first status packet). Pick a mode: Manual, Threshold (FWD/REV) or GOTO stream. Esc is E-STOP. Settings are saved to `master/resonant_wall_master.json` on exit.
4. Run `CAL` once per motor (MotorDriver Board only; it needs limit switches) so time-based positions and GOTO use the measured travel time.

## Protocol summary

| Packet | Bytes |
|---|---|
| Command (master → board) | `[0xA5, motorIdx, cmd, value]`, where `motorIdx 0xFF` means all motors |
| Status (board → master, every 250 ms) | `[0xA6, boardId, motorIdx, state, posPct, fault]` |

Command codes: FWD 0 · REV 1 · STOP 2 · HOME 3 · CAL 4 · GOTO 5. See [`docs/master-ui-and-gateway.md`](docs/master-ui-and-gateway.md) for the full tables.

## Status

- The motor driver board has been designed in KiCad but not yet fabricated. Its pinout is transcribed from the schematic and still needs to be verified against hardware.
- The VL53L0X has been bench-tested on an Arduino Uno.
- The master-node simulator (`simulator/`) runs as-is in any browser.
- The firmware and master app source is now in `firmware/` and `master/`, written from the specs in `docs/`. Neither sketch has been compiled against a real ESP32 core or run on hardware yet. Both were compiled on a host with stub headers. The motor board state machine was exercised against a simulated actuator, and the master app was tested against fake boards and a fake gateway on virtual serial ports.
- `vl53l0x_distance_test.ino` (the Uno sensor test) is not in this repo yet, and ToF occupancy is not wired into the master. The "Hallway occupied" toggle in the app stands in for it.
