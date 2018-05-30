#include "ProMini.h"

////////////////////////////////// Protocol used between ProMini and Arduino //////
Message messages[NUM_CODES] = {
  {SPEED_MSG, SPEED_LENGTH},
  {LINE_MSG, LINE_LENGTH},
  {PID_MSG, PID_LENGTH},
  {TURN_MSG, TURN_LENGTH}
};

/////////////// Configuration values ////////////////////////////////////////////////
// Time in milliseconds between control loops
#define LINE_UPDATE 30
#define SPEED_UPDATE 9

////////////// GLOBAL STATE ///////////////////////////////////////////////////////
volatile unsigned tacho = 0;                      // Count of tacho clicks
int currentSpeed = 0;                             // Global desired speed.

// Define devices
Motor   motor(DIR_PIN2, DIR_PIN1, SPEED_PIN);   // The h-Bridge motor
Steerer turner;            // The steering servo
SpeedSensor speedSensor;   // The derived device for calculating speed
LineSensor lineSensor;  // Pseudo sensor that corresponds to byte in data[]
volatile Arduino arduino;      // Object that manages comms, including the ISR stuff

Controller speedSetter(speedSensor, motor, MIN_MOTOR_POWER, SPEED_UPDATE, 0.1f, 1000.0f);
Controller lineFollower(lineSensor, turner, NEUTRAL_ANGLE, LINE_UPDATE, -0.6f, 1000.0f);

void setup()
{
  Serial.begin(115200);
  while (!Serial);    // Do nothing if no serial port is opened

  speedSensor.start();
  turner.start();
  motor.setDirection(FORWARDS);
  motor.off();
  turner.steer(NEUTRAL_ANGLE);
  Serial.println("Starting Pro Mini");
}

#define FOLLOWING 101
#define IN_TURN 103
#define WAIT_TURN 105

long startTime; // Used to stay in one state for a while.
long startTacho; //Used to stay in one state for a certain distance moved.
int moveState = FOLLOWING; // FOLLOWING or TURNING of some kind

byte turnBuffer[4]; // Maximum length of any data transfer
byte single; // Used to (temporarily) hold single bytes read from Arduino

void loop()
{
  if (arduino.getData(LINE_MSG, &single)) {
    lineSensor.setValue(single);
    Serial.print("Got Line Data: "); Serial.println(single);
  }
  if (arduino.getData(SPEED_MSG, &single)) {
    setSpeed(single); // Whether or not turning we may change speed.
    Serial.print("Got Speed Request: "); Serial.println(single);
  }
  if (arduino.getData(TURN_MSG, turnBuffer)) { // We may need to start a turn
    digitalWrite(MOVING_PIN, HIGH); // Tell the boss we have started.
    setSpeed(0);
    startTime = millis();
    moveState = WAIT_TURN; // Now the turn will start....
  }
  switch (moveState) {
    case FOLLOWING: // We may need to get new PID constants
      lineFollower.setActive(true);
      break;
    case WAIT_TURN:
      if (millis() > startTime + 4 * turnBuffer[0]) {
        startTacho = tacho;
        setSpeed(turnBuffer[1]);
        setTurn(turnBuffer[2]);
        moveState = IN_TURN;
      }
      break;
    case IN_TURN:
      if (tacho > startTacho + 5 * turnBuffer[3]) {
        setSpeed(0);
        digitalWrite(MOVING_PIN, LOW); // Tell the boss we have finished.
        moveState = FOLLOWING;
      }
      break;
  }
  lineFollower.correct(); // Follows coloured strip if the controller is active
  speedSetter.correct();  // Adjusts speed if the controller is active
}

void setTurn(byte where) {
  turner.steer(NEUTRAL_ANGLE + (((where - MID_POINT) * TURN_SCALE) / (MID_POINT + 1)));
}

void setSpeed(byte speed) {
  if (speed == 0) {
    speedSetter.setActive(false);
    motor.off();
    return;
  }
  int nextSpeed = ((speed - MID_POINT) * SPEED_SCALE) / (MID_POINT + 1);
  if (currentSpeed * nextSpeed < 0) {
    speedSensor.restart();
    motor.setDirection(currentSpeed < 0 ? FORWARDS : BACKWARDS);
  }
  currentSpeed = nextSpeed;
  speedSetter.setDesiredValue(currentSpeed > 0 ? currentSpeed : - currentSpeed);
  speedSetter.setActive(true);
}