#include <Servo.h>  // servo library
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LIS3MDL.h>

#define RFID_RST_PIN         4
#define RFID_SS_PIN          5
#define dir1Pin              7 // Motor direction pin
#define dir2Pin              8 // Motor direction pin
#define speedPin             6 // Needs to be a PWM pin to be able to control motor speed
#define steerPin            10
#define speedSensorInt       2 // Interupt raised by slotted speed sensor - tacho + speed.

int currentDirection = 1; // go forwards
int currentSpeed = 255;
int currentTurn = 55; // straight ahead

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);  // Create MFRC522 instance
Servo   servo;                               // servo control object
LIS3MDL mag;

const int MAX_SERVO = 1798;
const int MIN_SERVO = 771;
const int MIN_ANGLE = 10; //neutral is 55 degrees
const int NEUTRAL_ANGLE = 65;
const int MAX_ANGLE = 105;
const int MIN_SPEED = 100;
const int MAX_SPEED = 255;

volatile int tacho = 0;

void addToTacho() {
  tacho += currentDirection;
}

void setup()
{
  Serial.begin(115200);
  servo.attach(steerPin, MIN_SERVO, MAX_SERVO);
  servo.write(NEUTRAL_ANGLE);

  attachInterrupt(digitalPinToInterrupt(speedSensorInt), addToTacho, RISING);
  pinMode(dir1Pin, OUTPUT);
  pinMode(dir2Pin, OUTPUT);
  pinMode(speedPin, OUTPUT);

  // Set Motor off
  digitalWrite(dir1Pin, HIGH);
  digitalWrite(dir2Pin, HIGH);

  // Set up RFID reader
  SPI.begin();			// Init SPI bus
  mfrc522.PCD_Init();		// Init MFRC522
  //  mfrc522.PCD_WriteRegister(mfrc522.RFCfgReg, (0x07 << 4)); // set gain to maximum

  while (!Serial);		// Do nothing if no serial port is opened
  mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  Wire.begin();

  while (!mag.init()) {
    Serial.println("Waiting for mag sensor");
  }

  mag.enableDefault();
  mag.writeReg(LIS3MDL::CTRL_REG2, 0x60);
}

#define REPORT '?'
#define FORWARD 'F'
#define STOP 'S'
#define PARAMS '('
void loop()
{
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    switch (inByte) {
      case REPORT:
        // Look for new cards
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
          Serial.print("R" + String(readCardData()) + ":END:");
        }
        mag.read();
        Serial.print("M" + String(abs(mag.m.x) + abs(mag.m.y)) + ":END:");
        Serial.print("T" + String(tacho) + ":END:");
        break;
      case FORWARD:
        if (currentDirection == 1) { //Go forwards
          digitalWrite(dir1Pin, LOW);
          digitalWrite(dir2Pin, HIGH);
        } else {
          digitalWrite(dir1Pin, HIGH);
          digitalWrite(dir2Pin, LOW);
        }
        analogWrite(speedPin, MAX_SPEED);
        delay(200);
        analogWrite(speedPin, currentSpeed);
        break;
      case STOP:
        analogWrite(speedPin, 0);
        break;
      case PARAMS:
        // We need to parse "(speed:direction)"
        currentDirection = 1; // default is forwards
        currentSpeed = currentTurn = 0;

        boolean finished = false;
        int *value = &currentSpeed;
        while (!finished) {
          switch (inByte = Serial.read()) {
            case ')':
              finished = true;
              bound(&currentTurn, MIN_ANGLE, MAX_ANGLE);
              bound(&currentSpeed, MIN_SPEED, MAX_SPEED);
              servo.write(currentTurn);
              break;
            case ':':
              value = &currentTurn;
              break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              *value = *value * 10 + inByte - '0';
              break;
            case '-':
              currentDirection = -1;
              break;
          }
        }
    }
  }
}

#define BLOCKS 16
byte buffer[2 * BLOCKS + 2];

char *readCardData() {
  MFRC522::StatusCode status;
  byte size = BLOCKS + 2;
  #define VAL_OFFSET 6
  const char *fail = "FAILED";

  byte blockAddr = 6;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    return fail;
  }
  blockAddr = 10;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer + 16, &size);
  if (status != MFRC522::STATUS_OK) {
    return fail;
  }
  
  // second byte indicates length
  int length = buffer[1] - 3;  // Need to subtract 3 to get actual string length
  buffer[length + VAL_OFFSET] = 0;
  return buffer + VAL_OFFSET;
}


void bound(int *what, int low, int high) {
  if (*what < low) {
    *what = low;
  }
  if (*what > high) {
    *what = high;
  }
}

