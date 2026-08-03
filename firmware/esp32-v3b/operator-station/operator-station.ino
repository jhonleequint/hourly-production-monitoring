#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// PINS
// =====================================================
#define TRIG_PIN 5
#define ECHO_PIN 18

// Buttons
#define BTN_MODE 19
#define BTN_SOLDER 25
#define BTN_COMPONENT 26

// =====================================================
// I2C LCD
// =====================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// MODELS
// =====================================================
const char* models[] = {
  "CV-AX53(BR)1.6",
  "CV-EX520(BR)1.9",
  "CV-MR30G(BR)1.26"
};
const uint8_t TOTAL_MODELS = 3;

uint8_t currentModel = 0;

// false = menu
// true = production
bool modelConfirmed = false;

// =====================================================
// COUNTERS
// =====================================================
uint32_t productionCount = 0;
uint32_t solderDefectCount = 0;
uint32_t componentDefectCount = 0;

// =====================================================
// ULTRASONIC SENSOR
// =====================================================
#define DETECTION_DISTANCE 10

bool lastPieceDetected = false;

// =====================================================
// RETURN TO MENU (3 CLICKS)
// =====================================================
int modeClickCount = 0;
unsigned long lastModeClick = 0;
const unsigned long clickInterval = 800;

// =====================================================
// CREDITS SCREEN
// =====================================================
bool creditScreen = false;
unsigned long lastScroll = 0;
int scrollPosition = 0;

String creditText = "Desenvolvido por Jhon Lee Quint / 03/08/2026 01:11am   ";

// =====================================================
// ESP-NOW
// =====================================================
// Gateway (receiver) MAC address
// replace with the gateway MAC before deploying
uint8_t gatewayMAC[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

typedef struct {
  char model[32];
  char event[32];
  char category[32];
  uint32_t quantity;
} Message;

esp_now_peer_info_t peer;

// =====================================================
// ESP-NOW SEND
// =====================================================
void sendMessage(const char* event, const char* category = "") {
  Message msg;

  strcpy(msg.model, models[currentModel]);
  strcpy(msg.event, event);
  strcpy(msg.category, category);
  msg.quantity = 1;

  esp_now_send(gatewayMAC, (uint8_t*)&msg, sizeof(msg));
}

// =====================================================
// DISPLAY UPDATE
// =====================================================
void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);

  String model = models[currentModel];
  if (model.length() > 16) {
    model = model.substring(0, 16);
  }
  lcd.print(model);

  // Production counters are shown
  // only after a model is confirmed
  if (modelConfirmed) {
    lcd.setCursor(0, 1);

    lcd.print("P:");
    lcd.print(productionCount);
    lcd.print(" DEF:S");
    lcd.print(solderDefectCount);
    lcd.print(" C");
    lcd.print(componentDefectCount);
  }
}

// =====================================================
// ULTRASONIC READING
// =====================================================
float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  return duration * 0.034 / 2;
}

// =====================================================
// PRODUCTION COUNT
// =====================================================
void checkPiece() {
  if (!modelConfirmed) return;

  float distance = readDistance();
  bool detected = distance < DETECTION_DISTANCE;

  if (detected && !lastPieceDetected) {
    productionCount++;
    sendMessage("PRODUCTION", "");
    updateDisplay();
  }

  lastPieceDetected = detected;
}

// =====================================================
// MODE BUTTON
// =====================================================
void checkModeButton() {
  if (digitalRead(BTN_MODE) == LOW) {
    delay(50);

    unsigned long start = millis();

    while (digitalRead(BTN_MODE) == LOW) {
      delay(10);
    }

    unsigned long pressTime = millis() - start;

    // Held for 4 seconds:
    // confirm model
    if (pressTime >= 4000) {
      modelConfirmed = true;

      productionCount = 0;
      solderDefectCount = 0;
      componentDefectCount = 0;

      sendMessage("MODEL_SELECTED");
      updateDisplay();
    }
    // Short press: next model
    else {
      if (!modelConfirmed) {
        currentModel++;

        if (currentModel >= TOTAL_MODELS) {
          currentModel = 0;
        }

        updateDisplay();
      }
    }
  }
}

