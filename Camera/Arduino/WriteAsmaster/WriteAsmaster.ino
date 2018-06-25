#include <Wire.h>
#define BAUD_RATE 19200
#define CHAR_BUF 128

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(1000); // Give the OpenMV Cam time to bootup.
}

int counter = 0;
void loop() {
  Wire.beginTransmission(0x12);
  Wire.write(counter++);
  int retval = Wire.endTransmission();
  delay(500); // Don't loop to quickly.
}
