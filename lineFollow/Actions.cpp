#include "lineFollow.h"

#define MAG_STOP 40

int check_obstacles() {
  return min(echo_distances[0], echo_distances[1]);
}

bool magnetic_strip() {
  return magSensor.getNormalisedValue() > MAG_STOP;
}

//////////////////////////////////////////////////////////
// We could be at a junction RFID or some other tag
byte process_rfid(Status *status) {
  byte *str = rfid.readCardData();
  if (!str) {
    return;
  }
  if (!strncmp(str, "Dave", 4)) {
    status->saveState = status->state;
    status->state = USER_STOPPED;
  }
  if (!strncmp(str, "++++",4)) {
    status->desiredSpeed = min(status->desiredSpeed + 10, 40);
  }
  if (!strncmp(str, "----",4)) {
    status->desiredSpeed = max(status->desiredSpeed - 10, 20);
  }
  if (!strncmp(str, "Stop!", 5)) {
    status->saveState = status->state;
    status->state = USER_STOPPED;    
  }
  if (!strncmp(str, "Three Point", 11)) {
    status->saveState = status->state;
    status->state = THREEPOINT;    
  }
}

static long reportTime = 0;

void report(Status &status, unsigned paramCount, char **paramNames, unsigned **paramValues) {
  if (millis() < reportTime + 1000) {
    return;
  }
  reportTime = millis();
  Serial.print("R" + String(status.lastTag) + ":END:");
  Serial.print("Dleft:" + String(echo_distances[0]) + " right:" + String(echo_distances[1]) + ":END:");
  Serial.print("M" + String(magSensor.getNormalisedValue()) + ":END:");
  Serial.print("Z" + String(status.state) + ":END:");
  
  Serial.print("P");
  for (int index = 0 ; index < paramCount ; index++) {
    Serial.print(paramNames[index] + String(*(paramValues[index])));
  }
  Serial.print(":END:");
}

// Do all of the communication with the Pro Mini
void ProMini::stop() {
  // set up data lines to zero.
  // if we have just stopped then switch control line on then off again.
}
void ProMini::setFollowing() {
  // set control lines to desired speed if its new
  // set PID parameters if they are new
  // If new parameters then send the right commands
}
void ProMini::sendTurn(int which) {
  // Always set the data and start a turn
}
bool ProMini::moveEnded() {
  return digitalRead(MOVING_PIN) == LOW;
}

ProMini::ProMini(int clock, int isMoving): clockPin(clock), isMovingPin(isMoving){
  pinMode(clock, OUTPUT);
  pinMode(isMoving, INPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
}