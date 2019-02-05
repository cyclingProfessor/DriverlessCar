#include "SoftwareSerial.h"

String help = "Upload Menu:\n"
                     "\t ? - This Help\n"
                     "\t S - Check Sync\n"
                     "\t V - Get Bootloader Version\n"
                     "\t E - Enter Prgramming Mode, get Signature\n"
                     "\t R - reboot Mini\n";

SoftwareSerial mySerial(8, 9); // RX, TX

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  mySerial.begin(19200);
  pinMode(7, OUTPUT);          // sets the digital pin 7 as output, for resetting Mini
  Serial.print(help);
}

void loop() { // run over and over
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'R':
        digitalWrite(7, HIGH);       // sets the digital pin 13 on
        delay(10);                  // waits for a second
        digitalWrite(7, LOW);        // sets the digital pin 13 off
        delay(10);
        break;
      case 'S':
        mySerial.write(0x30); mySerial.write(0x20);
        break;
      case 'V':
        mySerial.write(0x41); mySerial.write(0x81); mySerial.write(0x20);
        delay(10);
        mySerial.write(0x41); mySerial.write(0x82); mySerial.write(0x20);
        break;
      case 'E': // Enter programming mode
        mySerial.write(0x50); mySerial.write(0x20);
        delay(10);
        mySerial.write(0x75); mySerial.write(0x20);
        break;
      case '?':
      default:
        Serial.print(help);
        break;
    }
    while (mySerial.available()) {
      Serial.print("Next: ");
      Serial.println(mySerial.read());
    }
  }
}
