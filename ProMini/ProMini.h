#ifndef PRO_MINI
#define PRO_MINI
#include <Arduino.h>
#include <Servo.h>  // servo library

// PIN Settings
#define DIR_PIN1             7 // Motor direction pin
#define DIR_PIN2             6 // Motor direction pin
#define SPEED_PIN            5 // Needs to be a PWM pin to be able to control motor speed
#define STEER_PIN            9
#define SPEED_SENSOR_INTR    3 // Interrupt raised by slotted speed sensor - tacho + speed.
#define CLOCK_PIN            2 // (Intr) digital pin to initiate parallel xfer
#define MOVING_PIN           8 // Am I trying to move?
#define DATA_0               A4
#define DATA_1               A5
#define DATA_2               A6
#define DATA_3               A7

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
#define TURN_SCALE (MAX_ANGLE - MIN_ANGLE) // turn -50 to 50

// Constants for a base 2 circular buffer - TIME_AVERAGE must be a power of two
#define TIME_AVERAGE (1 << 3)  // How many spot times we use to get current speed
#define TIME_AVERAGE_MASK (TIME_AVERAGE - 1)

//////////////////////////////////////////
class Transducer {
  public:
    virtual void setLevel(int what) = 0;
};

struct PID {
  unsigned Kp ; // 1000 time the proportional constant - default 60
  unsigned Kd ; // 1000 time the derivative constant - default 
};

///////////////////////////////////
// Necessary information describing the messages that may arrive
// code for beginning message and how many following data bytes
struct Message {
  unsigned code;
  unsigned length;
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

class LineSensor : public Sensor {
  private:
    unsigned value = 50;
  public:
    void setValue(unsigned);
    unsigned getNormalisedValue();
};

class SpeedSensor : public Sensor {
  private:
    bool moving;
  public:
    void start();
    void stop();
    void restart();
    unsigned getNormalisedValue();
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
    void setConstants(PID constants);
    void setActive(bool);
};

// This object deals with interrupts so all methods should be static
class Arduino {
  private:
    unsigned dataOffset[16]; // indexed by data code -> offset in data stores
    unsigned dataLength[16]; // indexed by data code -> length in data store
    byte *liveData;      // data for program to read from
    byte *data;          // ISR data to write to
    int data_length;         //Length expected for current message
    int data_count = 0;      // how much data we have left to read for this code
    int data_start;          // offset in data arrays for this message data
    byte *data_ptr;          // where we are reading the next data byte 
    bool read_code = true;   // First byte to read is a code byte
    byte registerCode[6];    // We cannot begin until we receive this.
    void buildData();        // build dereferencing structures
    static byte readPins();  // Utility function to read Data pins
    bool started = false;
    void readData();         // ISR
    friend void clockPinIsr();
  public:
    start();               // set pin modes, assign interrupt, build data structures
    bool getData(byte code, byte *buffer); // (Safe) Read of latest complete data
};


extern Controller speedSetter, lineFollower; //PID controllers
extern volatile unsigned tacho;    // Count of tacho clicks
extern SpeedSensor speedSensor;    // The derived device for calculating speed
extern volatile Arduino arduino;            // object for messaging
extern Motor   motor;   // The h-Bridge motor
extern Steerer turner;  // Servo for front wheel steering
extern Message messages[NUM_CODES];
#endif

