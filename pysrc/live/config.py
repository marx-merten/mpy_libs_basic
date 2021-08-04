# config.py Local configuration for mqtt_as demo programs.
from sys import platform
from mqtt_as import config
from ubinascii import hexlify
from machine import unique_id

import globals.pinout as pinout


# Not needed if you're only using ESP8266
#        wlan.connect("mwThings", 'S3cretThingz')

config['ssid'] = 'mwThings'
config['wifi_pw'] = 'S3cretThingz'

config['server'] = '172.17.0.46'
config['port'] = 1883

config['leds'] = 300

# TODO:Enter the MQTT Prefix here
MQTT_PREFIX = "iot/"
ONLINE_SUFFIX = 'device/connect'
NAME = "testdevice"