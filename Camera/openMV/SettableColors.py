import sensor, image, lcd, time, utime
from pyb import UART, LED, USB_VCP, Timer
from State import Reader, MSG_GOOD, MSG_BAD, MSG_NONE, MSG_PARTIAL, Message
from uos import stat
from sys import exit

# A message
# START: CMD: Length: Data : End
# e.g (CD1234)

######################################################################################

def statusOkay(img):
    return img.draw_rectangle(0,0,30,30, color=(0,255,0), fill=True)

def statusBad(img):
    return img.draw_rectangle(0,0,30,30, color=(255,0,0), fill=True)

def statusFind(img):
    return img.draw_rectangle(0,0,30,30, color=(0,0,0), fill=True)
###################################################################
# LCD is 128x160
# We are working on QVGA which is 320x240
def blobFind(statusDraw = None):
    clock.tick();
    img = sensor.snapshot()
    xPos = 160 # Centre of field
    for blob in img.find_blobs([threshold], area_threshold=450, merge=True, margin=10, roi=(0,180,320,30)):
        img.draw_rectangle(blob.rect(),fill=True)
        xPos = blob.cx() # Actually better to use some function defining the angle to the camera.
    img.crop(x_scale=128/320,y_scale=160/240)
    img.draw_rectangle(0,120,128,20)
    img.draw_string(30, 10, str(clock.fps()),scale=2)
    retval = str(xPos)
    retval = ("0" * (3 - len(retval))) + retval
    img.draw_string(64 - 50, 120, retval,scale=4, color=(0,0,255))

    if showImage:
        img.draw_image(message.picture, 20,20)
    lcd.display(statusDraw(img))
    return retval

#########################################
### One off Timer callback
def clearImage(tim):
    global showImage
    tim.deinit()
    showImage = False


###################################
# GLOBALS
clock = time.clock()
buffer = bytearray(20)
uart = UART(3, 460800)
#threshold = [64, 79, -49, -19, -28, -4] # Green Line
threshold = [0, 100, -128, 127, -128, 127]
reportMode = False
showImage = False

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
message = rdr.getMessage()
msgStatus = statusOkay

while(True):
    while uart.any() > 0:
        next = uart.readchar()
        rdr.handleChar(chr(next))
        uart.writechar(next)
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
        elif message.command == 'L': # Reset light balance
            sensor.set_auto_gain(True)
            sensor.set_auto_whitebal(True)
            sensor.skip_frames(time = 2000)
            sensor.set_auto_gain(False)
            sensor.set_auto_whitebal(False)
        elif message.command == 'R': # Toggle report mode.
            reportMode = not reportMode;
        elif message.command == 'P': # Show image file on screen.
            print("Got a Piccy")
            showImage = True
            tim = Timer(4, freq=0.2)
            tim.callback(clearImage)
    elif message.status == MSG_BAD:
        msgStatus = statusBad

    # Fast processing if we are getting a message from Bluno
    if message.status != MSG_PARTIAL:
        if reportMode:
            uart.write("POS:" + position)
            position = blobFind(statusFind)
        else:
            position = blobFind(msgStatus)
