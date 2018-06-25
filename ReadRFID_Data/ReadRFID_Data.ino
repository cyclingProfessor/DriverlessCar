#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t RST_PIN = 4;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 5;     // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

/**
   Initialize.
*/
void setup() {
  Serial.begin(115200); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
}

/**
   Main loop.
*/
void loop() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

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

