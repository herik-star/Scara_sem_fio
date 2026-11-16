/*
   Arduino Uno - Controle SCARA via CNC Shield V3 (Otimizado)
*/
#include <AccelStepper.h>
#include <Servo.h>
#include <math.h>

// Fins de Curso (Limit Switches)
#define limitSwitch1 11 // Z+ na CNC Shield (Ombro J1)
#define limitSwitch2 10 // Y+ na CNC Shield (Cotovelo J2)
#define limitSwitch3 9  // X+ na CNC Shield (Rotação Garra J3)
#define limitSwitch4 A3 // CoolEn na CNC Shield (Eixo Z)

#define EN_PIN 8        // Habilita os drivers na CNC Shield
#define GRIPPER_PIN A0  // Abort na CNC Shield (Servo da Garra)

// Motores de Passo (Interface 1 = Driver STEP/DIR)
AccelStepper stepper1(1, 2, 5);   // Junta 1 (Ombro)
AccelStepper stepper2(1, 3, 6);   // Junta 2 (Cotovelo)
AccelStepper stepper3(1, 4, 7);   // Junta 3 (Rotação Garra)
AccelStepper stepper4(1, 12, 13); // Eixo Z (Altura)

Servo gripperServo;

#define MAX_POSITIONS 40

const float theta1AngleToSteps = 44.444444;
const float theta2AngleToSteps = 35.555555;
const float phiAngleToSteps = 10.0;
const float zDistanceToSteps = 100.0;

int16_t data[10];
int16_t theta1Array[MAX_POSITIONS];
int16_t theta2Array[MAX_POSITIONS];
int16_t phiArray[MAX_POSITIONS];
int16_t zArray[MAX_POSITIONS];
uint8_t gripperArray[MAX_POSITIONS];

byte positionsCounter = 0;
int previousGripper = -1;

char serialBuffer[64];
byte bufferIndex = 0;

void setup() {
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  pinMode(limitSwitch1, INPUT_PULLUP);
  pinMode(limitSwitch2, INPUT_PULLUP);
  pinMode(limitSwitch3, INPUT_PULLUP);
  pinMode(limitSwitch4, INPUT_PULLUP);

  Serial.begin(115200);

  stepper1.setMaxSpeed(4000); stepper1.setAcceleration(2000);
  stepper2.setMaxSpeed(4000); stepper2.setAcceleration(2000);
  stepper3.setMaxSpeed(4000); stepper3.setAcceleration(2000);
  stepper4.setMaxSpeed(4000); stepper4.setAcceleration(2000);

  // Valores iniciais seguros antes do 1º comando HTML
  data[2] = 0; data[3] = 0; data[4] = 0; data[5] = 100;
  data[7] = 500; data[8] = 500;

  gripperServo.attach(GRIPPER_PIN, 600, 2500);
  data[6] = 100;
  gripperServo.write(data[6]);
  previousGripper = data[6];
  delay(500);

  homing();
}

void loop() {
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    if (inChar == '\n') {
      serialBuffer[bufferIndex] = '\0';
      parseData();
      bufferIndex = 0;
      processCommands();
    } else if (bufferIndex < 63) {
      serialBuffer[bufferIndex++] = inChar;
    }
  }

  // Modo Automático (Executar Programa Salvo)
  while (data[1] == 1) {
    stepper1.setSpeed(data[7]); stepper2.setSpeed(data[7]);
    stepper3.setSpeed(data[7]); stepper4.setSpeed(data[7]);
    stepper1.setAcceleration(data[8]); stepper2.setAcceleration(data[8]);
    stepper3.setAcceleration(data[8]); stepper4.setAcceleration(data[8]);

    for (byte i = 0; i < positionsCounter; i++) {
      if (checkSerialStop()) break;

      stepper1.moveTo(theta1Array[i]); stepper2.moveTo(theta2Array[i]);
      stepper3.moveTo(phiArray[i]); stepper4.moveTo(zArray[i]);

      while (stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0 || 
             stepper3.distanceToGo() != 0 || stepper4.distanceToGo() != 0) {
        stepper1.run(); stepper2.run(); stepper3.run(); stepper4.run();
      }

      if (i == 0 || gripperArray[i] != gripperArray[i - 1]) {
        gripperServo.write(gripperArray[i]);
        previousGripper = gripperArray[i];
        delay(400);
      }
    }
  }

  // Modo Manual (Sliders do HTML)
  stepper1.moveTo((int16_t)(data[2] * theta1AngleToSteps));
  stepper2.moveTo((int16_t)(data[3] * theta2AngleToSteps));
  stepper3.moveTo((int16_t)(data[4] * phiAngleToSteps));
  stepper4.moveTo((int16_t)(data[5] * zDistanceToSteps));

  stepper1.setSpeed(data[7]); stepper2.setSpeed(data[7]);
  stepper3.setSpeed(data[7]); stepper4.setSpeed(data[7]);
  stepper1.setAcceleration(data[8]); stepper2.setAcceleration(data[8]);
  stepper3.setAcceleration(data[8]); stepper4.setAcceleration(data[8]);

  while (stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0 || 
         stepper3.distanceToGo() != 0 || stepper4.distanceToGo() != 0) {
    stepper1.run(); stepper2.run(); stepper3.run(); stepper4.run();
  }

  if (data[6] != previousGripper && data[6] >= 0 && data[6] <= 180) {
    gripperServo.write(data[6]);
    previousGripper = data[6];
    delay(150);
  }
}

void parseData() {
  char *str = strtok(serialBuffer, ",");
  byte i = 0;
  while (str != NULL && i < 9) {
    data[i++] = atoi(str);
    str = strtok(NULL, ",");
  }
}

void processCommands() {
  if (data[0] == 1) { // Salvar Posição
    if (positionsCounter < MAX_POSITIONS) {
      theta1Array[positionsCounter] = data[2] * theta1AngleToSteps;
      theta2Array[positionsCounter] = data[3] * theta2AngleToSteps;
      phiArray[positionsCounter] = data[4] * phiAngleToSteps;
      zArray[positionsCounter] = data[5] * zDistanceToSteps;
      gripperArray[positionsCounter] = (uint8_t)data[6];
      positionsCounter++;
    }
    data[0] = 0;
  }
  if (data[0] == 2) { // Limpar Posições
    positionsCounter = 0;
    data[0] = 0;
  }
}

bool checkSerialStop() {
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    if (inChar == '\n') {
      serialBuffer[bufferIndex] = '\0';
      parseData();
      bufferIndex = 0;
      if (data[1] == 0) return true;
    } else if (bufferIndex < 63) {
      serialBuffer[bufferIndex++] = inChar;
    }
  }
  return false;
}

void homing() {
  auto homeAxis = [](AccelStepper &motor, int limitPin, int speed, long startPos, long targetPos) {
    while (digitalRead(limitPin) == 1) {
      motor.setSpeed(speed);
      motor.runSpeed();
    }
    motor.setCurrentPosition(startPos);
    delay(20);
    motor.moveTo(targetPos);
    while (motor.currentPosition() != targetPos) {
      motor.run();
    }
  };
  
  homeAxis(stepper4, limitSwitch4, 1500, 17000, 10000);
  homeAxis(stepper3, limitSwitch3, -1100, -1662, 0);
  homeAxis(stepper2, limitSwitch2, -1300, -5420, 0);
  homeAxis(stepper1, limitSwitch1, -1200, -8055, 0);
}