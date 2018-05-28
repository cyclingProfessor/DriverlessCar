#include "lineFollow.h"

///////////////////////////////////////////////
/// BT constants
#define REPORT '?'
#define START 'F'
#define STOP 'S'
#define PARAMS '('

////////////////////////////////////////////////////////////////////////////
void processBT(Status &status, unsigned paramCount, char **paramNames, unsigned **paramValues) {
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
      report(status, paramCount, paramNames, paramValues);
      break;
    case PARAMS:
      // read first character
      inByte = Serial.read();
      char name[3];
      name[2] = '\0';
      unsigned temp;
      while (inByte != ')') {
        // read next two characters
        name[0] = inByte;
        name[1] = Serial.read();
        int index;
        for (index = 0 ; index < paramCount ; index++) {
          if (!strncmp(name, paramNames[index], 2))
            break;
        }
        // Ignore unknown parameters.
        unsigned *currVar = (index < paramCount) ? paramValues[index] : &temp;
        *currVar = 0;
        inByte = Serial.read();
        while (inByte >= '0' && inByte <= '9') {
            *currVar = *currVar * 10 + inByte - '0';
            inByte = Serial.read();
        }
      }
    break;
  }
}


