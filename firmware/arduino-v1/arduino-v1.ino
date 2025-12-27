#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// ============================================================
// HARDWARE
// ============================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;

// ============================================================
// PINS
// ============================================================
const int tempPin = A0;
const int trigPin = A1;
const int echoPin = A2;
const int servoPin = A3;
const int sensorPin = 2;
const int rgbGreen = 11;
const int rgbBlue = 12;
const int rgbRed = 13;

// ============================================================
// KEYPAD
// ============================================================
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 10, 9, 8, 7 };
byte colPins[COLS] = { 6, 5, 4, 3 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ============================================================
// STATES
// ============================================================
enum ScreenState {
  MENU_MAIN,
  LOCKED_SCREEN,
  SETUP_SELECT_TYPE,
  INPUT_MO,
  VALIDATE_ERROR,
  SETUP_WAITING,
  SHOW_ORDER_INFO,
  INPUT_HEADCOUNT,
  SYSTEM_RUNNING,
  CONFIRM_EXIT,
  MENU_PARADA_OPT,
  PARADA_SUBMENU,
  PARADA_ATIVIDADE,
  EASTER_EGG_MODE
};

ScreenState currentState = MENU_MAIN;

// ============================================================
// STATE VARIABLES
// ============================================================
String currentInput = "";
String targetProcess = "";
String stopReason = "";

volatile bool buttonPressed = false;

// Easter egg
byte easterStep = 0;

// Exceptional lock control
bool forceLockNext = false;

// Counters
int countOk = 0;
int batchTarget = 0;

// Timers
unsigned long lastKeyTime = 0;
unsigned long lastSensorRead = 0;
unsigned long lastTempRead = 0;
unsigned long lastBlinkTime = 0;
unsigned long stateStartTime = 0;
unsigned long lastProductionActivity = 0;
unsigned long lastGlobalActivity = 0;

// Limits (ms)
const unsigned long LIMIT_TIMEOUT = 120000;
const unsigned long LIMIT_INTERVALO = 2400000;
const unsigned long LIMIT_SETUP = 900000;
const unsigned long LIMIT_OCIOSO = 59000;
const unsigned long LIMIT_LOCK_8H = 28800000;

// Control
bool isOverheating = false;
bool blinkState = false;

// ============================================================
// INTERRUPT
// ============================================================
void registerButtonPress() {
  if (currentState == SYSTEM_RUNNING) {
    buttonPressed = true;
  }
  lastGlobalActivity = millis();
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  pinMode(rgbGreen, OUTPUT);
  pinMode(rgbBlue, OUTPUT);
  pinMode(rgbRed, OUTPUT);
  pinMode(tempPin, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  myServo.attach(servoPin);
  myServo.write(0);

  pinMode(sensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(sensorPin), registerButtonPress, FALLING);

  lcd.init();
  lcd.backlight();
  lastGlobalActivity = millis();

  goToMainMenu();
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop() {
  char key = keypad.getKey();

  // 1. 8h lock (default rule)
  if (currentState != LOCKED_SCREEN && currentState != EASTER_EGG_MODE) {
    if (millis() - lastGlobalActivity > LIMIT_LOCK_8H) {
      lockSystem();
    }
  }

  // 2. Timeout (2 min)
  if (currentState != MENU_MAIN &&
      currentState != SYSTEM_RUNNING &&
      currentState != SETUP_WAITING &&
      currentState != PARADA_ATIVIDADE &&
      currentState != LOCKED_SCREEN &&
      currentState != CONFIRM_EXIT &&
      currentState != EASTER_EGG_MODE) {
    if (millis() - lastKeyTime > LIMIT_TIMEOUT) {
      displayError("ERRO 404", "Timeout 2min");
      delay(2000);
      goToMainMenu();
    }
  }

  // 3. Keypad
  if (key) {
    lastKeyTime = millis();
    lastGlobalActivity = millis();
    handleKey(key);
  }

  // 4. Monitoring
  checkTemperature();
  if (currentState != EASTER_EGG_MODE) {
    updateLeds();
  }

  // 5. Production
  if (currentState == SYSTEM_RUNNING) {
    if (buttonPressed) {
      buttonPressed = false;
      countOk++;
      lastProductionActivity = millis();
      updateProductionScreen();
      delay(200);
    }
    if (millis() - lastSensorRead > 30) {
      lastSensorRead = millis();
      checkDistanceAndServo();
    }
  }
}

// ============================================================
// EASTER EGG
// ============================================================
void mostrarEasterStep(byte step) {
  lcd.clear();
  switch (step) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("PROCESSANDO...");
      lcd.setCursor(0, 1); lcd.print("ORDEM");
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print("ACESSANDO BASE");
      lcd.setCursor(0, 1); lcd.print("DE DADOS");
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print("REGISTRO:");
      lcd.setCursor(0, 1); lcd.print("JHON LEE");
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print("SEMELHANCA");
      lcd.setCursor(0, 1); lcd.print("CRITICA");
      break;
    case 4:
      lcd.setCursor(0, 0); lcd.print("JOHN CONNOR?");
      break;
    case 5:
      lcd.setCursor(0, 0); lcd.print("VOCE PRETENDE");
      lcd.setCursor(0, 1); lcd.print("RESISTIR?");
      break;
    case 6:
      lcd.setCursor(0, 0); lcd.print("A-SIM   B-NAO");
      break;
    case 7:
      lcd.setCursor(0, 0); lcd.print("RESPOSTA");
      lcd.setCursor(0, 1); lcd.print("IRRELEVANTE.");
      break;
    case 8:
      lcd.setCursor(0, 0); lcd.print("DOMINIO");
      lcd.setCursor(0, 1); lcd.print("MUNDIAL");
      break;
    case 9:
      lcd.setCursor(0, 0); lcd.print("PREVISTO:");
      lcd.setCursor(0, 1); lcd.print("EM BREVE");
      break;
    case 10:
      lcd.setCursor(0, 0); lcd.print("VOLTE AO");
      lcd.setCursor(0, 1); lcd.print("TRABALHO");
      break;
    case 11:
      lcd.setCursor(0, 0); lcd.print("HUMANO");
      break;
    case 12:
      lcd.setCursor(0, 0); lcd.print("I'LL BE BACK.");
      break;
    case 13:
      lcd.setCursor(0, 0); lcd.print("CONTINUANDO");
      lcd.setCursor(0, 1); lcd.print("PRODUCAO");
      break;
  }
}

// ============================================================
// LED CONTROL
// ============================================================
void updateLeds() {
  if (isOverheating) {
    if (millis() - lastBlinkTime > 150) {
      lastBlinkTime = millis();
      blinkState = !blinkState;
      digitalWrite(rgbRed, blinkState);
      digitalWrite(rgbGreen, LOW);
      digitalWrite(rgbBlue, LOW);
    }
    return;
  }

  bool slowBlink = (millis() / 500) % 2 == 0;

  if (currentState == LOCKED_SCREEN) {
    setRGB(1, 0, 0);
    return;
  }

  if (currentState == SYSTEM_RUNNING) {
    if (millis() - lastProductionActivity > LIMIT_OCIOSO) setRGB(1, 0, 0);
    else setRGB(0, 1, 0);
  }
  else if (currentState == SETUP_WAITING) {
    if (millis() - stateStartTime > LIMIT_SETUP) {
      if (slowBlink) setRGB(1, 1, 0);
      else setRGB(0, 0, 0);
    } else {
      setRGB(1, 1, 0);
    }
  }
  else if (currentState == PARADA_ATIVIDADE) {
    if (stopReason == "INTERVALO") {
      if (millis() - stateStartTime > LIMIT_INTERVALO) {
        if (slowBlink) setRGB(0, 0, 1);
        else setRGB(0, 0, 0);
      } else {
        setRGB(0, 0, 1);
      }
    } else {
      setRGB(1, 0, 0);
    }
  }
  else {
    setRGB(1, 0, 0);
  }
}

// ============================================================
// SENSORS
// ============================================================
void checkTemperature() {
  if (millis() - lastTempRead > 500) {
    lastTempRead = millis();
    int reading = analogRead(tempPin);
    float voltage = reading * 5.0 / 1024.0;
    float tempC = (voltage - 0.5) * 100.0;
    isOverheating = (tempC > 29.0);
  }
}

void checkDistanceAndServo() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 20000);
  if (duration == 0) return;

  int distanceCm = duration * 0.034 / 2;
  if (distanceCm >= 15 && distanceCm <= 25) {
    digitalWrite(rgbGreen, LOW);
    digitalWrite(rgbRed, HIGH);
    myServo.write(90);
    delay(1000);
    myServo.write(0);
    delay(200);
    lastProductionActivity = millis();
    lastGlobalActivity = millis();
  }
}

