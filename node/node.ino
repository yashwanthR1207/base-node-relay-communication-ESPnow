#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// OLED SETTINGS
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
// NODE SETTINGS
// =====================================================

#define HAZARD_ID 2
#define BASE_ID 1

#define WIFI_CHANNEL 1

// =====================================================
// RELAY MAC
// =====================================================

uint8_t relayMAC[] =
{
  0x2C,
  0xF4,
  0x32,
  0x30,
  0xCD,
  0xD2
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
// ACK FLAGS
// =====================================================

volatile bool ackReceived = false;

volatile uint8_t receivedAckHops = 0;


// =====================================================
// OLED FUNCTION
// =====================================================

void showOLED(
  const char *line1,
  const char *line2,
  const char *line3,
  const char *line4
)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
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
// ESP-NOW RECEIVE CALLBACK
// IMPORTANT:
// DO NOT UPDATE OLED HERE
// =====================================================

void onDataRecv(
  const uint8_t *mac,
  const uint8_t *data,
  int len
)
{
  if (len != sizeof(MeshPacket))
  {
    return;
  }

  MeshPacket packet;

  memcpy(
    &packet,
    data,
    sizeof(MeshPacket)
  );


  // -----------------------------------------------
  // CHECK FOR ACK
  // -----------------------------------------------

  if (
    packet.type == 2 &&
    packet.destination == HAZARD_ID
  )
  {
    ackReceived = true;

    receivedAckHops = packet.hopCount;
  }
}


// =====================================================
// SEND FIRE ALERT
// =====================================================

bool sendEmergency()
{
  MeshPacket packet;

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

  Serial.println("SOURCE   : HAZARD NODE 02");
  Serial.println("EVENT    : FIRE");
  Serial.println("PRIORITY : P0");
  Serial.println("DEST     : BASE");
  Serial.println("HOPS     : 0");
  Serial.println("TTL      : 5");


  showOLED(
    "HAZARD NODE 02",
    "FIRE DETECTED!",
    "Priority: P0",
    "Sending..."
  );

  delay(300);


  esp_err_t result = esp_now_send(
    relayMAC,
    (uint8_t *)&packet,
    sizeof(packet)
  );


  if (result == ESP_OK)
  {
    Serial.println();
    Serial.println("FIRE ALERT SENT TO RELAY");
    Serial.println("WAITING FOR ACK...");

    showOLED(
      "HAZARD NODE 02",
      "FIRE DETECTED!",
      "ALERT SENT",
      "Waiting for ACK..."
    );

    return true;
  }
  else
  {
    Serial.println();
    Serial.print("SEND FAILED. ERROR: ");
    Serial.println(result);

    showOLED(
      "HAZARD NODE 02",
      "FIRE DETECTED!",
      "SEND FAILED!",
      "Retrying..."
    );

    return false;
  }
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(2000);


  // ===================================================
  // OLED START
  // ===================================================

  Serial.println();
  Serial.println("========================================");
  Serial.println("          HAZARD NODE 02");
  Serial.println("========================================");

  Serial.println("Starting OLED...");


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
    Serial.println("OLED NOT FOUND!");

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

  Serial.println("Starting WiFi...");

  WiFi.mode(WIFI_STA);

  delay(200);


  esp_wifi_set_channel(
    WIFI_CHANNEL,
    WIFI_SECOND_CHAN_NONE
  );

  delay(200);


  Serial.print("HAZARD MAC: ");
  Serial.println(
    WiFi.macAddress()
  );

  Serial.print("WIFI CHANNEL: ");
  Serial.println(
    WIFI_CHANNEL
  );


  // ===================================================
  // ESP-NOW
  // ===================================================

  Serial.println("Starting ESP-NOW...");


  if (
    esp_now_init()
    != ESP_OK
  )
  {
    Serial.println("ESP-NOW INIT FAILED");


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


  Serial.println("ESP-NOW OK");


  // ===================================================
  // ADD RELAY
  // ===================================================

  esp_now_peer_info_t peerInfo = {};


  memcpy(
    peerInfo.peer_addr,
    relayMAC,
    6
  );


  peerInfo.channel = WIFI_CHANNEL;

  peerInfo.encrypt = false;


  Serial.println("Adding relay...");


  if (
    esp_now_add_peer(
      &peerInfo
    )
    != ESP_OK
  )
  {
    Serial.println("RELAY PEER ADD FAILED");


    showOLED(
      "HAZARD NODE 02",
      "RELAY ERROR",
      "PEER ADD FAILED",
      "Check MAC"
    );


    while (true)
    {
      delay(1000);
    }
  }


  Serial.println("RELAY PEER ADDED");


  // ===================================================
  // REGISTER RECEIVE CALLBACK
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
    "NETWORK OK"
  );


  // ===================================================
  // IMPORTANT STARTUP WAIT
  // ===================================================

  Serial.println();
  Serial.println("Waiting for network...");
  Serial.println("5 seconds...");


  delay(5000);
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  static bool emergencyStarted = false;

  static unsigned long lastAttempt = 0;

  static int attempts = 0;

  const unsigned long retryInterval = 3000;

  const int maxAttempts = 5;


  // ===================================================
  // START FIRST EMERGENCY
  // ===================================================

  if (!emergencyStarted)
  {
    emergencyStarted = true;

    ackReceived = false;

    attempts = 0;

    sendEmergency();

    lastAttempt = millis();
  }


  // ===================================================
  // ACK RECEIVED
  // ===================================================

  if (ackReceived)
  {
    Serial.println();
    Serial.println("========================================");
    Serial.println("        ACKNOWLEDGEMENT RECEIVED");
    Serial.println("========================================");

    Serial.println("SOURCE      : BASE");

    Serial.println("DESTINATION : HAZARD");

    Serial.println("TYPE        : ACK");

    Serial.println("EVENT       : ACK");

    Serial.print("HOPS        : ");
    Serial.println(receivedAckHops);

    Serial.println("----------------------------------------");

    Serial.println("BASE CONFIRMED ALERT");

    Serial.println("ALERT DELIVERED SUCCESSFULLY");

    Serial.println("========================================");


    showOLED(
      "HAZARD NODE 02",
      "----------------",
      "ALERT DELIVERED",
      "ACK RECEIVED!"
    );


    // Stop retrying
    emergencyStarted = true;

    while (true)
    {
      delay(1000);
    }
  }


  // ===================================================
  // RETRY
  // ===================================================

  if (
    !ackReceived &&
    attempts < maxAttempts &&
    millis() - lastAttempt >= retryInterval
  )
  {
    attempts++;

    Serial.println();
    Serial.print("RETRY ATTEMPT: ");
    Serial.print(attempts);
    Serial.print("/");
    Serial.println(maxAttempts);


    showOLED(
      "HAZARD NODE 02",
      "FIRE DETECTED!",
      "RETRYING ALERT",
      "Please wait..."
    );


    delay(300);


    sendEmergency();


    lastAttempt = millis();
  }


  // ===================================================
  // ALL RETRIES FAILED
  // ===================================================

  if (
    !ackReceived &&
    attempts >= maxAttempts
  )
  {
    Serial.println();
    Serial.println("========================================");
    Serial.println("       ALERT DELIVERY FAILED");
    Serial.println("========================================");


    showOLED(
      "HAZARD NODE 02",
      "FIRE DETECTED!",
      "NO ACK RECEIVED",
      "NETWORK FAILED"
    );


    while (true)
    {
      delay(1000);
    }
  }
}
