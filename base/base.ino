#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define WIFI_CHANNEL 1

// =====================================================
// PACKET STRUCTURE
// =====================================================

struct MeshPacket
{
  uint8_t source;
  uint8_t destination;
  uint8_t type;
  uint8_t priority;
  uint8_t hopCount;
  uint8_t ttl;
  char event[20];
};

// =====================================================
// HELPER: ADD PEER DYNAMICALLY
// =====================================================
void addPeerIfNeeded(const uint8_t *mac)
{
  if (!esp_now_is_peer_exist(mac))
  {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = WIFI_CHANNEL;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
      Serial.println("ADDED RELAY MAC AS PEER");
    } else {
      Serial.println("FAILED TO ADD PEER");
    }
  }
}

// =====================================================
// RECEIVE CALLBACK
// =====================================================

void onDataRecv(const uint8_t *mac, const uint8_t *data, int len)
{
  if (len != sizeof(MeshPacket))
  {
    Serial.println();
    Serial.println("INVALID PACKET SIZE");
    return;
  }

  MeshPacket packet;
  memcpy(&packet, data, sizeof(MeshPacket));

  // If this is an ACK (Type 2), drop it (Base shouldn't receive ACKs)
  if (packet.type == 2) return;

  Serial.println();
  Serial.println("========================================");
  Serial.println("          EMERGENCY ALERT");
  Serial.println("========================================");

  Serial.print("SOURCE       : ");
  Serial.println(packet.source);

  Serial.print("DESTINATION  : ");
  Serial.println(packet.destination);

  Serial.print("TYPE         : ");
  Serial.println(packet.type);

  Serial.print("PRIORITY     : P");
  Serial.println(packet.priority);

  Serial.print("EVENT        : ");
  Serial.println(packet.event);

  Serial.print("HOPS         : ");
  Serial.println(packet.hopCount);

  Serial.print("TTL          : ");
  Serial.println(packet.ttl);

  Serial.println("----------------------------------------");

  if (packet.priority == 0)
  {
    Serial.println("CRITICAL PRIORITY ALERT");
  }

  if (strcmp(packet.event, "FIRE") == 0)
  {
    Serial.println("FIRE EMERGENCY DETECTED");
  }

  Serial.println("----------------------------------------");
  Serial.println("ALERT RECEIVED AT BASE");
  
  // =================================================
  // SEND ACK BACK TO HAZARD NODE (VIA RELAY)
  // =================================================

  Serial.println("SENDING ACKNOWLEDGEMENT...");

  // Dynamically add the relay MAC as a peer if it hasn't been added yet
  addPeerIfNeeded(mac);

  MeshPacket ackPacket;
  ackPacket.source = 1; // Base ID
  ackPacket.destination = packet.source; // Destination is the original sender (Hazard)
  ackPacket.type = 2; // Type 2 = ACK
  ackPacket.priority = packet.priority;
  ackPacket.hopCount = 0;
  ackPacket.ttl = 5;
  strcpy(ackPacket.event, "ACK");

  // We send the ACK back to the MAC address that delivered the packet (the Relay)
  esp_err_t result = esp_now_send(mac, (uint8_t *)&ackPacket, sizeof(ackPacket));

  if (result == ESP_OK) {
    Serial.println("ACK DELIVERED TO RELAY");
  } else {
    Serial.println("FAILED TO SEND ACK");
  }

  Serial.println("========================================");
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.println();
  Serial.println("========================================");
  Serial.println("          BASE STATION STARTING");
  Serial.println("========================================");

  Serial.print("BASE MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0)
  {
    Serial.println("ESP-NOW INIT FAILED");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
  Serial.println("ESP-NOW INITIALIZED");
  Serial.println("BASE STATION READY");
  Serial.println("========================================");
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  // Base Station continuously waits for packets
}
