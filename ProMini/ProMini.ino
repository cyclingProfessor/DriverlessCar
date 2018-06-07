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
Arduino arduino;      // Object that manages comms, including the ISR stuff

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
  lineFollower.setDesiredValue(50);
  Serial.println("Starting Pro Mini");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

  arduino.start();
//  Serial.print("Tacho is now: "); Serial.println(tacho);
}

#define FOLLOWING 101
#define IN_TURN 103
#define WAIT_TURN 105

unsigned long startTime; // Used to stay in one state for a while.
unsigned startTacho; //Used to stay in one state for a certain distance moved.
int moveState = FOLLOWING; // FOLLOWING or TURNING of some kind

byte turnBuffer[4]; // Maximum length of any data transfer
byte single; // Used to (temporarily) hold single bytes read from Arduino

/////////////// Debug Variables ////////////
// extern unsigned intrCounter;
// extern unsigned long usecTimes[TIME_AVERAGE];
// extern byte currentSpeedTime;
// extern volatile unsigned long saveT;
void loop()
{
//  Serial.print("Tacho is now: "); Serial.println(tacho);
//  Serial.print("usecTimes[currentSpeedTime] is now: "); Serial.println(usecTimes[currentSpeedTime]);
//  Serial.print("saveT is now: "); Serial.println(saveT);
//  Serial.print("Tacho is now: "); Serial.println(tacho);
//  Serial.print("IntrCount is now: "); Serial.println(intrCounter);
  if (arduino.getData(LINE_MSG, &single)) {
    lineSensor.setValue(single);
    moveState = FOLLOWING;
    Serial.print("Got Line Data: "); Serial.println(single);
    // Blue light on for first line Data!
    digitalWrite(LED_BUILTIN, HIGH);
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
    Serial.print("Got Turn Request: ");
    Serial.print("    Turn Speed:"); Serial.println(turnBuffer[1]);
    Serial.print("    Turn Angle:"); Serial.println(turnBuffer[2]);
  }
  switch (moveState) {
    case FOLLOWING: // We may need to get new PID constants
      lineFollower.setActive(true);
      break;
    case WAIT_TURN:
      lineFollower.setActive(false);
//      Serial.print("Waiting State.  Wait time: "); Serial.println(turnBuffer[0]);
      if (millis() > startTime + 4 * turnBuffer[0]) {
        startTacho = tacho;
        setSpeed(turnBuffer[1]);
        setTurn(turnBuffer[2]);
        moveState = IN_TURN;
//        Serial.println("Waiting State Finishes.  Setting Speed and Turn.");
        Serial.print("    Waiting for Tacho: "); Serial.println(turnBuffer[3]);
        Serial.print("    It begins at : "); Serial.println(startTacho);
      }
      break;
    case IN_TURN:
//      Serial.print(tacho); Serial.print(" = tacho Compared to "); Serial.println(startTacho + 5 * turnBuffer[3]);
      if (tacho > startTacho + 5 * turnBuffer[3]) {
        Serial.println("Turn Ended");
        setSpeed(0);
        digitalWrite(MOVING_PIN, LOW); // Tell the boss we have finished.
        moveState = FOLLOWING;
      }
      break;
  }
  lineFollower.correct(); // Follows coloured strip if the controller is active
  speedSetter.correct();  // Adjusts speed if the controller is active
}

void setTurn(int where) {
  Serial.print("Unconverted angle: "); Serial.println(where);
  int angle = NEUTRAL_ANGLE + (((where - MID_POINT) * (int)TURN_SCALE) / (MID_POINT + 1));
  Serial.print("Turning to angle: "); Serial.println(angle);
  turner.steer(angle);
}

void setSpeed(int speed) {
  if (speed == 0  || speed == MID_POINT) {
    speedSetter.setActive(false);
    motor.off();
    return;
  }
  // Check speed and last non-zero speed to see if we need to change the direction signals.
  int nextSpeed = ((speed - MID_POINT) * SPEED_SCALE) / (MID_POINT + 1);
  if (currentSpeed * nextSpeed < 0) {
    speedSensor.restart();
    motor.setDirection(currentSpeed < 0 ? FORWARDS : BACKWARDS);
  }
  currentSpeed = nextSpeed;
  speedSetter.setDesiredValue(currentSpeed > 0 ? currentSpeed : -currentSpeed);
  speedSetter.setActive(true);
}
