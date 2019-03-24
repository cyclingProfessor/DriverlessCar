#include <timerobj.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         9
#define SS_PIN          10
#define IRQ_PIN         8

HardwareSerial &Serial = Serial1;

MFRC522 mfrc522(SS_PIN, RST_PIN);
#define BLOCKS 16
byte buffer[2 * BLOCKS + 2];
#define VAL_OFFSET 6

volatile boolean bNewInt = false;
volatile boolean foundCard = false;
unsigned char regVal;

void readCard();
//Timer triggerTimer(50, readCard, t_period, NULL );

void setup() {
  Serial1.begin(115200); // Initialize serial communications with the PC
  while (!Serial1);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus

  mfrc522.PCD_Init();

  /* setup the IRQ pin*/
  pinMode(IRQ_PIN, INPUT_PULLUP);

  /*
      Allow the ... irq to be propagated to the IRQ pin
      For test purposes propagate the IdleIrq and loAlert
  */
  mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, 0x20);
  mfrc522.PCD_WriteRegister(mfrc522.DivIEnReg, 0x90);

  /*Activate the interrupt*/
  attachInterrupt(IRQ_PIN, readCard, RISING);
  activateRec(mfrc522);
  delay(100);
  Serial1.println("End setup");
}

/**
   Main loop.
*/
void loop() {

  //readCard();
  if (bNewInt) { //new read interrupt
    bNewInt = false;
    Serial.print("Interrupt.");
    if (foundCard) {
      dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
      Serial.print("<");
      Serial.print((char *)buffer + VAL_OFFSET);
      Serial.print(">");
    }
    Serial.println();
    foundCard = false;
  }

  Serial.println("Beginning to Wait for a Card.");
  delay(4000);
} //loop()

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial1.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial1.print(buffer[i], HEX);
  }
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
