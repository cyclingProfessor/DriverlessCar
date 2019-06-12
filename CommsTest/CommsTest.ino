#include <timerobj.h>
#include <SPI.h>
#include <MFRC522.h>

HardwareSerial &Serial = Serial1;

/////////////////// RFID Section ///////////
constexpr uint8_t RST_PIN = 9; 
constexpr uint8_t SS_PIN = 10; 
constexpr uint8_t IRQ_PIN = 15; // seems to clash with echo pin interrupt on when set to 8.

unsigned long currentID = 0L;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
constexpr uint8_t BLOCKS = 16;
byte buffer[2 * BLOCKS + 2];
constexpr uint8_t VAL_OFFSET = 6;

volatile boolean bNewInt = false;
volatile boolean foundCard = false;

///////////////// Sonar Section //////////////////////////////
#define RIGHT 0
#define LEFT 1
#define REAR 2
#define LONG_DISTANCE 1000

int echoPin[3] = {7,5,3};
int triggerPin[3] = {6,4,2};

typedef void (*FP)();
void rightEcho(), leftEcho(), rearEcho();
FP echoFn[3] = {rightEcho, leftEcho, rearEcho};

volatile unsigned duration[3];
volatile int cRight = 0, cLeft = 0, cRear = 0;

void pinger(void *data) {
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  for (int index = 0 ; index < 3 ; index++) {
    digitalWrite(triggerPin[index], HIGH);
  }
  delayMicroseconds(30);
  for (int index = 0 ; index < 3 ; index++) {
    digitalWrite(triggerPin[index], LOW);
  }
}

Timer triggerTimer(100, pinger, t_period, NULL );

//////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial1.begin(115200);
  Serial5.begin(115200);
  Serial2.begin(115200);
  for (int index = 0 ; index < 3 ; index++) {
    pinMode(triggerPin[index], OUTPUT);
    pinMode(echoPin[index], INPUT);
    digitalWrite(triggerPin[index], LOW);
    attachInterrupt(echoPin[index], echoFn[index], CHANGE);  // Attach interrupt to the sensor echo input
  }
  SPI.begin();
  mfrc522.PCD_Init(); // Init MFRC522 card
    /* setup the IRQ pin*/
  pinMode(IRQ_PIN, INPUT_PULLUP);

  mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, 0x20);
  mfrc522.PCD_WriteRegister(mfrc522.DivIEnReg, 0x90);

  /*Activate the interrupt*/
  attachInterrupt(IRQ_PIN, readCard, RISING);
  activateRec(mfrc522);
  delay(100);
}
boolean warnedFront = false, warnedRear = false;

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

  if (duration[REAR] < 1800) {
    if (!warnedRear) {
      Serial.println("Danger Behind");
      Serial5.write('B');
      warnedRear = true;
    }
  } else {
    warnedRear = false;
  }
  if (duration[RIGHT] < 1800 || duration[LEFT] < 1800) // Damn - obstacle.
  {
    if (!warnedFront) {
      Serial.println("Danger in Front");
      Serial5.write('F');
      warnedFront = true;
    }
  } else {
    warnedFront = false;
  }

  if (bNewInt) { //new read interrupt
    bNewInt = false;
    if (foundCard) {
      unsigned long nextID = getID();
      Serial.print("Emergency Stop!");
      Serial5.write('0');
      if (nextID != currentID) {
        currentID = nextID;
        Serial.print(" Card says: ");
        Serial.print("<");
        Serial.print((char *)buffer + VAL_OFFSET);
        Serial.print(">");
      }
      Serial.println();
    }
    foundCard = false;
  }
  delay(100);
}


/**
   MFRC522 interrupt serving routine
*/
void readCard() {
  bNewInt = true;
  clearInt(mfrc522);
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;
  mfrc522.PICC_ReadCardSerial(); //read the tag data

  if (!readCardData())
    return;
  activateRec(mfrc522);
  foundCard = true;
}

/*
   The function sending to the MFRC522 the needed commands to activate the reception
*/
inline void activateRec(MFRC522 mfrc522) {
  mfrc522.PCD_WriteRegister(mfrc522.FIFODataReg, mfrc522.PICC_CMD_REQA);
  mfrc522.PCD_WriteRegister(mfrc522.CommandReg, mfrc522.PCD_Transceive);
  mfrc522.PCD_WriteRegister(mfrc522.BitFramingReg, 0x87);
}

/*
   The function to clear the pending interrupt bits after interrupt serving routine
*/
inline void clearInt(MFRC522 mfrc522) {
  mfrc522.PCD_WriteRegister(mfrc522.ComIrqReg, 0x7F);
}


boolean readCardData() {
  MFRC522::StatusCode status;
  byte size = BLOCKS + 2;

  byte blockAddr = 6;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  blockAddr = 10;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer + 16, &size);
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  
  // second byte indicates length
  int length = buffer[1] - 3;  // Need to subtract 3 to get actual string length
  buffer[length + VAL_OFFSET] = 0;
  return true;
}

unsigned long getID(){
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  return hex_num;
}

void rightEcho() {
  calcDistance(RIGHT);
  cRight ++;
}
void leftEcho() {
  calcDistance(LEFT);
  cLeft++;
}
void rearEcho() {
  calcDistance(REAR);
  cRear ++;
}
static inline void calcDistance(int which) {
  // Echo is within 25mS if there is an object in view
  // Otherwise at least 38mS
  static int startTime[3];
  if (digitalRead(echoPin[which]) == HIGH) { // Start of a pulse!
    startTime[which] = micros();
  } else {
    duration[which] = micros() - startTime[which];
    //distCm[which] = (duration[which] < 25000) ? (duration[which])/ 58.2 : LONG_DISTANCE;
  }
}
