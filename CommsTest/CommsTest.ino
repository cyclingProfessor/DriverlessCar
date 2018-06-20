#include "Arduino.h"

#define SPEED_MSG 7
#define SPEED_LENGTH 1

#define PID_MSG 9
#define PID_LENGTH 4

#define TURN_MSG 11
#define TURN_LENGTH 4

#define LINE_MSG 13
#define LINE_LENGTH 1

#define CLOCK_PIN            2 
#define MOVING_PIN           10

int pins[] = {A0, A1, A2, A3};
const char *HELP = "Type R:Register, S:stop, F:Forwards, B:Backwards, T:Turn";

void setup() {
  Serial.begin(115200);
  while (!Serial);    // Do nothing if no serial port is opened
    pinMode(MOVING_PIN, INPUT);
  for (int index = 0 ; index < 4 ; index++) {
    pinMode(pins[index], OUTPUT);
  }
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Program to test the message passing to the Ardiuno Pro Mini");
  Serial.println("Reboot the Pro Mini then send the registration codes first");
  Serial.println(HELP);
}

void send(byte b) {
  delay(1);
  for (int index = 0 ; index < 4 ; index++) {
    digitalWrite(pins[index], (b & (1 << index)) > 0 ? HIGH : LOW);
  }
  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(CLOCK_PIN, HIGH);    
}

////////////////////////////////////////////////////////////////////
//calculates inverse of: ((byte - MID_POINT) * SPEED_SCALE) / (MID_POINT + 1);
// since this is what is done on the Pro Mini
byte toSpeedByte(int s) {
  return (s * (7 + 1) / 100) + 7;
}

////////////////////////////////////////////////////////////////////
//calculates inverse of: NEUTRAL_ANGLE + (((byte - MID_POINT) * TURN_SCALE) / (MID_POINT + 1));
// since this is what is done on the Pro Mini
byte toTurnByte(int t) {
  return (t - 65) * (7 + 1) / 50 + 7;
}


void loop() {
  int speed1;
  int angle1;
  int speed2;
  int angle2;
  int distance;
  
  if (Serial.available() > 0) {
    int inByte = Serial.read();

    switch (inByte) {
      case 'R':
        for (int index = 0 ; index < 6 ; index++) {
          send((7 * index + 3) & 0xF);
        }
        Serial.println("Sent registration codes.");
        break;
      case 'S':
        send(SPEED_MSG);
        send(0);
        Serial.println("Sent a STOP message.");
        break;
      case 'B':
        send(SPEED_MSG);
        send(4);
        Serial.println("Sent a BACKWARDS message.");
        break;
      case 'F':
        send(SPEED_MSG);
        send(10);
        Serial.println("Sent a FORWARDS message.");
        break;
      case 'T':
        //Serial.println("Type in \"Speed1 Angle1 -Speed2 Angle2 Dist\" for a test three point turn)";
        speed1 = Serial.parseInt();
        angle1 = Serial.parseInt();
        speed2 = Serial.parseInt();
        angle2 = Serial.parseInt();
        distance = Serial.parseInt();
        Serial.print("Speed 1: "); Serial.print(speed1); Serial.print(" : "); Serial.println(toSpeedByte(speed1));
        Serial.print("Angle 1: "); Serial.print(angle1); Serial.print(" : "); Serial.println(toTurnByte(angle1));
        Serial.print("Speed 2: "); Serial.print(speed2); Serial.print(" : "); Serial.println(toSpeedByte(speed2));
        Serial.print("Angle 2: "); Serial.print(angle2); Serial.print(" : "); Serial.println(toTurnByte(angle2));
        Serial.print("Moving PIN before: "); Serial.println(digitalRead(MOVING_PIN));
        send(TURN_MSG);
        // Send four parameters from status:
        // 10 => 40 ms delay
        // speed1
        // turn1
        // 10 => 2.5 revolutions = 40cm
        send(10); //wait a bit
        send(toSpeedByte(speed1));
        send(toTurnByte(angle1));
        send(distance); // quite a long movement
        Serial.print("Moving PIN after: "); Serial.println(digitalRead(MOVING_PIN));

        Serial.println("Sent a TURN message.");

        Serial.println("Now wait for first move To End");
        while (digitalRead(MOVING_PIN) != HIGH) {
          //Serial.println("Moving PIN Low");
          delay(50);
        }
        while (digitalRead(MOVING_PIN) != LOW) {
          //Serial.println("Moving PIN High");
          delay(50);
        }
        Serial.println("That's it then");

        delay(200);
        send(TURN_MSG);
        send(10); //wait a bit
        send(toSpeedByte(-1 * speed2));
        send(toTurnByte(angle2));
        send(distance); // quite a long movement
        Serial.println("Sent a TURN message.");

        Serial.println("Now wait for Moving To End");
        while (digitalRead(MOVING_PIN) != HIGH) {}
        while (digitalRead(MOVING_PIN) != LOW) {}
        Serial.println("That's it then");
        break;
      case 'h':
      case '?':
        Serial.println(HELP);
        break;
      default:
        break;
    }
  }
}
