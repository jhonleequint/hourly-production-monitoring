/*
  Production control station (ESP32) - V3[A]
  Author: Jhon Lee Quint
  Inspection, sorting and environmental monitoring
  using an ESP32 with sensors and actuators.
*/

#define SIMULACAO true

#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include "DHT.h" // Requires the "DHT sensor library" package

// ============================================================
// HARDWARE MAPPING
// ============================================================
// I2C LCD: SDA=21, SCL=22
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servos
Servo servoCor;  // Servo 1: color reject
Servo servoDist; // Servo 2: distance/height reject

// Environmental sensors
#define DHTPIN 23     // Digital pin (I/O)
#define DHTTYPE DHT11 // Or DHT22
DHT dht(DHTPIN, DHTTYPE);

// Pins
const int PIN_SERVO_COR = 13;
const int PIN_SERVO_DIST = 15;
const int PIN_AUDIO = 19;    // Buzzer/speaker
const int PIN_IR_COUNT = 39; // TCRT5000 (input only)
const int PIN_TEMP_ANA = 36; // Analog temperature sensor (input only)

// Ultrasonic
const int PIN_TRIG = 5;
const int PIN_ECHO = 34; // Input only

// Color sensor (TCS3200)
// In the simulator (SIMULACAO true), PIN_TCS_OUT is read as a digital input
const int PIN_TCS_OUT = 35; // Input only
const int PIN_TCS_S2 = 18;
const int PIN_TCS_S3 = 4;

// LEDs
const int PIN_LED_R = 16;
const int PIN_LED_G = 17;
const int PIN_LED_B = 2; // Onboard LED (GPIO 2) used as blue

// ============================================================
// 4x3 KEYPAD
// ============================================================
const byte ROWS = 4;
const byte COLS = 3;

// A/B/C/D column removed.
// Standard 4x3 layout:
// 1 2 3
// 4 5 6
// 7 8 9
// * 0 #
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

// Pins as wired in Wokwi (R1-R4, C1-C3)
byte rowPins[ROWS] = { 32, 33, 25, 26 };
byte colPins[COLS] = { 27, 14, 12 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ============================================================
// PASSWORDS AND SECURITY
// ============================================================
const String PASSWORDS[3] = { "1111", "2222", "3333" };
const unsigned long TIMEOUT_IDLE = 300000; // 5 minutes without activity
unsigned long lastInteractionTime = 0;
bool isLocked = true; // Starts locked

// ============================================================
// GENERAL VARIABLES
// ============================================================
// State machine
enum SystemState {
  STATE_LOCKED,       // Password lock
  STATE_MENU,         // Main menu
  STATE_PROD_RUNNING, // Production running
  STATE_ALARM_VIEW    // Showing environmental alarm
};
SystemState currentState = STATE_LOCKED;

// Counters and inputs
String inputBuffer = "";
int productionCount = 0;
bool irLastState = HIGH;

// Servo control (non-blocking)
struct ServoControl {
  bool active;
  int stage; // 0:Idle, 1:Wait, 2:Action, 3:Return
  unsigned long timer;
};
ServoControl sCor = { false, 0, 0 };
ServoControl sDist = { false, 0, 0 };

// Environment
float currentTemp = 0.0;
float currentHum = 0.0;
bool envAlarmActive = false;
unsigned long envSilenceTimer = 0; // Silenced while > millis()

// General timer
unsigned long lastEnvCheck = 0;

// ============================================================
// PROTOTYPES
// ============================================================
void lockSystem();
void checkSensors();
void updateServos();
void checkAutoLock();
void playTone(int freq, int duration);

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.backlight();

  // DHT
  dht.begin();

  // Pins
  pinMode(PIN_SERVO_COR, OUTPUT);
  pinMode(PIN_SERVO_DIST, OUTPUT);
  pinMode(PIN_AUDIO, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_TCS_OUT, INPUT);
  pinMode(PIN_TCS_S2, OUTPUT);
  pinMode(PIN_TCS_S3, OUTPUT);

  pinMode(PIN_IR_COUNT, INPUT); // Pull to GND if the sensor is open-collector without pull-up
  pinMode(PIN_TEMP_ANA, INPUT);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  // Initialize servos
  servoCor.attach(PIN_SERVO_COR);
  servoCor.write(0);
  servoDist.attach(PIN_SERVO_DIST);
  servoDist.write(0);

  // TCS3200 setup (real hardware)
  if (!SIMULACAO) {
    // S0/S1 hardwired to 20% scaling (3.3V/GND)
    // S2/S3 start low
    digitalWrite(PIN_TCS_S2, LOW);
    digitalWrite(PIN_TCS_S3, LOW);
  }

  lockSystem();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  char key = keypad.getKey();
  if (key) {
    lastInteractionTime = millis();
    handleKeypad(key);
  }

  checkAutoLock();

  // Continuous tasks
  if (currentState == STATE_PROD_RUNNING) {
    runProductionLogic();
  }

  // Environmental monitoring
  // Always runs, even when locked, for safety
  if (millis() - lastEnvCheck > 2000) {
    lastEnvCheck = millis();
    checkEnvironment();
  }

  updateServos();
  updateLeds();
}

