# Untitled - By: dave - Sat Jan 12 2019

import sensor, image, time
import sensor, image, lcd
import time
from pyb import LED
import uos

red_led   = LED(1)
green_led = LED(2)
blue_led  = LED(3)
ir_led    = LED(4)

def led_control(x):
    if   (x&1)==0: red_led.off()
    elif (x&1)==1: red_led.on()
    if   (x&2)==0: green_led.off()
    elif (x&2)==2: green_led.on()
    if   (x&4)==0: blue_led.off()
    elif (x&4)==4: blue_led.on()
    if   (x&8)==0: ir_led.off()
    elif (x&8)==8: ir_led.on()


sensor.reset() # Initialize the camera sensor.
sensor.set_pixformat(sensor.RGB565) # or sensor.GRAYSCALE
sensor.set_framesize(sensor.QQVGA2) # Special 128x160 framesize for LCD Shield.
sensor.skip_frames(time = 2000)
lcd.init() # Initialize the lcd screen.

clock = time.clock()
#uos.rename("main.py", "new.py")
#uos.rename("old.py", "main.py")
uos.sync()

while(True):
    clock.tick()
    led_control(5)
    time.sleep(100)
    led_control(0)
    time.sleep(100)
    img = sensor.snapshot()
    img.draw_string(0, 0, "FPS:" + str(clock.fps()))
    lcd.display(img) # Take a picture and display the image.
