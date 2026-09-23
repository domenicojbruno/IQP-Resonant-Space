// ResonantWallMotorBoard — peripheral firmware for one row of the Resonant Wall.
//
// ESP32-WROOM-32 driving up to 5 BTS7960 H-bridges (un-encoded linear actuators,
// one shared limit switch each). Takes 4-byte commands over ESP-NOW and reports
// a 6-byte status packet per motor every 250 ms. See:
//   docs/motor-board-peripheral-firmware.md  (pinout, transport, open items)
//   docs/master-ui-and-gateway.md            (protocol tables)
//
// Set BOARD_ID / MOTOR_COUNT / USE_OLD_BENCH_PINOUT / MASTER_MAC in config.h.
// Requires Arduino-ESP32 core 3.x.
#include <Arduino.h>
#include <Preferences.h>

#include "config.h"
#include "Protocol.h"
#include "Motor.h"
#include "CommsEspNow.h"
#include "CommsSerial.h"

static Motor motors[MOTOR_COUNT];
static Preferences prefs;
static uint32_t lastStatus = 0;
static bool espNowOk = false;

// Travel times from CAL survive reboots. Key "trav<i>", 0 = not calibrated.
static void travelKey(uint8_t i, char key[8]) { snprintf(key, 8, "trav%u", i); }

static uint32_t loadTravel(uint8_t i) {
  char key[8];
  travelKey(i, key);
  return prefs.getUInt(key, 0);
}

static void saveTravel(uint8_t i, uint32_t ms) {
  char key[8];
  travelKey(i, key);
  prefs.putUInt(key, ms);
}

// Apply a command to one motor or, for MOTOR_ALL, to every motor on this board.
static bool dispatch(uint8_t motorIdx, uint8_t cmd, uint8_t value) {
  if (cmd >= CMD_COUNT) return false;
  uint32_t now = millis();
  if (motorIdx == MOTOR_ALL) {
    bool ok = true;
    for (uint8_t i = 0; i < MOTOR_COUNT; i++) ok &= motors[i].command(cmd, value, now);
    return ok;
  }
  if (motorIdx >= MOTOR_COUNT) return false;
  return motors[motorIdx].command(cmd, value, now);
}

static void updateDriverEnable() {
#if PIN_DRV_EN >= 0
  bool any = false;
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) any |= motors[i].driving();
  digitalWrite(PIN_DRV_EN, any ? HIGH : LOW);
#endif
}

static void sendStatus() {
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    StatusPacket pkt = {STATUS_MAGIC, BOARD_ID, i, motors[i].state(), motors[i].posPct(), motors[i].fault()};
    if (espNowOk) CommsEspNow::sendStatus(pkt);
#if SERIAL_CONSOLE_ENABLED
    CommsSerial::sendStat(i, pkt.state, pkt.posPct, pkt.fault);
#endif
  }
}

void setup() {
#if PIN_DRV_EN >= 0
  pinMode(PIN_DRV_EN, OUTPUT);
  digitalWrite(PIN_DRV_EN, LOW);  // motors off until something drives
#endif

  Serial.begin(SERIAL_BAUD);
  delay(200);

  espNowOk = CommsEspNow::begin();
  char mac[18];
  CommsEspNow::macString(mac);

  Serial.printf("\n# ResonantWallMotorBoard  board=%d motors=%d pinout=%s\n", BOARD_ID, MOTOR_COUNT,
                USE_OLD_BENCH_PINOUT ? "old-bench" : "MotorDriver");
  Serial.printf("# MAC %s  channel=%d  espnow=%s\n", mac, ESPNOW_WIFI_CHANNEL, espNowOk ? "ok" : "FAILED");
  if (!CommsEspNow::masterMacSet()) {
    Serial.println("# MASTER_MAC not set: commands are accepted, no status is sent");
  }

  prefs.begin("rwall", false);

  bool uart0Free = !SERIAL_CONSOLE_ENABLED;
#if !SERIAL_CONSOLE_ENABLED
  Serial.println("# bench console disabled; releasing UART0 to the limit switches");
  Serial.flush();
  Serial.end();
#endif

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    int8_t limit = PIN_LIMIT[i];
    // GPIO1/GPIO3 are UART0 TX/RX; only usable as limit inputs with the console off.
    if ((limit == 1 || limit == 3) && !uart0Free) limit = -1;
    uint32_t travel = loadTravel(i);
    motors[i].begin(i, PIN_RPWM[i], PIN_LPWM[i], limit, (MOTOR_INVERT_MASK >> i) & 1, travel);
#if SERIAL_CONSOLE_ENABLED
    Serial.printf("# M%u travel=%lums%s limit=%s\n", i, (unsigned long)motors[i].travelMs(),
                  travel ? "" : " (default, run CAL)", limit >= 0 ? "yes" : "none");
#endif
  }
#if SERIAL_CONSOLE_ENABLED
  Serial.println("# console: <motor>,<CMD>[,<value>]  motor 0-4 or *  CMD FWD REV STOP HOME CAL GOTO");
#endif
}

void loop() {
  CommandPacket pkt;
  while (CommsEspNow::poll(pkt)) {
    dispatch(pkt.motorIdx, pkt.cmd, pkt.value);
  }

#if SERIAL_CONSOLE_ENABLED
  CommsSerial::Command c;
  while (CommsSerial::poll(c)) {
    if (dispatch(c.motorIdx, c.cmd, c.value)) CommsSerial::replyOk(c.motorIdx);
    else CommsSerial::replyErr("rejected");
  }
#endif

  uint32_t now = millis();
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    motors[i].update(now);
    if (motors[i].takeNewCalibration()) saveTravel(i, motors[i].travelMs());
  }
  updateDriverEnable();

  if (now - lastStatus >= STATUS_PERIOD_MS) {
    lastStatus = now;
    sendStatus();
  }
}
