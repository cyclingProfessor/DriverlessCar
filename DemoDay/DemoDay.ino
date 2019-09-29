#include <timerobj.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>

/////////////////////////////////
// Movement Table:
//  30 (far left) - 60 degrees left 170/60 := 2.8 degrees per blob
//  330 (far right) - 50 degrees right := 2.6 degrees per blob
// 200 (straight) 
// 
// Maximum 50 degres right, 60 degrees left (18 inch).
//////////////////////////////////////////////////

HardwareSerial &Serial = Serial1;

///////////////////// Image Section ///////////////////////////////////
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


/////////////////// RFID Section ///////////
constexpr uint8_t NFC_RST_PIN = 9; 
constexpr uint8_t NFC_SS_PIN = 10; 

unsigned char currentID[7];

// We are no longer using interrupts (they do not work) for NFC, since obtaining the id is very fast.
// Also we do not bother to read the NFC data any more.
MFRC522 mfrc522(NFC_SS_PIN, NFC_RST_PIN);   // Create MFRC522 instance.

///////////////// Sonar Section //////////////////////////////
#define RIGHT 0
#define LEFT 1
#define REAR 2
char distCodes[] = {'R', 'L', 'B'};


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

static int  saveRight = 0;
static int  saveLeft = 0;
static int  saveRear = 0;
int distanceMessage(int dir, int current, int saved) {
  if (abs(current - saved) > 100) {
    Serial1.print("[E"); // E is for Echo.
    Serial1.print(distCodes[dir]);
    Serial1.print(current);
    Serial1.print(']');
  }
  if (current < 1800) {
    Serial5.write(dir != REAR ? 'F' : 'B');
  }
  return current;      
}


///////////////////// Miscellaneous //////////////////////////////////////
unsigned long lastTime = millis();

boolean warnedFront = false, warnedRear = false;
String motorControl = "0+-LRCs";
////////////////////////////////////////////////////////////////////

void setup() {
  Serial1.begin(115200);
  Serial5.begin(115200);
  Serial2.begin(460800);

  // Wait for a connection - which can happen now or later.
  while (Serial1.read() != '!') {
  }
  
  for (int index = 0 ; index < 3 ; index++) {
    pinMode(triggerPin[index], OUTPUT);
    pinMode(echoPin[index], INPUT);
    digitalWrite(triggerPin[index], LOW);
    attachInterrupt(echoPin[index], echoFn[index], CHANGE);  // Attach interrupt to the sensor echo input
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
  lastTime = millis();
  Serial1.println("Car Initialised");
}

///////////////////////////////////////////////////////////////////////////////////
void loop() {
  // At the moment just send  stuff between { and } to the Camera.
  if (Serial1.available()) {      // If anything comes in Serial (USB/pins 0,1/BLE),
    char next = Serial1.read();
    if (next == '{') {
      boolean cameraSuccess = true;
      while (next != '}') {
        cameraSuccess &= sendNext(next);
        next = Serial1.read();
      }
      cameraSuccess &= sendNext('}');   // Send '}' to the camera
      Serial1.print("Camera Message: ");
      Serial1.println(cameraSuccess ? "Succeeded" : "Failed");
    } else {
      if (motorControl.indexOf(next) != -1) { // Simply ignore the (re-)connection !
        Serial1.write(next);  // Echo standard stuff ...
        Serial5.write(next);   // ... and send it to D1
      }
    }
  }
  // {} are out of band for Camera and never used in the message body.
  // [] are out of band for WEMOS
  // Commands can be opaque here - but the reply from the Wemos or the Camera are parsed to get status.

  // Wemos Parser:
  // [Cddd..] where each command takes a fixed length piece of data.
  // C is F,L,B for danger (no data)
  // C is S for speed - two (offset by 5) numeric characters
  // C is + for greater turn and - for less turn.
  // C is ~ for turn with values 03-14 encoded as two numeric characters
  // C is Z for zero speed (stop)
  if (Serial5.available()) {
    Serial1.println("Wemos D1 Lite");
    while (Serial5.available()) {
      Serial1.write(Serial5.read());
    }
    Serial1.println();
  }
  // The camera can only send out position data.  Parse this and send it as Motor commands
  if (Serial2.available()) {
    int ch = 'P';
    while (Serial2.available() && ch != ':') {
      ch = Serial2.read();
    }
    int where = Serial2.parseInt();
      Serial5.write('~');
      if (where >= 160) {
        Serial5.write(9 + (where - 160) * 5/100);        
      } else {
        Serial5.write(9 + (where - 160) * 6/140);        
      }
  }

  saveRight = distanceMessage(RIGHT, duration[RIGHT], saveRight);
  saveLeft = distanceMessage(LEFT, duration[LEFT], saveLeft);
  saveRear = distanceMessage(REAR, duration[REAR], saveRear);


  unsigned char *nextID = readCard();
  if (nextID != NULL && memcmp(currentID, nextID, 7)) {
    Serial.print("Emergency Stop!");
    Serial5.write('0');
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
    Serial.print("Long Delay: ");
    Serial.println(millis() - lastTime);
  }
  lastTime = millis();
}

/**
   MFRC522
*/
inline unsigned char *readCard() {
  if (!mfrc522.PICC_IsNewCardPresent()) return NULL;
  if (!mfrc522.PICC_ReadCardSerial()) return NULL;
  printUID(mfrc522.uid.uidByte);

  if (mfrc522.uid.size != 7) return NULL;
  return mfrc522.uid.uidByte; 
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
