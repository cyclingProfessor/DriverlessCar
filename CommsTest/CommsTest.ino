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

void loop() {
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
        Serial.print("Moving PIN before: "); Serial.println(digitalRead(MOVING_PIN));
        send(TURN_MSG);
        // Send four parameters from status - as a test send four values of 10:
        // 10 * 4 == 40 ms delay
        // 10 speed -> (10-7) * 50 / 7 = 20
        // 10 turn -> quite hard turn.
        // 10 * 5 = 50 clicks running -> 2.5 revolutions = 40cm
        send(10); //wait a bit
        send(12); // quite fast
        send(10); // turn one way
        send(12); // quite a long movement
        Serial.print("Moving PIN after: "); Serial.println(digitalRead(MOVING_PIN));
        // Now wait for the MOVING_PIN to go low! With a cheeky timeout
        unsigned long start;
        start = millis();
        Serial.println("Sent a TURN message.");

        Serial.println("Now wait for first move To End");
        delay(100);
        while (digitalRead(MOVING_PIN) != LOW) {
          Serial.println("Moving PIN High");
          delay(50);
        }
        Serial.println("That's it then");

        delay(200);
        send(TURN_MSG);
        send(10); //wait a bit
        send(4); // quite fast backwards
        send(4); // turn the other way
        send(12); // quite a long movement
        Serial.println("Sent a TURN message.");

        // Now wait for the MOVING_PIN to go low! With a cheeky timeout
        start = millis();
        Serial.println("Now wait for Moving To End");
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
