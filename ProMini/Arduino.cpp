////////////////// Communication with Arduino //////////////////////
#include "ProMini.h"

void clockPinIsr() {
  arduino.readData();
}

Arduino::start() {
  pinMode(CLOCK_PIN, INPUT);
  pinMode(DATA_0,INPUT);
  pinMode(DATA_1,INPUT);
  pinMode(DATA_2,INPUT);
  pinMode(DATA_3,INPUT);
  pinMode(MOVING_PIN, OUTPUT);
  buildData();
  for (int index = 0 ; index < 6 ; index++) {
    registerCode[index] = (7 * index + 3) & 0xF;
  }
  attachInterrupt (digitalPinToInterrupt (CLOCK_PIN), clockPinIsr, RISING);
}


void Arduino::buildData() {
  int offset = 0;
  for (int index = 0 ; index < NUM_CODES ; index++) {
    dataOffset[messages[index].code] = offset;
    offset += messages[index].length + 1; //One for freshness indicator
    dataLength[messages[index].code] = messages[index].length; // Fast lookup of 
  }
  // Now allocate actual data buffers
  liveData = malloc(offset);
  data = malloc(offset);
  for (int index = 0 ; index < offset ; index++) {
    liveData[index] = data[index] = 0;
  }
}

////////////////////////////////////
byte Arduino::readPins() {
  byte value = 0;
  if (analogRead(DATA_3) > 500) {
    value |= 8;
  }
  if (analogRead(DATA_2) > 500) {
    value |= 4;
  }
  if (digitalRead(DATA_1) == HIGH) {
    value |= 2;
  }
  if (digitalRead(DATA_0) == HIGH) {
    value |= 1;
  }
  return value;
}
// volatile int eGotData = -1234;
// volatile int eStarted = -5;
// volatile byte eCode = 158;
volatile byte neValue;
volatile unsigned eCounter = 0;
byte eValue = 123;

///////////////////////
void Arduino::readData() { // ISR on rising edge of CLOCK_PIN
  byte value = readPins();
  neValue = value;
  eCounter++;
  if (!started) {
    data_count = (value == registerCode[data_count]) ? data_count + 1 : 0;
//    eStarted = data_count;
    if (data_count == 6) {
      started = true;
      read_code = true;
    }
  }
  else if (read_code) {
//    eCode = value;
    data_start = dataOffset[value];
    data_ptr = data + data_start;
    data_length = data_count = dataLength[value];
    data[data_start + data_length] = 0; // clear freshness
    read_code = false;
  } else {
    *data_ptr++ = value;
    if (--data_count == 0) {  // copy across whole data
      *data_ptr = 1; // set freshness
      memcpy(liveData + data_start, data + data_start, data_length + 1);
      read_code = true;
//      eGotData = data_start;
    }
  }
}

bool Arduino::getData(byte code, byte *buffer) {
//  Serial.print("Got data!: "); Serial.println(eGotData);
//  Serial.print("Code: "); Serial.println(eCode);
  if (neValue != eValue) {
    eValue = neValue;
    Serial.print("Value: "); Serial.println(eValue);
    Serial.print("Counter: "); Serial.println(eCounter);
  }
  if (!started) {
    Serial.println("Not yet Started");
  }
  unsigned offset = dataOffset[code];
  unsigned length = dataLength[code];
  if (!liveData[offset + length]) {
    return false;
  }
  // Tiny critical section.  We use liveData copy
  // to avoid the race condition where we only ask for data when it is partial.
  noInterrupts();
  liveData[offset + length] = 0; // reset freshness indicator
  memcpy(buffer, liveData + data_start, data_length);
  interrupts();
}