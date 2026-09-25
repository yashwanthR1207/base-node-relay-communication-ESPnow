#include <WiFi.h>
#include <esp_now.h>

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

uint8_t relayMAC[] = {
  0x2C, 0xF4, 0x32, 0x30, 0xCD, 0xD2
};

void setup()
{
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  Serial.print("ESP32 #2 MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("WiFi channel: ");
  Serial.println(WiFi.channel());

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW INIT FAILED");
    return;
  }

  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, relayMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("FAILED TO ADD RELAY");
    return;
  }

  Serial.println("ESP32 #2 READY");
}

void loop()
{
  MeshPacket packet;
  packet.source = 2;
  packet.destination = 1;
  packet.type = 1;
  packet.priority = 0; // 0 for Critical Priority Alert
  packet.hopCount = 1;
  packet.ttl = 10;
  strcpy(packet.event, "FIRE");

  esp_err_t result = esp_now_send(
    relayMAC,
    (uint8_t *)&packet,
    sizeof(packet)
  );

  Serial.print("SEND RESULT: ");

  if (result == ESP_OK)
    Serial.println("OK");
  else
    Serial.println("FAILED");

  delay(2000);
}
