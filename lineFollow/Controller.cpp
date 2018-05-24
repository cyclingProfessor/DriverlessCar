#include "lineFollow.h"

//////////////////////////////////////////////////////////////////////////////////////
// closed loop controller
////////////////////////////////////////////////////////////////////////////
Controller::Controller(Sensor &s, Transducer &t, unsigned neutralLevel, int interval, float Kp, float Kd) : transducer(t), sensor(s)
{
  updateInterval = interval;
  this->neutralLevel = neutralLevel;
  this->Kp = Kp;
  this->Kd = Kd;
  active = false;
  lastUpdate = millis();
  lastDelta = 0;  // Always zero for the first error!
}

void Controller::setDesiredValue(unsigned val) {
  desired = val;
}

void Controller::setConstants(float Kp, float Kd) {
  this->Kp = Kp;
  this->Kd = Kd;
}

void Controller::setActive(bool active) {
  this->active = active;
}

void Controller::correct()
{
  if (!active || ((millis() - lastUpdate) < updateInterval)) // no need to update
  {
    return;
  }
  lastUpdate = millis();

  int value = sensor.getNormalisedValue();  // 0 - 100

  float delta = desired - value;

  float differential = delta - lastDelta;
  // If delta (required change) is negative then we go below the neutral level for the actuator.
  // If delta = 10, lastDelta = 20 then we are getting better, so we want to damp down the level.
  // In this case differential is -10 so can just add a multipl eof this to the level.

  int level = neutralLevel + (delta / Kp); + (differential / Kd);
  //if (&sensor == &camera)
  //  Serial.println("Desired = " + String(desired) + " Actual = " + String(value) + " Delta = " + String(delta) + " Kp = " + String(Kp) + " Level = " + String(level));
  transducer.setLevel(level);
}

