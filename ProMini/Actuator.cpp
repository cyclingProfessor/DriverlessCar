#include "ProMini.h"

//////////////////////////////////////////////////////////////////////////
// Utility function to bound the value of an int between two set values
//////////////////////////////////////////////////////////////////////////
static void bound(unsigned *what, unsigned low, unsigned high) {
  if (*what < low) {
    *what = low;
  }
  if (*what > high) {
    *what = high;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
Motor::Motor(int p1, int p2, int c) : dirPin1(p1), dirPin2(p2), ctrlPin(c) {
  pinMode(dirPin1, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(ctrlPin, OUTPUT);

  this->setDirection(FORWARDS);
  this->off();
}
void Motor::off() {
  analogWrite(ctrlPin, 0);
}

void Motor::setDirection(int direction) {
  if (direction == BACKWARDS) {
    digitalWrite(dirPin2, LOW);
    digitalWrite(dirPin1, HIGH);
  } else {
    digitalWrite(dirPin1, LOW);
    digitalWrite(dirPin2, HIGH);
  }
}

void Motor::setLevel(unsigned speed) {
  setSpeed(speed);
}

void Motor::setSpeed(unsigned value) {
  bound(&value, MIN_MOTOR_POWER, MAX_MOTOR_POWER);
  analogWrite(ctrlPin, value);
}

//////////////////////////////////////////////////////////////////////////////////////
void Steerer::start() {
  servo.attach(STEER_PIN, MIN_SERVO, MAX_SERVO);
  this->neutral();
}

void Steerer::setLevel(unsigned angle) {
  bound(&angle, NEUTRAL_ANGLE - 18, NEUTRAL_ANGLE + 25);
  steer(angle);
}

void Steerer::neutral() {
  steer(NEUTRAL_ANGLE);
}

void Steerer::steer(unsigned angle) {
  bound(&angle, MIN_ANGLE, MAX_ANGLE);
  servo.write(angle);
}


