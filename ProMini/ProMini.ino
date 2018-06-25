#include "ProMini.h"

////////////////////////////////// Protocol used between ProMini and Arduino //////
Message messages[NUM_CODES] = {
  {SPEED_MSG, SPEED_LENGTH},
  {LINE_MSG, LINE_LENGTH},
  {PID_MSG, PID_LENGTH},
  {TURN_MSG, TURN_LENGTH}
};

/////////////// Configuration values ////////////////////////////////////////////////
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
Controller lineFollower(lineSensor, turner, NEUTRAL_ANGLE, LINE_UPDATE, -0.75f, 1000.0f);

void setup()
{
  Serial.begin(BAUD_RATE);
  while (!Serial);    // Do nothing if no serial port is opened

  speedSensor.start();
  turner.start();
  motor.setDirection(FORWARDS);
  motor.off();
  turner.steer(NEUTRAL_ANGLE);
  lineFollower.setDesiredValue(LINE_DESIRED);
  Serial.println("Starting Pro Mini");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

  arduino.start();
//  Serial.print("Tacho is now: "); Serial.println(tacho);
}

unsigned long startTime; // Used to stay in one state for a while.
unsigned startTacho; //Used to stay in one state for a certain distance moved.
int moveState = FOLLOWING; // FOLLOWING or TURNING of some kind

byte turnBuffer[MAX_MESSAGE_LENGTH]; // Maximum length of any data transfer
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
    digitalWrite(MOVING_PIN, HIGH); // Tell the boss we have started a turn.
    setSpeed(MID_POINT);  // Stopped!
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
      if (tacho > startTacho + 8 * turnBuffer[3]) {
        Serial.println("Turn Ended");
        setSpeed(MID_POINT);
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
  int angle = nibbleToTurn(where);
  Serial.print("Turning to angle: "); Serial.println(angle);
  turner.steer(angle);
}

// TODO: Really want to actively break if the speed is too fast.
// At present for going forwards we can only switch the motor off
// This will be just excellent for nice stopping.
// This will move the logic for zero speed to the specific controller.
// Unfortunately without quadrature encoding we doe not "know" whether we are currently moving
//     moving forwards of backwards - but of course we do.
void setSpeed(int speed) {
  int nextSpeed = nibbleToSpeed(speed);
  currentSpeed = nextSpeed;

  if (nextSpeed == 0) {
    speedSetter.setActive(false);
    motor.off();
    return;
  }
  if (nextSpeed > 0) {
    motor.setDirection(FORWARDS);
    speedSetter.setDesiredValue(currentSpeed);
  } else {
    motor.setDirection(BACKWARDS);
    speedSetter.setDesiredValue(-currentSpeed);
  }
  speedSetter.setActive(true);
}
