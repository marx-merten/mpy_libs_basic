# Default Logging
import random

import utime as time
import ulogging as logging
import machine
import utils

import cfled

LOG = logging.getLogger(__name__)
LOG.info("STARTING")
def test1(count):
    # haldebug.meminfo()
    p = machine.Pin(23)
    l = cfled.RmtLed(pin=p,bpp=4,leds=count)
    l.clear()
    l.display()

    # haldebug.meminfo()
    return l

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
        col=(random.randint(0,255),random.randint(0,255),random.randint(0,255),random.randint(0,255))
        fill(leds,col,count)
        disp(leds)

@utils.timed_function
def cycle(leds,step,count,offset):
    for i in range(count):
        hue=((i*step)+offset)%255
        leds.set(i,hsv_plain=(hue,200,200))
    leds.display()

f=0

def fullcycle(leds):
    for i in range (1000):
        cycle(leds,10,len(leds),i*10)


def scroll(leds,step):
    for _ in range (1000):
        leds>>=step
        time.sleep_ms(40)
        leds.display()
