#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define SDA_PIN 25
#define SCL_PIN 26

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


// =====================================================
// NODE
// =====================================================

#define HAZARD_ID 2
#define BASE_ID 1

#define WIFI_CHANNEL 1


// =====================================================
// RELAY MACs
// =====================================================

uint8_t relayAMAC[] =
{
  0x2C, 0xF4, 0x32, 0x30, 0xCD, 0xD2
};

uint8_t relayBMAC[] =
{
  0x8C, 0xAA, 0xB5, 0x6B, 0x08, 0xAA
};


// =====================================================
// PACKET
// =====================================================

struct MeshPacket
{
  uint16_t messageID;

  uint8_t source;
  uint8_t destination;
  uint8_t type;
  uint8_t priority;
  uint8_t hopCount;
  uint8_t ttl;

  char event[20];
};


// =====================================================
// ACK VARIABLES
// =====================================================

volatile bool ackReceived = false;

volatile uint16_t ackMessageID = 0;

volatile uint8_t ackHops = 0;


// =====================================================
// OLED
// =====================================================

void showOLED(
  const char *line1,
  const char *line2,
  const char *line3,
  const char *line4
)
{
  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);


  display.setCursor(0, 0);
  display.println(line1);

  display.setCursor(0, 16);
  display.println(line2);

  display.setCursor(0, 32);
  display.println(line3);

  display.setCursor(0, 48);
  display.println(line4);

  display.display();
}


// =====================================================
// RECEIVE CALLBACK
// NO OLED HERE
// =====================================================

void onDataRecv(
  const uint8_t *mac,
  const uint8_t *data,
  int len
)
{
  if (
    len != sizeof(MeshPacket)
  )
  {
    return;
  }


  MeshPacket packet;

  memcpy(
    &packet,
    data,
    sizeof(MeshPacket)
  );


  if (
    packet.type == 2 &&
    packet.destination == HAZARD_ID
  )
  {
    ackMessageID = packet.messageID;

    ackHops = packet.hopCount;

    ackReceived = true;
  }
}


// =====================================================
// SEND EMERGENCY THROUGH BOTH RELAYS
// =====================================================

