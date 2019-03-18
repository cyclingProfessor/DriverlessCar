/*
Raw data example
This example reads raw readings from the magnetometer gyroscope and the accelerometer and then
displays them in serial monitor.
*/

#include <MPU9255.h>//include MPU9255 library

MPU9255 mpu;

void setup() {
  Serial1.begin(115200);//initialize Serial port

  if(mpu.init())
  {
  Serial1.println("initialization failed");
  }
  else
  {
  Serial1.println("initialization succesful!");
  }

}

void loop() {
  mpu.read_acc();//get data from the accelerometer
  mpu.read_gyro();//get data from the gyroscope
  mpu.read_mag();//get data from the magnetometer

  //print all data in serial monitor
  Serial1.print("AX: ");
  Serial1.print(mpu.ax);
  Serial1.print(" AY: ");
  Serial1.print(mpu.ay);
  Serial1.print(" AZ: ");
  Serial1.print(mpu.az);
  Serial1.print("    GX: ");
  Serial1.print(mpu.gx);
  Serial1.print(" GY: ");
  Serial1.print(mpu.gy);
  Serial1.print(" GZ: ");
  Serial1.print(mpu.gz);
  Serial1.print("    MX: ");
  Serial1.print(mpu.mx);
  Serial1.print(" MY: ");
  Serial1.print(mpu.my);
  Serial1.print(" MZ: ");
  Serial1.println(mpu.mz);
  delay(1000);
}
