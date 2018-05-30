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

void setup() {
    Serial.begin(115200);
    while (!Serial);    // Do nothing if no serial port is opened
    pinMode(MOVING_PIN, INPUT);
    for (int index = 0 ; index < 4 ; index++) {
      pinMode(pins[index], OUTPUT);
    }
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    for (int index = 0 ; index < 6 ; index++) {
      send((7 * index + 3) & 0xF);
    }
}

void send(byte b) {
    delay(1);
    for (int index = 0 ; index < 4 ; index++) {
      digitalWrite(pins[index], (b & (1 << index)) > 0 ? HIGH : LOW);
    }
    digitalWrite(CLOCK_PIN, LOW);
    digitalWrite(CLOCK_PIN, HIGH);    
}

byte count = 11; // 7 means speed 0.
void loop() {
    send(SPEED_MSG);
    send(count);
    count = 14 - count;

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
}