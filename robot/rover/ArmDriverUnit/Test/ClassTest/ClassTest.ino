#include "PinSetup.h"
#include "StepperMotor.h"
#include "DCMotor.h"
#include "ServoMotor.h"

#define BAUD_RATE 115200 // serial baud rate
#define BUFFER_SIZE 100  // size of the buffer for the serial commands

int motor;

struct budgeInfo {
  int whichMotor;
  int whichDir;
  int whichSpeed;
  unsigned int whichTime;
} budgeCommand;

char serialBuffer[BUFFER_SIZE]; // serial buffer used for early- and mid-stage tesing without ROSserial
int tempSpeedVar;
unsigned int tempTimeVar;

StepperMotor motor1(M1_ENABLE_PIN, M1_DIR_PIN, M1_STEP_PIN, M1_ENCODER_A, M1_ENCODER_B);
//DCMotor motor2(M2_PWM_PIN, M2_ENCODER_A, M2_ENCODER_B); // sabertooth
DCMotor motor2(M2_DIR_PIN, M2_PWM_PIN, M2_ENCODER_A, M2_ENCODER_B); // for new driver
StepperMotor motor3(M3_ENABLE_PIN, M3_DIR_PIN, M3_STEP_PIN, M3_ENCODER_A, M3_ENCODER_B);
StepperMotor motor4(M4_ENABLE_PIN, M4_DIR_PIN, M4_STEP_PIN, M4_ENCODER_A, M4_ENCODER_B);
ServoMotor motor5(M5_PWM_PIN, M5_ENCODER_A, M5_ENCODER_B);
ServoMotor motor6(M6_PWM_PIN, M6_ENCODER_A, M6_ENCODER_B);

void setup() {
  pinSetup();
  Serial.begin(BAUD_RATE); Serial.setTimeout(50); // checks serial port every 50ms
}

void loop() {

  if (Serial.available()) { // if a message was sent to the Teensy
    Serial.readBytesUntil(10, serialBuffer, BUFFER_SIZE); // read through it until NL
    Serial.print("GOT: "); Serial.println(serialBuffer); // send back what was received

    char* msgElem = strtok(serialBuffer, " "); // look for first element (first tag)
    if (String(msgElem) == "motor") { // msgElem is a char array so it's safer to convert to string first
      msgElem = strtok(NULL, " "); // go to next msg element (motor number)
      motor = atoi(msgElem);
      if (motor > 0 && motor < 10) budgeCommand.whichMotor = motor;
      else Serial.println("bad motor number");
      Serial.print("parsed motor "); Serial.println(budgeCommand.whichMotor);
      msgElem = strtok(NULL, " "); // find the next message element (direction tag)
      if (String(msgElem) == "direction") { // msgElem is a char array so it's safer to convert to string first
        msgElem = strtok(NULL, " "); // go to next msg element (direction)
        switch (*msgElem) { // determines motor direction
          case '0': // arbitrarily (for now) decided 0 is clockwise
            budgeCommand.whichDir = CLOCKWISE;
            Serial.println("parsed direction clockwise");
            break;
          case '1': // arbitrarily (for now) decided 1 is counter-clockwise
            budgeCommand.whichDir = COUNTER_CLOCKWISE;
            Serial.println("parsed direction counter-clockwise");
            break;
        }
      }
      msgElem = strtok(NULL, " "); // find the next message element (speed tag)
      if (String(msgElem) == "speed") { // msgElem is a char array so it's safer to convert to string first
        msgElem = strtok(NULL, " "); // find the next message element (integer representing speed level)
        tempSpeedVar = atoi(msgElem); // converts to int
        if (tempSpeedVar <= MAX_SPEED) { // make sure the subtraction made sense. Tf it's above 9, it doesn't
          budgeCommand.whichSpeed = tempSpeedVar; // set the actual speed
          Serial.print("parsed speed level: "); Serial.println(budgeCommand.whichSpeed);
        }
      }
      msgElem = strtok(NULL, " "); // find the next message element (time tag)
      if (String(msgElem) == "time") { // msgElem is a char array so it's safer to convert to string first
        msgElem = strtok(NULL, " "); // find the next message element (time in seconds)
        tempTimeVar = atoi(msgElem); // converts to int
        if (tempTimeVar <= MAX_BUDGE_TIME && tempTimeVar >= MIN_BUDGE_TIME) { // don't allow budge movements to last a long time
          budgeCommand.whichTime = tempTimeVar;
          Serial.print("parsed time interval "); Serial.print(budgeCommand.whichTime); Serial.println("ms");
        }
      }
    }
    memset(serialBuffer, 0, BUFFER_SIZE); //empty the buffer
  }

  if (budgeCommand.whichMotor > 0 && budgeCommand.whichMotor <= 6) {
    Serial.print("motor "); Serial.print(budgeCommand.whichMotor); Serial.println(" to move");
  }
  switch (budgeCommand.whichMotor) {
    case MOTOR1:
      motor1.budge(budgeCommand.whichDir, budgeCommand.whichSpeed, budgeCommand.whichTime);
      break;
    case MOTOR2:
      motor2.budge(budgeCommand.whichDir, budgeCommand.whichSpeed, budgeCommand.whichTime);
      break;
    case MOTOR3:
      motor3.budge(budgeCommand.whichDir, budgeCommand.whichSpeed, budgeCommand.whichTime);
      break;
    case MOTOR4:
      motor4.budge(budgeCommand.whichDir, budgeCommand.whichSpeed, budgeCommand.whichTime);
      break;
    case MOTOR5:
      motor5.budge(budgeCommand.whichDir, budgeCommand.whichSpeed, budgeCommand.whichTime);
      break;
    case MOTOR6:
      motor6.budge(budgeCommand.whichDir, budgeCommand.whichSpeed, budgeCommand.whichTime);
      break;
  }
  budgeCommand.whichMotor = 0; // reset whichMotor so the microcontroller doesn't try to move a motor next loop
  // in practice perhaps the whole struct should be destroyed, otherwise it must be reset each time
}

