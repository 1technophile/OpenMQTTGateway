# Adding protocols

Adding your device protocol to OpenMQTTGateway enable to increase interoperability and to create new use cases with your device. You will find below some guidance to do that. 

## RF or IR
For adding RF and IR propotocols to OpenMQTTGateway the best way is to do a pull request to [RCSwitch](https://github.com/1technophile/rc-switch), [Pilight](https://github.com/pilight/pilight) for RF and [IRRemoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) for IR.

## BLE
For BLE devices you can do a pull request directly to the [OpenMQTTGateway](https://github.com/1technophile/OpenMQTTGateway) repository.

Currently we support the reading of advertizing BLE devices, advertizing means that the BLE device broadcast regularly its sensor data without the need of a BLE connection.

