# Send line data to the Arduino for line tracking.
#
# Mode is controlled by the (digital) line P6.
#    HIGH - disable interupts and find and send directions
#    LOW - enable interupts and calibrate
#
#####################################################################################################
# Calibrate: Flashes at CALIBRATE_FLASH milliseconds rate (BLUE) led until calibrated then ((YELLOW)) led permanent
#
#
#####################################################################################################
# Direction: It uses a [LED] heartbeat to show that it is working - RED, WHITE, GREEN - but only image
# grabs when the heartbeat light is off.  Loops each REPORT_TIME milliseconds.
# MAGENTA (red + blue) means "I am lost".
#
# Each loop:
#     Either: Switch on light (showing direction)
#     Or: Switch off light and send centre of blob of correct colour (0-15).

import sensor, image, time, lcd
from pyb import LED, delay

CALIBRATE_FLASH = 40 # flash speed while calibrating
REPORT_TIME = 0 # time between sending direction updates
LAMBDA = 0.0 # rolling average constant: nval = lambda * oval + (1-lambda) * nval

sensor.reset()
lcd.init() # Initialize the lcd screen.

sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QQVGA2) # Special 128x160 framesize for LCD Shield.
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking
current_position = -1 # I am LOST

WIDTH = sensor.width()
clock = time.clock()                # Create a clock object to track the F

def leds(on_arr):
    for led in range(1,4):
        if (on_arr[led - 1]):
            LED(led).on()
        else:
            LED(led).off()

def show(posn): # passed normalised value from 0 to 100 (or less than 0 for NONE)
    if posn < 0: # Magenta for "I am Lost"
        leds((True, False, True))
    elif (posn <= 6): # RED
        leds((True, False, False))
    elif (posn < 8): # WHITE
        leds((True, True, True))
    else: # GREEN
        leds((False, True, False))

def calibrate(threshold):
    retval = False
    img = sensor.snapshot()
    img.draw_rectangle(0,80, 128, 30)
    lines = [l for l in img.find_line_segments(roi=(4,85, 120, 20), merge_distance=4) if l.theta() > 150 or l.theta() < 30]
    for line in lines:
        img.draw_line(line.line())
    if len(lines) == 2:
        left = min(lines[0].x1(), lines[1].x1())
        right = max(lines[0].x1(), lines[1].x1())
        if (right - left) < 30 and right - left > 15:
            stats = img.get_statistics(roi = (left + 5, 85, right - left - 5, 105))
            threshold[3] = int((stats.a_uq() + threshold[3]) / 2)
            threshold[5] = int((stats.b_uq() + threshold[5]) / 2)

            blobs = img.find_blobs([threshold], x_stride=3, y_stride=3, pixels_threshold=10, area_threshold=8, roi=(0,85, 128, 20),merge=True, margin=5)
            if len(blobs) == 1:
                retval = True
                img.draw_rectangle(blobs[0].rect(), fill=True, color=(255,0,0))
                pass
    lcd.display(img)
    return retval

def findLine(threshold):
    global current_position
    clock.tick()
    img = sensor.snapshot()
    show(current_position) # turns on leds

    save_position = current_position
    current_position = -1
    for blob in img.find_blobs([threshold], x_stride=3, y_stride=3, pixels_threshold=10, area_threshold=8, roi=(0,85, 128, 20),merge=True, margin=5):
        img.draw_rectangle(blob.rect())
        img.draw_cross(blob.cx(), blob.cy())
        current_position = int(save_position * LAMBDA + blob.cx() * 15 / WIDTH * (1 - LAMBDA))
    show(current_position) # turns on leds
    lcd.display(img)
    print(current_position)
    print(clock.fps())              # Note: OpenMV Cam runs about half as fast when connected

#############################################################################################
# MAIN LOOP
###############################################################################################



# Here we calibrate once, then we do direction processing.
show_led = False
counter = 0
# Color Tracking Thresholds (L Min, L Max, A Min, A Max, B Min, B Max)
threshold = [0,100, -128, 0, -128, 0]
while counter <= 20:
    # flash gold or blue depending on whether we are calibrated
    if counter > 20: # we are calibrated
        leds((show_led, show_led, False))
        delay(5 * CALIBRATE_FLASH)
    else:
        leds((False, False, show_led))
        if (not show_led):
            if calibrate(threshold):
                counter = counter + 1
            else:
                counter = 0
    show_led = not show_led
    delay(CALIBRATE_FLASH)

# be nice to the line finder
threshold[3] += 10
threshold[5] += 10
while(True):
    findLine(threshold)
