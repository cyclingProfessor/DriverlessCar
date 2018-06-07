#ifndef LINE_FOLLOW
#define LINE_FOLLOW
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LIS3MDL.h>
#include <NewPing.h>

// PIN Settings
// SPI MOSI and MISO are pins 11 and 12, 13 cannot be used.
#define RFID_RST_PIN         4
#define RFID_SS_PIN          5
#define TRIGGER_PIN_LEFT     7  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN_LEFT        3  // Arduino pin tied to echo pin on ping sensor.
#define TRIGGER_PIN_RIGHT    6  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN_RIGHT       9  // Arduino pin tied to echo pin on ping sensor.
#define CAMERA_PIN           8 // Arduino pin tied to control pin on OpenMV camera
#define CLOCK_PIN            2 // digital pin - true-> follow or false-> steer
#define MOVING_PIN           10 // Am I trying to move?

#define MIN_CAR_SPEED 0u
#define MAX_CAR_SPEED 100u
#define FORWARDS 1
#define BACKWARDS (-FORWARDS)

// Number of echo sensors to poll
#define ECHO_LEFT 0
#define ECHO_RIGHT 1
#define MAX_DISTANCE 50

// Parameters for turn settings.
#define MIN_ANGLE 10u //neutral is 65-70 degrees
#define NEUTRAL_ANGLE 65u
#define MAX_ANGLE 105u
#define MIN_CAR_SPEED 0u
#define MAX_CAR_SPEED 100u

///////////////////////////////////////////////
/// State machine constants
#define FOLLOWING 0
#define USER_STOPPED 1
#define OBSTACLE_STOPPED 2

#define THREEPOINT 3
#define THREEPOINT_ONE 4
#define THREEPOINT_FINISH 5

////////////////////////////////////////////
// Constants for message passing
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
// Main should only read the data when the ready flag (freshness indicator) is set.
#define NUM_CODES 4 // How many message types

#define SPEED_MSG 7
#define SPEED_LENGTH 1

#define PID_MSG 9
#define PID_LENGTH 4

#define TURN_MSG 11
#define TURN_LENGTH 4

#define LINE_MSG 13
#define LINE_LENGTH 1

#define MID_POINT ((1 << 3) - 1) // The middle value of a four bit message value.
#define SPEED_SCALE 100 // speed -100 to 100
#define TURN_SCALE 50 // turn -50 to 50

/////////////////////////////
struct TurnParams {
  unsigned speedStep1;
  unsigned speedStep2;
  unsigned angleStep1;
  unsigned angleStep2;
};

struct PID {
  unsigned Kp ; // 1000 time the proportional constant - default 60
  unsigned Kd ; // 1000 time the derivative constant - default 
};

struct Status {
  char lastTag[16];
  byte state;
  byte saveState; // previous state to return to if necessary

  unsigned desiredSpeed;
  TurnParams turnParams;
  PID lineController; 
};


/////////////////////////////////////////////////////////////////////////////////////
class Sensor {
  public:
    virtual unsigned getNormalisedValue() = 0;
};

//////////////////////////////////////////////////////////////////////////////////////
class MagSensor : public Sensor {
  private:
    LIS3MDL mag;
  public:
    void start();
    unsigned getNormalisedValue();
    void update();
};

//////////////////////////////////////////////////////////////////////////////////////
class CameraSensor : public Sensor {
  private:
    const int controlPin;
    bool watching;
  public:
    CameraSensor(int);
    void watch();
    void calibrate();
    unsigned getNormalisedValue();
    bool isWatching();
};

//////////////////////////////////////////////////////////////////////////////////////
class RFID {
  private:
    static const int BLOCKS = 16;
    byte buffer[2 * BLOCKS + 2];
  public:
    void start();
    const byte *readCardData();
};

class EchoMonitor {
  private:
    static int max_distance; // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
    static unsigned int pingSpeed; // Frequency at which we ping
    static unsigned long pingTimer; // Holds the next ping time.
    static int which_echo;  // which sonar to ping next
  public:
    static void start(int freq);
    static void update();
};

class ProMini {
  private:
    const int clockPin;
    const int isMovingPin;
    int dataPins[4];  // truly are const but cannot be initialised as such!
    byte speed = 0;
    byte toSpeedByte(int speed);
    byte toTurnByte(int angle);
    void send(byte);
  public:
    void start();
    void setStopped();
    void setFollowing();
    bool getMoveEnded();
    void setTurn(int which);
    ProMini(int clock, int isMoving, int pins[4]);
};

extern int echo_distances[2];
extern MFRC522 mfrc522;
extern RFID rfid;
extern MagSensor magSensor;                            // The magnetic sensor
extern CameraSensor camera;                            // OpenMV camera
extern ProMini proMini;
extern Status status;

void processBT(Status &, unsigned count, char **names, unsigned **values);
void threePoint();
int check_obstacles();
bool magnetic_strip();
byte process_rfid(Status *);
void report(Status &, unsigned count, char **names, unsigned **values);

#endif

