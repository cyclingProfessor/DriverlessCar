#include "lineFollow.h"

///////////////////////////////////////////////
/// BT constants
#define REPORT '?'
#define START 'F'
#define STOP 'S'
#define PARAMS '('

////////////////////////////////////////////////////////////////////////////
void processBT(Status &status) {
  if (Serial.available() <= 0) {
    return;
  }
  int inByte = Serial.read();
  switch (inByte) {
    case STOP:
      if (status.state != USER_STOPPED) {
        status.saveState = status.state;
        status.state = USER_STOPPED;
      }
      break;
    case START:
      status.state = status.saveState;
      break;
    case REPORT:
      report(status);
      break;
    case PARAMS:
      // at the moment, we receive (speedStep1:angleStep1:speedStep2:angleStep2:globalSpeed)
      // example (20:65:20:65:20)
      status.currentSpeed = 0;
      unsigned *configVars[5];

      //@TODO: rearrange these according to the protocol in the future (or make something more intelligent)
      configVars[0] = &(status.turnParams.speedStep1);
      configVars[1] = &(status.turnParams.angleStep1);
      configVars[2] = &(status.turnParams.speedStep2);
      configVars[3] = &(status.turnParams.angleStep2);
      configVars[4] = &(status.currentSpeed);

      unsigned **currVar = configVars;
      *(*currVar) = 0;
      boolean finished = false;
      while (!finished) {
        switch (inByte = Serial.read()) {
          case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
            *(*currVar) = *(*currVar) * 10 + inByte - '0';
            break;
          case ')':
            finished = true;
            break;
          case ':':
            currVar++;
            *(*currVar) = 0;
            break;
        }
      }
      int index = 0;
      while (inByte != ')') {
        if (index < 15 && inByte != ':') {
          status.userCode[index] = inByte;
          index = index + 1;
        }
        inByte = Serial.read();
      }
      status.userCode[index] = '\0';
      speedSetter.setDesiredValue(status.currentSpeed);
      break;
  }
}


