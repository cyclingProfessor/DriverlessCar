#include "LineFollow.h"

////////////////////////////////////////////////////////////////////////////
void processBT(Status &status, unsigned paramCount, char const **paramNames, int **paramValues) {
  if (Serial.available() <= 0) {
    return;
  }
  int inByte = Serial.read();
  switch (inByte) {
    case STOP:
      if (status.state != USER_STOPPED) {
        if (status.state != OBSTACLE_STOPPED) {
          status.saveState = status.state;
        }
        status.state = USER_STOPPED;
      }
      break;
    case START:
      if (status.state == USER_STOPPED) {
        status.state = status.saveState;
      }
      break;
    case REPORT:
      report(status, paramCount, paramNames, paramValues);
      break;
    case PARAMS:
      // read first character
      inByte = Serial.read();
      char name[3];
      name[2] = '\0';
      int temp;
      while (inByte != END_PARAM) {
        // read next two characters
        name[0] = inByte;
        name[1] = Serial.read();
        unsigned index;
        for (index = 0 ; index < paramCount ; index++) {
          if (!strncmp(name, paramNames[index], 2))
            break;
        }
        // Ignore unknown parameters.
        int *currVar = (index < paramCount) ? paramValues[index] : &temp;
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


