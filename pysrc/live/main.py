# Default Logging
import time
import random
import ulogging as logging
import cfled
import machine
import utils

LOG = logging.getLogger(__name__)
LOG.info("STARTING")
def test1(count):
    # haldebug.meminfo()
    p = machine.Pin(23)
    f=cfled.RmtLed(pin=p,bpp=4,leds=count)
    f.clear()
    f.display()

    # haldebug.meminfo()
    return f

@utils.timed_function
def fill(leds,value,count):
    for i in range(count):
        leds.set(i,value)

@utils.timed_function
def disp(leds):
    leds.display()


def mass_test(leds,count,re):
    for _ in range(re):
        # time.sleep_ms(20)
        col=(random.randint(0,255),random.randint(0,255),random.randint(0,255),random.randint(0,255));
        fill(leds,col,count)
        disp(leds)

@utils.timed_function
def cycle(leds,step,count,offset):
    for i in range(count):
        hue=((i*step)+offset)%255
        leds.set(i,hsv_plain=(hue,200,200))
    leds.display()
f=0

def fullcycle():
    global f
    f=test1(100)
    for i in range (1000):
        cycle(f,10,100,i*10)


def scroll(leds,step):
    for _ in range (1000):
        leds>>=step
        time.sleep_ms(40)
        leds.display()