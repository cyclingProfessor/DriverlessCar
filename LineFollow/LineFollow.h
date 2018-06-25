#ifndef LINE_FOLLOW
#define LINE_FOLLOW
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LIS3MDL.h>
#include <NewPing.h>
#include <CarLib.h>

// Echo and Magnet sensors
#define MAX_DISTANCE 50
#define NO_OBJECT 1000
#define OBSTACLE_STOP 30  // see something at this distance and stop
#define OBSTACLE_START 40 // wait until its this far away to start
#define MAG_STOP 40  // Stop when this is the magnetism!

///////////////////////////////////////////////
/// State machine constants
#define FOLLOWING 0
#define USER_STOPPED 1
#define OBSTACLE_STOPPED 2

#define THREEPOINT 3
#define THREEPOINT_ONE 4
#define THREEPOINT_FINISH 5
////////////////////////////////////////////

/////////////////////////////
struct TurnParams {
  int speedStep1;
  int speedStep2;
  unsigned angleStep1;
  unsigned angleStep2;
};

struct PID {
  int Kp ; // 1000 time the proportional constant - default 60
  int Kd ; // 1000 time the derivative constant - default 
};

#define MAX_TAG_LENGTH 16
struct Status {
  char lastTag[MAX_TAG_LENGTH];
  byte state;
  byte saveState; // previous state to return to if necessary

  int desiredSpeed;
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
    byte speed = 0;   // Translated speed to a 0-15 scale (7 == stop)
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

void processBT(Status &, unsigned count, char const **names, int **values);
void threePoint();
int check_obstacles();
bool magnetic_strip();
void process_rfid(Status *);
void report(Status &, unsigned count, char const **names, int **values);
void sendInfo(String);

#endif
