// config.h — per-board settings for ResonantWallMotorBoard.
//
// One image is flashed to all three boards. Before flashing, set BOARD_ID and
// MOTOR_COUNT for the row the board drives (see docs/resonant-wall-master-spec.md §3):
//
//   Board A (top row)    BOARD_ID 0, MOTOR_COUNT 5
//   Board B (middle row) BOARD_ID 1, MOTOR_COUNT 3
//   Board C (bottom row) BOARD_ID 2, MOTOR_COUNT 5
//
// Requires Arduino-ESP32 core 3.x (esp_now_recv_info_t callback, ledcAttach).
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Board identity
// ---------------------------------------------------------------------------
#define BOARD_ID     0
#define MOTOR_COUNT  5

// 1 = older prototype board on the bench (RPWM/LPWM only, no limit switches,
//     no DRV_EN). 0 = MotorDriver Board (KiCad design in hardware/).
#define USE_OLD_BENCH_PINOUT 1

// ---------------------------------------------------------------------------
// ESP-NOW link
// ---------------------------------------------------------------------------
// MAC of the master's ESP-NOW gateway (it prints "MAC xx:xx:..." at boot).
// All zeros = not set yet: the board still takes commands but sends no status.
static const uint8_t MASTER_MAC[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Must equal the gateway's channel (gateway default 1, "CHAN <n>" changes it).
#define ESPNOW_WIFI_CHANNEL 1

// Status packet period, one packet per motor.
#define STATUS_PERIOD_MS 250

// ---------------------------------------------------------------------------
// USB-serial bench console (115200 8N1)
// ---------------------------------------------------------------------------
// Local debug only. On the MotorDriver Board, L0/L1 sit on GPIO3/GPIO1 (RX0/TX0),
// so the console and those two limit switches cannot both be used. With the
// console enabled on that board, M0/M1 run without limit switches (HOME/CAL/GOTO
// on them will time out into FAULT). With it disabled, the board prints its
// boot banner and MAC, then releases UART0 to the limit switches.
#define SERIAL_CONSOLE_ENABLED USE_OLD_BENCH_PINOUT
#define SERIAL_BAUD 115200

// ---------------------------------------------------------------------------
// Motor drive
// ---------------------------------------------------------------------------
#define PWM_FREQ_HZ        20000   // above audible range; BTS7960 is fine up to 25 kHz
#define PWM_RES_BITS       8       // duty 0-255, matches the protocol value range
#define HOME_PWM           255     // drive strength for HOME / CAL / GOTO
#define LIMIT_DEBOUNCE_MS  15      // switch must read the same for this long

// Travel time used before a motor has been calibrated with CAL.
#define DEFAULT_TRAVEL_MS  10000
// Runaway timeout = travel time × this factor. No limit trip within that → FAULT.
#define RUNAWAY_FACTOR     1.5f
// Upper bound for the homing and extend phases of CAL (travel time not known yet).
#define CAL_TIMEOUT_MS     60000
// HOMING_CLEAR: how long to drive off a pressed switch before trying the other way.
#define HOMING_CLEAR_MS    1500
// GOTO requests closer than this to the current estimate are ignored.
#define GOTO_DEADBAND_PCT  2

// Bit n set = swap RPWM/LPWM for motor n (use if an actuator is wired reversed).
// Convention: FWD = extend = RPWM, REV = retract = LPWM.
#define MOTOR_INVERT_MASK  0x00

// ---------------------------------------------------------------------------
// Pinout — transcribed from the schematic PDF, NOT yet verified against a
// fabricated board. See docs/motor-board-peripheral-firmware.md.
// -1 = not connected.
// ---------------------------------------------------------------------------
#if USE_OLD_BENCH_PINOUT
// Older prototype board. Silkscreen M1–M5 = motor index 0–4 here.
#define PIN_DRV_EN -1   // R_EN/L_EN hardwired to 3.3 V on this board
static const int8_t PIN_RPWM[5]  = {13, 26, 33, 18, 22};
static const int8_t PIN_LPWM[5]  = {27, 25, 32, 19, 23};
static const int8_t PIN_LIMIT[5] = {-1, -1, -1, -1, -1};
#else
// MotorDriver Board. DRV_EN is shared by all five BTS7960 (10k pull-down → off at boot).
#define PIN_DRV_EN 2
static const int8_t PIN_RPWM[5]  = {12, 26, 32, 18, 22};
static const int8_t PIN_LPWM[5]  = {13, 25, 27, 19, 23};
// One switch per actuator, shared by both ends. Switch → GND, internal pull-up,
// active LOW. L0/L1 share UART0 with the USB-serial chip (see above).
static const int8_t PIN_LIMIT[5] = {3, 1, 21, 14, 15};
#endif

// Reserved on the MotorDriver Board, no driver code yet:
//   IR0–IR4 (phased out): 36, 39, 34, 35, 5
//   LED strip data:       4

static_assert(MOTOR_COUNT >= 1 && MOTOR_COUNT <= 5, "MOTOR_COUNT must be 1-5");
static_assert(BOARD_ID >= 0 && BOARD_ID <= 2, "BOARD_ID must be 0-2");
