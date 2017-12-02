# OpenMQTTGateway
[![](https://github.com/1technophile/OpenMQTTGateway/blob/master/img/OpenMQTTGateway.jpg)](https://github.com/1technophile/OpenMQTTGateway/wiki)

## Overview

OpenMQTTGateway act as a WIFI or Ethernet gateway between your 433mhz/315mhz/Infrared signal and a MQTT broker. It is compatible with:
Arduino uno(limited)/mega(advised) + W5100
ESP8266: NodeMCU V1.0 NodeMCU V2.0, NodeMCU V3.0, ESP8266 12F and Wemos D1.
 
  It enables to:
* Send RF signals corresponding to received MQTT data (MQTT->RF)
* Publish MQTT data related to a received 433Mhz signal (RF-->MQTT)
* Send RFM69 signals corresponding to received MQTT data (MQTT->RFM69)
* Publish MQTT data related to a received RFM69 signal (RFM69-->MQTT)
* Send IR signal corresponding to received MQTT data (MQTT->IR)
* Publish MQTT data related to received IR signal (IR-->MQTT)
* Publish MQTT data related to BLE beacons or Mi flora(BT-->MQTT)

It supports also sensors; DHT, HC SR501, ADC, I2C bus, INA226

You can also upload the code to a Sonoff RF Bridge for 433mhz applications

See the [wiki](https://github.com/1technophile/OpenMQTTGateway/wiki) for more information:  
https://github.com/1technophile/OpenMQTTGateway/wiki

A list of supported 433mhz devices is available [here](https://community.home-assistant.io/t/433tomqttto433-gateway-device-list/7819):  
https://community.home-assistant.io/t/433tomqttto433-gateway-device-list/7819

