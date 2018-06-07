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
volatile byte currentSpeedTime = TIME_AVERAGE - 1; // Start at final position in the circular buffer
#define SPEED_CONSTANT 3000u // for turning time/clicks into speed.
#define TIME_OFFSET (4 * TIME_AVERAGE * SPEED_CONSTANT) // offset time so that initial speed can be 0
#define TICKS_PER (4)

////////////////// DEBUG Variable /////////////////
// volatile unsigned intrCounter = 0;
// volatile unsigned long saveT = 555;
static inline unsigned long adj_millis() {
  return millis() + TIME_OFFSET;
}

void addToTacho() {
//  intrCounter++;
  unsigned long t = adj_millis();
  if (t <= usecTimes[currentSpeedTime] + 2) {// TOO FAST => Bounce
//    saveT = t;
    return;
  }
  currentSpeedTime = (currentSpeedTime + 1) & TIME_AVERAGE_MASK; // increment index
  tacho += 1;
  usecTimes[currentSpeedTime] = t; // Put time into place
}

///////////////////////////////////////////////////////////////////////////////////////
// Make the speed/distance sensor active
void SpeedSensor::start() {
  // Create delta between adj_millis() past usecTimes[], so that speeds starts at 0
  // usecTimes[TIME_AVERAGE - 1] must be approx adj_millis() so that all ticks count.
  for (int index = 0 ; index < TIME_AVERAGE ; index++) {
    usecTimes[index] = 4 * index * SPEED_CONSTANT;
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

  // Here index + (TIME_AVERAGE - M) is M ticks ago. 
  byte index = (currentSpeedTime + (TIME_AVERAGE - TICKS_PER)) & TIME_AVERAGE_MASK;
  return SPEED_CONSTANT / (adj_millis() - usecTimes[index]);
}

void LineSensor::setValue(unsigned read) {
  value = (read * 50) / 7 ; // now 7 should be 50, 0-> 0, 15 ->100

}
unsigned LineSensor::getNormalisedValue() {
  return value;
}
