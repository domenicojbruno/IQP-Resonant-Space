// CommsEspNow.h — the real master link. The board is addressed by its WiFi MAC;
// the master's ESP-NOW gateway sends 4-byte command packets (unicast, or the
// broadcast MAC FF:FF:FF:FF:FF:FF for wall-wide commands) and receives 6-byte
// status packets back at MASTER_MAC.
#pragma once

#include <stdint.h>
#include "Protocol.h"

namespace CommsEspNow {

// Starts WiFi in station mode on ESPNOW_WIFI_CHANNEL and registers MASTER_MAC.
bool begin();

// Pops the next received command. Safe to call from loop(); the receive
// callback runs in the WiFi task and only queues packets.
bool poll(CommandPacket &out);

// Sends one status packet to MASTER_MAC. No-op while MASTER_MAC is unset.
void sendStatus(const StatusPacket &pkt);

bool masterMacSet();

// "xx:xx:xx:xx:xx:xx" of this board.
void macString(char out[18]);

}  // namespace CommsEspNow
