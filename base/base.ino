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

void onDataRecv(const uint8_t *mac, const uint8_t *data, int len)
{
  if (len != sizeof(MeshPacket))
  {
    Serial.println();
    Serial.println("INVALID PACKET SIZE");

    Serial.print("Received: ");
    Serial.print(len);

    Serial.print(" bytes | Expected: ");
    Serial.print(sizeof(MeshPacket));

    Serial.println(" bytes");
    return;
  }

  MeshPacket packet;

  memcpy(&packet, data, sizeof(MeshPacket));

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
  Serial.println("========================================");
}

void setup()
{
  Serial.begin(115200);

  delay(1000);

  WiFi.mode(WIFI_STA);

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

void loop()
{
}
