#ifndef ROBOTMOTOR_H
#define ROBOTMOTOR_H
#include <Arduino.h>
#include "PinSetup.h"
#include "PidController.h"

#define BUDGE_TIMEOUT 200 // if command hasn't been received in this amount of ms, stop turning

enum motor_direction {CLOCKWISE = -1, COUNTER_CLOCKWISE = 1}; // defines motor directions
enum loop_state {OPEN_LOOP = 1, CLOSED_LOOP}; // defines whether motor control is open loop or closed loop
enum motor_type {DC_MOTOR = 1, POSITION_SERVO, CONTINUOUS_SERVO, STEPPER_MOTOR};

class RobotMotor {
  public:
  // these variables are set at start and normally don't change during the main loop
  int motorType;
  static int numMotors; // keeps track of how many motors there are
  int encoderPinA, encoderPinB;
  int limSwitchCw, limSwitchCcw, limSwitchFlex, limSwitchExtend;
  volatile int triggerState, limitSwitchState;
  elapsedMillis sinceTrigger;
  float gearRatio, gearRatioReciprocal; // calculating this beforehand improves speed of floating point calculations
  float encoderResolutionReciprocal; // calculating this beforehand improves speed of floating point calculations
  float maxJointAngle, minJointAngle; // joint angle limits, used to make sure the arm doesn't bend too far and break itself
  float minHardAngle, maxHardAngle, minSoftAngle, maxSoftAngle;
  bool hasAngleLimits; // a wrist which wants to turn infinitely will be constrained by angle limits
  bool isOpenLoop; // decides whether to use the PID or not
  bool hasRamping; // decides whether to ramp the speed in open loop
  volatile int rotationDirection;
  // int maxSpeed;
  PidController pidController; // used for speed and angle control
  // these variables change during the main loop
  volatile long encoderCount; // incremented inside encoder interrupts, keeps track of how much the motor shaft has rotated and in which direction
  volatile bool movementDone; // this variable is what allows the timer interrupts to make motors turn. can be updated within said interrupts
  elapsedMillis sinceBudgeCommand; // timeout for budge commands, elapsedMillis can't be volatile
  volatile bool isBudging;
  // setup functions
  RobotMotor();
  void attachEncoder(int encA, int encB, uint32_t port, int shift, int encRes);
  void attachLimitSwitches(char type, int switch1, int switch2);
  void setAngleLimits(float minHardAngle, float maxHardAngle, float minSoftAngle, float maxSoftAngle); // sets joint limits so the arm doesn't break from trying to reach physically impossible angles
  bool withinJointAngleLimits(float angle); // checks if angle is within limits
  bool hasEncoder;
  virtual void setVelocity(int motorDir, float motorSpeed) = 0; // sets motor speed and direction until next timer interrupt
  virtual void stopRotation(void) = 0;
  // void setMaxSpeed();
  int calcDirection(float error); // updates rotationDirection based on the angular error inputted
  bool setDesiredAngle(float angle); // if the angle is valid, update desiredAngle and return true. else return false.
  float getDesiredAngle(void); // return copy of the desired angle, not a reference to it
  virtual bool calcCurrentAngle(void) = 0;
  float getCurrentAngle(void);
  float getImaginedAngle(void);
  void setImaginedAngle(float angle); // for debugging mostly, overwrite current angle value
  void setSoftwareAngle(float angle);
  //void goToAngle(float angle) = 0;
  void switchDirectionLogic(void); // tells the motor to reverse the direction for a motor's control... does this need to be virtual?
  int getDirectionLogic(void); // returns the directionModifier;
  private:
  // doesn't really make sense to have any private variables for this parent class.
  // note that virtual functions must be public in order for them to be accessible from motorArray[]
  protected:
  // the following variables are specific to encoders
  uint32_t encoderPort; // address of the port connected to a particular encoder pin
  int encoderShift; // how many bits to shift over to find the encoder pin state
  int encoderResolution; // ticks per revolution
  volatile float currentAngle; // can be updated within timer interrupts
  volatile float imaginedAngle;
  float desiredAngle;
  int directionModifier;
};

int RobotMotor::numMotors = 0; // must initialize variable outside of class

RobotMotor::RobotMotor() {
  numMotors++;
  movementDone = true; // by default the movement has finished so the motors don't need to move
  hasAngleLimits = false; // only set to true when setAngleLimits() is called
  isOpenLoop = true; // by default don't use PID
  hasRamping = false; // by default don't ramp the speed
  imaginedAngle = 0.0;
  rotationDirection = 0; // by default invalid value
  directionModifier = 1; // this flips the direction sign if necessary;
  isBudging = false;
  triggerState = 0;
  limitSwitchState = 0;
}

void RobotMotor::attachEncoder(int encA, int encB, uint32_t port, int shift, int encRes) // :
// encoderPinA(encA), encoderPinB(encB), encoderPort(port), encoderShift(shift), encoderResolution(encRes)
{
  hasEncoder = true;
  encoderPinA = encA;
  encoderPinB = encB;
  encoderPort = port;
  encoderShift = shift;
  encoderResolution = encRes;
  encoderResolutionReciprocal = 1 / (float) encRes;
}

void RobotMotor::attachLimitSwitches(char type, int switch1, int switch2) {
  if (type == 'f') // flex type
  {
    limSwitchFlex = switch1;
    limSwitchExtend = switch2;
  }
  else
    if (type == 'c') // clock type
    {
      limSwitchCw = switch1;
      limSwitchCcw = switch2;
    }
}

void RobotMotor::setAngleLimits(float minH, float maxH, float minS, float maxS){
  minJointAngle = minS;
  maxJointAngle = maxS;
  minHardAngle = minH;
  maxHardAngle = maxH;
  minSoftAngle = minS;
  maxSoftAngle = maxS;
  hasAngleLimits = true;
}

bool RobotMotor::withinJointAngleLimits(float angle) {
  if (!hasAngleLimits || (angle > minJointAngle && angle < maxJointAngle))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool RobotMotor::setDesiredAngle(float angle) {
  if (withinJointAngleLimits(angle))
  {
    desiredAngle = angle;
    return true;
  }
  else
  {
    return false;
  }
}

float RobotMotor::getDesiredAngle(void) {
  return desiredAngle;
}

int RobotMotor::calcDirection(float error) {
  if (error >= 0)
  {
    rotationDirection = directionModifier * COUNTER_CLOCKWISE;
  }
  else
  {
    rotationDirection = directionModifier * CLOCKWISE;
  }
  return rotationDirection;
}

void RobotMotor::switchDirectionLogic(void) {
  directionModifier = directionModifier * -1;
}

int RobotMotor::getDirectionLogic(void)
{
  return directionModifier;
}

float RobotMotor::getCurrentAngle(void)
{
  return currentAngle;
}

float RobotMotor::getImaginedAngle(void)
{
  return imaginedAngle;
}

void RobotMotor::setImaginedAngle(float angle)
{
  imaginedAngle = angle;
}

void RobotMotor::setSoftwareAngle(float angle)
{
  if (isOpenLoop){
    imaginedAngle = angle;
  }
  else {
    // needs to be tested to make sure that a call to calcCurrentAngle will return the same thing as getCurrentAngle
    currentAngle = angle;
    encoderCount = angle * gearRatio * encoderResolution / 360.0;
  }
}

#endif