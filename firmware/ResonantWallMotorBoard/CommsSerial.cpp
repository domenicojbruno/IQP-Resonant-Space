// CommsSerial.cpp — see CommsSerial.h.
#include "CommsSerial.h"

#include <Arduino.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "Protocol.h"
#include "config.h"

namespace CommsSerial {

static char buf[48];
static size_t bufLen = 0;
static bool overflow = false;

static char *trim(char *s) {
  while (isspace((unsigned char)*s)) s++;
  char *e = s + strlen(s);
  while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
  return s;
}

static bool parseUint(const char *s, long maxVal, long &out) {
  if (!*s) return false;
  char *end;
  long v = strtol(s, &end, 10);
  if (*end || v < 0 || v > maxVal) return false;
  out = v;
  return true;
}

bool parseLine(const char *lineIn, Command &out, const char **err) {
  char line[sizeof(buf)];
  strncpy(line, lineIn, sizeof(line) - 1);
  line[sizeof(line) - 1] = '\0';

  char *fields[4];
  int n = 0;
  char *p = line;
  fields[n++] = p;
  while ((p = strchr(p, ',')) != nullptr) {
    *p++ = '\0';
    if (n == 4) { *err = "too_many_fields"; return false; }
    fields[n++] = p;
  }
  for (int i = 0; i < n; i++) fields[i] = trim(fields[i]);
  if (n < 2) { *err = "usage:<motor>,<CMD>[,<value>]"; return false; }

  long v;
  if (strcmp(fields[0], "*") == 0) {
    out.motorIdx = MOTOR_ALL;
  } else if (parseUint(fields[0], MOTOR_COUNT - 1, v)) {
    out.motorIdx = (uint8_t)v;
  } else {
    *err = "bad_motor";
    return false;
  }

  for (char *c = fields[1]; *c; c++) *c = toupper((unsigned char)*c);
  int cmd = -1;
  for (int i = 0; i < CMD_COUNT; i++) {
    if (strcmp(fields[1], CMD_NAMES[i]) == 0) cmd = i;
  }
  if (cmd < 0) { *err = "bad_cmd"; return false; }
  out.cmd = (uint8_t)cmd;
  out.value = 0;

  bool needsValue = (cmd == CMD_FWD || cmd == CMD_REV || cmd == CMD_GOTO);
  if (needsValue) {
    if (n < 3) { *err = "missing_value"; return false; }
    if (!parseUint(fields[2], cmd == CMD_GOTO ? 100 : 255, v)) { *err = "bad_value"; return false; }
    out.value = (uint8_t)v;
  } else if (n > 2 && fields[2][0] != '\0') {
    // Value is ignored for STOP/HOME/CAL, but must still be a number.
    if (!parseUint(fields[2], 255, v)) { *err = "bad_value"; return false; }
  }
  return true;
}

bool poll(Command &out) {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (bufLen < sizeof(buf) - 1) buf[bufLen++] = c;
      else overflow = true;
      continue;
    }
    buf[bufLen] = '\0';
    bool wasOverflow = overflow;
    bufLen = 0;
    overflow = false;
    char *line = trim(buf);
    if (!*line) continue;
    if (wasOverflow) { replyErr("line_too_long"); continue; }
    const char *err = nullptr;
    if (parseLine(line, out, &err)) return true;
    replyErr(err);
  }
  return false;
}

void replyOk(uint8_t motorIdx) {
  if (motorIdx == MOTOR_ALL) Serial.println("OK,*");
  else Serial.printf("OK,%u\n", motorIdx);
}

void replyErr(const char *reason) {
  Serial.printf("ERR,%s\n", reason);
}

void sendStat(uint8_t motorIdx, uint8_t state, uint8_t posPct, uint8_t fault) {
  Serial.printf("STAT,%u,%s,%u,%s\n", motorIdx,
                state <= ST_FAULT ? STATE_NAMES[state] : "?", posPct,
                fault <= FAULT_RUNAWAY_TIMEOUT ? FAULT_NAMES[fault] : "?");
}

}  // namespace CommsSerial
