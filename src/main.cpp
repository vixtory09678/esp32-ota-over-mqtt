#include <Arduino.h>
#include <PubSubClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "mbedtls/md5.h"

WiFiClient client;
PubSubClient mqtt(client);

#define SSID     "YOUR_WIFI"
#define PASSWORD "YOUR_PASSWORD"

#define MQTT_HOST                           "YOUR_MQTT_HOST"
#define MQTT_PORT                           1883
#define MQTT_CLIENT                         "I_AM_{TEAM}"
#define MQTT_UPDATE_FIRMWARE_TOPIC          "ota"
#define MQTT_UPDATE_FIRMWARE_FEEDBACK_TOPIC "ota/feedback"

void setupWiFi() {
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.printf("WiFi Connected.\n[IP] %s\n", WiFi.localIP().toString().c_str());
}

bool calculateChecksum(uint8_t *data, int len, uint8_t dataToBeHashed[16]) {
  mbedtls_md5_context ctx;
  if (mbedtls_md5_starts_ret(&ctx) != 0) {
    return false;
  }

  if (mbedtls_md5_update_ret(&ctx, data, len) != 0) {
    return false;
  }

  return mbedtls_md5_finish_ret(&ctx, dataToBeHashed) == 0;
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.println("Try to connect mqtt broker");
    if (mqtt.connect(MQTT_CLIENT)) {
      Serial.println("Mqtt connected.");
      mqtt.subscribe(MQTT_UPDATE_FIRMWARE_TOPIC);
      mqtt.subscribe("ota_test");
    } else {
      Serial.println("Mqtt connect fail\nWill try again in 5sec.");
      delay(5000);
    }
  }
}

bool isFirst             = true;
bool firmwareReadyUpdate = false;
uint32_t totalSize       = 0;
uint32_t currentSize     = 0;

void onDataReceive(char *topic, uint8_t *buff, unsigned int size) {
  Serial.printf("data incomming [%u] bytes\n", size);

  if (strcmp(topic, MQTT_UPDATE_FIRMWARE_TOPIC) != 0) {
    return;
  }

  if (isFirst) {
    totalSize = (uint32_t)(buff[0] << 24) | (uint32_t)(buff[1] << 16) | (uint32_t)(buff[2] << 8) |
                (uint32_t)(buff[3] & 0xff);
    Update.begin(totalSize);
    Serial.printf("total file size is %d\n", totalSize);
    isFirst = false;
    return;
  }

  uint16_t sizeOfPayload     = (buff[0] << 8) | (buff[1] & 0xFF);
  uint8_t *payload           = static_cast<uint8_t *>(malloc(sizeOfPayload));
  uint8_t checksumPacket[16] = {0};

  uint8_t *pRemaining = &buff[2 + sizeOfPayload + 16];
  int remaining       = (uint16_t)(pRemaining[0] << 8) | pRemaining[1] & 0xFF;

  memcpy(payload, &buff[2], sizeOfPayload);
  memcpy(checksumPacket, &buff[2 + sizeOfPayload], 16);
  currentSize += sizeOfPayload;

  Serial.printf("payload size is %d\n", sizeOfPayload);
  Serial.printf("progress %d/%d\n", currentSize, totalSize);

  uint8_t chksumCal[16] = {0};
  if (calculateChecksum(payload, sizeOfPayload, chksumCal)) {
    int checksumOk = memcmp(chksumCal, checksumPacket, 16);

    if (checksumOk == 0) {
      size_t writenBuffer = Update.write(payload, sizeOfPayload);
      Serial.printf("write buffer [%d]\n", writenBuffer);
    }

    mqtt.publish(MQTT_UPDATE_FIRMWARE_FEEDBACK_TOPIC, checksumOk == 0 ? "ok" : "fail");
  }

  Serial.printf("remaining is %d\n", remaining);
  if (remaining == 0) {
    if (currentSize == totalSize) {
      firmwareReadyUpdate = true;
    } else {
      Update.abort();
    }
  }
  delete payload;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setupWiFi();

  mqtt.setBufferSize(2048);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onDataReceive);
}

void loop() {
  // put your main code here, to run repeatedly:

  if (firmwareReadyUpdate) {
    Serial.printf("confirm remaining.. [%d]\n", Update.remaining());
    Serial.println("updating..");
    if (Update.hasError()) {
      Serial.println(Update.errorString());
      firmwareReadyUpdate = false;
      return;
    }

    if (Update.end()) {
      Serial.println("update firmware successful\nreboot in 3 sec.");
      delay(3000);
      ESP.restart();
    }

    firmwareReadyUpdate = false;
  }

  if (!mqtt.connected()) {
    Serial.println("connecting fail");
    Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi is still connected"
                                                 : "WiFi connection lost");
    if (!WiFi.isConnected()) {
      setupWiFi();
    }

    connectMqtt();
  }

  mqtt.loop();
}