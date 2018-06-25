#include "CarLib.h"

int pins[] = {MSG_PIN0, MSG_PIN1, MSG_PIN2, MSG_PIN3};
const char *HELP = "Type R:Register, S:stop, F:Forwards, B:Backwards, T sp1 ang1 sp2 ang2 dist:3-pointTurn";

void setup() {
  Serial.begin(BAUD_RATE);
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
          send(reg_code(index));
        }
        Serial.println("Sent registration codes.");
        break;
      case 'S':
        send(SPEED_MSG);
        send(toSpeedNibble(0));
        Serial.println("Sent a STOP message.");
        break;
      case 'B':
        send(SPEED_MSG);
        send(toSpeedNibble(-40));
        Serial.println("Sent a BACKWARDS message.");
        break;
      case 'F':
        send(SPEED_MSG);
        send(toSpeedNibble(50));
        Serial.println("Sent a FORWARDS message.");
        break;
      case 'T':
        //Serial.println("Type in \"Speed1 Angle1 -Speed2 Angle2 Dist\" for a test three point turn)";
        speed1 = Serial.parseInt();
        angle1 = Serial.parseInt();
        speed2 = Serial.parseInt();
        angle2 = Serial.parseInt();
        distance = Serial.parseInt();
        Serial.print("Speed 1: "); Serial.print(speed1); Serial.print(" : "); Serial.println(toSpeedNibble(speed1));
        Serial.print("Angle 1: "); Serial.print(angle1); Serial.print(" : "); Serial.println(toTurnNibble(angle1));
        Serial.print("Speed 2: "); Serial.print(speed2); Serial.print(" : "); Serial.println(toSpeedNibble(speed2));
        Serial.print("Angle 2: "); Serial.print(angle2); Serial.print(" : "); Serial.println(toTurnNibble(angle2));
        Serial.print("Moving PIN before: "); Serial.println(digitalRead(MOVING_PIN));
        send(TURN_MSG);
        // Send four parameters from status:
        // 10 => 40 ms delay
        // speed1
        // turn1
        // 10 => 2.5 revolutions = 40cm
        send(10); //wait a bit
        send(toSpeedNibble(speed1));
        send(toTurnNibble(angle1));
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
        send(toSpeedNibble(-1 * speed2));
        send(toTurnNibble(angle2));
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
