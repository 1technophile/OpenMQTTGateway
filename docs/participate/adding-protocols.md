# Adding protocols

Adding your device protocol to OpenMQTTGateway enables it to increase interoperability and to create new use cases with your device. Below you will find some guidance to do that. 

## RF or IR
For adding RF and IR protocols to OpenMQTTGateway the best way is to do a pull request to [RCSwitch](https://github.com/1technophile/rc-switch), [Pilight](https://github.com/pilight/pilight) for RF, and [IRRemoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) for IR.

## BLE
For BLE message decoding OpenMQTTGateway uses the [Theengs Decoder](https://decoder.theengs.io/) library. New device decoder pull requests can be submitted directly to the [GitHub repository](https://github.com/theengs/decoder).

Currently we support the reading of advertizing BLE devices, advertizing means that the BLE device broadcasts regularly its sensor data without the need of a BLE connection.

