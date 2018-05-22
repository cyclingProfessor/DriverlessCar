#include <Wire.h>
#define BAUD_RATE 19200
#define CHAR_BUF 128
#define GET_VALUE 0
#define CALIBRATE 1

int mode = CALIBRATE;
void setup() {
  Serial.begin(BAUD_RATE);
  Wire.begin();
  pinMode(A1, OUTPUT);
  digitalWrite(A1, LOW);
  Serial.println("In Calibrate mode.  Type 0 to read line on the camera.");
  delay(1000); // Give the OpenMV Cam time to bootup.
}

// This needs careful timeout management.
void loop() {
  if (Serial.available() > 0) {
    int inByte = Serial.read();

    switch (inByte) {
      case '0':
        digitalWrite(A1, HIGH);
        mode = GET_VALUE;
        Serial.println("Mode set to read camera - look for read, white, green and magenta as the car is moved.");
        break;
      case '1':
        digitalWrite(A1, LOW);
        mode = GET_VALUE;
        Serial.println("Mode set to calibrate - look for Blue flashing changing  to Blue when it finds the line.");
        break;
      case 'h':
      default:
        Serial.println("Type 0 to read line or 1 to calibrate the camera.");
        break;
    }
  }
  if (mode == GET_VALUE) {
    int32_t temp = 0;
    char buff[CHAR_BUF] = {0};

    Serial.println("Asking for data");
    Serial.println(Wire.requestFrom(0x12, 9));
    Serial.println("Got some data");
    delay(1); // Give some setup time...
    while(Wire.available()) {
        buff[temp++] = Wire.read();
    }
    Serial.println(buff);
  }
  delay(30); // Don't loop to quickly.
}
