#include "lineFollow.h"

int check_obstacles() {
  return min(echo_distances[0], echo_distances[1]);
}

#define TAG_RIGHT_TURN 1
#define TAG_LEFT_TURN 2
#define TAG_STRAIGHT_ON 4

Junction::Junction(char *n, byte b) : name(n), properties(b) {}
bool Junction::hasRight() {
  return properties | TAG_RIGHT_TURN;
}
bool Junction::hasLeft() {
  return properties | TAG_LEFT_TURN;
}
bool Junction::hasStraight() {
  return properties | TAG_STRAIGHT_ON;
}

// Junction tags have string "J:name"
Junction tags[] {
  Junction("A1", TAG_RIGHT_TURN),
  Junction("A2", TAG_LEFT_TURN)
};

//////////////////////////////////////////////////////////
// We could be at a junction RFID or some other tag
byte process_rfid(Status *status) {
  byte *str = rfid.readCardData();
  if (!str) {
    return;
  }
  strncpy(status->lastTag, str, 16);
  if (!strncmp(str, "J:", 2)) { // We are at a junction tag
    char *junction = str + 2; // Now the junction could be a surprise!
    if (status->junction) { // we are already on a junction - so must be leaving it
      status->state = FOLLOWING;
      status->junction = NULL;
    } else {
      status->state = ENTER_JUNCTION;
      status->junction = tags + 1;
    }
  }
  if (!strncmp(str, "Dave", 4)) {
    status->saveState = status->state;
    status->state = USER_STOPPED;
  }
  if (!strncmp(str, "++++",4)) {
    status->currentSpeed = min(status->currentSpeed + 10, 40);
    speedSetter.setDesiredValue(status->currentSpeed);
  }
  if (!strncmp(str, "----",4)) {
    status->currentSpeed = max(status->currentSpeed - 10, 20);
    speedSetter.setDesiredValue(status->currentSpeed);
  }
  if (!strncmp(str, "Stop!", 5)) {
    status->userCode[0] = '@';
    status->saveState = status->state;
    status->state = USER_STOPPED;    
  }
  if (!strncmp(str, "Three Point", 11)) {
    status->saveState = status->state;
    status->state = THREEPOINT;    
  }
}

void stop() {
  motor.off();
  speedSetter.setActive(false);
  turner.neutral();
  lineFollower.setActive(false);
}

void follow(int speed) {
  speedSetter.setDesiredValue(speed);
  speedSetter.setActive(true);
  lineFollower.setDesiredValue(30);  // A guess
  lineFollower.setActive(true);
}

static long turnTime = 0;
void turn(int speed, int end, unsigned fullTurn, unsigned relaxedTurn) {
  // Too busy - slow things down
  if (millis() < turnTime + 50) {
    return;
  }
  turnTime = millis();
  int currentTurn;
  if ((tacho >= end) && (magSensor.getNormalisedValue() < 20)) { // we are to the right of the curve, relax a bit
    currentTurn = relaxedTurn;
  } else {
    currentTurn = fullTurn;
  }
  lineFollower.setActive(false);

  //Speed up a bit because it is hard to turn!
  speedSetter.setDesiredValue(speed);
  turner.steer(currentTurn);
}

static long reportTime = 0;

void report(Status &status) {
  if (millis() < reportTime + 1000) {
    return;
  }
  reportTime = millis();
  Serial.print("R" + String(status.lastTag) + ":END:");
  Serial.print("D" + String(echo_distances[0]) + " Right:" + String(echo_distances[1]) + ":END:");
  Serial.print("M" + String(magSensor.getNormalisedValue()) + ":END:");
  Serial.print("T" + String(tacho) + ":END:");
  Serial.print("S" + String(speedSensor.getNormalisedValue()) + ":END:");
  Serial.print("P" + String(status.state) + " " + String(status.turnParams.speedStep1) +" " + String(status.turnParams.angleStep1) +" " + String(status.turnParams.speedStep2) +" " + String(status.turnParams.angleStep2) +" " + String(status.currentSpeed) + ":END:");
}
#define TURN_START 30     // How long to turn full at the start of a turn

void enter_junction(Status &status, int &end, unsigned &fullTurn, unsigned &relaxedTurn) {
  Junction *j = status.junction;

  if (status.onRight) {
    if (j->hasRight()) {
      fullTurn = MAX_ANGLE;
      relaxedTurn = MAX_ANGLE - 5;
      status.state = TURNING;
      end = tacho + TURN_START;
    } else if (j->hasStraight()) {
      status.state = FOLLOWING;
    } else {
      // Damn - we are on the right and have to turn left! Wing it.
      fullTurn = MIN_ANGLE;
      relaxedTurn = MIN_ANGLE + 5;
      status.state = TURNING;
      end = tacho + 2 * TURN_START;
    }
  } else {
    Serial.println("I am following the left");
    if (j->hasLeft()) {
      Serial.println("I can turn left");
      fullTurn = MIN_ANGLE;
      relaxedTurn = MIN_ANGLE + 5;
      status.state = TURNING;
      end = tacho + TURN_START;
    } else if (j->hasStraight()) {
      status.state = FOLLOWING;
      Serial.println("I can't turn left, but can go straight");
    } else {
      // Damn - we are on the left and have to turn right! Wing it.
      fullTurn = MAX_ANGLE;
      relaxedTurn = MAX_ANGLE - 5;
      status.state = TURNING;
      end = tacho + 2 * TURN_START;
    }
  }
}
