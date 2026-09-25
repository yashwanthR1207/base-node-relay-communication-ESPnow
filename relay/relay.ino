#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t baseMAC[] = {
  0xD4, 0xE9, 0xF4, 0xB2, 0xA4, 0x58
};

void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len)
{
  Serial.print("RECEIVED DATA FROM NODE, LEN: ");
  Serial.println(len);

  uint8_t result = esp_now_send(
    baseMAC,
    data,
    len
  );

  if (result == 0)
  {
    Serial.println("FORWARDED TO BASE");
  }
  else
  {
    Serial.println("FORWARD FAILED");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  Serial.print("ESP8266 MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0)
  {
    Serial.println("ESP-NOW INIT FAILED");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  if (esp_now_add_peer(
        baseMAC,
        ESP_NOW_ROLE_SLAVE,
        0,
        NULL,
        0) != 0)
  {
    Serial.println("FAILED TO ADD BASE");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("ESP8266 RELAY READY");
}

void loop()
{
}
