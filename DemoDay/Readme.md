Demo Day Software.
==================
Camera: SettableColors.py, State.py, CompressUtil.py
These are in the Camera/openMV folder
 State does all communications management
 CompressUtil uncompresses received images
 SettableColors can read images, switch between line following and calibration, and can process calibration messages.  It also sends periodic status updates.

Bluno: DemoDay.ino
  Initialises SD card, NFC reader, distances sensors, camera and Wemos.
  Does not yet use MMU.

Wemos D1 Lite:
  Processes speed and position from quad encoders on motor.
  Listens on either WiFi or Serial to commands: turn, speed, stop etc.,
  Stop from an obstruction knows whether the object is behind or in front so that it only stops when it is travelling towards the object.
  One command gets a status report.

===================

Build System.
1. Must increase the Rx gain on the ANtenna to make RFID cards read fast enough.  This means adding one line to MFSC522 before PICC_..
2. Must change cpp flags to enable C++ 2011 standard to use SPI I2C library for MMU.
3. Must change SPI to SPI1 in the SDCard library so that it looks for the SDCard on the secondary SPI bus.