// ============================================================
// LOCK LOGIC
// ============================================================
void checkAutoLock() {
  if (currentState != STATE_LOCKED && (millis() - lastInteractionTime > TIMEOUT_IDLE)) {
    // Idle for 5 min
    lockSystem();
  }
}

void lockSystem() {
  currentState = STATE_LOCKED;
  inputBuffer = "";
  lcd.clear();
  lcd.print("SISTEMA BLOQ.");
  lcd.setCursor(0, 1);
  lcd.print("SENHA: ");
}

void unlockSystem() {
  currentState = STATE_MENU;
  lastInteractionTime = millis();
  lcd.clear();
  lcd.print("1-Iniciar Prod");
  lcd.setCursor(0, 1);
  lcd.print("2-Config Infos");
}

void handleKeypad(char key) {
  // Key beep
  playTone(2000, 50);

  if (currentState == STATE_LOCKED) {
    if (key == '#') {
      // Validate password
      bool passOk = false;
      for (int i = 0; i < 3; i++) {
        if (inputBuffer == PASSWORDS[i]) passOk = true;
      }

      if (passOk) {
        unlockSystem();
      } else {
        lcd.clear();
        lcd.print("SENHA INCORRETA");
        playTone(500, 500);
        delay(1000);
        lockSystem(); // Reset screen
      }
    } else if (key == '*') {
      inputBuffer = "";
      lcd.setCursor(7, 1);
      lcd.print("    ");
      lcd.setCursor(7, 1);
    } else {
      if (inputBuffer.length() < 4) {
        inputBuffer += key;
        lcd.print("*");
      }
    }
  } else if (currentState == STATE_MENU) {
    if (key == '1') {
      currentState = STATE_PROD_RUNNING;
      lcd.clear();
      lcd.print("PRODUCAO ATIVA");
      productionCount = 0;
    }
  } else if (currentState == STATE_PROD_RUNNING) {
    if (key == '*') { // BACK
      currentState = STATE_MENU;
      unlockSystem(); // Redraw menu
    }
  }

  // Silence environmental alarm
  if (envAlarmActive && key == '#') {
    envSilenceTimer = millis() + 300000; // 5 minutes
    lcd.clear();
    lcd.print("ALARME SILENCIADO");
    delay(1000);

    // Redraw previous screen
    if (currentState == STATE_PROD_RUNNING) {
      lcd.clear();
      lcd.print("PRODUCAO ATIVA");
    } else if (currentState == STATE_MENU) {
      unlockSystem();
    }
  }
}

