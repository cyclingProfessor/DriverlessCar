/*
  The sensor outputs provided by the library are the raw 16-bit values
  obtained by concatenating the 8-bit high and low magnetometer data registers.
  They can be converted to units of gauss using the
  conversion factors specified in the datasheet for your particular
  device and full scale setting (gain).

  Example: An LIS3MDL gives a magnetometer X axis reading of 1292 with its
  default full scale setting of +/- 4 gauss. The GN specification
  in the LIS3MDL datasheet (page 8) states a conversion factor of 6842
  LSB/gauss (where LSB means least significant bit) at this FS setting, so the raw
  reading of 1292 corresponds to 1292 / 6842 = 0.1888 gauss.
*/

#include <Wire.h>
#include <LIS3MDL.h>

LIS3MDL mag;

char report[80];

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  if (!mag.init())
  {
    Serial.println("Failed to detect and initialize magnetometer!");
    while (1);
  }

  mag.enableDefault();
  mag.writeReg(LIS3MDL::CTRL_REG2, 0x60);
}

unsigned long t = millis();
void loop()
{
  if (millis() > t + 1000) {
    t = millis();
    mag.read();
    double L2Answer = sqrt((double)((long)mag.m.x * mag.m.x + (long)mag.m.y * mag.m.y + (long)mag.m.z * mag.m.z)); // L2 norm of magnetic strength
    snprintf(report, sizeof(report), "%6d %6d %6d 2:", mag.m.x, mag.m.y, mag.m.z, L2Answer);
    Serial.print(report); Serial.println(L2Answer);
  }
}
