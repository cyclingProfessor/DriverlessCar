#include "LineFollow.h"

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
static const int SPEED_DELTA = 10;
void process_rfid(Status *status) {
  char *str = (char *) rfid.readCardData();
  if (!str) {
    return;
  }
  strncpy(status->lastTag, str, 16);
  if (!strncmp(str, "++++",4)) {
    status->desiredSpeed = min(status->desiredSpeed + SPEED_DELTA, MAX_CAR_SPEED);
  }
  if (!strncmp(str, "----",4)) {
    status->desiredSpeed = max(status->desiredSpeed - SPEED_DELTA, MIN_EFFECTIVE_SPEED);
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
  sendInfo(RFID_TAG_CODE + String(status.lastTag));
  sendInfo(SONAR_CODE + String("left:") + String(echo_distances[0]) + " right:" + String(echo_distances[1]));
  sendInfo(MAGNET_CODE + String(magSensor.getNormalisedValue()));
  sendInfo(LINE_CODE + String(camera.getNormalisedValue()));
  sendInfo(STATUS_CODE + String(status.state));
  
  Serial.print(PARAMETERS_CODE);
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
    send(toSpeedNibble(speed));
  }
}

void ProMini::setFollowing() {
  send(LINE_MSG);
  send(camera.getNormalisedValue());
  if (speed != status.desiredSpeed) {
    speed = status.desiredSpeed;
    send(SPEED_MSG);
    send(toSpeedNibble(speed)); //always positive
  }
}

void ProMini::setTurn(int which) {
  // Set the last speed to zero since the car stops after turning
  speed = 0;
  
  // Always set the data and start a turn
  send(TURN_MSG);
  send(PRE_TURN_WAIT); // Wait for 30 ms before turning
  if (which == 0) {
    send(toSpeedNibble(status.turnParams.speedStep1));
    send(toTurnNibble(status.turnParams.angleStep1));
  } else {
    send(toSpeedNibble(-1 * status.turnParams.speedStep2));
    send(toTurnNibble(status.turnParams.angleStep2));
  }
  send(TURN_TACHO); // Distance to move.
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

ProMini::ProMini(int clock, int isMoving, int pins[NUM_BITS_PER_MESSAGE]): clockPin(clock), isMovingPin(isMoving){
  pinMode(clockPin, OUTPUT);
  pinMode(isMovingPin, INPUT);
  for (int index = 0 ; index < NUM_BITS_PER_MESSAGE ; index++) {
    dataPins[index] = pins[index];
    pinMode(dataPins[index], OUTPUT);
  }
}

void ProMini::start() {
  // Send registration codes
  for (int index = 0 ; index < REG_CODE_COUNT ; index++) {
    send(reg_code(index));
  }  
}

void ProMini::send(byte b) {
  delayMicroseconds(300); // Give time for previous ISR on the ProMini to finish.
  for (int index = 0 ; index < NUM_BITS_PER_MESSAGE ; index++) {
    digitalWrite(dataPins[index], (b & (1 << index)) > 0 ? HIGH : LOW);
  }
  digitalWrite(clockPin, LOW);
  digitalWrite(clockPin, HIGH);
}