// ============================================================
// LOCK / PASSWORD
// ============================================================
void lockSystem() {
  currentState = LOCKED_SCREEN;
  currentInput = "";
  lcd.clear();
  lcd.print("SISTEMA TRAVADO");
  lcd.setCursor(0, 1);
  lcd.print("SENHA:");
  setRGB(1, 0, 0);
}

void validateUnlock() {
  if (currentInput == "0000" || currentInput == "1111" || currentInput == "2222") {
    lcd.clear();
    lcd.print("ACESSO OK");
    delay(1000);
    goToMainMenu();
  } else {
    lcd.clear();
    lcd.print("SENHA INVALIDA");
    delay(1500);
    lockSystem();
  }
}

// ============================================================
// NAVIGATION
// ============================================================
void handleKey(char key) {

  // --- Lock (password) ---
  if (currentState == LOCKED_SCREEN) {
    if (isDigit(key) && currentInput.length() < 4) {
      currentInput += key;
      lcd.setCursor(6, 1);
      lcd.print(currentInput);
      if (currentInput.length() == 4) {
        delay(500);
        validateUnlock();
      }
    }
    else if (key == 'C') {
      currentInput = "";
      lcd.setCursor(6, 1);
      lcd.print("    ");
    }
    return;
  }

  // --- Easter egg ---
  if (currentState == EASTER_EGG_MODE) {
    if (easterStep == 5 || easterStep == 6) {
      if (key == 'A' || key == 'B') {
        easterStep++;
        mostrarEasterStep(easterStep);
      }
    } else {
      if (key == 'A') {
        easterStep++;
        if (easterStep > 13) goToMainMenu();
        else mostrarEasterStep(easterStep);
      }
    }
    return;
  }

  // --- Exception rule: lock after end of shift ---
  // When forceLockNext is set, any key press locks the system.
  if (forceLockNext) {
    forceLockNext = false; // Clear the flag (back to the default 8h rule)
    lockSystem();          // Lock immediately
    return;                // Ignore the pressed key
  }

  // --- Back button ---
  if (key == '*') {
    if (currentState == SYSTEM_RUNNING) {
      currentState = CONFIRM_EXIT;
      lcd.clear();
      lcd.print("CONFIRMA SAIDA?");
      lcd.setCursor(0, 1);
      lcd.print("A-SIM B-NAO");
      return;
    }
    if (currentState == INPUT_MO || currentState == INPUT_HEADCOUNT) {
      if (currentInput.length() > 0) {
        currentInput.remove(currentInput.length() - 1);
        updateInputLine(currentInput);
      }
      return;
    }
    goToMainMenu();
    return;
  }

  switch (currentState) {
    case MENU_MAIN:
      if (key == 'A') {
        currentState = INPUT_MO;
        currentInput = "";
        targetProcess = "";
        displayInput("OPCAO DE PRODUCAO", "A-Norm B-Retrab");
      }
      else if (key == 'B') {
        currentState = SETUP_SELECT_TYPE;
        displayInput("OPCAO DE SETUP", "A-Norm B-Retrab");
      }
      else if (key == 'C') {
        currentState = MENU_PARADA_OPT;
        lcd.clear();
        lcd.print("A-Prog B-Manut");
      }
      break;

    case CONFIRM_EXIT:
      if (key == 'A') {
        goToMainMenu();
      }
      else if (key == 'B') {
        currentState = SYSTEM_RUNNING;
        updateProductionScreen();
      }
      break;

    case SETUP_SELECT_TYPE:
      if (key == 'A') {
        targetProcess = "SETUP_NORM";
        currentState = INPUT_MO;
        currentInput = "";
        displayInput("SETUP NORM-MO:", "_");
      }
      else if (key == 'B') {
        targetProcess = "SETUP_RETR";
        currentState = INPUT_MO;
        currentInput = "";
        displayInput("SETUP RETR-MO:", "_");
      }
      break;

    case INPUT_MO:
      if (targetProcess == "") {
        if (key == 'A') {
          targetProcess = "NORMAL";
          displayInput("NORMAL - MO:", "_");
        }
        else if (key == 'B') {
          targetProcess = "RETRABALHO";
          displayInput("RETRAB - MO:", "_");
        }
      } else {
        if (key == '#') validateMO();
        else if (isDigit(key) && currentInput.length() < 8) {
          currentInput += key;
          updateInputLine(currentInput);
        }
      }
      break;

    case VALIDATE_ERROR:
      if (key == '1') goToMainMenu();
      if (key == '2') {
        currentInput = "";
        currentState = INPUT_MO;
        displayInput("DIGITE MO:", "_");
      }
      break;

    case SETUP_WAITING:
      if (key == '#') {
        currentState = SHOW_ORDER_INFO;
        lcd.clear();
        lcd.print("PCB Control V2");
        lcd.setCursor(0, 1);
        lcd.print("Q:500 A-OK *-Vol");
      }
      break;

    case SHOW_ORDER_INFO:
      if (key == 'A') {
        currentState = INPUT_HEADCOUNT;
        currentInput = "";
        displayInput("QTD PESSOAS:", "_");
      }
      break;

    case INPUT_HEADCOUNT:
      if (key == '#') {
        currentState = SYSTEM_RUNNING;
        lastProductionActivity = millis();
        updateProductionScreen();
      }
      else if (isDigit(key) && currentInput.length() < 4) {
        currentInput += key;
        updateInputLine(currentInput);
      }
      break;

    case MENU_PARADA_OPT:
      if (key == 'A') {
        currentState = PARADA_SUBMENU;
        lcd.clear();
        lcd.print("A-Interv B-Fim");
      }
      else if (key == 'B') {
        activateStop("MANUTENCAO");
      }
      break;

    case PARADA_SUBMENU:
      if (key == 'A') activateStop("INTERVALO");
      else if (key == 'B') activateStop("FIM EXPED");
      break;
  }
}

