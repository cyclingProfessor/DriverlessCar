#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>

HardwareSerial &Serial = Serial1;

/////////////////// RFID Section ///////////
constexpr uint8_t NFC_RST_PIN = 9; 
constexpr uint8_t NFC_SS_PIN = 10; 

unsigned char currentID[7];
unsigned long lastTime = millis();

// We are no longer using interrupts (they do not work) for NFC, since obtaining the id is very fast.
// Also we do not bother to read the NFC data any more.
MFRC522 mfrc522(NFC_SS_PIN, NFC_RST_PIN);   // Create MFRC522 instance.

// File format for an 'image' is
// P6:W:H:uuuuuuu:[COMMAND - 20 bytes]
// Where the colons and [] are not in the file (just indicative separators
// Width and Height are unsigned chars.
// uuuuuuu is the PICC uid as a sequence of unsigned chars - which will match the file name
// [C..] is a text based command.
 
const int chipSelect = 31;

const int MAX_SIZE = 1000; // Prevent large images
struct image_t {
  unsigned len;
  unsigned char width;
  unsigned char height;
  unsigned char *data;
  byte id[7];
} *images[10];
unsigned imageCount = 0;

bool readMagic(File fp) {
  return (fp.read() == 'P'&& fp.read() == '6');
}

image_t *readFile(File fp) {
  // Check first characters
  if (!readMagic(fp)) return false;

  image_t *retval = new image_t();
  retval->width = fp.read();
  retval->height = fp.read();
  fp.read(retval->id, 7);
  retval->len = fp.size() - 31;
  fp.seek(31);
  retval->data = new unsigned char[retval->len];
  if (fp.read(retval->data, retval->len) != retval->len) {
    delete retval->data;
    delete retval;
    return NULL;
  }
  return retval;
}

void printUID(byte *uid) {
  char output[15];
  output[14] = '\0';
  for (int index = 0 ; index < 7; index++) {
    sprintf(output + 2 * index, "%02x", uid[index]);
  }
  Serial.println(output);
}

void setup()
{
  // Open Serial1 communications and wait for port to open:
  Serial1.begin(115200);
  Serial2.begin(460800);
  while (!Serial1) {
    ; // wait for Serial1 port to connect. Needed for Leonardo only
  }
  SPI.begin();
  mfrc522.PCD_Init(); // Init MFRC522 card
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

  Serial1.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial1.println("Card failed, or not present");
    return;
  }
  Serial1.println("card initialized.");
  delay(2000);

  File dir = SD.open("NFCTags");
  File entry = dir.openNextFile();
  while (entry) {
    Serial1.println(entry.name());
    images[imageCount] = readFile(entry);
    if (images[imageCount] != NULL && images[imageCount]->width == 50 && images[imageCount]->height == 44) {
      printUID(images[imageCount]->id);
      imageCount ++;
    }
    entry = dir.openNextFile();
  }
}

void sendFile(int index)
{
  image_t *im = images[index];
  int size = im->len;
  Serial.print("Sending image for:");
  printUID(im->id);
  sendNext('{');
  sendNext('P');
  sendNext((im->len >> 6) & 0x3F); // Max 12 bits length sent.
  sendNext(im->len & 0x3F);

  bool keepGoing = true;
  int count = 0;
  while (count < size && keepGoing) {
    keepGoing &= sendNext(im->data[count]);
    count++;
  }
  sendNext('}');
  if (!keepGoing) {
    Serial1.println("error sending IMAGE file");
  }
}


bool sendNext(unsigned char next) {
  Serial2.write(next);
  while (!Serial2.available()) {}
  return Serial2.read() == next;
}

/**
   MFRC522 - Need to change to return the uid string.
*/
inline unsigned char *readCard() {
  if (!mfrc522.PICC_IsNewCardPresent()) return NULL;
  if (!mfrc522.PICC_ReadCardSerial()) return NULL;
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  printUID(mfrc522.uid.uidByte);

  if (mfrc522.uid.size != 7) return NULL;
  return mfrc522.uid.uidByte; 
}

void loop() {
  // loop waiting to see a new NFC Tag

  unsigned char *nextID = readCard();
  if (nextID != NULL && memcmp(currentID, nextID, 7)) {
    memcpy(currentID, nextID, 7);
    for (int index = 0 ; index < imageCount ; index++) {
      if (!memcmp(nextID, images[index]->id, 7)) {
        Serial1.print("Sending image");
        Serial1.println(index);
        sendFile(index);
      }
    }
  }
  delay(10);
  if (millis() - lastTime > 50) {
    Serial1.print("Long Delay: ");
    Serial1.println(millis() - lastTime);
  }
  lastTime = millis();
}
