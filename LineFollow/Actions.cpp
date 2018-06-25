#include "lineFollow.h"

#define MAG_STOP 40

int check_obstacles() {
  return min(echo_distances[0], echo_distances[1]);
}

bool magnetic_strip() {
  return magSensor.getNormalisedValue() > MAG_STOP;
}

void sendInfo(String str) {
  Serial.print(str);
  Serial.print(END_MSG);
}
//////////////////////////////////////////////////////////
// We could be at a junction RFID or some other tag
void process_rfid(Status *status) {
  char *str = (char *) rfid.readCardData();
  if (!str) {
    return;
  }
  strncpy(status->lastTag, str, 16);
  if (!strncmp(str, "Dave", 4)) {
    status->saveState = status->state;
    status->state = USER_STOPPED;
  }
  if (!strncmp(str, "++++",4)) {
    status->desiredSpeed = min(status->desiredSpeed + 10, 70);
  }
  if (!strncmp(str, "----",4)) {
    status->desiredSpeed = max(status->desiredSpeed - 10, 30);
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

static unsigned long reportTime = 0;

void report(Status &status, unsigned paramCount, char const **paramNames, int **paramValues) {
  if (millis() < reportTime + 1000) {
    return;
  }
  reportTime = millis();
  sendInfo("R" + String(status.lastTag));
  sendInfo("Dleft:" + String(echo_distances[0]) + " right:" + String(echo_distances[1]));
  sendInfo("M" + String(magSensor.getNormalisedValue()));
  sendInfo("L" + String(camera.getNormalisedValue()));
  sendInfo("Z" + String(status.state));
  
  Serial.print("P");
  for (unsigned index = 0 ; index < paramCount ; index++) {
    Serial.print(paramNames[index] + String(*(paramValues[index])));
  }
  Serial.print(END_MSG);
}

// Do all of the communication with the Pro Mini
void ProMini::setStopped() {
  if (speed != 0) {
    speed = 0;
    send(SPEED_MSG);
    send(toSpeedByte(speed));
  }
}

void ProMini::setFollowing() {
  send(LINE_MSG);
  send(camera.getNormalisedValue());
  if (speed != status.desiredSpeed) {
    speed = status.desiredSpeed;
    send(SPEED_MSG);
    send(toSpeedByte(speed)); //always positive
  }
}

////////////////////////////////////////////////////////////////////
//calculates inverse of: ((byte - MID_POINT) * SPEED_SCALE) / (MID_POINT + 1);
// since this is what is done on the Pro Mini
byte ProMini::toSpeedByte(int s) {
  return (s * (MID_POINT + 1) / SPEED_SCALE) + MID_POINT;
}

////////////////////////////////////////////////////////////////////
//calculates inverse of: NEUTRAL_ANGLE + (((byte - MID_POINT) * TURN_SCALE) / (MID_POINT + 1));
// since this is what is done on the Pro Mini
byte ProMini::toTurnByte(int t) {
  return (t - NEUTRAL_ANGLE) * (MID_POINT + 1) / TURN_SCALE + MID_POINT;
}

void ProMini::setTurn(int which) {
  // Set the last speed to zero since the car stops after turning
  speed = 0;
  
  // Always set the data and start a turn
  send(TURN_MSG);
  send(10); // Wait for 30 ms before turning
  if (which == 0) {
    send(toSpeedByte(status.turnParams.speedStep1));
    send(toTurnByte(status.turnParams.angleStep1));
  } else {
    send(toSpeedByte(-1 * status.turnParams.speedStep2));
    send(toTurnByte(status.turnParams.angleStep2));
  }
  send(15); // Distance to move.
  // Wait for the move to begin. Never wait too long.
  int count = 0;
  while (digitalRead(isMovingPin) != HIGH && count < 20) {
     delay(50);
     count++;
  }
}

bool ProMini::getMoveEnded() {
  // Check the isMovingPin which is set low by the ProMini after the move finishes.
  return digitalRead(isMovingPin) == LOW;
}

ProMini::ProMini(int clock, int isMoving, int pins[4]): clockPin(clock), isMovingPin(isMoving){
  pinMode(clockPin, OUTPUT);
  pinMode(isMovingPin, INPUT);
  for (int index = 0 ; index < 4 ; index++) {
    dataPins[index] = pins[index];
    pinMode(dataPins[index], OUTPUT);
  }
}

void ProMini::start() {
  // Send registration codes
  for (int index = 0 ; index < 6 ; index++) {
    send((7 * index + 3) & 0xF);
  }  
}

void ProMini::send(byte b) {
  delayMicroseconds(300); // Give time for previous ISR on the ProMini to finish.
  for (int index = 0 ; index < 4 ; index++) {
    digitalWrite(dataPins[index], (b & (1 << index)) > 0 ? HIGH : LOW);
  }
  digitalWrite(clockPin, LOW);
  digitalWrite(clockPin, HIGH);
}
