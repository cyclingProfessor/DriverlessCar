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
#define CLOCK_PIN            4 // digital pin - true-> follow or false-> steer
#define MOVING_PIN           8 // Am I trying to move?

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
#define SPEED_OFFSET 0
#define LINE_OFFSET 2
#define PID_OFFSET 4
#define TURN_OFFSET 11
#define READ_TYPE 0
#define READ_DATA 1

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
  public:
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

extern Controller speedSetter, lineFollower;
extern volatile unsigned tacho;                      // Count of tacho clicks
extern SpeedSensor speedSensor;                        // The derived device for calculating speed
extern Motor   motor                               ;   // The h-Bridge motor
extern Steerer turner;
extern byte data[24];
#endif

