import sensor, image, lcd, time
import time, utime
from pyb import UART, LED, USB_VCP
from State import Reader, MSG_GOOD, MSG_BAD, MSG_NONE, Message, message

# A message
# START: CMD: Length: Data : End
# e.g (CD1234)

######################################################################################

def statusOkay(img):
    return img.draw_rectangle(0,0,30,30, color=(0,255,0), fill=True)

def statusBad(img):
    return img.draw_rectangle(0,0,30,30, color=(255,0,0), fill=True)

###################################################################
def blobFind(showTime = False, statusDraw = None):
    clock.tick();
    img = sensor.snapshot()
    xPos = 160 # Centre of field
    for blob in img.find_blobs([threshold], pixels_threshold=50, area_threshold=50, merge=True, margin=10, roi=(0,180,320,30)):
        img.draw_rectangle(blob.rect(),fill=True)
        xPos = blob.cx() # Actually better to use some function defining the angle to the camera.
    img.crop(x_scale=0.3,y_scale=0.5)
    if showTime:
        img.draw_string(10, 30, str(clock.fps()),scale=4)

    lcd.display(statusDraw(img))
    retval = str(xPos)
    return ("0" * (3 - len(retval))) + retval


###################################
# GLOBALS
clock = time.clock()
buffer = bytearray(20)
uart = UART(3, 9600, timeout=100, timeout_char=50)
#threshold = [64, 79, -49, -19, -28, -4]
threshold = [0, 100, -128, 127, -128, 127]

##########################################################################
# MAIN CODE
#############################################################################
###########################################################################################

sensor.reset() # Initialize the camera sensor.
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False) # must be turned off for color tracking
sensor.set_auto_whitebal(False) # must be turned off for color tracking

lcd.init() # Initialize the lcd screen.

rdr = Reader()
msgStatus = statusOkay

while(True):
    while uart.any() > 0:
        # change mode depending on UART input.
        rdr.handleChar(chr(uart.readchar()))
    if message.status == MSG_GOOD:
        message.status = MSG_NONE
        msgStatus = statusOkay
        if message.command == 'C':
            threshold[0] = int(''.join(message.dataBuffer[0:3]))
            threshold[1] = int(''.join(message.dataBuffer[3:6]))
            index = 6
            for i in range(4):
                threshold[i + 2] = int(''.join(message.dataBuffer[index:index + 3])) - 128
                index = index + 3
            print(threshold)
    elif message.status == MSG_BAD:
        msgStatus = statusBad

    blobFind(True, msgStatus)
    #uart.write("POS:" + blobFind(True))
