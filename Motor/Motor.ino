#include "Encoder.h"
#include "Ticker.h"
#include <ESP8266WiFi.h>
#include "Servo.h"

const char *Version = "Motor/Servo Controller V1.0"; 
const char *ssid = "ESPcarAP";
const char *password = "thisisnotaspoon";
int usingWiFi = false;

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
// Since this is run by the Wemos D1 Lite it can read from either Serial, or fron the Wifi

const char *HELP = "H/? - This help meassage\n"
 "+ -Motor more forwards\n"
 "- -Motor more backwards\n"
 "0 -Motor stop\n"
 "s - print stats\n"
 "L - trun servo to Left\n"
 "R - Turn Servo to Right\n"
 "C - Centre servo\n"
 "W - Toggle WiFi Access Point\n";

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
#define SPEED_STEP 10
int desiredSpeed = 0;

const int NEUTRAL_ANGLE = 90;
int angle = NEUTRAL_ANGLE;

/////////////////// Hardware considerations ////////////////////////////
const int MAX_SERVO = 2000;
const int MIN_SERVO = 1000;
const int GLITCH = 40;  // How much to overturn to compensate for hysteresis
const int DELAY= 45; // How long to wait for a turn to complete.
unsigned long waitTime;
bool turnBack = false;

////////////////////// Speed and position calculations //////////////////
volatile int spCalcOfffset = 0;
volatile int speedEstimate[4]; // Average over 4 readngs.
volatile int currentSpeed = 0;
volatile int lastPosition = 0;
volatile unsigned long lastTime = 0L;
int diffP = 0;
int diffT = 0;

//////////////////////////////// Objects ////////////////////////////
Encoder enc(quadPinYellow,quadPinGreen);
Ticker speedSetter;
WiFiServer server(3000);
WiFiClient client;
Servo   servo;

///////////////////////////////// CODE //////////////////////////////////////////
void setup() {  // Setup runs once per reset
  delay(1000);  // Wait for pins to settle.
  Serial.begin(115200);
  Serial.println(Version);
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
}

void setSpeed() {
  static int counter = 0;
  static int motorPower = 0;
  static const int startPower = 300;
  
  unsigned long time = micros(); // Remember to check for overflow.
  int position = enc.read();
  if (time > lastTime) {
    // update speed estimate
    diffP = lastPosition - position;
    diffT = time - lastTime;
    // Circular buffer the speed estimate.
    
    // spCalcOfffset points to last entered speed - so remove this from total.
    currentSpeed -= speedEstimate[spCalcOfffset];
    
    // Put in new guess
    speedEstimate[spCalcOfffset] = (10000L * diffP) / diffT;
    currentSpeed += speedEstimate[spCalcOfffset];
    
    spCalcOfffset = (spCalcOfffset + 1) & 3;
  }
  lastTime = time;
  lastPosition = position;

  // Now set speed itself! Only every ten clicks
  counter ++;
  if (counter > 9) {
    counter = 0;
    if (desiredSpeed == 0) { // Stop is easy.
      analogWrite(speedPin, 0);//Sets speed variable via PWM
      digitalWrite(dir1Pin, HIGH);
      digitalWrite(dir2Pin, HIGH);
      return;      
    }
    if (currentSpeed > -5 && currentSpeed < 5) { // Overcome stopped friction.
        motorPower = startPower;
    }

    boolean forwards = (desiredSpeed >= 0);
    if (currentSpeed < desiredSpeed - 10) {
        motorPower += forwards ? 10 : -10 ;
    }
    if (currentSpeed > desiredSpeed + 10) {
        motorPower += forwards ? -10 : 10 ;
    }
    analogWrite(speedPin, motorPower);//Sets speed variable via PWM
    digitalWrite(dir1Pin, forwards ? HIGH : LOW);
    digitalWrite(dir2Pin, forwards ? LOW : HIGH);
  }
}

void toggleWiFi() {
  usingWiFi = ! usingWiFi;
  if (usingWiFi) {
    WiFi.softAP(ssid, password);
//      IPAddress myIP = WiFi.softAPIP();
    server.begin();
  } else {
    if (client) {
      client.stop();
    }
    server.stop();
    WiFi.softAPdisconnect(true); 
  }
}

int getByte() {
  if (usingWiFi) { // Is the WiFi server running?
    if (!client || !client.connected()) { // Does it NOT a current client?
      client = server.available(); // Get a WifFi client.
    }
    if (client) { // Do we have a current WiFi client?
      if (client.connected()) {
        if (client.available() > 0) {
          return(client.read()); // Return point here for reading a WiFi 
        }
      } else { // Client has died - kill out WiFiClient object.
        client.stop();
      }
    }
  }

  // Here we have not managed to read from Wifi: try Serial
  if (Serial.available()) {
    return Serial.read();
  }
  return 0;  // Do nothing!
}

void report(Stream &out) {
  out.print("EstimatedSPeed: ");
  out.println(currentSpeed);
  out.print("Desired Speed: ");
  out.println(desiredSpeed);
  out.flush();
}

void help(Stream & out) {
  out.print(Version);
  out.print(HELP);
  out.flush();
}


void loop() {
  int inByte = getByte();
  
  if (turnBack) {
    if (millis() > waitTime + DELAY) {
      turnBack = false;
      servo.write(angle);
    }
  }

  switch (toupper(inByte)) {
    case 'W':
      toggleWiFi();
      break;
    case 'H':
    case '?':
      if (usingWiFi) help(client);
       help(Serial);
      break;
    case '+': // Motor 1 Forward
      desiredSpeed = min(desiredSpeed + SPEED_STEP, 90);
      break;
    case '-': // Motor 1 Stop (Brake)
      desiredSpeed = max(desiredSpeed - SPEED_STEP, -60);
      break;
    case '0':
      desiredSpeed = 0;
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
      if (usingWiFi) report(client);
        report(Serial);
      break;
    case 'F':
      // Blockage in front
      desiredSpeed = min(desiredSpeed, 0);
      break;
    case 'B':
      // Blockage behind
      desiredSpeed = max(desiredSpeed, 0);
      break;
    default:
      break;
  }
}
