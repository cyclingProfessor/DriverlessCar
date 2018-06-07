#include "lineFollow.h"

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
  byte value = 7;
  Wire.requestFrom(0x12, 1);
  if (Wire.available()) {
    value = Wire.read();
  }
  return (value <= 15) ? value : 7 ;
}

//////////////////////////////////////////////////////////////////////////////////////
void MagSensor::start() {
  while (!mag.init());
  mag.enableDefault();
  mag.writeReg(LIS3MDL::CTRL_REG2, 0x60);
}

unsigned MagSensor::getNormalisedValue() {
  return (abs(mag.m.x) + abs(mag.m.y) + abs(mag.m.z)) / 100u;
}

void MagSensor::update() {
    mag.read();
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
void EchoMonitor::update() {
  long duration, distance;
  if (millis() >= pingTimer) {   // pingSpeed milliseconds since last ping, do another ping.
    pingTimer += pingSpeed;      // Set the next ping time.
    distance = sonar[which_echo].ping_cm(50);
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

