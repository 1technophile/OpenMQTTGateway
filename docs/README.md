# OpenMQTTGateway

## Overview

OpenMQTTGateway act as a WIFI or Ethernet gateway between your 433mhz/315mhz/Infrared/BLE/GSM/GPRS devices and a MQTT broker.

The MQTT broker can be connected to your favorite home automation controller (Home Assistant, Openhab, Domoticz) or NodeRED, as long as it supports MQTT, the system can talk to the gateway and by the way your different devices on both directions.

[![](https://github.com/1technophile/OpenMQTTGateway/blob/master/img/OpenMQTTGateway.jpg)](https://github.com/1technophile/OpenMQTTGateway/wiki)

It can be installed on:
* Arduino uno(limited)/mega(advised) + W5100
* ESP8266: NodeMCU V1.0 NodeMCU V2.0, NodeMCU V3.0, ESP8266 12F and Wemos D1
* ESP32
* Sonoff RF bridge.
 
  It enables to:
* Send RF signals corresponding to received MQTT data (MQTT->RF)
* Publish MQTT data related to a received 433Mhz signal (RF-->MQTT)
* Send RFM69 signals corresponding to received MQTT data (MQTT->RFM69)
* Publish MQTT data related to a received RFM69 signal (RFM69-->MQTT)
* Send IR signal corresponding to received MQTT data (MQTT->IR)
* Publish MQTT data related to received IR signal (IR-->MQTT)
* Publish MQTT data related to BLE beacons or Mi flora(BT-->MQTT)
* Publish MQTT data related to received SMS (2G-->MQTT)
* Send SMS corresponding to received MQTT data (MQTT->2G)

It supports also sensors; DHT, HC SR501, ADC, I2C bus, INA226, TSL2561
Or can actutate a relay.

## Functions:
* Bidirectional, the gateway can send and receive signals, for example if you have RF wall plug you can either control them with your home automation software (MQTT-->RF) or the physical remote control. When you press a button on the physical remote the wall plug will switch ON and the button in your home automation software (RF-->MQTT) will be updated.
* Signal duplicate removal, with RF of IR we get a lot of duplication, to avoid this the gateway is able to filter the number of code sent to the MQTT broker during a defined time `time_avoid_duplicate`.
* BLE beacons detection, the gateway publish the beacons detected thanks to an HM-10 module and publish the signal strength of each module address. By this way you can easily address presence detection of people, animals or things.
* Addons possibility, you could add directly to the gateway any sensor you want, just create a new `Z<addon> .ino` file into the gateway folder with your code and call it after `client.loop();` into `OpenMQTTGateway.ino`. The program is provided with a DHT sensor addon implementation (`ZaddonDHT.ino`)
* Advanced signal details publication, so as to know the details of what the gateway receive it publish by MQTT to `subject433toMQTTAdvanced` for RF and `subjectIRtoMQTTAdvanced `for IR the details of the signal received (this signal has to be compatible with RCSwitch and IRRemote libraries to be read).
* Acknowledgement, to be sure that the gateway received the payload from MQTT it sends an acknowledgement on a `user_defined` topic, `subjectGTWRFtoMQTT` for RF or `subjectGTWIRtoMQTT` for IR enabling to update the state of your switch or other component into your home automation.
* Multi boards compatibility, the gateway has been currently tested with **ESP8266 12F**, **NodeMCU V1**, **NodeMCU V2**, **Wemos D1 mini**, **Arduino Uno**, **ESP32** (RF and BLE gateways) and the **Arduino Mega**.
* Multi protocols handling, the gateway is based on RCSwitch, NewRemoteSwitch, RFM69, A6lib and IRRemote libraries, you can define the protocol you want to use.
* Configuration by web portal with **ESP8266** based boards

See the [wiki](https://github.com/1technophile/OpenMQTTGateway/wiki) for more information:  
https://github.com/1technophile/OpenMQTTGateway/wiki

Download OpenMQTTGateway from the [releases page](https://github.com/1technophile/OpenMQTTGateway/releases)

The reference sheet, with the list of all functions, pinouts is [here](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=0)

A list of supported 433mhz devices (and others) is available [here](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=2126158079), door/window sensors, PIR sensors, smoke detectors, weather stations...

A list of compatible components to build your gateway is available [here](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277), nodemcu, esp32, emitters and receivers...

If you want to buy a coffee or other beverage so as to give me some support, here is a way to do it : 

<a href="https://www.buymeacoffee.com/1technophile" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" style="height: auto !important;width: auto !important;" ></a>
