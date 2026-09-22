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
| [`docs/resonant-wall-master-spec.md`](docs/resonant-wall-master-spec.md) | Wall topology, motor addressing, actuation model and command semantics |
| [`docs/motor-board-peripheral-firmware.md`](docs/motor-board-peripheral-firmware.md) | Motor board firmware notes: ESP-NOW transport, pinout, open items |
| [`docs/master-ui-and-gateway.md`](docs/master-ui-and-gateway.md) | PC master app and ESP-NOW gateway: packet formats, line protocol, control logic |
| [`docs/vl53l0x-arduino-uno-test.md`](docs/vl53l0x-arduino-uno-test.md) | VL53L0X range test on an Arduino Uno: wiring, findings, ESP32 porting notes |
| [`docs/images/TOFpins.png`](docs/images/TOFpins.png) | VL53L0X breakout pinout |
| [`hardware/motor-driver-board/`](hardware/motor-driver-board/) | KiCad PCB (`.kicad_pcb`) and schematic PDF for the motor driver board |
| [`datasheets/`](datasheets/) | ESP32-WROOM-32, VL53L0X, BTS7960 and Arduino Uno (A000066) datasheets |

## Protocol summary

| Packet | Bytes |
|---|---|
| Command (master → board) | `[0xA5, motorIdx, cmd, value]`, where `motorIdx 0xFF` means all motors |
| Status (board → master, every 250 ms) | `[0xA6, boardId, motorIdx, state, posPct, fault]` |

Command codes: FWD 0 · REV 1 · STOP 2 · HOME 3 · CAL 4 · GOTO 5. See [`docs/master-ui-and-gateway.md`](docs/master-ui-and-gateway.md) for the full tables.

## Status

- The motor driver board has been designed in KiCad but not yet fabricated. Its pinout is transcribed from the schematic and still needs to be verified against hardware.
- The VL53L0X has been bench-tested on an Arduino Uno.
- Firmware and master app source (`ResonantWallMotorBoard`, `resonant_wall_master.py`, `ResonantWallGateway.ino`, `vl53l0x_distance_test.ino`) are not in this repo yet.
