# Untitled - By: dave - Sat Oct 28 2017

import sensor, image, pyb
from pyb import LED

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.B128X64)
sensor.skip_frames()
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking
sensor.set_windowing((0,35, 128, 10))
thresholds = [(0, 0, -0, -0, -0, -3)] # Either blue or green tape.

# Go for 100 frames with no fails!
counter = 0
a_max = 0
b_max = 0
led_counter = 0
while(counter < 100):
    led_counter = led_counter + 1;
    if led_counter > 10:
        LED(2).on()
        if led_counter > 20:
            led_counter = 0
    else:
        LED(2).off()
        img = sensor.snapshot()
        lines = img.find_line_segments(merge_distance=4)
        if len(lines) > 1:
            left = min(lines[0].x1(), lines[1].x1())
            right = max(lines[0].x1(), lines[1].x1())
            if (right - left) < 30 and right - left > 15:
                #img.draw_rectangle((left, 0, right - left, 10), 0, 4, True)
                stats = img.get_statistics(roi = (left, 0, right - left, 10))
                a_max = int((stats.a_uq() + a_max) / 2)
                b_max = int((stats.b_uq() + b_max) / 2)
                thresholds = [(0,100, -128, a_max + 5, -128, b_max + 5)]

                counter = counter + 1
                blobs = img.find_blobs(thresholds)
                for blob in blobs:
                    img.draw_rectangle(blob.rect())
                    img.draw_cross(blob.cx(), blob.cy())
            else:
                counter = 0
        else:
            counter = 0

LED(2).on()
print ("final thresholds")
print(thresholds)
while True:
    pass
