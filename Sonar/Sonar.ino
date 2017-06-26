#include <Wire.h>
#include <NewPing.h>

#define TRIGGER_PIN  A4  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN     A5  // Arduino pin tied to echo pin on ping sensor.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, 50); // NewPing setup of pins and maximum distance.

unsigned int pingSpeed = 50; // How frequently are we going to send out a ping 
unsigned long pingTimer;     // Holds the next ping time.

void setup() {
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  pingTimer = millis(); // Start now.
  Wire.begin();
  Wire.end(); // Sketch hangs if we start Wire but do not end it
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  if (millis() >= pingTimer) {   // pingSpeed milliseconds since last ping, do another ping.
    pingTimer += pingSpeed;      // Set the next ping time.
    // alternate sensors
    Serial.println(sonar.ping_cm());
  }
}
