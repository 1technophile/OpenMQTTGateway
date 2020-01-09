# Troubleshooting

## Compilation/build error
This badge [![Build Status](https://travis-ci.org/1technophile/OpenMQTTGateway.svg?branch=master)](https://travis-ci.org/1technophile/OpenMQTTGateway) show you the state of the compilation of the master and this one [![Build Status](https://travis-ci.org/1technophile/OpenMQTTGateway.svg?branch=development)](https://travis-ci.org/1technophile/OpenMQTTGateway) for the development branch. If you see a green badge this means that the code compilation is OK with the configuration given in the docs/platformio.ini. Check your IDE environment version, boards version, libraries version before submitting an issue or a question.
Verify especially that the libraries provided into the [the release page](https://github.com/1technophile/OpenMQTTGateway/releases) are located into your "sketchbook folder"/libraries if your are using the Arduino IDE.

## ESP32 compilation errors related to wifi
If you get one or several of the following errors:

`error: 'WIFI_STA' was not declared in this scope`

`error: 'class WiFiClass' has no member named 'mode'`

`error: no matching function for call to 'WiFiClass::macAddress()`

You have a conflict between Arduino default wifi library and ESP32 one. So as to resolve this issue you should move or remove the Arduino wifi library (Arduino Sketchbook folder\libraries\WiFi) in order to enable the IDE to take the one from ESP32 (Arduino Sketchbook folder\hardware\espressif\arduino-esp32\libraries\WiFi)
More info on [this topic](https://community.openmqttgateway.com/t/esp32-compilation-error/144/5?u=1technophile)

## Not able to send or receive RF or IR 
→ Verify your power supply voltage with a multimeter, it should be 5V (can be 12V for FS1000A emitter),  please note that on NodeMCU V3 the Vin does not supply 5V contrary to NodeMCU V1

→ Verify your wiring

→ To eliminate issues of OpenMqttGateway or you home controller try uploading basic examples from the libraries directly (like [SendDemo](https://github.com/sui77/rc-switch/tree/master/examples/SendDemo) for RF or IRSendDemo for IR) and execute them. If it doesn't work this means that you have mostly an issue related with your hardware or due to IDE/library version used.

Regarding the IR led emitter you can replace it with a normal led and see if it light on when you send an mqtt command

→ If you are only unable to receive RF on nodemcu (or if it only works when a serial connection is active):

try with D2 instead of D3
and put
define RF_RECEIVER_PIN 4 // D2 on nodemcu
in config_rf.h
instead of
define RF_RECEIVER_PIN 0 // D3 on nodemcu

## Exception seen on serial monitor:
Hey I got a callback 
malloc
memcpy
7
Exception (2):

→ You are not using the last update of ESP8266 into board manager, go to your Arduino IDE and update it, should be at least 2.3.0

## You don't see the messages appearing on your broker but they appears on the serial monitor
This is due to a too small mqtt packet size, open pubsubclient.h from pubsubclient library and set:
try to modify in pubsubclient.h:
`#define MQTT_MAX_PACKET_SIZE 1024`
The other option is to use the [pubsubclient library](https://github.com/1technophile/OpenMQTTGateway/tree/master/lib/pubsubclient) provided in the OMG repository folder.

## Your Arduino with w5100 Ethernet shield does not connect to network until you press Reset button
If you notice that your Arduino with w5100 Ethernet shield does not connect to network until you press its Reset button, but connects fine if you connect the Arduino with a USB cable to a computer/laptop with Arduino IDE running and open Serial Monitor, the problem is most likely the Ethernet shield and/or the power supply you're using.
According to this [video](https://www.youtube.com/watch?v=9ZBeprOqC3w&feature=youtu.be), w5100 clones sometimes struggle to initialise because the reset pin wasn't held low long enough. The solution is simple - add a 0.1uF (100nF) capacitor between the pins on the reset switch. You can get more details [here](http://forum.arduino.cc/index.php?topic=28175.15).
But even with this fix your board might not work well with a specific PSU. I would recommend try at least one different one and also try bigger capacitor (some report using 47uF)

If you didn't find your answer here post a question to the forum:
[![Community forum](https://img.shields.io/badge/community-forum-brightgreen.svg)](https://community.openmqttgateway.com)
