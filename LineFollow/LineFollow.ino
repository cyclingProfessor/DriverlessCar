#include "LineFollow.h"

////////////// GLOBAL STATE ///////////////////////////////////////////////////////
int echo_distances[2];

// Define devices
RFID    rfid;                                   // The RFID reader
MagSensor magSensor;                            // The magnetic sensor
CameraSensor camera(CAMERA_PIN);                // The OpenMV camera
int MESSAGE_PINS[] = {MSG_PIN0,MSG_PIN1,MSG_PIN2,MSG_PIN3};
ProMini proMini(CLOCK_PIN, MOVING_PIN, MESSAGE_PINS);

Status status; // global status

const char *paramNames[] = {SPEED1, ANGLE1, SPEED2, ANGLE2, BASE_SPEED, PROP_K, DERIV_K};
const unsigned paramCount = sizeof(paramNames) / sizeof(paramNames[0]);
int *paramValues[paramCount] = {
   &(status.turnParams.speedStep1),
   (int *) &(status.turnParams.angleStep1), // Allowed type coercion since unsigned turn < 180 
   &(status.turnParams.speedStep2),
   (int *) &(status.turnParams.angleStep2), // Allowed type coercion since unsigned turn < 180 
   &(status.desiredSpeed),
   &(status.lineController.Kp),
   &(status.lineController.Kd)
   };

void setup()
{
  Serial.begin(BAUD_RATE);

  while (!Serial);    // Do nothing if no serial port is opened
  sendInfo("#Comms established.");

  Wire.begin(); // Begins comms to I2C devices.
  sendInfo("#Wire (I2C) started.");
  rfid.start();
  sendInfo("#RFID Sensor started.");
  magSensor.start();
  sendInfo("#Mag Sensor started.");
  EchoMonitor::start(250);
  sendInfo("#Ping Sensors started.");

  status.desiredSpeed = (MAX_CAR_SPEED - MIN_CAR_SPEED) / 4;
  proMini.start();
  proMini.setStopped();

  // In the beginning we wait for a BT start command
  status.state = USER_STOPPED;
  status.saveState = FOLLOWING; // The state to move to after we finish being stopped
  strncpy(status.lastTag, "NONE YET", MAX_TAG_LENGTH);
  // Set some reasonable defaults.
  status.turnParams.angleStep1 = ANGLE1_DEF;
  status.turnParams.angleStep2 = ANGLE2_DEF;
  status.turnParams.speedStep1 = SPEED1_DEF;
  status.turnParams.speedStep2 = SPEED2_DEF;
  
  sendInfo("#Watching the world, awaiting your commands.");
}

static inline boolean stopped() {
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
      sendInfo("#Oops - WTF!");
      break;
  }
}

///////////////////////////////////////////
