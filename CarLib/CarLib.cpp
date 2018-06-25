#include "CarLib.h"
/////////////////////////////////////////////////////////////////////
// Given a four bit number builds the required speed value
// Negative speeds indicate reverse.
// MID_POINT means "stop"
int nibbleToSpeed(int speed) {
    return ((speed - MID_POINT) * SPEED_SCALE) / (MID_POINT + 1);
}

/////////////////////////////////////////////////////////////////////
// Given a four bit number builds the required turn angle for the turn servo
// NEUTRAL_ANGLE means "straight".
int nibbleToTurn(int where) {
  return NEUTRAL_ANGLE + (((where - MID_POINT) * (int)TURN_SCALE) / (MID_POINT + 1));
}

////////////////////////////////////////////////////////////////////
//calculates inverse of nibbleToSpeed
// since this is what is done on the Pro Mini
byte toSpeedNibble(int s) {
  return (s * (MID_POINT + 1) / SPEED_SCALE) + MID_POINT;
}

////////////////////////////////////////////////////////////////////
//calculates inverse of nibbleToTurn
// since this is what is done on the Pro Mini
byte toTurnNibble(int t) {
  return (t - NEUTRAL_ANGLE) * (MID_POINT + 1) / TURN_SCALE + MID_POINT;
}

////////////////////////////////////////////////////
// Registration codes to begin comms with ProMini
byte reg_code(int index) {
  return (7 * index + 3) & 0xF;
}
