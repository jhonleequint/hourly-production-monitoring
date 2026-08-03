#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <IRremote.h>

// ============================================================
// PINOUT
// ============================================================
#define PIN_BUZZER 4
#define PIN_TRIG 5
#define PIN_ECHO 18
#define PIN_DHT 15
#define PIN_SERVO 2
#define PIN_IR 35
#define PIN_RED 23
#define PIN_GREEN 19
#define PIN_BLUE 32

// ============================================================
// SETTINGS
// ============================================================
#define VALID_MO "1234"
#define TEMP_LIMIT 29.0
#define HUM_LIMIT 80.0
#define IDLE_LIMIT_MS 59000
#define LOCK_TIMEOUT_MS 28800000
#define DIST_MIN 15
#define DIST_MAX 25

// ============================================================
// OBJECTS
// ============================================================
Adafruit_SSD1306 display(128, 64, &Wire, -1);
DHT dht(PIN_DHT, DHT22);
Servo servoSort;
IRrecv irrecv(PIN_IR);
decode_results results;

const byte LINHAS = 4;
const byte COLUNAS = 3;

char keys[LINHAS][COLUNAS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

byte pinLin[LINHAS] = { 13, 12, 14, 27 };
byte pinCol[COLUNAS] = { 26, 25, 33 };

Keypad keypad = Keypad(makeKeymap(keys), pinLin, pinCol, LINHAS, COLUNAS);

// ============================================================
// STATES (MENU - FIXED)
// ============================================================
enum State {
  LOCKED_SCREEN,
  MENU_MAIN,
  PROD_SELECT_TYPE,
  INPUT_MO,
  VALIDATE_ERROR,
  SHOW_ORDER_INFO,
  INPUT_HEADCOUNT,
  SYSTEM_RUNNING,
  CONFIRM_EXIT,
  SETUP_SELECT_TYPE,
  SETUP_WAITING,
  MENU_PARADA_OPT,
  PARADA_SUBMENU,
  PARADA_ATIVIDADE
};

volatile State currentState = LOCKED_SCREEN;

// ============================================================
// VARIABLES
// ============================================================
String currentInput = "";
String targetProcess = "";
String stopReason = "";
int countOk = 0;
unsigned long lastActivity = 0;
unsigned long lastProdPulse = 0;
bool forceLockNext = false;

// Sensor values shared between tasks
volatile float sharedTemp = 0.0;
volatile float sharedHum = 0.0;

// ============================================================
// PROTOTYPES
// ============================================================
void processKeyLogic(char key);
void handleAndon();
void updateDisplay();

// --- Tasks ---
void TaskInterface(void *pvParameters);
void TaskLogic(void *pvParameters);
void TaskSensors(void *pvParameters);

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  // Make sure the buzzer starts off
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }
  display.setTextColor(WHITE);

  dht.begin();

  // Servo setup (owns Timer 1)
  ESP32PWM::allocateTimer(1);
  servoSort.setPeriodHertz(50);
  servoSort.attach(PIN_SERVO, 500, 2400);
  servoSort.write(0);

  irrecv.enableIRIn();

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);

  lastActivity = millis();

  xTaskCreatePinnedToCore(TaskInterface, "Interface", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskLogic, "Logic", 8192, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(TaskSensors, "Sensors", 4096, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);
}

