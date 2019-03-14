void setup() {
  Serial1.begin(115200);
  Serial5.begin(115200);
  Serial2.begin(115200);
}

void loop() {
  if (Serial1.available()) {      // If anything comes in Serial (USB/pins 0,1/BLE),
    char next = Serial1.read();
    Serial1.println(next);
    Serial5.write(next);   // read it and send it to D1
  }

  if (Serial5.available()) {
    Serial1.println("Wemos D1 Lite");
    while (Serial5.available()) {
      Serial1.write(Serial5.read());
    }
    Serial1.println();
  }

  if (Serial2.available()) {
    Serial1.println("Camera");
    while (Serial2.available()) {
      Serial1.write(Serial2.read());
    }
    Serial1.println();
  }
}
