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
    time.sleep(500)
    try:
        pyb.disable_irq()
        data = bus.recv(1) # receive 1 byte.
        print(data[0])
    except OSError as err:
        print_exception(err)
    pyb.enable_irq(True)

