#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ======================================================
// MESSAGE (must match the operator station struct)
// ======================================================
typedef struct {
  char model[32];
  char event[32];
  char category[32];
  uint32_t quantity;
} Message;

Message receivedMessage;

// ======================================================
// ESP-NOW RECEIVE
// ======================================================
void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));

  Serial.println();
  Serial.println("==============================");
  Serial.println("PACOTE RECEBIDO");
  Serial.println("==============================");

  // Sender MAC
  Serial.print("MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) {
      Serial.print(":");
    }
  }
  Serial.println();

  // Received fields
  Serial.print("MODEL: ");
  Serial.println(receivedMessage.model);
  Serial.print("EVENT: ");
  Serial.println(receivedMessage.event);
  Serial.print("CATEGORY: ");
  Serial.println(receivedMessage.category);
  Serial.print("QTY: ");
  Serial.println(receivedMessage.quantity);

  // ======================================================
  // LINE FOR THE DESKTOP APPLICATION
  // Format: MODEL|EVENT|CATEGORY|QTY
  // ======================================================
  Serial.print(receivedMessage.model);
  Serial.print("|");
  Serial.print(receivedMessage.event);
  Serial.print("|");
  Serial.print(receivedMessage.category);
  Serial.print("|");
  Serial.println(receivedMessage.quantity);
}

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP-NOW GATEWAY");
  Serial.println("==============================");

  WiFi.mode(WIFI_STA);

  Serial.print("MAC DO GATEWAY: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERRO AO INICIAR ESP-NOW");
    while (true) {
      delay(1000);
    }
  }

  esp_now_register_recv_cb(onReceive);

  Serial.println();
  Serial.println("AGUARDANDO DADOS...");
}

// ======================================================
// LOOP
// ======================================================
void loop() {
  delay(10);
}
