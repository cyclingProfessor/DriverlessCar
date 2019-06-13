Complementary Infrastructure.
=============================
We live in a new age of automation, where the bins talk to the street lights and the buses converse with the traffic signals.
Sensors, tiny computers, powerful motors and effective batteries have all combined to make the impossible possible.  We expect that the autonomous car will take over from the driver and that it will not drive in the same way.  Why use vision alone when all other traffic is intelligent, and the roads, and the whole place, is intelligent?
This model has five computers: an Arduino, an RFID reader, a complex 3 axis multiple sensor and a mobile phone.  It has four power supplies.  It has 14 sensors.  It can run for three hours on one set of AA batteries.  It follows a magnetic stripe, watches for pedestrians, and responds to RFID tags.  It carefully monitors its own speed and position.  Using all these myriad signals it efficiently gets from place to place.
It is a miracle that demonstrates the integrated and complementary infrastructure.

====================================================

Now moved some common code into the CarLib library - so after cloning the git archive you will need to copy this into your arduino IDE library folder.

===================

Build System.
1. Must increase the Rx gain on the ANtenna to make RFID cards read fast enough.  This means adding one line to MFSC522 before PICC_..
2. Must change cpp flags to enable C++ 2011 standard to use SPI I2C library for MMU.
3. Must change SPI to SPI1 in the SDCard library so that it looks for the SDCard on the secondary SPI bus.