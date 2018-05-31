#include "ProMini.h"
// Wheel diameter ~ 6cm, so circumference ~ 20cm
// 20 clicks per rotation.
// So each click is approximately 1cm

// Max Travelling 1m per sec is speed 100 clicks per second == 10ms per click.
// So we can detect bounce if time per click is less than 3ms

// Speed is calculated as K/(currentClick - clickTwoAgo)
// For 1m per second three clicks is 30 ms.
// So K/30 = 100;
// Hence K 3000;

// Since the speed sensor has no quad counting it just counts clicks
// and cannot tell direction.  So it needs notice (restart) if switching direction.

///////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt for speed sensor
/////////// Circular buffer of previous tacho clicks ////////////////////////
volatile unsigned long usecTimes[TIME_AVERAGE];
volatile byte currentSpeedTime = TIME_AVERAGE_MASK; // Will start at position zero in the circular buffer

void addToTacho() {
  long t = millis();
  if (t <= usecTimes[currentSpeedTime] + 2) {// TOO FAST => Bounce
    return;
  }
  currentSpeedTime = (currentSpeedTime + 1) & TIME_AVERAGE_MASK; // increment index
  tacho += 1;
  usecTimes[currentSpeedTime] = t; // Put time into place
}

///////////////////////////////////////////////////////////////////////////////////////
void SpeedSensor::start() {
  // Make the speed/distance sensor active
  unsigned long nowTime = millis();
  unsigned startTime = nowTime - (TIME_AVERAGE * 10000U); 
  for (int i = 0 ; i < TIME_AVERAGE ; i++ ) {
    usecTimes[i] = startTime + 10000U * i;  // This sets initial speed to zero.
  }
  attachInterrupt(digitalPinToInterrupt(SPEED_SENSOR_INTR), addToTacho, RISING);
  moving =true;
}
void SpeedSensor::stop() {
  detachInterrupt(digitalPinToInterrupt(SPEED_SENSOR_INTR));
  moving = false;
}
void SpeedSensor::restart() {
  this->stop();
  this->start();
}

unsigned SpeedSensor::getNormalisedValue() {
  if (!moving) {
    return 0;
  }

  // Copy index so that it is coherent throughout calculation
  byte index = currentSpeedTime;

  return 3000u / (millis() - usecTimes[(index + 2) & TIME_AVERAGE_MASK]);
}

void LineSensor::setValue(unsigned read) {
  value = (read * 50) / 7 ; // now 7 should be 50, 0-> 0, 15 ->100

}
unsigned LineSensor::getNormalisedValue() {
  return value;
}
