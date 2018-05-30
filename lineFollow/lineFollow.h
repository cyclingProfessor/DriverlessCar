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


///////////////////////////////////////////////
/// State machine constants
#define FOLLOWING 0
#define USER_STOPPED 1
#define OBSTACLE_STOPPED 2

#define THREEPOINT 3
#define THREEPOINT_ONE 4
#define THREEPOINT_FINISH 5

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
  public:
    static void stop();
    static void setFollowing();
    static bool moveEnded();
    static void sendTurn(int which);
    ProMini(int clock, int isMoving);
};

extern int echo_distances[2];
extern MFRC522 mfrc522;
extern RFID rfid;
extern MagSensor magSensor;                            // The magnetic sensor
extern CameraSensor camera;                            // OpenMV camera
extern ProMini proMini;

void processBT(Status &, unsigned count, char **names, unsigned **values);
void threePoint();
int check_obstacles();
bool magnetic_strip();
byte process_rfid(Status *);
void report(Status &, unsigned count, char **names, unsigned **values);

#endif

