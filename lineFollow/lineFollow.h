#ifndef LINE_FOLLOW
#define LINE_FOLLOW
#include <Servo.h>  // servo library
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LIS3MDL.h>
#include <NewPing.h>

// PIN Settings
#define RFID_RST_PIN         4
#define RFID_SS_PIN          5
#define DIR_PIN1             7 // Motor direction pin
#define DIR_PIN2             8 // Motor direction pin
#define SPEED_PIN            6 // Needs to be a PWM pin to be able to control motor speed
#define STEER_PIN            10
#define SPEED_SENSOR_INTR    2 // Interupt raised by slotted speed sensor - tacho + speed.
#define TRIGGER_PIN_LEFT     A2  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN_LEFT        3  // Arduino pin tied to echo pin on ping sensor.
#define TRIGGER_PIN_RIGHT    A3  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN_RIGHT       9  // Arduino pin tied to echo pin on ping sensor.
#define CAMERA_PIN           A1 // Arduino pin tied to control pin on OpenMV camera

// Useful Physical Constants
#define MAX_SERVO 1798
#define MIN_SERVO 771
#define MIN_ANGLE 10u //neutral is 65-70 degrees
#define NEUTRAL_ANGLE 65u
#define MAX_ANGLE 105u
#define MIN_MOTOR_POWER 0u
#define MAX_MOTOR_POWER 255u
#define MIN_CAR_SPEED 0u
#define MAX_CAR_SPEED 100u
#define FORWARDS 1
#define BACKWARDS (-FORWARDS)

// Number of echo sensors to poll
#define ECHO_LEFT 0
#define ECHO_RIGHT 1
#define MAX_DISTANCE 50

// Constants for a base 2 circular buufer - TIME_AVERAGE must be a power of two
#define TIME_AVERAGE (1 << 3)  // How many spot times we use to get current speed
#define TIME_AVERAGE_MASK (TIME_AVERAGE - 1)

///////////////////////////////////////////////
/// State machine constants
#define FOLLOWING 0
#define TURNING 1
#define ENTER_JUNCTION 2
#define USER_STOPPED 3
#define OBSTACLE_STOPPED 4

#define THREEPOINT 5
#define THREEPOINT_ONE 6
#define THREEPOINT_TWO 7
#define THREEPOINT_THREE 8
#define THREEPOINT_FINISH 9

//////////////////////////////////////////
class Transducer {
  public:
    virtual void setLevel(int what) = 0;
};
/////////////////////////////////////
class Junction {
  private:
    const char *name;
    const byte properties;
  public:
    Junction(char *n, byte b);
    bool hasRight();
    bool hasLeft();
    bool hasStraight();
};

/////////////////////////////

struct TurnParams {
  unsigned speedStep1;
  unsigned speedStep2;
  unsigned angleStep1;
  unsigned angleStep2;
};

struct Status {
  // Am I on a junction?
  Junction *junction;  // NULL if between junctions

  char lastTag[16];

  // Am I on the right side of the line
  bool onRight;
  char userCode[16];
  byte state;
  byte saveState; // previous state to return to if necessary

  unsigned currentSpeed;
  TurnParams turnParams;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
class Motor: public Transducer {
  private:
    const int dirPin1;
    const int dirPin2;
    const int ctrlPin;
  public:
    Motor(int p1, int p2, int c);
    void setDirection(int direction);
    void setLevel(int speed);
    void setSpeed(unsigned value); // bounds input to "effective" speeds.
    void off();  // needed as setSpeed(0) cannot work.  Speeds from 0 to MIN_MOTOR_POWER just whine!
};

//////////////////////////////////////////////////////////////////////////////////////
class Steerer: public Transducer {
  private:
    Servo servo;
  public:
    void start();
    void setLevel(int angle);
    void steer(unsigned angle);
    void neutral();
};

/////////////////////////////////////////////////////////////////////////////////////
class Sensor {
  public:
    virtual unsigned getNormalisedValue() = 0;
};

///////////////////////////////////////////////////////////////////////////////////////

class SpeedSensor : public Sensor {
  private:
    bool moving;
  public:
    void start();
    void stop();
    void restart();
    unsigned getNormalisedValue();
};

//////////////////////////////////////////////////////////////////////////////////////
class MagSensor : public Sensor {
  private:
    LIS3MDL mag;
  public:
    void start();
    unsigned getNormalisedValue();
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

class Controller
{
  private:
    int updateInterval;      // interval between updates
    unsigned long lastUpdate; // last update of position
    Transducer &transducer;
    Sensor &sensor;
    int desired;
    unsigned neutralLevel;
    int lastDelta;
    float Kp;
    float Kd;
    bool active;
    void setLevel(unsigned inp);
  public:
    Controller(Sensor &s, Transducer &t, unsigned minInput, int interval, float Kp, float Kd);
    void setDesiredValue(unsigned out);
    void correct();
    void setConstants(float Kp, float Kd);
    void setActive(bool);
};

class EchoMonitor {
  private:
    static int max_distance; // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
    static unsigned int pingSpeed; // Frequency at which we ping
    static unsigned long pingTimer; // Holds the next ping time.
    static int which_echo;  // which sonar to ping next
  public:
    static void start(int freq);
    static bool update();
};

extern Controller speedSetter, lineFollower;
extern volatile unsigned tacho;                      // Count of tacho clicks
extern int echo_distances[2];
extern MFRC522 mfrc522;
extern RFID rfid;
extern MagSensor magSensor;                            // The magnetic sensor
extern CameraSensor camera;                            // OpenMV camera
extern SpeedSensor speedSensor;                        // The derived device for calculating speed
extern Motor   motor                               ;   // The h-Bridge motor
extern Steerer turner;


void processBT(Status &);
void threePoint();
int check_obstacles();
byte process_rfid(Status *);
void stop();
void follow(int speed);
void report(Status &);
void enter_junction(Status &status, int &end, unsigned &fullTurn, unsigned &relaxedTurn);
void turn(int speed, int end, unsigned fullTurn, unsigned relaxedTurn);

#endif

