#include "ProMini.h"

// Uses one digital output pin: MOVING_PIN
// Other pins are all used for input - clocked in from the pins A0-A3
// Hence we interrupt on CLOCK_PIN Rising.
// Message: Type followed by....
//     1-> next ONE byte is desired speed.
//     2-> next ONE bytes are line value.
//     3-> next FOUR bytes are Kd, Kp
//     4-> next FOUR bytes are Wait Time, Angle, Speed, Duration
/////////////////////////////////////////////////////////////////////////
// Each data set is followed by a ready flag.
// Main should only read the data when the ready flag is set  to 1.
byte data[24];
#define SPEED_MSG 7
#define PID_MSG 9
#define TURN_MSG 11
#define LINE_MSG 13

#define SPEED_OFFSET 0
#define LINE_OFFSET 2
#define PID_OFFSET 4
#define TURN_OFFSET 11
#define READ_TYPE 0
#define READ_DATA 1
int data_count = 0;
int data_offset = 0;
int read_state = READ_TYPE;

void readData() {
  byte value = 0;
  if (digitalRead(A3) == HIGH) {
    value |= 8;
  }
  if (digitalRead(A2) == HIGH) {
    value |= 4;
  }
  if (digitalRead(A1) == HIGH) {
    value |= 2;
  }
  if (digitalRead(A0) == HIGH) {
    value |= 1;
  }
  switch (read_state) {
    case READ_TYPE:
      switch(value) {
        case SPEED_MSG:
          data_offset = SPEED_OFFSET;
          data_count = 1;
          break;
        case LINE_MSG:
          data_offset = LINE_OFFSET;
          data_count = 1;
          break;
        case PID_MSG:
          data_offset = PID_OFFSET;
          data_count = 4;
          break;
        case TURN_MSG:
          data_offset = TURN_OFFSET;
          data_count = 4;
          break;
      }
      data[data_offset + data_count] = 0;
      read_state = READ_DATA;
      break;
    case READ_DATA:
      data[data_offset++] = value;
      data_count--;
      if (data_count == 0) {
         data[data_offset] = 1;
         read_state = READ_TYPE;
      }
      break;
  }
}

/////////////// Configuration values ////////////////////////////////////////////////
// Time in milliseconds between control loops
#define DELTA_TIME 30

////////////// GLOBAL STATE ///////////////////////////////////////////////////////
volatile unsigned tacho = 0;                      // Count of tacho clicks
int currentSpeed = 0;

// Define devices
Motor   motor(DIR_PIN2, DIR_PIN1, SPEED_PIN);   // The h-Bridge motor
Steerer turner;            // The steering servo
SpeedSensor speedSensor;   // The derived device for calculating speed
LineSensor lineSensor;

Controller speedSetter(speedSensor, motor, MIN_MOTOR_POWER, DELTA_TIME, 0.1f, 1000.0f);
Controller lineFollower(lineSensor, turner, NEUTRAL_ANGLE, DELTA_TIME, -0.6f, 1000.0f);

void setup()
{
  Serial.begin(115200);
  while (!Serial);    // Do nothing if no serial port is opened

  speedSensor.start();
  turner.start();
  motor.setDirection(FORWARDS);
  motor.off();
  turner.steer(NEUTRAL_ANGLE);

  pinMode(CLOCK_PIN, INPUT);
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  pinMode(A2,INPUT);
  pinMode(A3,INPUT);
  pinMode(MOVING_PIN, OUTPUT);
  attachInterrupt (digitalPinToInterrupt (CLOCK_PIN), readData, RISING);
}

#define FOLLOWING 101
#define IN_TURN 103
#define WAIT_TURN 105

long startTime;
long startTacho;
int moveState = FOLLOWING;

void loop()
{
  if (data[TURN_OFFSET + 4] == 1) { // We may need to start a turn 
    // received new turn parameters.
    digitalWrite(MOVING_PIN, HIGH); // Tell the boss.
    motor.off();
    startTime = millis();
    data[TURN_OFFSET + 4] = 0;
    moveState = WAIT_TURN; // Now the turn will start....
  }
  switch (moveState) {
    case FOLLOWING: // We may need to get new PID constants
      if (data[PID_OFFSET + 4] == 1) {
        int Kp = data[PID_OFFSET] << 4 + data[PID_OFFSET + 1];
        int Kd = data[PID_OFFSET + 2] << 4 + data[PID_OFFSET + 3];
        data[PID_OFFSET + 4] = 0;
      }
      setSpeed(data[SPEED_OFFSET]); // Always read the speed data)
      break;
    case WAIT_TURN:
      if (millis() > startTime + data[TURN_OFFSET] * 4) {
        startTacho = tacho;
        setSpeed(data[TURN_OFFSET + 1]);
        setTurn(data[TURN_OFFSET + 2]);
        moveState = IN_TURN;
      }
      break;
    case IN_TURN:
      if (tacho > startTacho + data[TURN_OFFSET + 3] * 5) {
         speedSetter.setActive(false);
         motor.off();
         digitalWrite(MOVING_PIN, LOW); // Tell the boss.
         moveState = FOLLOWING;
      }
      break;
  }
  lineFollower.correct(); // Follows coloured strip if the controller is active
  speedSetter.correct();  // Adjusts speed if the controller is active
}

void setTurn(byte where) {
  turner.steer(NEUTRAL_ANGLE + (((where - 7) * 50) / 8));
}

void setSpeed(byte speed) {
  if (speed == 0) {
    speedSetter.setActive(false);
    motor.off();
    return;
  }
  if (currentSpeed * (speed - 7) < 0) {
    speedSensor.restart();
    motor.setDirection(currentSpeed < 0 ? FORWARDS : BACKWARDS);
  }
  currentSpeed = ((speed - 7) * 100) / 8;
  speedSetter.setDesiredValue(currentSpeed);
  speedSetter.setActive(true);
}