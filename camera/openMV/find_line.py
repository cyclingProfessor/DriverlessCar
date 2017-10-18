import sensor, image, time
from pyb import LED


sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking
clock = time.clock()

# Color Tracking Thresholds (L Min, L Max, A Min, A Max, B Min, B Max)
thresholds = [(0, 100, -50, -40, -10, 5)] # generic_green band on insulation tape

def leds(on_arr):
    for led in range(1,4):
        if (on_arr[led - 1]):
            LED(led).on()
        else:
            LED(led).off()

while(True):
    clock.tick()
    img = sensor.snapshot()
    total = 0
    count = 0
    for blob in img.find_blobs(thresholds, x_stride=3, y_stride=3, pixels_threshold=10, area_threshold=8, roi=(0,100, 320, 20),merge=True, margin=5):
        img.draw_rectangle(blob.rect())
        img.draw_cross(blob.cx(), blob.cy())
        #print(blob.cx())
        if (blob.cx() < 160):
            leds((True, False, False))
        elif (blob.cx() > 200):
            leds((False, True, False))
        elif (blob.cx() < 200 and blob.cx() > 160):
            leds((True, True, True))
        else:
            leds((False, False, False))


