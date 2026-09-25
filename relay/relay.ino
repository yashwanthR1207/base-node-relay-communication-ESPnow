#include <ESP8266WiFi.h>

extern "C"
{
  #include <user_interface.h>
}

#include <espnow.h>

// =====================================================
// RELAY SETTINGS
// =====================================================

#define RELAY_ID 3
#define WIFI_CHANNEL 1

// =====================================================
// MAC ADDRESSES
// =====================================================

// ESP32 #1 BASE
uint8_t baseMAC[] = {
  0xD4, 0xE9, 0xF4, 0xB2, 0xA4, 0x58
};

// ESP32 #2 HAZARD
uint8_t hazardMAC[] = {
  0xC0, 0xCD, 0xD6, 0x84, 0xA1, 0x04
};

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
// RECEIVE PACKET
// =====================================================

void onDataRecv(
  uint8_t *mac,
  uint8_t *data,
  uint8_t len
)
{
  // =================================================
  // CHECK PACKET SIZE
  // =================================================

  if (len != sizeof(MeshPacket))
  {
    Serial.println();
    Serial.println("INVALID PACKET SIZE");

    Serial.print("Received: ");
    Serial.println(len);

    Serial.print("Expected: ");
    Serial.println(sizeof(MeshPacket));

    return;
  }

  MeshPacket packet;

  memcpy(
    &packet,
    data,
    sizeof(MeshPacket)
  );

  // =================================================
  // DISPLAY PACKET
  // =================================================

  Serial.println();
  Serial.println("========================================");
  Serial.println("          PACKET RECEIVED");
  Serial.println("========================================");

  Serial.print("SOURCE      : ");
  Serial.println(packet.source);

  Serial.print("DESTINATION : ");
  Serial.println(packet.destination);

  Serial.print("TYPE        : ");
  Serial.println(packet.type);

  Serial.print("PRIORITY    : P");
  Serial.println(packet.priority);

  Serial.print("EVENT       : ");
  Serial.println(packet.event);

  Serial.print("HOPS        : ");
  Serial.println(packet.hopCount);

  Serial.print("TTL         : ");
  Serial.println(packet.ttl);

  Serial.println("----------------------------------------");

  // =================================================
  // TTL CHECK
  // =================================================

  if (packet.ttl == 0)
  {
    Serial.println("PACKET DROPPED");
    Serial.println("TTL EXPIRED");
    Serial.println("========================================");
    return;
  }

  // =================================================
  // UPDATE HOP INFORMATION
  // =================================================

  packet.hopCount++;
  packet.ttl--;

  // =================================================
  // ACK PACKET
  // TYPE 2 = ACK
  // =================================================

  if (packet.type == 2)
  {
    Serial.println();
    Serial.println("ACK FROM BASE");
    Serial.println("FORWARDING ACK TO HAZARD...");

    uint8_t result = esp_now_send(
      hazardMAC,
      (uint8_t *)&packet,
      sizeof(packet)
    );

    if (result == 0)
    {
      Serial.println("ACK FORWARDED TO HAZARD");
    }
    else
    {
      Serial.print("ACK FORWARD FAILED. ERROR: ");
      Serial.println(result);
    }

    Serial.println("========================================");

    return;
  }

  // =================================================
  // EMERGENCY PACKET
  // TYPE 1 = SOS
  // TYPE 3 = FIRE
  // =================================================

  if (packet.type == 1 || packet.type == 3)
  {
    Serial.println();
    Serial.println("EMERGENCY PACKET");
    Serial.println("FORWARDING TO BASE...");

    uint8_t result = esp_now_send(
      baseMAC,
      (uint8_t *)&packet,
      sizeof(packet)
    );

    if (result == 0)
    {
      Serial.println("EMERGENCY FORWARDED TO BASE");
    }
    else
    {
      Serial.print("EMERGENCY FORWARD FAILED. ERROR: ");
      Serial.println(result);
    }

    Serial.println("========================================");

    return;
  }

  // =================================================
  // UNKNOWN PACKET
  // =================================================

  Serial.println("UNKNOWN PACKET TYPE");
  Serial.println("========================================");
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  // =================================================
  // WIFI
  // =================================================

  WiFi.mode(WIFI_STA);

  // FORCE CHANNEL 1
  wifi_set_channel(WIFI_CHANNEL);

  delay(100);

  // =================================================
  // RELAY INFORMATION
  // =================================================

  Serial.println();
  Serial.println("========================================");
  Serial.println("       ESP8266 RELAY NODE");
  Serial.println("========================================");

  Serial.print("RELAY MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("WIFI CHANNEL: ");
  Serial.println(WIFI_CHANNEL);

  // =================================================
  // ESP-NOW INITIALIZATION
  // =================================================

  if (esp_now_init() != 0)
  {
    Serial.println("ESP-NOW INIT FAILED");
    return;
  }

  // =================================================
  // RELAY ROLE
  // =================================================

  esp_now_set_self_role(
    ESP_NOW_ROLE_COMBO
  );

  // =================================================
  // ADD BASE PEER
  // =================================================

  if (
    esp_now_add_peer(
      baseMAC,
      ESP_NOW_ROLE_COMBO,
      WIFI_CHANNEL,
      NULL,
      0
    ) != 0
  )
  {
    Serial.println("FAILED TO ADD BASE");
    return;
  }

  // =================================================
  // ADD HAZARD PEER
  // =================================================

  if (
    esp_now_add_peer(
      hazardMAC,
      ESP_NOW_ROLE_COMBO,
      WIFI_CHANNEL,
      NULL,
      0
    ) != 0
  )
  {
    Serial.println("FAILED TO ADD HAZARD");
    return;
  }

  // =================================================
  // RECEIVE CALLBACK
  // =================================================

  esp_now_register_recv_cb(onDataRecv);

  // =================================================
  // READY
  // =================================================

  Serial.println();
  Serial.println("BASE PEER ADDED");
  Serial.println("HAZARD PEER ADDED");
  Serial.println("BIDIRECTIONAL RELAY READY");
  Serial.println("========================================");
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  // Relay continuously waits for packets
}
