// Firmware do Lambari-ROV — leitura de controle Bluetooth (Bluepad32) e
// mixagem de 3 motores.
//
// Baseado no exemplo oficial "Controller.ino" do Bluepad32
// (Ricardo Quesada, Apache 2.0), adaptado para ESP32 nativo — NÃO para
// placas com módulo NINA.
//
// IMPORTANTE: este firmware exige o board package "Bluepad32" instalado via
// Boards Manager, não a combinação "ESP32 by Espressif + biblioteca
// Bluepad32". Ver: https://github.com/ricardoquesada/bluepad32

#include <Bluepad32.h>

// ---------------------------------------------------------------------
// Pinagem — ajuste conforme a fiação real da sua dry box.
// Cada motor usa 2 pinos PWM (RPWM/LPWM) + 2 pinos de enable (R_EN/L_EN)
// no BTS7960. Os pinos de enable funcionam como um "kill switch" de
// hardware: o watchdog os derruba antes mesmo de zerar o PWM.
// ---------------------------------------------------------------------

struct MotorPins {
  uint8_t rpwm;
  uint8_t lpwm;
  uint8_t r_en;
  uint8_t l_en;
};

const MotorPins MOTOR_ESQUERDO = {25, 26, 32, 33};
const MotorPins MOTOR_DIREITO  = {27, 14, 15, 4};
const MotorPins MOTOR_VERTICAL = {18, 13, 22, 23}; 

const uint8_t CH_ESQ_R = 0, CH_ESQ_L = 1;
const uint8_t CH_DIR_R = 2, CH_DIR_L = 3;
const uint8_t CH_VER_R = 4, CH_VER_L = 5;

const uint32_t PWM_FREQ = 20000;
const uint8_t PWM_RES = 8;

const unsigned long WATCHDOG_TIMEOUT_MS = 300;
const int DEADZONE = 60;
const unsigned long LOOP_PERIOD_MS = 20;

ControllerPtr myController = nullptr;
unsigned long lastControllerUpdateMs = 0;
unsigned long nextLoopTimeMs = 0;
bool watchdogTripped = true;

void setupMotor(const MotorPins& m, uint8_t chR, uint8_t chL) {
  pinMode(m.r_en, OUTPUT);
  pinMode(m.l_en, OUTPUT);
  digitalWrite(m.r_en, LOW);
  digitalWrite(m.l_en, LOW);

  ledcSetup(chR, PWM_FREQ, PWM_RES);
  ledcAttachPin(m.rpwm, chR);
  ledcSetup(chL, PWM_FREQ, PWM_RES);
  ledcAttachPin(m.lpwm, chL);

  ledcWrite(chR, 0);
  ledcWrite(chL, 0);
}

void enableMotor(const MotorPins& m, bool enabled) {
  digitalWrite(m.r_en, enabled ? HIGH : LOW);
  digitalWrite(m.l_en, enabled ? HIGH : LOW);
}

void driveMotor(uint8_t chR, uint8_t chL, int value) {
  value = constrain(value, -255, 255);
  if (value >= 0) {
    ledcWrite(chR, value);
    ledcWrite(chL, 0);
  } else {
    ledcWrite(chR, 0);
    ledcWrite(chL, -value);
  }
}

void allMotorsOff() {
  driveMotor(CH_ESQ_R, CH_ESQ_L, 0);
  driveMotor(CH_DIR_R, CH_DIR_L, 0);
  driveMotor(CH_VER_R, CH_VER_L, 0);
  enableMotor(MOTOR_ESQUERDO, false);
  enableMotor(MOTOR_DIREITO, false);
  enableMotor(MOTOR_VERTICAL, false);
}

int applyDeadzone(int value) {
  if (abs(value) < DEADZONE) return 0;
  return value;
}

void onConnectedController(ControllerPtr ctl) {
  if (myController == nullptr) {
    Serial.println("Controle conectado.");
    myController = ctl;
    lastControllerUpdateMs = millis();
    watchdogTripped = false;
    ctl->setColorLED(0, 255, 0);
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    Serial.println("Controle desconectado — motores travados.");
    myController = nullptr;
    watchdogTripped = true;
    allMotorsOff();
  }
}

void setup() {
  Serial.begin(115200);

  setupMotor(MOTOR_ESQUERDO, CH_ESQ_R, CH_ESQ_L);
  setupMotor(MOTOR_DIREITO, CH_DIR_R, CH_DIR_L);
  setupMotor(MOTOR_VERTICAL, CH_VER_R, CH_VER_L);
  allMotorsOff();

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();

  nextLoopTimeMs = millis();
}

void loop() {
  bool dataUpdated = BP32.update();

  unsigned long now = millis();
  if (now < nextLoopTimeMs) return;
  nextLoopTimeMs += LOOP_PERIOD_MS;

  if (myController && myController->isConnected() && myController->isGamepad()) {
    if (dataUpdated) {
      lastControllerUpdateMs = now;
      watchdogTripped = false;
    }
  }

  if (now - lastControllerUpdateMs > WATCHDOG_TIMEOUT_MS) {
    if (!watchdogTripped) {
      Serial.println("Watchdog: sem sinal do controle — motores travados.");
      watchdogTripped = true;
    }
    allMotorsOff();
    return;
  }

  if (watchdogTripped || myController == nullptr) return;

  int forward  = applyDeadzone(-myController->axisY());
  int turn     = applyDeadzone(myController->axisX());
  int vertical = applyDeadzone(-myController->axisRY());

  int leftValue  = constrain(forward + turn, -512, 512);
  int rightValue = constrain(forward - turn, -512, 512);

  int leftPwm  = map(leftValue, -512, 512, -255, 255);
  int rightPwm = map(rightValue, -512, 512, -255, 255);
  int vertPwm  = map(vertical, -512, 512, -255, 255);

  enableMotor(MOTOR_ESQUERDO, true);
  enableMotor(MOTOR_DIREITO, true);
  enableMotor(MOTOR_VERTICAL, true);

  driveMotor(CH_ESQ_R, CH_ESQ_L, leftPwm);
  driveMotor(CH_DIR_R, CH_DIR_L, rightPwm);
  driveMotor(CH_VER_R, CH_VER_L, vertPwm);
}