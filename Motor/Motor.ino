#include "Encoder.h"
#include "Ticker.h"
#include <ESP8266WiFi.h>
#include "Servo.h"

const char *Version = "Motor/Servo Controller V1.0"; 
const char *ssid = "ESPcarAP";
const char *password = "thisisnotaspoon";
WiFiClient client;

/***********************************************************************************************************\
* See https://docs.google.com/spreadsheets/d/1bhfsjG0d4l3q6j9Bx6Ja7MYPekk8pvvGrTb8yf0j5cc/edit?usp=sharing
* for details on ESP8266 pinout and its relationship to D1 Mini
\***********************************************************************************************************/

//Keyboard Controls:
//
// + -Motor more forwards
// - -Motor more backwards
// 0 -Motor stop
// s - print stats
// L - trun servo to Left
// R - Turn Servo to Right
// C - Centre servo
//

const char *HELP = "H/? - This help meassage\n"
 "+ -Motor more forwards\n"
 "- -Motor more backwards\n"
 "0 -Motor stop\n"
 "s - print stats\n"
 "L - trun servo to Left\n"
 "R - Turn Servo to Right\n"
 "C - Centre servo\n";

// Declare L298N Dual H-Bridge Motor Controller directly since there is not a library to load.
// Change to using Pololo TB6612FNG Mosfet controller

/////////////////////////////////////// Pin Assignments /////////////////////////
int dir1Pin = D3; // => to TB6612FNG Pin INA1
int dir2Pin = D4; // => to TB6612FNG Pin INA2
int speedPin = D5; // => to TB6612FNG Pin PWMA (Needs to be a PWM pin to be able to control motor speed)
int steerPin = D6; // => Servo Control Wire (Orange) (Needs to be a PWM pin to be able to control servo angle)
int servoMotorPower = D8; // => Mosfet Board Signal, TB6612FNG STDY (Needs to start LOW (which is very rare on ESP8266))
int quadPinYellow = D1; // Needs Interupt
int quadPinGreen = D2; // Needs Interupt

///////////////////////////// Tunable constants /////////////////////////////////
const int speedList[] = {-1000, -800, -600, -400, 0, +400, +600, +800, +1000};
const int speedSize = sizeof(speedList) / sizeof(speedList[0]);
const int stopIndex = 4;
int speedIndex = stopIndex;

const int NEUTRAL_ANGLE = 90;
int angle = NEUTRAL_ANGLE;

/////////////////// Hardware considerations ////////////////////////////
const int MAX_SERVO = 2000;
const int MIN_SERVO = 1000;
const int GLITCH = 40;  // How much to overturn to compensate for hysteresis
const int DELAY= 45; // How long to wait for a trun to complete.
unsigned long waitTime;
bool turnBack = false;

////////////////////// Speed and position calculations //////////////////
volatile int speedEstimate = 0;
volatile int lastPosition = 0;
volatile unsigned long lastTime = 0L;
int diffP = 0;
int diffT = 0;

//////////////////////////////// Objects ////////////////////////////
Encoder enc(quadPinYellow,quadPinGreen);
Ticker speedSetter;
WiFiServer server(3000);
Servo   servo;

///////////////////////////////// CODE //////////////////////////////////////////
void setup() {  // Setup runs once per reset
  Serial.begin(115200);
  Serial.println(Version);
  delay(1000);  // Wait for pins to settle.
  pinMode(steerPin, OUTPUT);
  servo.attach(steerPin, MIN_SERVO, MAX_SERVO);
  servo.write(angle);
  delay(10);  // Wait for servo ctrl pin to settle.
  
  pinMode(servoMotorPower, OUTPUT);
  digitalWrite(servoMotorPower, HIGH);

  //Define TB6612FNG Dual H-Bridge Motor Controller Pins
  pinMode(dir1Pin, OUTPUT);  
  pinMode(dir2Pin, OUTPUT);
  pinMode(speedPin, OUTPUT);
  analogWrite(speedPin, 0);//Sets speed variable via PWM
  digitalWrite(dir1Pin, HIGH);
  digitalWrite(dir2Pin, HIGH);      
  
  speedSetter.attach_ms(10, setSpeed);

  int retID = WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  server.begin();
}

void setSpeed() {
  unsigned long time = micros(); // Remember to check for overflow.
  int position = enc.read();
  if (time > lastTime) {
    // update speed estimate
    diffP = position - lastPosition;
    diffT = time - lastTime;
    speedEstimate = 10000L * diffP / diffT;
  }
  lastTime = time;
  lastPosition = position;
}

void loop() {
  int inByte = 0;
  if (!client || !client.connected()) {
    client = server.available();
  }
  if (client) {
    if (client.connected()) {
      if (client.available() > 0) {
        inByte = client.read();
      }
    } else {
      client.stop();
    }
  }

  if (turnBack) {
    if (millis() > waitTime + DELAY) {
      turnBack = false;
      servo.write(angle);
    }
  }

  switch (toupper(inByte)) {
    case 'H':
    case '?':
      client.println(Version);
      client.println(HELP);
      client.flush();
      break;
      
    case '+': // Motor 1 Forward
      speedIndex = min(speedIndex + 1, speedSize);
      break;
    case '-': // Motor 1 Stop (Brake)
      speedIndex = max(speedIndex - 1, 0);
      break;
    case '0':
      speedIndex = stopIndex;
      break;        
    case 'L':
      // always arrive from the same direction (from a higher angle)
      angle -= 10;;
      servo.write(angle);
      break;
    case 'R':
      // always arrive from the same direction (from a higher angle)
      angle += 10;
      servo.write(angle+GLITCH);
      waitTime = millis();
      turnBack = true;
      break;
    case 'C':
      angle = NEUTRAL_ANGLE;
      servo.write(angle+GLITCH);
      waitTime = millis();
      turnBack = true;
      break;
    case 'S':
      client.print("diffT: ");
      client.println(diffT);
      client.print("DiffP: ");
      client.println(diffP);
      client.print("EstimatedSPeed: ");
      client.println(speedEstimate);
      client.println("   "); // Creates a blank line printed on the client monitor         
      client.print("Encoder Value");
      client.println(enc.read());
      client.print("Speed: ");
      client.println(speedList[speedIndex]);
      client.flush();
      break;
    default:
      break;
  }
  if (speedList[speedIndex] > 0) {
    analogWrite(speedPin, speedList[speedIndex]);//Sets speed variable via PWM
    digitalWrite(dir1Pin, LOW);
    digitalWrite(dir2Pin, HIGH);
  } else if (speedList[speedIndex] < 0) {
    analogWrite(speedPin, -1 * speedList[speedIndex]);//Sets speed variable via PWM
    digitalWrite(dir1Pin, HIGH);
    digitalWrite(dir2Pin, LOW);
  } else {
    analogWrite(speedPin, 0);//Sets speed variable via PWM
    digitalWrite(dir1Pin, HIGH);
    digitalWrite(dir2Pin, HIGH);      
  }
}
