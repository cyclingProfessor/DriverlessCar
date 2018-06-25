#ifndef PRO_MINI
#define PRO_MINI
#include <Arduino.h>
#include <Servo.h>  // servo library
#include <CarLib.h>

#define FORWARDS 1
#define BACKWARDS (-FORWARDS)

#define FOLLOWING 101
#define IN_TURN 103
#define WAIT_TURN 105

// Time in milliseconds between control loops
#define LINE_UPDATE 30
#define SPEED_UPDATE 9

#define LINE_DESIRED 50

// Constants for a base 2 circular buffer - TIME_AVERAGE must be a power of two
#define TIME_AVERAGE (1 << 3)  // How many spot times we use to get current speed
#define TIME_AVERAGE_MASK (TIME_AVERAGE - 1)

//////////////////////////////////////////
class Transducer {
  public:
    virtual void setLevel(unsigned what) = 0;
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
    void setLevel(unsigned speed);
    void setSpeed(unsigned value); // bounds input to "effective" speeds.
    void off();  // needed as setSpeed(0) cannot work.  Speeds from 0 to MIN_MOTOR_POWER just whine!
};

//////////////////////////////////////////////////////////////////////////////////////
class Steerer: public Transducer {
  private:
    Servo servo;
  public:
    void start();
    void setLevel(unsigned angle);
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
    unsigned long updateInterval;      // interval between updates
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
    byte * volatile data;          // ISR data to write to
    volatile int data_length;         //Length expected for current message
    volatile int data_count = 0;      // how much data we have left to read for this code
    volatile int data_start;          // offset in data arrays for this message data
    volatile byte *data_ptr;          // where we are reading the next data byte 
    volatile bool read_code = true;   // First byte to read is a code byte
    byte registerCode[6];    // We cannot begin until we receive this.
    void buildData();        // build dereferencing structures
    static byte readPins();  // Utility function to read Data pins
    volatile bool started = false;
    void readData();         // ISR
    friend void clockPinIsr();
  public:
    void start();               // set pin modes, assign interrupt, build data structures
    bool getData(byte code, byte *buffer); // (Safe) Read of latest complete data
};


extern Controller speedSetter, lineFollower; //PID controllers
extern volatile unsigned tacho;    // Count of tacho clicks
extern SpeedSensor speedSensor;    // The derived device for calculating speed
extern Arduino arduino;            // object for messaging
extern Motor   motor;   // The h-Bridge motor
extern Steerer turner;  // Servo for front wheel steering
extern Message messages[NUM_CODES];
#endif