bool sendEmergency(
  uint16_t messageID
)
{
  MeshPacket packet;


  packet.messageID = messageID;

  packet.source = HAZARD_ID;

  packet.destination = BASE_ID;

  packet.type = 3;

  packet.priority = 0;

  packet.hopCount = 0;

  packet.ttl = 5;


  strcpy(
    packet.event,
    "FIRE"
  );


  Serial.println();
  Serial.println("========================================");
  Serial.println("       FIRE EMERGENCY ALERT");
  Serial.println("========================================");

  Serial.print("MESSAGE ID : ");
  Serial.println(messageID);

  Serial.println("SOURCE     : HAZARD NODE 02");

  Serial.println("EVENT      : FIRE");

  Serial.println("PRIORITY   : P0");

  Serial.println("DEST       : BASE");

  Serial.println("HOPS       : 0");

  Serial.println("TTL        : 5");


  showOLED(
    "HAZARD NODE 02",
    "FIRE DETECTED!",
    "P0 - DUAL PATH",
    "Sending..."
  );


  delay(300);


  // ===================================================
  // RELAY A
  // ===================================================

  esp_err_t resultA =
    esp_now_send(
      relayAMAC,
      (uint8_t *)&packet,
      sizeof(packet)
    );


  if (
    resultA == ESP_OK
  )
  {
    Serial.println(
      "SENT TO RELAY A"
    );
  }
  else
  {
    Serial.print(
      "RELAY A SEND ERROR: "
    );

    Serial.println(resultA);
  }


  // ===================================================
  // RELAY B
  // ===================================================

  esp_err_t resultB =
    esp_now_send(
      relayBMAC,
      (uint8_t *)&packet,
      sizeof(packet)
    );


  if (
    resultB == ESP_OK
  )
  {
    Serial.println(
      "SENT TO RELAY B"
    );
  }
  else
  {
    Serial.print(
      "RELAY B SEND ERROR: "
    );

    Serial.println(resultB);
  }


  showOLED(
    "HAZARD NODE 02",
    "FIRE DETECTED!",
    "ALERT SENT",
    "Waiting for ACK..."
  );


  return (
    resultA == ESP_OK ||
    resultB == ESP_OK
  );
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(2000);


  // ===================================================
  // OLED
  // ===================================================

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  )
  {
    Serial.println(
      "OLED NOT FOUND!"
    );

    while (true)
    {
      delay(1000);
    }
  }


  Serial.println("OLED OK");


  showOLED(
    "HAZARD NODE 02",
    "----------------",
    "OLED OK",
    "Starting..."
  );


  delay(2000);


  // ===================================================
  // WIFI
  // ===================================================

  WiFi.mode(WIFI_STA);

  delay(200);


  esp_wifi_set_channel(
    WIFI_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );


  delay(200);


  Serial.println();
  Serial.println("========================================");
  Serial.println("          HAZARD NODE 02");
  Serial.println("========================================");

  Serial.print("MAC: ");

  Serial.println(
    WiFi.macAddress()
  );

  Serial.print("CHANNEL: ");

  Serial.println(
    WIFI_CHANNEL
  );


  // ===================================================
  // ESP-NOW
  // ===================================================

  if (
    esp_now_init()
    != ESP_OK
  )
  {
    Serial.println(
      "ESP-NOW INIT FAILED"
    );

    showOLED(
      "HAZARD NODE 02",
      "ESP-NOW ERROR",
      "INITIALIZATION",
      "FAILED!"
    );

    while (true)
    {
      delay(1000);
    }
  }


  Serial.println(
    "ESP-NOW INITIALIZED"
  );


  // ===================================================
  // RELAY A PEER
  // ===================================================

  esp_now_peer_info_t peerA = {};


  memcpy(
    peerA.peer_addr,
    relayAMAC,
    6
  );


  peerA.channel =
    WIFI_CHANNEL;

  peerA.encrypt =
    false;


  if (
    esp_now_add_peer(
      &peerA
    )
    == ESP_OK
  )
  {
    Serial.println(
      "RELAY A ADDED"
    );
  }
  else
  {
    Serial.println(
      "RELAY A ADD FAILED"
    );
  }


  // ===================================================
  // RELAY B PEER
  // ===================================================

  esp_now_peer_info_t peerB = {};


  memcpy(
    peerB.peer_addr,
    relayBMAC,
    6
  );


  peerB.channel =
    WIFI_CHANNEL;

  peerB.encrypt =
    false;


  if (
    esp_now_add_peer(
      &peerB
    )
    == ESP_OK
  )
  {
    Serial.println(
      "RELAY B ADDED"
    );
  }
  else
  {
    Serial.println(
      "RELAY B ADD FAILED"
    );
  }


  // ===================================================
  // CALLBACK
  // ===================================================

  esp_now_register_recv_cb(
    onDataRecv
  );


  // ===================================================
  // READY
  // ===================================================

  Serial.println();
  Serial.println("========================================");
  Serial.println("       HAZARD NODE READY");
  Serial.println("========================================");


  showOLED(
    "HAZARD NODE 02",
    "----------------",
    "SYSTEM READY",
    "SELF-HEALING ON"
  );


  // Give network time to stabilize
  delay(5000);


  // ===================================================
  // MESSAGE ID
  // ===================================================

  uint16_t messageID =
    millis() & 0xFFFF;


  ackReceived = false;


  // ===================================================
  // SEND FIRST ALERT
  // ===================================================

  sendEmergency(
    messageID
  );


  // ===================================================
  // WAIT FOR ACK
  // ===================================================

  unsigned long startTime =
    millis();


  while (
    !ackReceived &&
    millis() - startTime < 5000
  )
  {
    delay(10);
  }


  // ===================================================
  // ACK RECEIVED
  // ===================================================

  if (
    ackReceived &&
    ackMessageID == messageID
  )
  {
    Serial.println();
    Serial.println("========================================");
    Serial.println("        ACKNOWLEDGEMENT RECEIVED");
    Serial.println("========================================");

    Serial.print("MESSAGE ID : ");
    Serial.println(ackMessageID);

    Serial.print("HOPS       : ");
    Serial.println(ackHops);

    Serial.println("----------------------------------------");

    Serial.println(
      "BASE CONFIRMED ALERT"
    );

    Serial.println(
      "ALERT DELIVERED SUCCESSFULLY"
    );

    Serial.println(
      "SELF-HEALING NETWORK OK"
    );

    Serial.println(
      "========================================");


    showOLED(
      "HAZARD NODE 02",
      "----------------",
      "ALERT DELIVERED",
      "ACK RECEIVED!"
    );
  }
  else
  {
    // =================================================
    // NO ACK
    // =================================================

    Serial.println();
    Serial.println("========================================");
    Serial.println("       NO ACK RECEIVED");
    Serial.println("========================================");

    Serial.println(
      "RETRYING BOTH PATHS..."
    );


    showOLED(
      "HAZARD NODE 02",
      "NO ACK RECEIVED",
      "SELF-HEALING...",
      "Retrying paths"
    );


    // =================================================
    // RETRY
    // =================================================

    ackReceived = false;


    delay(1000);


    sendEmergency(
      messageID
    );


    startTime =
      millis();


    while (
      !ackReceived &&
      millis() - startTime < 5000
    )
    {
      delay(10);
    }


    // =================================================
    // SECOND ACK
    // =================================================

    if (
      ackReceived &&
      ackMessageID == messageID
    )
    {
      Serial.println();
      Serial.println(
        "SELF-HEALING SUCCESSFUL"
      );


      showOLED(
        "HAZARD NODE 02",
        "SELF-HEALING OK",
        "ALERT DELIVERED",
        "ACK RECEIVED!"
      );
    }
    else
    {
      Serial.println();
      Serial.println(
        "NETWORK DELIVERY FAILED"
      );


      showOLED(
        "HAZARD NODE 02",
        "DELIVERY FAILED",
        "NO ACK RECEIVED",
        "CHECK NETWORK"
      );
    }
  }
}


void loop()
{
}
