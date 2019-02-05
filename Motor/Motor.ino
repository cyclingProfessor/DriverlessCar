#include "Encoder.h"
#include "Ticker.h"
#include <ESP8266WiFi.h>
#include "Servo.h"

const char *ssid = "ESPcarAP";
const char *password = "thisisnotaspoon";
WiFiClient client;

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

// Declare L298N Dual H-Bridge Motor Controller directly since there is not a library to load.
// Change to using Pololo TB6612FNG Mosfet controller

// Motor 1
int dir1PinA = D5;
int dir2PinA = D6;
int speedPinA = D2; // Needs to be a PWM pin to be able to control motor speed
int enablePin = D3; // Enable Motor control
int steerPin = D8;
int servoPower = D0;

const int MAX_SERVO = 2000;
const int MIN_SERVO = 1000;
const int MIN_ANGLE = 10; //neutral is 55 degrees
const int NEUTRAL_ANGLE = 90;
const int MAX_ANGLE = 105;

volatile int speedEstimate = 0;
volatile int lastPosition = 0;
volatile unsigned long lastTime = 0L;
int diffP = 0;
int diffT = 0;
const int speedList[] = {-1000, -800, -600, -400, 0, +400, +600, +800, +1000};
const int speedSize = sizeof(speedList) / sizeof(speedList[0]);
const int stopIndex = 4;
int speedIndex = stopIndex;
int angle = NEUTRAL_ANGLE;
unsigned long waitTime;
bool turnBack = false;
const int GLITCH = 40;  // How much to overturn to compensate for hysteresis
const int DELAY= 45; // How long to wait for a trun to complete.

Encoder enc(D1,D7);
Ticker speedSetter;
WiFiServer server(3000);
Servo   servo;                               // servo control object

void setup() {  // Setup runs once per reset
  pinMode(servoPower, OUTPUT);
  digitalWrite(servoPower, LOW);
  pinMode(steerPin, OUTPUT);
  servo.attach(steerPin, MIN_SERVO, MAX_SERVO);
  servo.write(angle);

  delay(1000);
  //Define TB6612FNG Dual H-Bridge Motor Controller Pins
  pinMode(dir1PinA, OUTPUT);  
  pinMode(dir2PinA, OUTPUT);
  pinMode(speedPinA, OUTPUT);
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, HIGH);
  digitalWrite(servoPower, HIGH);
  
  speedSetter.attach_ms(10, setSpeed);

  int retID = WiFi.softAP(ssid, password);             // Start the access point
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

  switch (inByte) {
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
    case 's':
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
    analogWrite(speedPinA, speedList[speedIndex]);//Sets speed variable via PWM
    digitalWrite(dir1PinA, LOW);
    digitalWrite(dir2PinA, HIGH);
  } else if (speedList[speedIndex] < 0) {
    analogWrite(speedPinA, -1 * speedList[speedIndex]);//Sets speed variable via PWM
    digitalWrite(dir1PinA, HIGH);
    digitalWrite(dir2PinA, LOW);
  } else {
    analogWrite(speedPinA, 0);//Sets speed variable via PWM
    digitalWrite(dir1PinA, HIGH);
    digitalWrite(dir2PinA, HIGH);      
  }
}
