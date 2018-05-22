# Send line data to the Arduino for line tracking.
#
# Mode is controlled by the (digital) line P6.
#    HIGH - disable interupts and find and send directions
#    LOW - enable interupts and calibrate
#
#####################################################################################################
# Calibrate: Flashes at CALIBRATE_FLASH milliseconds rate (BLUE) led until calibrated then (BLUE) led permanent
#
# Nothing implemented yet!
#
#####################################################################################################
# Direction: It uses a [LED] heartbeat to show that it is working - RED, WHITE, GREEN - but only image
# grabs when the heartbeat light is off.  Loops each REPORT_TIME milliseconds.
# MAGENTA (red + blue) means "I am lost".
#
# Each loop:
#     Either: Switch on light (showing direction)
#     Or: Switch off light and send centre of blob of correct colour (0-100).

import sensor, image, time
import pyb, ustruct
from pyb import Pin, Timer, LED

CALIBRATE_FLASH = 40 # flash speed while calibrating
REPORT_TIME = 200 # time between sending direction updates
LAMBDA = 0.8 # rolling average constant: nval = lambda * oval + (1-lambda) * nval

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking

WIDTH = sensor.width()

# There are 3 possible errors for i2c.send: timeout(116), general purpose(5) or busy(16) for err.arg[0]
bus = pyb.I2C(2, pyb.I2C.SLAVE, addr=0x12)
bus.deinit() # Fully reset I2C device...
bus = pyb.I2C(2, pyb.I2C.SLAVE, addr=0x12)

activePin = Pin('P6', Pin.IN, Pin.PULL_DOWN)

# Color Tracking Thresholds (L Min, L Max, A Min, A Max, B Min, B Max)
thresholds = [(0, 100, -57, -17, -35, 3)] # generic_green band insulation tape - many light conditions

def leds(on_arr):
    for led in range(1,4):
        if (on_arr[led - 1]):
            LED(led).on()
        else:
            LED(led).off()

def show(posn): # passed normalised value from 0 to 100 (or less than 0 for NONE)
    if posn < 0: # Magenta for "I am Lost"
        leds((True, False, True))
    elif (posn <= 40): # RED
        leds((True, False, False))
    elif (posn < 60): # WHITE
        leds((True, True, True))
    else: # GREEN
        leds((False, True, False))

# We need to think about calibration - later
def calibrate(): # if not calibrated then swithces leds on for one call, off for the next.  Otherwise turns them on.
    sensor.snapshot()
    leds((False, False, True))

current_position = -1 # I am LOST
def sendData():
    global current_position
    leds((False, False, False)) # Turn off leds in order to snapshot
    img = sensor.snapshot()
    show(current_position) # turns on leds

    save_position = current_position
    current_position = -1
    for blob in img.find_blobs(thresholds, x_stride=3, y_stride=3, pixels_threshold=10, area_threshold=8, roi=(0,100, 320, 20),merge=True, margin=5):
        img.draw_rectangle(blob.rect())
        img.draw_cross(blob.cx(), blob.cy())
        current_position = int(save_position * LAMBDA + blob.cx() * 100 / WIDTH * (1 - LAMBDA))
    show(current_position) # turns on leds

    #pyb.disable_irq()
    text = "XPOS:{:04d}".format(current_position)
    data = ustruct.pack("<%ds" % len(text), text)
    print(text)
    try:
        bus.send(data, timeout=500) # Send the data second.
    except OSError as err:
        pass
    #pyb.enable_irq()


#############################################################################################
# MAIN LOOP
###############################################################################################
while(True):
    # Here we either calibrate each time around the loop, or we do direction processing.
    while activePin.value():
        sendData()
        time.sleep(REPORT_TIME)

    while not activePin.value():
        calibrate(); # flash light until calibrated.
        time.sleep(CALIBRATE_FLASH)
