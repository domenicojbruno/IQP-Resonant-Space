// CommsEspNow.cpp — see CommsEspNow.h.
#include "CommsEspNow.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

#include "config.h"

namespace CommsEspNow {

static QueueHandle_t rxQueue = nullptr;
static bool masterSet = false;

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  if (len != (int)sizeof(CommandPacket) || data[0] != CMD_MAGIC) return;
  CommandPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));
  xQueueSend(rxQueue, &pkt, 0);  // drop if full rather than block the WiFi task
}

bool begin() {
  rxQueue = xQueueCreate(16, sizeof(CommandPacket));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onRecv);

  static const uint8_t zero[6] = {0};
  masterSet = memcmp(MASTER_MAC, zero, 6) != 0;
  if (masterSet) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, MASTER_MAC, 6);
    peer.channel = 0;  // follow the current channel
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) masterSet = false;
  }
  return true;
}

bool poll(CommandPacket &out) {
  return rxQueue && xQueueReceive(rxQueue, &out, 0) == pdTRUE;
}

void sendStatus(const StatusPacket &pkt) {
  if (!masterSet) return;
  esp_now_send(MASTER_MAC, (const uint8_t *)&pkt, sizeof(pkt));
}

bool masterMacSet() { return masterSet; }

void macString(char out[18]) {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

}  // namespace CommsEspNow
