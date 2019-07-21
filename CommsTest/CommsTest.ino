#include <timerobj.h>
#include <SPI.h>
#include <MFRC522.h>

/////////////////////////////////
// Movement Table:
//  30 (far left) - 60 degrees left 170/60 := 2.8 degrees per blob
//  330 (far right) - 50 degrees right := 2.6 degrees per blob
// 200 (straight) 
// 
// Maximum 50 degres right, 60 degrees left (18 inch).
//////////////////////////////////////////////////

HardwareSerial &Serial = Serial1;

/////////////////// RFID Section ///////////
constexpr uint8_t RST_PIN = 9; 
constexpr uint8_t SS_PIN = 10; 

unsigned long currentID = 0L;

// We are no longer using interrupts (they do not work) for NFC, since obtaining the id is very fast.
// Also we do not bother to read the NFC data any more.
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

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

int led = 13;  // Pin 13 has an LED connected on most Arduino boards.

//////////////////////////////////////////////////////////////////////////////////
long lastTime = 0L;
void setup() {
  Serial1.begin(115200);
  Serial5.begin(115200);
  Serial2.begin(460800);
  for (int index = 0 ; index < 3 ; index++) {
    pinMode(triggerPin[index], OUTPUT);
    pinMode(echoPin[index], INPUT);
    digitalWrite(triggerPin[index], LOW);
    attachInterrupt(echoPin[index], echoFn[index], CHANGE);  // Attach interrupt to the sensor echo input
  }
  SPI.begin();
  mfrc522.PCD_Init(); // Init MFRC522 card
  delay(100);
  lastTime = millis();
}
boolean warnedFront = false, warnedRear = false;
String motorControl = "0+-LRCs";

void loop() {
  // At the moment just send  stuff between { and } to the Camera.
  if (Serial1.available()) {      // If anything comes in Serial (USB/pins 0,1/BLE),
    Serial1.print("Starting to read BLE at: "); Serial1.println(millis());
    char next = Serial1.read();
    if (next == '{') {
      Serial1.print("Camera Message: {");
      while (next != '}') {
        Serial2.write(next);   // read it and send it to Camera
        next = Serial1.read();
        Serial1.print(next);
      }
      Serial2.write(next);   // Send '}' to the camera
      Serial1.println();
    } else {
      Serial1.write(next);  // Echo standard stuff ...
      if (motorControl.indexOf(next) != -1) {  
        Serial5.write(next);   // ... and send it to D1
      }
    }
    Serial1.print("Finishing reading BLE at: "); Serial1.println(millis());
  }
  // {} are out of band for Camera and never used in the message body.
  // [] are out of band for WEMOS
  // Commands can be opaque here - but the reply from the Wemos or the Camera are parsed to get status.

  // Wemos Parser:
  // [Cddd..] where each command takes a fixed length piece of data.
  // C is F,L,B for danger (no data)
  // C is S for speed - two (offset by 5) numeric cahracters
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
  unsigned long nextID = readCard();
  if (nextID != 0L) {
      Serial.print("Emergency Stop!");
      Serial5.write('0');
      if (nextID != currentID) {
        currentID = nextID;
        Serial.print(" Card says: ");
        Serial.print(currentID);
      }
      Serial.println();
  }

  delay(10);
  if (millis() - lastTime > 50) {
    Serial.print("Long Delay: ");
    Serial.println(millis() - lastTime);
  }
  lastTime = millis();
}


/**
   MFRC522 interrupt serving routine
*/
inline unsigned long readCard() {
  if (!mfrc522.PICC_IsNewCardPresent()) return 0L;
  if (!mfrc522.PICC_ReadCardSerial()) return 0L;
  return getID(); 
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
