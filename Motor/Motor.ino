#include "Encoder.h"
#include "Ticker.h"
#include <ESP8266WiFi.h>
#include "Servo.h"

const char *Version = "Motor/Servo Controller V1.1"; 
const char *ssid = "ESPcarAP";
const char *password = "thisisnotaspoon";
int usingWiFi = false;

/***********************************************************************************************************\
* See https://docs.google.com/spreadsheets/d/1bhfsjG0d4l3q6j9Bx6Ja7MYPekk8pvvGrTb8yf0j5cc/edit?usp=sharing
* for details on ESP8266 pinout and its relationship to D1 Mini
\***********************************************************************************************************/

//WiFi Controls:
//
// + -Motor more forwards
// - -Motor more backwards
// 0 -Motor stop
// s - print stats
// L - Turn servo to Left
// R - Turn Servo to Right
// C - Centre servo
//
// Since this is run by the Wemos D1 Lite it can read from either Serial or from the Wifi

// UART protocol also has 'W' to toggle WiFi, '|n' and '~n' for direct speed and turn control.
char SPEED_CONTROL = '|';
char TURN_CONTROL = '~';

enum Command {
  NONE, HELP, WIFI, REPORT, TURN_LEFT, TURN_RIGHT, TURN_STRAIGHT, MOTOR_PLUS, MOTOR_MINUS, MOTOR_STOP, SET_SPEED, SET_TURN, BLOCK_BACK, BLOCK_FRONT
};

// Change to using Pololo TB6612FNG Mosfet controller

Command translate[128];
int commandArgument = 10;
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
volatile int speedEstimate[4]; // Average over 4 readings.
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
  for (int command = 0 ; command < 128 ; command++) {
    translate[command] = NONE;
  }
  translate['H'] = translate['?'] = HELP;
  translate['+'] = MOTOR_PLUS;
  translate['-'] = MOTOR_MINUS;
  translate['0'] = MOTOR_STOP;
  translate['s'] = REPORT;
  translate['L'] = TURN_LEFT;
  translate['R'] = TURN_RIGHT;
  translate['C'] = TURN_STRAIGHT;
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

Command getCommand() {
  if (usingWiFi) { // Is the WiFi server running?
    if (!client || !client.connected()) { // Does it NOT a current client?
      client = server.available(); // Get a WifFi client.
    }
    if (client) { // Do we have a current WiFi client?
      if (client.connected()) {
        if (client.available() > 0) {
          return(translate[client.read()]); // Return point here for reading a WiFi
        }
      } else { // Client has died - kill our WiFiClient object.
        client.stop();
      }
    }
  }

  // Here we have not managed to read from Wifi: try Serial protocol
  if (Serial.available()) {
    int next = Serial.read();
    if (translate[next] != NONE) {
      return translate[next];
    }
    // Protocol bytes are always not control bytes so cannot be misunderstood.  
    // They are '|' for speed and '~' for Turn, followed by an argument byte
    // Speed : take of ten from byte value, and multiply by ten.
    if (next == 'W') {
      return WIFI;
    }
    if (next == 'F') {
      return BLOCK_FRONT;
    }
    if (next == 'B') {
      return BLOCK_BACK;
    }
    // There will be another character - just wait here now.
    while (!Serial.available()) {
    }
    if (next == SPEED_CONTROL) {
      commandArgument = (Serial.read() - 10) * 10;
      return SET_SPEED;
    }
    if (next == TURN_CONTROL) {
      commandArgument = 10 * Serial.read();
      return SET_TURN;
    }
  }
  return NONE;  // Do nothing!
}

void report(Stream &out) {
  out.print("EstimatedSPeed: ");
  out.println(currentSpeed);
  out.print("Desired Speed: ");
  out.println(desiredSpeed);
  out.print("Turn Angle: ");
  out.println(angle); 
  out.print("WiFi: ");
  out.println(usingWiFi ? "ON" : "OFF");
  out.flush();
}

void help(Stream & out) {
  out.print(Version);
  out.print(HELP);
  out.flush();
}


void loop() {
  int turnTo;
  if (turnBack) {
    if (millis() > waitTime + DELAY) {
      turnBack = false;
      servo.write(angle);
    }
  }

  switch (getCommand()) {
    case WIFI: // Only UART - Toggle Wifi
      toggleWiFi();
      break;
    case HELP: // 'H' or '?'
      if (usingWiFi) help(client);
       help(Serial);
      break;
    case MOTOR_PLUS: // Only from Wifi - Motor 1 Forward
      desiredSpeed = min(desiredSpeed + SPEED_STEP, 90);
      break;
    case MOTOR_MINUS: // Only Wifi - Motor 1 Stop (Brake)
      desiredSpeed = max(desiredSpeed - SPEED_STEP, -60);
      break;
    case MOTOR_STOP: 
      desiredSpeed = 0;
      break;        
    case TURN_LEFT:
      // always arrive from the same direction (from a higher angle)
      angle -= 10;;
      servo.write(angle);
      break;
    case TURN_RIGHT:
      // always arrive from the same direction (from a higher angle)
      angle += 10;
      servo.write(angle+GLITCH);
      waitTime = millis();
      turnBack = true;
      break;
    case TURN_STRAIGHT:
      angle = NEUTRAL_ANGLE;
      servo.write(angle+GLITCH);
      waitTime = millis();
      turnBack = true;
      break;
    case REPORT:
      if (usingWiFi) report(client);
        report(Serial);
      break;
    case BLOCK_FRONT:
      // Blockage in front
      desiredSpeed = min(desiredSpeed, 0);
      break;
    case BLOCK_BACK:
      // Blockage behind
      desiredSpeed = max(desiredSpeed, 0);
      break;
    case SET_SPEED:
      desiredSpeed = max(min(commandArgument, 90), -60);
      break;
    case SET_TURN:
      turnTo = commandArgument;
      if (turnTo > angle) { // Turn Right
        // always arrive from the same direction (from a higher angle)
        turnTo += GLITCH;
        waitTime = millis();
        turnBack = true;
      } 
      servo.write(turnTo);
      angle = commandArgument;
      break;
    default:
      break;
  }
}
