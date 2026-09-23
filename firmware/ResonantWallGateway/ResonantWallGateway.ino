// ResonantWallGateway — USB-serial ↔ ESP-NOW bridge for the Resonant Wall master.
//
// A PC has no ESP-NOW radio, so one extra ESP32 stays plugged into the PC and
// relays packets. Its MAC is what the motor boards need as MASTER_MAC, and its
// channel must match their ESPNOW_WIFI_CHANNEL.
//
// Line protocol, 115200 8N1, lines end in '\n' (docs/master-ui-and-gateway.md):
//   PC → GW  SEND <mac> <hex>   transmit bytes; FF:FF:FF:FF:FF:FF = broadcast
//            CHAN <1-13>        set WiFi channel (replies READY chan=<n>)
//            MAC?               reply MAC <mac>
//            PING               reply PONG
//   GW → PC  MAC <mac>, READY chan=<n>   at boot
//            ACK <mac> OK|FAIL           ESP-NOW delivery result for each SEND
//            RX <mac> <hex>              packet received from a board
//            ERR <reason>                bad line
//
// Works with Arduino-ESP32 core 2.x and 3.x (see the send-callback note below).
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <ctype.h>
#include <string.h>

#define SERIAL_BAUD      115200
#define DEFAULT_CHANNEL  1
#define MAX_PAYLOAD      ESP_NOW_MAX_DATA_LEN  // 250

// Events are produced in the WiFi task and printed from loop().
struct Event {
  enum Kind : uint8_t { ACK, RX } kind;
  bool ok;
  uint8_t mac[6];
  uint8_t len;
  uint8_t data[32];  // board packets are 6 bytes; longer ones are truncated in the log
};

static QueueHandle_t events;
static uint8_t channel = DEFAULT_CHANNEL;
static char line[600];
static size_t lineLen = 0;
static bool lineOverflow = false;

// ---------------------------------------------------------------------------
// Send callback, core-version independent.
//
// Core 2.x / 3.0–3.2 (ESP-IDF < 5.5):  void cb(const uint8_t *mac, esp_now_send_status_t)
// Core 3.3+ (ESP-IDF 5.5):             void cb(const wifi_tx_info_t *info, esp_now_send_status_t)
//
// Instead of testing version macros, SendCb is specialised on whatever
// esp_now_send_cb_t actually is, and macOf() picks the right way to get the
// destination MAC out of the first argument.
// ---------------------------------------------------------------------------
static inline const uint8_t *macOf(const uint8_t *mac) { return mac; }
template <typename Info>
static inline const uint8_t *macOf(const Info *info) { return info->des_addr; }

static void handleSent(const uint8_t *mac, esp_now_send_status_t status) {
  Event ev = {};
  ev.kind = Event::ACK;
  ev.ok = (status == ESP_NOW_SEND_SUCCESS);
  if (mac) memcpy(ev.mac, mac, 6);
  xQueueSend(events, &ev, 0);
}

template <typename Fn> struct SendCb;
template <typename First>
struct SendCb<void (*)(First, esp_now_send_status_t)> {
  static void cb(First first, esp_now_send_status_t status) { handleSent(macOf(first), status); }
};

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Event ev = {};
  ev.kind = Event::RX;
  memcpy(ev.mac, info->src_addr, 6);
  ev.len = len > (int)sizeof(ev.data) ? sizeof(ev.data) : (uint8_t)len;
  memcpy(ev.data, data, ev.len);
  xQueueSend(events, &ev, 0);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void printMac(const uint8_t mac[6]) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static bool parseMac(const char *s, uint8_t mac[6]) {
  unsigned v[6];
  char tail;
  if (sscanf(s, "%x:%x:%x:%x:%x:%x%c", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &tail) != 6) return false;
  for (int i = 0; i < 6; i++) {
    if (v[i] > 0xFF) return false;
    mac[i] = (uint8_t)v[i];
  }
  return true;
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  c = (char)toupper((unsigned char)c);
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

// Accepts "A5FF0200" or "A5 FF 02 00". Returns byte count or -1.
static int parseHex(const char *s, uint8_t *out, int max) {
  int n = 0;
  while (*s) {
    if (isspace((unsigned char)*s)) { s++; continue; }
    int hi = hexNibble(s[0]);
    int lo = s[1] ? hexNibble(s[1]) : -1;
    if (hi < 0 || lo < 0 || n >= max) return -1;
    out[n++] = (uint8_t)(hi << 4 | lo);
    s += 2;
  }
  return n;
}

static void setChannel(uint8_t ch) {
  channel = ch;
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
}

static bool ensurePeer(const uint8_t mac[6]) {
  if (esp_now_is_peer_exist(mac)) return true;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;  // follow the current channel, so CHAN needs no peer rewrite
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  return esp_now_add_peer(&peer) == ESP_OK;
}

static void printMyMac() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.print("MAC ");
  printMac(mac);
  Serial.println();
}

