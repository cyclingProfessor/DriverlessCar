#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t RST_PIN = 9;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;     // Configurable, see typical pin layout above

HardwareSerial &Serial = Serial1;
unsigned long currentID = 0L;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

/**
   Initialize.
*/
void setup() {
  Serial1.begin(115200); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
}

/**
   Main loop.
*/
void loop() {
  Serial1.println("Present tag now to RFID Reader - I am starting to wait.");
  delay(2000);

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

  unsigned long nextID = getID();
  if (nextID == currentID) {
    return ;
  }

  currentID = nextID;
   
  // Show some details of the PICC (that is: the tag/card)
  //Serial.print(F("Card UID:"));
  //dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  char *fred =readCardData();
  Serial.println("R" + String(fred) + ":END:");
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
  Serial.println("Length = : " + String(length));
  buffer[length + VAL_OFFSET] = 0;
  return buffer + VAL_OFFSET;
}

unsigned long getID(){
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  return hex_num;
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void dump_byte_array(byte *buffer, byte bufferSize) {
  char string[10];
  for (byte i = 0; i < bufferSize; i++) {
    sprintf(string, "%02x (%c), ", buffer[i], isprint(buffer[i]) ? buffer[i] : '?');
    Serial.print(string);
  }
}
