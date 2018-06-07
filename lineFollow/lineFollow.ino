#include "lineFollow.h"

/////////////// Configuration values ////////////////////////////////////////////////

////////////// GLOBAL STATE ///////////////////////////////////////////////////////
int echo_distances[2];

// Define devices
RFID    rfid;                                   // The RFID reader
MagSensor magSensor;                            // The magnetic sensor
CameraSensor camera(CAMERA_PIN);                // The OpenMV camera
static int msg_pins[] = {A0,A1,A2,A3};
ProMini proMini(CLOCK_PIN, MOVING_PIN, msg_pins);

Status status; // global status

const unsigned paramCount = 7;
char *paramNames[paramCount] = {"s1", "a1", "s2", "a2", "sp", "kp", "kd"};
unsigned *paramValues[paramCount] = {
   &(status.turnParams.speedStep1),
   &(status.turnParams.angleStep1),
   &(status.turnParams.speedStep2),
   &(status.turnParams.angleStep2),
   &(status.desiredSpeed),
   &(status.lineController.Kp),
   &(status.lineController.Kd)
   };

void setup()
{
  Serial.begin(115200);

  while (!Serial);    // Do nothing if no serial port is opened
  Serial.println("#Comms established.");

  Wire.begin(); // Begins comms to I2C devices.
  Serial.println("#Wire (I2C) started.");
  rfid.start();
  Serial.println("#RFID Sensor started.");
  magSensor.start();
  Serial.println("#Mag Sensor started.");
  EchoMonitor::start(250);
  Serial.println("#Ping Sensors started.");

  status.desiredSpeed = (MAX_CAR_SPEED - MIN_CAR_SPEED) / 2;
  proMini.start();
  proMini.setStopped();

  // In the beginning we wait for a BT start command
  status.state = USER_STOPPED;
  status.saveState = FOLLOWING; // The state to move to after we finish being stopped
  strncpy(status.lastTag, "NONE YET", 16);
  Serial.println("#Watching the world, awaiting your commands.");
}

#define OBSTACLE_STOP 20  // see something at this distance and stop
#define OBSTACLE_START 30 // wait until its this far away to start

boolean stopped() {
  return status.state == OBSTACLE_STOPPED || status.state == USER_STOPPED;
}

void loop()
{
  //Serial.println(millis());
  EchoMonitor::update();  // Checks ping distance if the time is right
  magSensor.update(); // Get the magnetic value under the car.

  // These checks are free as they actually do not access sensors.
  if ((check_obstacles() < OBSTACLE_STOP || magnetic_strip()) && !stopped()) {
    status.saveState = status.state;
    status.state = OBSTACLE_STOPPED;
  }

  // process_rfid will change the status
  process_rfid(&status);

  // ProcessBT will only ever change status
  // From any state to USER_STOPPED
  // From USER_STOPPED it restores the old state itself
  // It sets the desired speed, three point values and PID camera controller constants
  processBT(status, paramCount, paramNames, paramValues);

  //////////////////////////////////
  // Do an action based on the state
  switch (status.state) {
    case FOLLOWING:
      camera.watch();
      proMini.setFollowing();
      break;
    case OBSTACLE_STOPPED:
      proMini.setStopped();
      if (check_obstacles() > OBSTACLE_START && !magnetic_strip()) {
        status.state = status.saveState;
      }
      break;

    // The next states are for random three point turns - induced by RFID.
    case THREEPOINT: // start first move
      proMini.setTurn(0);
      status.state = THREEPOINT_ONE;
      break;
    case THREEPOINT_ONE: // Completing first move (marked by tacho)
      if (proMini.getMoveEnded()) {
        proMini.setTurn(1);
        status.state = THREEPOINT_FINISH;
      }
      break;
    case THREEPOINT_FINISH: // completing second move (marked by tacho)
      if (proMini.getMoveEnded()) {
        status.state = FOLLOWING;
      }
      break;
    case USER_STOPPED:
      proMini.setStopped();
      camera.calibrate();
      break;
    default:
      Serial.println("Oops - WTF!");
      break;
  }
}

///////////////////////////////////////////
