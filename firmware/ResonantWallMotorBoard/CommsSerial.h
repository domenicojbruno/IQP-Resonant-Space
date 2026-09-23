// CommsSerial.h — USB-serial bench console (115200 8N1, lines end in '\n').
//
// Local debug only, for typing commands from the Arduino Serial Monitor (or the
// master app's "USB serial" link) without an ESP-NOW gateway:
//
//   in:  <motor>,<CMD>[,<value>]      motor = 0..MOTOR_COUNT-1 or * for all
//        e.g.  2,FWD,200   *,STOP   0,GOTO,50
//   out: OK,<motor>                   command accepted (<motor> echoes the input, * included)
//        ERR,<reason>
//        STAT,<i>,<STATE>,<pos>,<FAULT>   every STATUS_PERIOD_MS per motor, pos 255 = unknown
#pragma once

#include <stdint.h>

namespace CommsSerial {

// Parsed console command. motorIdx is MOTOR_ALL (0xFF) for '*'.
struct Command {
  uint8_t motorIdx;
  uint8_t cmd;
  uint8_t value;
};

// Reads available bytes; returns true when a complete, valid line was parsed.
// Invalid lines are answered with ERR,<reason> here.
bool poll(Command &out);

void replyOk(uint8_t motorIdx);
void replyErr(const char *reason);
void sendStat(uint8_t motorIdx, uint8_t state, uint8_t posPct, uint8_t fault);

// Parses one line (without the newline). On failure, sets *err to a reason.
// Exposed for testing.
bool parseLine(const char *line, Command &out, const char **err);

}  // namespace CommsSerial
