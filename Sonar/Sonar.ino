#include <NewPing.h>

#define TRIGGER_PIN  9  // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN     10  // Arduino pin tied to echo pin on ping sensor.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, 50); // NewPing setup of pins and maximum distance.

unsigned int pingSpeed = 50; // How frequently are we going to send out a ping 
unsigned long pingTimer;     // Holds the next ping time.

void setup() {
  Serial1.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  pingTimer = millis(); // Start now.
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  if (millis() >= pingTimer) {   // pingSpeed milliseconds since last ping, do another ping.
    pingTimer += pingSpeed;      // Set the next ping time.
    // alternate sensors
    Serial1.println(sonar.ping_cm());
  }
}