// =====================================================
// 3 CLICKS TO RETURN TO MENU
// =====================================================
void checkReturnMenu() {
  if (digitalRead(BTN_MODE) == LOW) {
    delay(50);

    while (digitalRead(BTN_MODE) == LOW);

    unsigned long now = millis();

    if (now - lastModeClick < clickInterval) {
      modeClickCount++;
    } else {
      modeClickCount = 1;
    }

    lastModeClick = now;

    if (modeClickCount >= 3) {
      modelConfirmed = false;

      productionCount = 0;
      solderDefectCount = 0;
      componentDefectCount = 0;

      modeClickCount = 0;

      updateDisplay();
    }
  }
}

// =====================================================
// SOLDER DEFECT
// =====================================================
void checkSolderButton() {
  if (digitalRead(BTN_SOLDER) == LOW) {
    delay(50);

    if (modelConfirmed) {
      solderDefectCount++;

      sendMessage("DEFECT", "SOLDER");
      updateDisplay();
    }

    while (digitalRead(BTN_SOLDER) == LOW);
  }
}

// =====================================================
// COMPONENT DEFECT
// =====================================================
void checkComponentButton() {
  if (digitalRead(BTN_COMPONENT) == LOW) {
    delay(50);

    if (modelConfirmed) {
      componentDefectCount++;

      sendMessage("DEFECT", "COMPONENT");
      updateDisplay();
    }

    while (digitalRead(BTN_COMPONENT) == LOW);
  }
}

// =====================================================
// CREDITS SCREEN
// =====================================================
void checkCreditMode() {
  if (digitalRead(BTN_MODE) == LOW && digitalRead(BTN_SOLDER) == LOW && digitalRead(BTN_COMPONENT) == LOW) {
    delay(300);

    creditScreen = true;
    scrollPosition = 0;

    lcd.clear();
  }
}

// =====================================================
// CREDITS SCROLL
// =====================================================
void showCredits() {
  if (millis() - lastScroll > 300) {
    lastScroll = millis();

    lcd.clear();

    String part = creditText.substring(scrollPosition, scrollPosition + 16);

    lcd.setCursor(0, 0);
    lcd.print(part);

    scrollPosition++;

    if (scrollPosition > creditText.length() - 16) {
      scrollPosition = 0;
    }
  }
}

// =====================================================
// CANCEL CREDITS
// =====================================================
void cancelCredits() {
  if (digitalRead(BTN_MODE) == LOW || digitalRead(BTN_SOLDER) == LOW || digitalRead(BTN_COMPONENT) == LOW) {
    creditScreen = false;
    modelConfirmed = false;

    updateDisplay();

    delay(300);
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  // -------------------------------
  // PINS
  // -------------------------------
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_SOLDER, INPUT_PULLUP);
  pinMode(BTN_COMPONENT, INPUT_PULLUP);

  // -------------------------------
  // LCD
  // -------------------------------
  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Engenharia");

  lcd.setCursor(0, 1);
  lcd.print("Contador V3");

  delay(2000);

  updateDisplay();

  // -------------------------------
  // ESP-NOW
  // -------------------------------
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW ERROR");
    return;
  }

  memcpy(peer.peer_addr, gatewayMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;

  esp_now_add_peer(&peer);

  Serial.println("SYSTEM READY");
}

// =====================================================
// MAIN LOOP
// =====================================================
void loop() {
  // =================================================
  // PRIORITY 1
  // CREDITS SCREEN
  // =================================================
  checkCreditMode();

  if (creditScreen) {
    showCredits();
    cancelCredits();
    return;
  }

  // =================================================
  // PRIORITY 2
  // OPERATION
  // =================================================
  if (modelConfirmed) {
    checkPiece();
    checkReturnMenu();
  } else {
    checkModeButton();
  }

  // =================================================
  // DEFECTS
  // =================================================
  checkSolderButton();
  checkComponentButton();

  delay(50);
}
