// Protocol.h — wire format shared with the master (master/resonant_wall_master.py).
// Tables in docs/master-ui-and-gateway.md must stay in sync with this file.
#pragma once

#include <stdint.h>

static const uint8_t CMD_MAGIC    = 0xA5;
static const uint8_t STATUS_MAGIC = 0xA6;
static const uint8_t MOTOR_ALL    = 0xFF;
static const uint8_t POS_UNKNOWN  = 255;

// Command packet (master → board), 4 bytes: [0xA5, motorIdx, cmd, value]
//   FWD/REV: value = PWM 0-255
//   GOTO:    value = 0-100 % (0 = retracted, 100 = extended)
//   others:  value ignored
enum Cmd : uint8_t {
  CMD_FWD  = 0,
  CMD_REV  = 1,
  CMD_STOP = 2,
  CMD_HOME = 3,
  CMD_CAL  = 4,
  CMD_GOTO = 5,
  CMD_COUNT
};

// Status packet (board → master), 6 bytes: [0xA6, boardId, motorIdx, state, posPct, fault]
enum MotorState : uint8_t {
  ST_UNINIT       = 0,  // never homed, position unknown
  ST_HOMING_CLEAR = 1,  // backing off a switch that was already pressed
  ST_HOMING_SEEK  = 2,  // retracting until the switch trips
  ST_IDLE         = 3,
  ST_MOVING_FWD   = 4,
  ST_MOVING_REV   = 5,
  ST_CALIBRATING  = 6,
  ST_FAULT        = 7,
};

enum FaultCode : uint8_t {
  FAULT_NONE            = 0,
  FAULT_HOMING_TIMEOUT  = 1,
  FAULT_RUNAWAY_TIMEOUT = 2,
};

struct __attribute__((packed)) CommandPacket {
  uint8_t magic;
  uint8_t motorIdx;
  uint8_t cmd;
  uint8_t value;
};

struct __attribute__((packed)) StatusPacket {
  uint8_t magic;
  uint8_t boardId;
  uint8_t motorIdx;
  uint8_t state;
  uint8_t posPct;
  uint8_t fault;
};

static_assert(sizeof(CommandPacket) == 4, "command packet must be 4 bytes");
static_assert(sizeof(StatusPacket) == 6, "status packet must be 6 bytes");

// Names used by the bench console (CommsSerial.cpp).
static const char *const CMD_NAMES[CMD_COUNT] = {"FWD", "REV", "STOP", "HOME", "CAL", "GOTO"};
static const char *const STATE_NAMES[] = {"UNINIT", "HOMING_CLEAR", "HOMING_SEEK", "IDLE",
                                          "MOVING_FWD", "MOVING_REV", "CALIBRATING", "FAULT"};
static const char *const FAULT_NAMES[] = {"NONE", "HOMING_TIMEOUT", "RUNAWAY_TIMEOUT"};
