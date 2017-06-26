#include <Servo.h>  // servo library
#define STEER_PIN            10
#define MAX_SERVO 1798
#define MIN_SERVO 771
#define MIN_ANGLE 40u //neutral is 55 degrees
#define NEUTRAL_ANGLE 55u
#define MAX_ANGLE 70u

Servo servo;  // servo control object

void setup()
{
  Serial.begin(115200);
  servo.attach(STEER_PIN, MIN_SERVO, MAX_SERVO);
  servo.write(NEUTRAL_ANGLE);
}


void loop()
{
}


