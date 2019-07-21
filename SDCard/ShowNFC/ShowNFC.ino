#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>

/////////////////// RFID Section ///////////
constexpr uint8_t RST_PIN = 9; 
constexpr uint8_t SS_PIN = 10; 

unsigned long currentID = 0L;

// We are no longer using interrupts (they do not work) for NFC, since obtaining the id is very fast.
// Also we do not bother to read the NFC data any more.
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

// File format for an 'image' is
// P6:W:H:uuuuuuu:[COMMAND - 20 bytes]
// Where the colons and [] are not in the file (just indicative separators
// Width and Height are unsigned chars.
// uuuuuuu is the PICC uid as a sequence of unsigned chars - which will match the file name
// [C..] is a text based command.
 
const int chipSelect = 31;

struct image_t {
  unsigned length;
  unsigned char width;
  unsigned char height;
  unsigned char *data;
  unsigned char id[7];
}
const int MAX_SIZE = 1000; // Prevent large images
image_t *images[10];

unsigned bool readMagic(File fp) {
  return (fp.read() == 'P'&& fp.read() == '6');
}

image_t *readFile(File fp) {
  // Check first characters
  if (!readMagic(fp)) return false;

  image_t *retval = new image_t();
  retval->width = fp.read();
  retval->height = fp.read();
  retval->length = fp.size() - 4;
  retval->data = new unsigned char[retval->length];
  if (fread(fp, retval->data, 1, retval->length) != retval->length) {
    delete retval->data;
    delete retval;
    return null;
  }
  return retval;
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
  lastTime = millis();

  Serial1.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial1.println("Card failed, or not present");
    return;
  }
  Serial1.println("card initialized.");
  delay(2000);

  dir = open("NFCTags");
  entry = dir.openNextFile();
  while (entry) {
    if (entry.name().len() == 7) {
      images[imageCount] = readFile(entry);
      if (images[imageCount != null) {
        memcpy(images[imageCount]->id, entry.name())
      }
      imageCount ++;
    }
    entry = dir.openNextFile();
  }
}

void sendFile(int index)
{  
  bool keepGoing = true;
  int count = 0;
  image_t *im = images[index];
  int size = im->length;
  sendNext('{');
  sendNext('P');
  sendNext(im->width);
  sendNext(im->height);
  
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
  if (!mfrc522.PICC_IsNewCardPresent()) return null;
  if (!mfrc522.PICC_ReadCardSerial()) return null;
  if (mfrc.uid.size != 7) return null;
  return mfrc.uid.uidByte; 
}

void loop() {
  // loop waiting to see a new NFC Tag

  unsigned char *nextID = readCard();
  if (nextID != null && nextID != currentID) {
    currentID = nextID;
    for (int index = 0 ; index < imageCount ; index++) {
      if (!memcmp(next_ID, images[index]->id, 7)) {
        Serial.println(nextID);
        sendFile(index);
      }
    }
  }
  delay(10);
  if (millis() - lastTime > 50) {
    Serial.print("Long Delay: ");
    Serial.println(millis() - lastTime);
  }
  lastTime = millis();
}
