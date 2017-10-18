#include "lineFollow.h"

/////////////// Configuration values ////////////////////////////////////////////////
// Time in milliseconds between control loops
#define DELTA_TIME 30

////////////// GLOBAL STATE ///////////////////////////////////////////////////////
volatile unsigned tacho = 0;                      // Count of tacho clicks
int echo_distances[2];
bool moving = false;

// Define devices
RFID    rfid;                                   // The RFID reader
Motor   motor(DIR_PIN1, DIR_PIN2, SPEED_PIN);   // The h-Bridge motor
Steerer turner;                                 // The steering servo
MagSensor magSensor;                            // The magnetic sensor
SpeedSensor speedSensor;                        // The derived device for calculating speed
Controller speedSetter(speedSensor, motor, MIN_MOTOR_POWER, DELTA_TIME, 0.1f, 1.0f);
Controller lineFollower(magSensor, turner, NEUTRAL_ANGLE, 19, 0.8f, 0.1f);

Status status; // global status

void setup()
{
  Serial.begin(115200);

  while (!Serial);    // Do nothing if no serial port is opened
  speedSensor.start();
  turner.start();
  rfid.start();
  magSensor.start();
  EchoMonitor::start(250);

  status.junction = NULL;
  status.currentSpeed = (MAX_CAR_SPEED - MIN_CAR_SPEED) / 3;

  //turn params defauult
  status.turnParams = {(MAX_CAR_SPEED - MIN_CAR_SPEED) / 2, (MAX_CAR_SPEED - MIN_CAR_SPEED) / 2.5, MAX_ANGLE, MIN_ANGLE };

  turner.steer(NEUTRAL_ANGLE);
  speedSetter.setDesiredValue(status.currentSpeed);

  // In the beginning we wait for a BT start command
  status.state = USER_STOPPED;
  status.saveState = FOLLOWING; // The state to move to after we finish being stopped
  status.onRight = false;
  strncpy(status.lastTag, "NONE YET", 16);
}

// Used for turning
static unsigned fullTurn;
static unsigned relaxedTurn;
static int end;
static long timerStart;

#define OBSTACLE_STOP 20  // see something at this distance and stop
#define OBSTACLE_START 30 // wait until its this far away to start
#define RIGHT_SIDE (0.8f) // Kp to make the line following keep to the right
#define LEFT_SIDE (-0.8f) // Kp to make the line following keep to the left

void loop()
{
  speedSetter.correct();  // Adjusts speed if the controller is active
  lineFollower.correct(); // Follows magnetic strip if the controller is active

  EchoMonitor::update();  // Checks ping distance if the time is right

  // These checks are free as they actually do not access sensors.
  if (check_obstacles() < OBSTACLE_STOP) {
    status.saveState = status.state;
    status.state = OBSTACLE_STOPPED;
  }

  // process_rfid will change the status to ENTER_JUNCTION or LEAVE_JUNCTION
  // It can also just stop the car
  // .. and it can change the car's speed.
  // It updates the rfid string in the status.
  process_rfid(&status);

  // Periodic report to Serial.
  // report(status);

  // ProcessBT will ony ever change state
  //    From any state to BT_STOPPED
  //    From BT_STOPPED it restores the old state itself
  //    It also sets the desired speed
  //    Could do more - over to the TEAM
  processBT(status);

  //////////////////////////////////
  // Do an action based on the state
  switch (status.state) {
    case FOLLOWING:
      follow(status.currentSpeed); // Make both controllers active, with a desired value for magnetism and speed.
      break;
    case OBSTACLE_STOPPED:
      stop();
      if (check_obstacles() > OBSTACLE_START) {
        status.state = status.saveState;
      }
      break;
    case ENTER_JUNCTION:
      // Use the junction and whether we are on the right
      // All of these paramters are references.
      enter_junction(status, end, fullTurn, relaxedTurn);
      break;
    case TURNING:
      // Turn strongly to begin with.  Status will eventually get updated by either BT or RFID.
      turn((MAX_CAR_SPEED - MIN_CAR_SPEED) / 3, end, fullTurn, relaxedTurn);
      break;

    // The next states are just for random three point turns, which can only occur on straights.
    case THREEPOINT: // start first move
      speedSetter.setActive(true);
      lineFollower.setActive(false);
      turner.steer(status.turnParams.angleStep1);
      motor.setDirection(BACKWARDS);
      speedSetter.setDesiredValue(status.turnParams.speedStep1);

      end = tacho + 120;
      status.state = THREEPOINT_ONE;
      break;
    case THREEPOINT_ONE: // Completes first move (marked by tacho)
      if (tacho >= end) {
        motor.off();
        timerStart = millis();
        status.state = THREEPOINT_TWO;
      }
      break;
    case THREEPOINT_TWO: // wait before starting second move, then start it
      if (millis() > timerStart + 50) {
        motor.setDirection(FORWARDS);
        turner.steer(status.turnParams.angleStep2);

        speedSetter.setDesiredValue(status.turnParams.speedStep2);
        end = tacho + 105;
        status.state = THREEPOINT_FINISH;
      }
      break;
    case THREEPOINT_FINISH: // complete second move (marked by tacho)
      if (tacho >= end) {
        motor.off();
        motor.setDirection(FORWARDS);
        turner.steer(NEUTRAL_ANGLE);
        status.state = FOLLOWING;
      }
      break;
    case USER_STOPPED:
      stop();
      break;
    default:
      Serial.println("Oops - WTF!");
      break;
  }
}

///////////////////////////////////////////

