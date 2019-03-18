#include <timerobj.h>

#define RIGHT 0
#define LEFT 1
#define REAR 2
#define LONG_DISTANCE 1000

int echoPin[3] = {7,5,3};
int triggerPin[3] = {6,4,2};

typedef void (*FP)();
void rightEcho(), leftEcho(), rearEcho();
FP echoFn[3] = {rightEcho, leftEcho, rearEcho};

volatile int distCm[3] = {LONG_DISTANCE, LONG_DISTANCE, LONG_DISTANCE};
volatile unsigned duration[3];
volatile int cRight = 0, cLeft = 0, cRear = 0;

void pinger(void *data) {
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  for (int index = 0 ; index < 3 ; index++) {
    digitalWrite(triggerPin[index], HIGH);
  }
  delayMicroseconds(30);
  for (int index = 0 ; index < 3 ; index++) {
    digitalWrite(triggerPin[index], LOW);
  }
}

Timer triggerTimer(100, pinger, t_period, NULL );

void setup() {
  Serial1.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  for (int index = 0 ; index < 3 ; index++) {
    pinMode(triggerPin[index], OUTPUT);
    pinMode(echoPin[index], INPUT);
    digitalWrite(triggerPin[index], LOW);
    attachInterrupt(echoPin[index], echoFn[index], CHANGE);  // Attach interrupt to the sensor echo input
  }
  delay(100);
}

void loop() {
  Serial1.print("Distance to Rear: ");
  Serial1.println(duration[REAR]/ 58.2);
  Serial1.print("Distance to RIGHT: ");
  Serial1.println(duration[RIGHT] / 58.2);
  Serial1.print("Distance to LEFT: ");
  Serial1.println(duration[LEFT] / 58.2);
  Serial1.println("-----------------------------------------");
  delay(1000);
}


void rightEcho() {
  calcDistance(RIGHT);
  cRight ++;
}
void leftEcho() {
  calcDistance(LEFT);
  cLeft++;
}
void rearEcho() {
  calcDistance(REAR);
  cRear ++;
}
static inline void calcDistance(int which) {
  // Echo is within 25mS if there is an object in view
  // Otherwise at least 38mS
  static int startTime[3];
  if (digitalRead(echoPin[which]) == HIGH) { // Start of a pulse!
    startTime[which] = micros();
  } else {
    duration[which] = micros() - startTime[which];
    //distCm[which] = (duration[which] < 25000) ? (duration[which])/ 58.2 : LONG_DISTANCE;
  }
}
