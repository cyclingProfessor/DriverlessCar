#ifndef CARLIB
#define CARLIB
#include <Arduino.h>

// Always use this please.
#define BAUD_RATE            115200

#define MIN_CAR_SPEED (-90)
#define MAX_CAR_SPEED 100
#define MIN_EFFECTIVE_SPEED 40 // We stop if we try to go slower either backwards or forwards

// Parameters for turn settings.
#define MIN_ANGLE 10 //neutral is 65-70 degrees
#define NEUTRAL_ANGLE 65
#define MAX_ANGLE 105

#define NUM_BITS_PER_MESSAGE (4)
#define MAX_MESSAGE ((1 << NUM_BITS_PER_MESSAGE) - 1)
#define MID_POINT ((1 << (NUM_BITS_PER_MESSAGE - 1)) - 1) // The middle value of a four bit message value.
#define SPEED_SCALE 100 // speed -100 to 100
#define TURN_SCALE (MAX_ANGLE - MIN_ANGLE) // turn -50 to 50


////////////////////////////////////////////
// Constants for message passing
// Message: Type followed by....
//     1-> next ONE byte is desired speed.
//     2-> next ONE bytes are line value.
//     3-> next FOUR bytes are Kd, Kp
//     4-> next FOUR bytes are Wait Time, Angle, Speed, Duration
#define NUM_CODES 4 // How many message types

#define SPEED_MSG 7
#define SPEED_LENGTH 1

#define PID_MSG 9
#define PID_LENGTH 4

#define TURN_MSG 11
#define TURN_LENGTH 4

#define LINE_MSG 13
#define LINE_LENGTH 1

#define MAX_MESSAGE_LENGTH (4)
#define REG_CODE_COUNT (6) // How many codes to send to start ProMini

// Constants for communicating with the Android (Serial port) App
//
// Single letter strings are sent on Serial output of the UNO to get info from messages.
// Info is finished with the END_MSG string.

#define END_MSG ":END:"
#define RFID_TAG_CODE "R"
#define SONAR_CODE "S"
#define MAGNET_CODE "M"
#define LINE_CODE "L"
#define STATUS_CODE "Z"
#define PARAMETERS_CODE "P"

// The following are the single letters received and interpreted on the UNO
// All are single characters - basic commands - except PARAMS
// PARAMS is followed by a sequence of NAME:NUM pairs.
// Each name is precisely two ASCII characters.  The sequence ends with END_PARAM
#define REPORT '?'
#define START 'F'
#define STOP 'S'
#define PARAMS '('
#define END_PARAM ')'

// The parameter list is as follows (unknown parameters are just ignored)
#define SPEED1 "s1" // How fast to do FIRST turn of Three Point Turn
#define SPEED2 "s1" // How fast to do SECOND turn of Three Point Turn
#define ANGLE1 "a1" // Wheel angle for FIRST turn of Three Point Turn
#define ANGLE2 "a2" // Wheel angle for SECOND turn of Three Point Turn
#define BASE_SPEED "sp" // How fast to do follow the line
#define PROP_K "kp" // Proportional constant for line following PID controller
#define DERIV_K "kd" // Derivative constant for line following PID controller

// Guesses:
#define ANGLE1_DEF 30
#define ANGLE2_DEF 95
#define SPEED1_DEF 50
#define SPEED2_DEF 50
#define PRE_TURN_WAIT 10
#define TURN_TACHO 15

/////////////////////////////////////
// I2C code for camera
#define CAMERA_ADDR (0x12)

/////////////////////////////////////
// Pins and other physical constants.
#if defined(ARDUINO_AVR_UNO) 
// PIN Settings
// SPI MOSI and MISO are pins 11 and 12, 13 cannot be used.
// Also note that the I2C pins are shared with A3 and A4 - so these cannot be used.
// To send a Turn to the ProMini we send a message then wait until the MOVING_PIN
//   has gone HIGH - indicating the turn has begun.
//   The turn has ended when we (eventually) see MOVING_PIN == LOW.
#define RFID_RST_PIN         4
#define RFID_SS_PIN          5
#define TRIGGER_PIN_LEFT     7  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN_LEFT        3  // Arduino pin tied to echo pin on ping sensor.
#define TRIGGER_PIN_RIGHT    6  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN_RIGHT       9  // Arduino pin tied to echo pin on ping sensor.
#define CAMERA_PIN           8 // Arduino pin tied to control pin on OpenMV camera
#define CLOCK_PIN            2 // digital pin - true-> follow or false-> steer
#define MOVING_PIN           10 // Am I trying to move?
#define MSG_PIN0             A0 // Four data pins used to send nibble to ProMini
#define MSG_PIN1             A1 
#define MSG_PIN2             A2 
#define MSG_PIN3             A3
#endif

#if defined ARDUINO_AVR_PRO
// PIN Settings
//   For comms with Arduino:
//     Uses one digital output pin: MOVING_PIN
//     Other pins are all used for input - clocked in from the pins A0-A3
//     Hence we interrupt on CLOCK_PIN Rising.
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
#define MIN_MOTOR_POWER 0u
#define MAX_MOTOR_POWER 255u
#endif
#endif

int nibbleToSpeed(int speed);
int nibbleToTurn(int where);
byte toSpeedNibble(int s);
byte toTurnNibble(int t);
byte reg_code(int index);
