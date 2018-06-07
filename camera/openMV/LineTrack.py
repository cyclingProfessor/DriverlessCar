# Send line data to the Arduino for line tracking.
import sensor, image, time
import pyb, ustruct
from pyb import Pin, Timer, LED

sensor.reset()

sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.B128X64)
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking

WIDTH = sensor.width()

# There are 3 possible errors for i2c.send: timeout(116), general purpose(5) or busy(16) for err.arg[0]
bus = pyb.I2C(2, pyb.I2C.SLAVE, addr=0x12)
bus.deinit() # Fully reset I2C device...
bus = pyb.I2C(2, pyb.I2C.SLAVE, addr=0x12)

thresholds = [(0, 100, -128, 16, -128, -22)]
#thresholds = [(0, 100, -128, -11, -128, -7)]
# (0, 100, -128, 44, -128, -31) light off kitchen
# (0, 100, -128, 127, -128, -13) light on kitchen

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

current_position = -1 # I am LOST
def findLine():
    global current_position
    img = sensor.snapshot()
    show(current_position) # turns on leds

    current_position = -1
    for blob in img.find_blobs(thresholds, x_stride=3, y_stride=3, pixels_threshold=10, area_threshold=8, roi=(0,35, 128, 10),merge=True, margin=5):
        img.draw_rectangle(blob.rect())
        img.draw_cross(blob.cx(), blob.cy())
        current_position = blob.cx() * 100 / WIDTH
    show(current_position) # turns on leds

    print(current_position)

#############################################################################################
# MAIN LOOP
###############################################################################################
while(True):
    findLine()