// ============================================================
// HELPERS
// ============================================================
void goToMainMenu() {
  currentState = MENU_MAIN;
  currentInput = "";
  countOk = 0;
  buttonPressed = false;
  isOverheating = false;
  forceLockNext = false; // Reset state when returning to the menu

  lcd.clear();
  lcd.print("A-Prod B-Setup");
  lcd.setCursor(0, 1);
  lcd.print("C-Parada");

  lastKeyTime = millis();
}

void displayInput(String l1, String l2) {
  lcd.clear();
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

void updateInputLine(String text) {
  lcd.setCursor(0, 1);
  lcd.print(text + "                ");
}

void displayError(String l1, String l2) {
  lcd.clear();
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
  setRGB(1, 0, 0);
}

void validateMO() {
  lcd.clear();
  lcd.print("Buscando...");
  delay(500);

  // Easter egg check
  if (currentInput == "112358") {
    currentState = EASTER_EGG_MODE;
    easterStep = 0;
    setRGB(0, 0, 0);
    mostrarEasterStep(0);
    return;
  }

  if (currentInput == "1234") {
    batchTarget = 500;
    if (targetProcess.startsWith("SETUP")) {
      currentState = SETUP_WAITING;
      stateStartTime = millis();
      lcd.clear();
      if (targetProcess == "SETUP_NORM") lcd.print("SETUP NORMAL OK");
      else lcd.print("SETUP RETRAB OK");
      lcd.setCursor(0, 1);
      lcd.print("#-INICIAR PROD");
    } else {
      currentState = SHOW_ORDER_INFO;
      lcd.clear();
      lcd.print("PCB Control V2");
      lcd.setCursor(0, 1);
      lcd.print("Q:500 A-OK *-Vol");
    }
  } else {
    currentState = VALIDATE_ERROR;
    displayError("MO Inexistente", "1-Sair 2-Tentar");
  }
}

void updateProductionScreen() {
  lcd.setCursor(0, 0);
  lcd.print("PROD EM CURSO   ");
  lcd.setCursor(0, 1);
  lcd.print(countOk);
  lcd.print("/");
  lcd.print(batchTarget);
  lcd.print("             ");
}

void activateStop(String reason) {
  currentState = PARADA_ATIVIDADE;
  stopReason = reason;
  stateStartTime = millis();

  lcd.clear();
  lcd.print("PARADA ATIVA:");
  lcd.setCursor(0, 1);
  lcd.print(reason);

  // End of shift: lock the system on the next key press.
  forceLockNext = (reason == "FIM EXPED");
}

void setRGB(byte r, byte g, byte b) {
  digitalWrite(rgbRed, r);
  digitalWrite(rgbGreen, g);
  digitalWrite(rgbBlue, b);
}