// ============================================================
// INTERFACE TASK
// ============================================================
void TaskInterface(void *pvParameters) {
  for (;;) {
    char key = keypad.getKey();
    if (key) {
      lastActivity = millis();
      if (forceLockNext) {
        forceLockNext = false;
        currentState = LOCKED_SCREEN;
        currentInput = "";
      } else {
        processKeyLogic(key);
      }
    }

    if (irrecv.decode(&results)) {
      if (currentState == SYSTEM_RUNNING) {
        countOk++;
        lastProdPulse = millis();
        // Short IR beep (too brief to interfere with the servo)
        tone(PIN_BUZZER, 4000, 50);
      }
      irrecv.resume();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ============================================================
// SENSORS TASK (servo/buzzer coordination)
// ============================================================
void TaskSensors(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(2000)); // Initial delay so the sensors can settle

  for (;;) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) sharedTemp = t;
    if (!isnan(h)) sharedHum = h;

    if (currentState == SYSTEM_RUNNING) {
      digitalWrite(PIN_TRIG, LOW);
      delayMicroseconds(2);
      digitalWrite(PIN_TRIG, HIGH);
      delayMicroseconds(10);
      digitalWrite(PIN_TRIG, LOW);

      long dur = pulseIn(PIN_ECHO, HIGH);
      float d = dur * 0.034 / 2;

      // Filtering and sorting logic
      if (d > 2 && d < 400) {
        if (d >= DIST_MIN && d <= DIST_MAX) {

          // --- Key point ---
          // 1. Stop the buzzer (frees the timer for the servo)
          noTone(PIN_BUZZER);

          // 2. Move the servo
          servoSort.write(90);
          vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for the reject cycle
          servoSort.write(0);

          // The buzzer resumes on the next handleAndon() pass
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(150));
  }
}

// ============================================================
// LOGIC TASK
// ============================================================
void TaskLogic(void *pvParameters) {
  for (;;) {
    if (millis() - lastActivity > LOCK_TIMEOUT_MS && currentState != LOCKED_SCREEN) {
      currentState = LOCKED_SCREEN;
      currentInput = "";
    }
    handleAndon();
    updateDisplay();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ============================================================
// MENU (FIXED)
// ============================================================
void processKeyLogic(char key) {
  switch (currentState) {
    case LOCKED_SCREEN:
      if (key == 'C' || key == '*') {
        currentInput = "";
      } else {
        currentInput += key;
        if (currentInput.length() == 4) {
          if (currentInput == "0000" || currentInput == "1111" || currentInput == "2222") {
            currentState = MENU_MAIN;
          }
          currentInput = "";
        }
      }
      break;

    case MENU_MAIN:
      if (key == '1') { currentState = PROD_SELECT_TYPE; targetProcess = ""; }
      if (key == '2') { currentState = SETUP_SELECT_TYPE; targetProcess = ""; }
      if (key == '3') currentState = MENU_PARADA_OPT;
      break;

    case PROD_SELECT_TYPE:
      if (key == '1') { targetProcess = "NORMAL"; currentState = INPUT_MO; currentInput = ""; }
      if (key == '2') { targetProcess = "RETRAB"; currentState = INPUT_MO; currentInput = ""; }
      break;

    case INPUT_MO:
      if (key == '#') {
        if (currentInput == VALID_MO) currentState = SHOW_ORDER_INFO;
        else currentState = VALIDATE_ERROR;
      } else if (key == '*') {
        if (currentInput.length() > 0) currentInput.remove(currentInput.length() - 1);
      } else {
        currentInput += key;
      }
      break;

    case VALIDATE_ERROR:
      if (key == '1') currentState = MENU_MAIN;
      if (key == '2') { currentState = INPUT_MO; currentInput = ""; }
      break;

    case SHOW_ORDER_INFO:
      if (key == '1') {
        if (targetProcess.startsWith("SETUP")) currentState = SETUP_WAITING;
        else { currentState = INPUT_HEADCOUNT; currentInput = ""; }
      }
      if (key == '*') currentState = MENU_MAIN;
      break;

    case INPUT_HEADCOUNT:
      if (key == '#') {
        currentState = SYSTEM_RUNNING;
        countOk = 0;
        lastProdPulse = millis();
      } else {
        currentInput += key;
      }
      break;

    case SYSTEM_RUNNING:
      if (key == '*') currentState = CONFIRM_EXIT;
      break;

    case CONFIRM_EXIT:
      if (key == '1') currentState = MENU_MAIN;
      if (key == '2') currentState = SYSTEM_RUNNING;
      break;

    case SETUP_SELECT_TYPE:
      if (key == '1') { targetProcess = "SETUP NORM"; currentState = INPUT_MO; currentInput = ""; }
      if (key == '2') { targetProcess = "SETUP RETR"; currentState = INPUT_MO; currentInput = ""; }
      break;

    case SETUP_WAITING:
      if (key == '#') { currentState = SYSTEM_RUNNING; countOk = 0; }
      break;

    case MENU_PARADA_OPT:
      if (key == '1') currentState = PARADA_SUBMENU;
      if (key == '2') { stopReason = "MANUTENCAO"; currentState = PARADA_ATIVIDADE; }
      break;

    case PARADA_SUBMENU:
      if (key == '1') { stopReason = "INTERVALO"; currentState = PARADA_ATIVIDADE; }
      if (key == '2') {
        stopReason = "FIM EXPED";
        currentState = PARADA_ATIVIDADE;
        forceLockNext = true;
      }
      break;

    case PARADA_ATIVIDADE:
      if (stopReason != "FIM EXPED") currentState = MENU_MAIN;
      break;
  }
}

// ============================================================
// PERIPHERALS (ANDON)
// ============================================================
void handleAndon() {
  int r = 0, g = 0, b = 0;

  // Alarm takes priority.
  // Skip until the sensor has a reading (temp > 0.1) to avoid beeping at power-on.
  if (sharedTemp > 0.1 && (sharedTemp > TEMP_LIMIT || sharedHum > HUM_LIMIT)) {
    if ((millis() / 250) % 2 == 0) {
      r = 1;
      // Beep only while the servo is idle.
      // (Not tracked here: TaskSensors has priority and calls noTone()
      // before moving the servo, which cuts this tone() when needed.)
      tone(PIN_BUZZER, 1000);
    } else {
      noTone(PIN_BUZZER);
    }
  } else {
    // No alarm: keep silent.
    // (Calling noTone() every pass can cut the IR beep short;
    // ideally it would only stop a tone started here.
    // Kept simple so the alarm always stops.)
    if (sharedTemp <= TEMP_LIMIT && sharedHum <= HUM_LIMIT) {
      // May cut the 50 ms IR beep.
      // Acceptable for now.
      noTone(PIN_BUZZER);
    }

    switch (currentState) {
      case LOCKED_SCREEN:
        b = 1;
        break;
      case SYSTEM_RUNNING:
        if (millis() - lastProdPulse > IDLE_LIMIT_MS) r = 1;
        else g = 1;
        break;
      case SETUP_WAITING:
        if ((millis() / 500) % 2 == 0) { r = 1; g = 1; }
        else { r = 1; g = 1; }
        break;
      case PARADA_ATIVIDADE:
        if (stopReason == "INTERVALO") b = 1;
        else r = 1;
        break;
      default:
        r = 1; g = 1; b = 1;
        break;
    }
  }

  digitalWrite(PIN_RED, r);
  digitalWrite(PIN_GREEN, g);
  digitalWrite(PIN_BLUE, b);
}

// ============================================================
// DISPLAY
// ============================================================
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  switch (currentState) {
    case LOCKED_SCREEN:
      display.println("SISTEMA TRAVADO");
      display.println("SENHA:");
      display.print(currentInput);
      break;

    case MENU_MAIN:
      display.println("1-Prod 2-Setup");
      display.println("3-Parada");
      break;

    case PROD_SELECT_TYPE:
    case SETUP_SELECT_TYPE:
      display.println("OPCAO:");
      display.println("1-Norm 2-Retrab");
      break;

    case INPUT_MO:
      display.print(targetProcess);
      display.println("-MO:");
      display.println(currentInput);
      display.println("#-Ok *-Del");
      break;

    case VALIDATE_ERROR:
      display.println("MO INEXISTENTE");
      display.println("1-Sair 2-Tentar");
      break;

    case SHOW_ORDER_INFO:
      display.println("PCB Control V2");
      display.print("Q:500 ");
      display.println("1-OK");
      display.println("*-Voltar");
      break;

    case INPUT_HEADCOUNT:
      display.println("QTD PESSOAS:");
      display.println(currentInput);
      display.println("#-Iniciar");
      break;

    case SYSTEM_RUNNING:
      display.println("PROD EM CURSO");
      display.print(countOk);
      display.print("/");
      display.println("500");
      display.print("T:");
      display.print(sharedTemp, 1);
      display.print("C ");
      display.print("H:");
      display.print(sharedHum, 0);
      display.println("%");
      break;

    case CONFIRM_EXIT:
      display.println("CONFIRMA SAIDA?");
      display.println("1-SIM 2-NAO");
      break;

    case SETUP_WAITING:
      display.println("SETUP OK");
      display.println("#-INICIAR PROD");
      break;

    case MENU_PARADA_OPT:
      display.println("1-Prog 2-Manut");
      break;

    case PARADA_SUBMENU:
      display.println("1-Interv 2-Fim");
      break;

    case PARADA_ATIVIDADE:
      display.println("PARADA ATIVA:");
      display.println(stopReason);
      if (stopReason == "FIM EXPED") display.println("[BLOQUEIO PROX]");
      break;
  }
  display.display();
}