// ============================================================
// PRODUCTION
// ============================================================
void runProductionLogic() {
  // 1. IR counter (TCRT5000)
  // HIGH with no object, LOW on detection (invert if the sensor differs)
  // Active LOW (object reflects light)
  int irVal = digitalRead(PIN_IR_COUNT);
  if (irVal == LOW && irLastState == HIGH) {
    productionCount++;
    // Update LCD only on change
    lcd.setCursor(0, 1);
    lcd.print("Qtd: ");
    lcd.print(productionCount);
    lastInteractionTime = millis(); // Activity counts as interaction
  }
  irLastState = irVal;

  // 2. Color sensor -> servo 1
  bool isRed = false;
  if (SIMULACAO) {
    // Simulator: HIGH means red
    if (digitalRead(PIN_TCS_OUT) == HIGH) isRed = true;
  } else {
    // Real TCS3200 reading
    // Read red channel (S2 LOW, S3 LOW)
    digitalWrite(PIN_TCS_S2, LOW);
    digitalWrite(PIN_TCS_S3, LOW);
    int pulseLen = pulseIn(PIN_TCS_OUT, LOW, 20000); // 20 ms timeout
    // Needs calibration on real hardware (e.g. red < 100, blue > 200)
    // Placeholder threshold:
    if (pulseLen > 0 && pulseLen < 150) isRed = true;
  }

  if (isRed && !sCor.active) {
    triggerServo(sCor);
  }

  // 3. Distance sensor -> servo 2
  // Ultrasonic trigger
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 25000); // Max 25 ms (~4 m)
  int cm = duration * 0.034 / 2;

  if (duration > 0 && cm >= 18 && cm <= 20) {
    if (!sDist.active) triggerServo(sDist);
  }
}

// ============================================================
// SERVO CONTROL
// ============================================================
void triggerServo(ServoControl &svc) {
  svc.active = true;
  svc.stage = 1;
  svc.timer = millis();
}

void updateServos() {
  manageServoCycle(sCor, servoCor);
  manageServoCycle(sDist, servoDist);
}

void manageServoCycle(ServoControl &svc, Servo &motor) {
  if (!svc.active) return;

  unsigned long elapsed = millis() - svc.timer;

  if (svc.stage == 1) { // Initial 1 s delay
    if (elapsed >= 1000) {
      motor.write(90); // Activate
      svc.stage = 2;   // Now hold for 3 s
    }
  } else if (svc.stage == 2) { // Holding for 3 s
    // 1 s delay + 3 s active = 4000 ms total
    if (elapsed >= 4000) {
      motor.write(0); // Return
      svc.stage = 3;
      svc.active = false; // Done
    }
  }
}

// ============================================================
// ENVIRONMENT
// ============================================================
void checkEnvironment() {
  // Read DHT
  float h = dht.readHumidity();

  // Read analog temperature (backup)
  int val = analogRead(PIN_TEMP_ANA);
  float mv = (val / 4095.0) * 3300;
  float tempAna = mv / 10.0; // e.g. LM35, 10 mV/°C

  // DHT temperature:
  float t = dht.readTemperature();
  if (isnan(t)) t = currentTemp; // Read failed
  if (isnan(h)) h = currentHum;

  bool alarmCondition = false;

  // Rule: temp > 30 OR hum < 1 OR hum > 60
  if (t > 30.0 || h < 01.0 || h > 60.0) alarmCondition = true;

  currentTemp = t;
  currentHum = h;
  envAlarmActive = alarmCondition;

  if (envAlarmActive) {
    // Alarm actions
    // 1. Red LED blinks (handled in updateLeds)
    // 2. Audio (unless silenced)
    bool isSilenced = (millis() < envSilenceTimer);

    if (!isSilenced) {
      playTone(1000, 100); // Fast intermittent beep
    }

    // 3. On-screen warning (overrides the display on every check)
    lcd.setCursor(0, 0);
    lcd.print("ALERTA AMBIENTE!");
    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print((int)t);
    lcd.print(" H:");
    lcd.print((int)h);
    lcd.print("%   ");
  }
}

void updateLeds() {
  if (envAlarmActive) {
    // Blink red
    bool state = (millis() / 250) % 2;
    digitalWrite(PIN_LED_R, state);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);
  } else {
    // Normal: green while producing, blue otherwise
    digitalWrite(PIN_LED_R, LOW);
    if (currentState == STATE_PROD_RUNNING) {
      digitalWrite(PIN_LED_G, HIGH);
      digitalWrite(PIN_LED_B, LOW);
    } else {
      digitalWrite(PIN_LED_G, LOW);
      digitalWrite(PIN_LED_B, HIGH); // Blue on standby
    }
  }
}

void playTone(int freq, int duration) {
  // Uses the arduino-esp32 tone(); fall back to ledcWriteTone on older cores
  tone(PIN_AUDIO, freq, duration);
}
