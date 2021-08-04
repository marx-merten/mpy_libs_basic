

# ESP32 nodeMCU 32S
#                                   +------------------------------+
#                          VIN/3V3 -|                              |- GND
#                        RESET--EN -|                [ LED1       ]|- GPIO23--VPSIMOSI
# (I Only)           ADC0---GPIO36 -| [            ] [ LED2       ]|- GPIO22--I2C SCL
# (I Only)           ADC3---GPIO39 -| [            ] [            ]|- GPIO1---TX0
# (I Only)           ADC6---GPIO34 -| [            ] [            ]|- GPIO3---RX0
# (I Only)           ADC7---GPIO35 -| [            ] [ DEBUG      ]|- GPIO21--I2C SDA
#            Touch9--ADC4---GPIO32 -| [            ] [            ]|- GND
#            Touch8--ADC5---GPIO33 -| [            ] [            ]|- GPIO19--VSPIMISO
#              DAC1--ADC18--GPIO25 -| [            ] [            ]|- GPIO18--VSPI/SCK
#              DAC2--ADC19--GPIO26 -| [            ] [            ]|- GPIO5---VSPI/SS
#            Touch7--ADC17--GPIO27 -| [            ] [            ]|- GPIO17--TX1
#  HSPI/CLK--Touch6--ADC16--GPIO14 -| [*jtag*      ] [            ]|- GPIO16--RX1
#  HSPIMISO--Touch5--ADC15--GPIO12 -| [*jtag*      ] [            ]|- GPIO4---ADC10----TOUCH0
#                              GND -| [            ] [            ]|- GPIO0---ADC11----TOUCH1
#  HSPIMOSI--Touch4--ADC14--GPIO13 -| [*jtag*      ] [            ]|- GPIO2---ADC12----TOUCH2
#                 FLASH/D2---GPIO9 -| [**reserved**] [ **jtag**   ]|- GPIO15--ADC13----TOUCH3--HSPI/CS
#                 FLASH/D3--GPIO10 -| [**reserved**] [**reserved**]|- GPIO8---FLASH/D1
#                 FLASHCMD--GPIO11 -| [**reserved**] [**reserved**]|- GPIO7---FLASH/D0
#                           VIN/5V -|                [**reserved**]|- GPIO6---FLASHSCK
#                                   +------------+-----+-----------+
#                                      (EN)     | USB |  (Prog)
#                                              +-----+
# <JTAG PINS>

# SW Matrix

PIN_LED1 = 23
PIN_LED2 = 22
PIN_DEBUG = 21
PIN_DEBUG2 = 19
