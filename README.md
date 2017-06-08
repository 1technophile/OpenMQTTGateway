[![Build Status](https://travis-ci.org/1technophile/OpenMQTTGateway.svg?branch=master)](https://travis-ci.org/1technophile/OpenMQTTGateway)
[![Gitter chat](https://img.shields.io/gitter/room/nwjs/nw.js.svg)](https://gitter.im/OpenMQTTGateway/Questions_support)

[![](https://github.com/1technophile/OpenMQTTGateway/blob/master/img/OpenMQTTGateway.jpg)](https://github.com/1technophile/OpenMQTTGateway/wiki)

A list of supported 433mhz devices is available here:
https://community.home-assistant.io/t/433tomqttto433-gateway-device-list/7819

# Overview

OpenMQTTGateway act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker. It is compatible with :
Arduino boards + W5100
ESP8266: nodemcu V1.0, nodemcu V3.0, Wemos D1
 
  It enables to:
* Send RF signals corresponding to received MQTT data (MQTT->RF)
* Publish MQTT data related to a received 433Mhz signal (RF-->MQTT)
* Send IR signal corresponding to received MQTT data (MQTT->IR)
* Publish MQTT data related to received IR signal (IR-->MQTT)
* Publish MQTT data related to BLE beacons (BT-->MQTT)

You can add extra sensors directly to the gateway if you like!

Functions:
* Bidirectional, the gateway can send and receive signals, for example if you have RF wall plug you can either control them with your home automation software (MQTT-->RF) or the physical remote control. When you press a button on the physical remote the wall plug will switch ON and the button in your home automation software (RF-->MQTT) will be updated.
* Signal duplicate removal, with RF of IR we get a lot of duplication, to avoid this the gateway is able to filter the number of code sent to the MQTT broker during a defined time “time_avoid_duplicate”
* BLE beacons detection, the gateway publish the beacons detected thanks to an HM 10 module and publish the signal strength of each module adress. By this way you can easily adress presence detection of people, things or others...
* Addons possibility, you could add directly to the gateway any sensor you want, just create a new Z<addon> .ino file into the gateway folder with your code and call it after client.loop(); into OpenMQTTGateway.ino. The program is provided with a DHT sensor addon implementation (ZaddonDHT.ino)
* Advanced signal details publication, so as to know the details of what the gateway receive it publish by MQTT to subject433toMQTTAdvanced for RF and subjectIRtoMQTTAdvanced for IR the details of the signal received (this signal has to be compatible with RCSwitch and IRRemote libraries to be read).
* Acknowledgement, to be sure that the gateway received the payload from MQTT it sends an acknowledgement on a user_defined topic "subjectGTWRFtoMQTT" for RF or "subjectGTWIRtoMQTT" for IR enabling to update the state of your switch or other component into your home automation
* Multi boards compatibility, the gateway has been currently tested ok with ESP8266 12F, NodeMCU V1, NodeMCU V2, Wemos D1 mini, Arduino Uno, Arduino Mega 
* Multi protocols handling, the gateway is based on RCSwitch and IRRemote libraries, you can define the protocol you want to use by specifying it inside the topic.

See wiki for more info
https://github.com/1technophile/OpenMQTTGateway/wiki

-examples_with_gpio_usage/433toMQTTto433nFan_ESP8266 - ESP8266 gateway between 433mhz and a fan's remote control, from MQTT to the fan only.
 -Tutorial is here https://github.com/rickybrent/433toMQTTto433_ESP8266/wiki/Flower-Angel-18R-ST6
