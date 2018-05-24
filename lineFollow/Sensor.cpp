#include "lineFollow.h"
#include <NewPing.h>

///////////////////////////////////////////////////////////////////////////////////////////////
// Interupt for speed sensor
/////////// Circular buffer of previous tacho clicks ////////////////////////
volatile unsigned long usecTimes[TIME_AVERAGE];
volatile byte currentSpeedTime = TIME_AVERAGE_MASK; // Will start at position zero in the circular buffer

void addToTacho() {
  long t = millis();
  if (t == usecTimes[currentSpeedTime]) {// TOO FAST => Bounce
    return;
  }
  currentSpeedTime = (currentSpeedTime + 1) & TIME_AVERAGE_MASK; // increment index
  tacho += 1;
  usecTimes[currentSpeedTime] = t; // Put time into place
}

///////////////////////////////////////////////////////////////////////////////////////
void SpeedSensor::start() {
  // Make the speed/distance sensor active
  unsigned long nowTime = millis();
  unsigned startTime = nowTime - (TIME_AVERAGE * 10000U); 
  for (int i = 0 ; i < TIME_AVERAGE ; i++ ) {
    usecTimes[i] = startTime + 10000U * i;  // Now we set initial speed to zero.
  }
  attachInterrupt(digitalPinToInterrupt(SPEED_SENSOR_INTR), addToTacho, RISING);
  moving =true;
}
void SpeedSensor::stop() {
  detachInterrupt(digitalPinToInterrupt(SPEED_SENSOR_INTR));
  moving = false;
}
void SpeedSensor::restart() {
  this->stop();
  this->start();
}

unsigned SpeedSensor::getNormalisedValue() {
  if (!moving) {
    return 0;
  }
  // There are twenty clicks per revolution = 10cm
  // Hence this should be of the order of 1sec - so approx 1 thousand
  // Travelling 1m per sec is speed 100.  10cm per sec is speed 10.

  // Copy index so that it is coherent throughout calculation
  byte index = currentSpeedTime;
  //DEBUG
  //Serial.print(index) ; Serial.print(", ");
  //for (int i = 0 ; i < TIME_AVERAGE ; i++ ) {
  //  Serial.print(usecTimes[i]) ; Serial.print(", ");
  //}

  return 2500u / (millis() - usecTimes[(index + 2) & TIME_AVERAGE_MASK]);
}

//////////////////////////////////////////////////////////////////////////////////////
CameraSensor::CameraSensor(int pin): controlPin(pin) {
    watching = false;
    pinMode(controlPin, OUTPUT);
    CameraSensor::calibrate();
}

void CameraSensor::watch() {
    digitalWrite(controlPin, HIGH);
    watching = true;
}
void CameraSensor::calibrate() {
  digitalWrite(controlPin, LOW);
  watching = false;
}

bool CameraSensor::isWatching() {
	return watching;
}

// This must not be called unless the camera is started
unsigned CameraSensor::getNormalisedValue() {
	if (!watching) {
		return 0; // This should never happen
	}
	int32_t temp = 0;
    char buff[10] = {0};

    Wire.requestFrom(0x12, 9);
    while(Wire.available()) {
        buff[temp++] = Wire.read();
    }
    buff[temp] = '\0';
    return atoi(buff + 5);
}

//////////////////////////////////////////////////////////////////////////////////////
void MagSensor::start() {
  while (!mag.init());
  mag.enableDefault();
  mag.writeReg(LIS3MDL::CTRL_REG2, 0x60);
}

unsigned MagSensor::getNormalisedValue() {
  mag.read();
  //double L2Answer = sqrt((double)((long)mag.m.x * mag.m.x + (long)mag.m.y * mag.m.y + (long)mag.m.z * mag.m.z)); // L2 norm of magnetic strength
  //unsigned retval = (unsigned)((L2Answer - 1000.0) / 50.0);
  //return retval;
  return (abs(mag.m.x) + abs(mag.m.y) + abs(mag.m.z)) / 100u;
}

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);  // Create RFID reader instance

//////////////////////////////////////////////////////////////////////////////////////
void RFID::start() {
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
}

///////////////////////////////////////////////////////////////////////////////////////
// This utility function just reads and returns the text from a noticed RFID tag.
///////////////////////////////////////////////////////////////////////////////////////
const byte *RFID::readCardData() {
  const int VAL_OFFSET = 6;
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return NULL;
  }

  MFRC522::StatusCode status;
  byte size = BLOCKS + 2;

  byte blockAddr = 6;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    return NULL;
  }
  blockAddr = 10;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer + 16, &size);
  if (status != MFRC522::STATUS_OK) {
    return NULL;
  }

  // Avoid seeing the same card twice!
  mfrc522.PICC_HaltA();

  // second byte indicates length
  int length = buffer[1] - 3;  // Need to subtract 3 to get actual string length
  buffer[length + VAL_OFFSET] = 0;
  return buffer + VAL_OFFSET;
}

//////////////////////////// Echo locator stuff ///////////////////////////////////////////////////////
NewPing sonar[2] = {
  NewPing(TRIGGER_PIN_LEFT, ECHO_PIN_LEFT, MAX_DISTANCE),
  NewPing(TRIGGER_PIN_RIGHT, ECHO_PIN_RIGHT, MAX_DISTANCE)
};

void EchoMonitor::start(int freq) {
  pinMode(TRIGGER_PIN_LEFT, OUTPUT);
  pinMode(ECHO_PIN_LEFT, INPUT);
  pinMode(TRIGGER_PIN_RIGHT, OUTPUT);
  pinMode(ECHO_PIN_RIGHT, INPUT);
  pingSpeed = freq;
  pingTimer = millis();
  echo_distances[0] = echo_distances[1] = 1000;
}
bool EchoMonitor::update() {
  long duration, distance;
  if (millis() >= pingTimer) {   // pingSpeed milliseconds since last ping, do another ping.
    pingTimer += pingSpeed;      // Set the next ping time.
    Wire.end(); // Avoids conflict with the timings
    distance = sonar[which_echo].ping_cm(50);
    Wire.begin();
    if (distance > 0) {
      echo_distances[which_echo] = distance; 
    } else {
      echo_distances[which_echo] = 1000; 
    }
    which_echo = 1 - which_echo;
  }
}
unsigned int EchoMonitor::pingSpeed; // Frequency at which we ping
unsigned long EchoMonitor::pingTimer; // Holds the next ping time.
int EchoMonitor::which_echo = 0;  // which sonar to ping next