// ---------------------------------------------------------------------------
// Line handling
// ---------------------------------------------------------------------------
static void handleLine(char *s) {
  while (isspace((unsigned char)*s)) s++;
  char *end = s + strlen(s);
  while (end > s && isspace((unsigned char)end[-1])) *--end = '\0';
  if (!*s) return;

  if (strncmp(s, "SEND ", 5) == 0) {
    char *macStr = s + 5;
    while (*macStr == ' ') macStr++;
    char *hex = strchr(macStr, ' ');
    if (!hex) { Serial.println("ERR usage: SEND <mac> <hex>"); return; }
    *hex++ = '\0';
    uint8_t mac[6];
    uint8_t payload[MAX_PAYLOAD];
    if (!parseMac(macStr, mac)) { Serial.println("ERR bad mac"); return; }
    int n = parseHex(hex, payload, sizeof(payload));
    if (n <= 0) { Serial.println("ERR bad hex"); return; }
    if (!ensurePeer(mac)) { Serial.println("ERR add peer failed"); return; }
    if (esp_now_send(mac, payload, n) != ESP_OK) {
      // No send callback will follow, so report the failure here.
      Serial.print("ACK ");
      printMac(mac);
      Serial.println(" FAIL");
    }
  } else if (strncmp(s, "CHAN ", 5) == 0) {
    int ch = atoi(s + 5);
    if (ch < 1 || ch > 13) { Serial.println("ERR channel must be 1-13"); return; }
    setChannel((uint8_t)ch);
    Serial.printf("READY chan=%u\n", channel);
  } else if (strcmp(s, "MAC?") == 0) {
    printMyMac();
  } else if (strcmp(s, "PING") == 0) {
    Serial.println("PONG");
  } else {
    Serial.println("ERR unknown command");
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  events = xQueueCreate(64, sizeof(Event));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  setChannel(DEFAULT_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERR esp_now_init failed");
    return;
  }
  esp_now_register_send_cb(SendCb<esp_now_send_cb_t>::cb);
  esp_now_register_recv_cb(onRecv);

  printMyMac();
  Serial.printf("READY chan=%u\n", channel);
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line[lineLen] = '\0';
      if (lineOverflow) Serial.println("ERR line too long");
      else handleLine(line);
      lineLen = 0;
      lineOverflow = false;
    } else if (lineLen < sizeof(line) - 1) {
      line[lineLen++] = c;
    } else {
      lineOverflow = true;
    }
  }

  Event ev;
  while (xQueueReceive(events, &ev, 0) == pdTRUE) {
    if (ev.kind == Event::ACK) {
      Serial.print("ACK ");
      printMac(ev.mac);
      Serial.println(ev.ok ? " OK" : " FAIL");
    } else {
      Serial.print("RX ");
      printMac(ev.mac);
      Serial.print(' ');
      for (uint8_t i = 0; i < ev.len; i++) Serial.printf("%02X", ev.data[i]);
      Serial.println();
    }
  }
}
