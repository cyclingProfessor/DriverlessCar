import sensor, image, time
import pyb, ustruct
from pyb import Pin, Timer, LED

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking

# There are 3 possible errors for i2c.send: timeout(116), general purpose(5) or busy(16) for err.arg[0]
# The hardware I2C bus for your OpenMV Cam is always I2C bus 2.
bus = pyb.I2C(2, pyb.I2C.SLAVE, addr=0x12)
bus.deinit() # Fully reset I2C device...
bus = pyb.I2C(2, pyb.I2C.SLAVE, addr=0x12)

# Color Tracking Thresholds (L Min, L Max, A Min, A Max, B Min, B Max)
thresholds = [(0, 100, -50, -40, -10, 5)] # generic_green band on insulation tape

def leds(on_arr):
    for led in range(1,4):
        if (on_arr[led - 1]):
            LED(led).on()
        else:
            LED(led).off()

current_position = 1000
while(True):
    pyb.enable_irq(True)
    img = sensor.snapshot()
    save_position = current_position
    current_position = 1000
    for blob in img.find_blobs(thresholds, x_stride=3, y_stride=3, pixels_threshold=10, area_threshold=8, roi=(0,100, 320, 20),merge=True, margin=5):
        img.draw_rectangle(blob.rect())
        img.draw_cross(blob.cx(), blob.cy())
        current_position = blob.cx()
        if (blob.cx() <= 160): # RED
            leds((True, False, False))
        elif (blob.cx() >= 200): # GREEN
            leds((False, True, False))
        elif (blob.cx() < 200 and blob.cx() > 160): # WHITE
            leds((True, True, True))
    if current_position == 1000:
        leds((False, False, False))

    time.sleep(10)
    if current_position != save_position:
        time.sleep(40)
        text = "XPOS:{}".format(current_position, '04d')
        data = ustruct.pack("<%ds" % len(text), text)
        print(data)
        try:
            pyb.disable_irq()
            bus.send(ustruct.pack("<h", len(data)), timeout=1000) # Send the len first (16-bits).
            try:
                bus.send(data, timeout=1000) # Send the data second.
            except OSError as err:
                pass
        except OSError as err:
            pass
