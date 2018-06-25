import pyb, ustruct
import time
from pyb import Pin, Timer

# The hardware I2C bus for your OpenMV Cam is always I2C bus 2.
bus = pyb.I2C(2, pyb.I2C.SLAVE, addr=0x12)
bus.deinit() # Fully reset I2C device...
bus = pyb.I2C(2, pyb.I2C.SLAVE, addr=0x12)
print("Waiting for Arduino...")

# There are 3 possible errors for i2c.send: timeout(116), general purpose(5) or busy(16) for err.arg[0]
counter = 0
while(True):
    time.sleep(50)
    counter = counter + 1
    text = "Hello World:{}".format(counter, '03d')
    data = ustruct.pack("<%ds" % len(text), text)
    try:
        pyb.disable_irq()
        bus.send(ustruct.pack("<h", len(data)), timeout=10) # Send the len first (16-bits).
        try:
            bus.send(data, timeout=10) # Send the data second.
            print("Sent Data" + text) # Only reached on no error.
        except OSError as err:
            pyb.enable_irq(True)
    except OSError as err:
        print_exception(err)
    pyb.enable_irq(True)

