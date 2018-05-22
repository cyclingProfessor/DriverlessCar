#include "Servo.h"

Servo servo;  // servo control object
#define STEER_PIN            10
#define MAX_SERVO 1798
#define MIN_SERVO 771
#define MIN_ANGLE 10u //neutral is 65-70 degrees
#define NEUTRAL_ANGLE 65u
#define MAX_ANGLE 105u

void setup()
{
  Serial.begin(115200);
  servo.attach(STEER_PIN, MIN_SERVO, MAX_SERVO);
  Serial.println("Enter Position");
}

int num = 0;
int angle = NEUTRAL_ANGLE;
void loop()
{
  int inCh = Serial.read();
  if (isDigit(inCh)) {
    num = num * 10 + inCh - '0';
  } else if (inCh == '\n') {
    Serial.print(num);
    Serial.println(" degrees");
    num = constrain(num, MIN_ANGLE, MAX_ANGLE); 
    Serial.print("After constraining, ");
    Serial.print(num);
    Serial.println(" degrees");
    Serial.println("Enter Position");
    angle = num;
    num = 0;
  }
  servo.write(angle);
  delay(15);

}

