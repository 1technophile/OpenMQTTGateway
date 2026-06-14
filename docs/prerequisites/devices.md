# Devices

Here is an overview of devices supported by OpenMQTTGateway:

## For radio frequency devices 
OpenMQTTGateway can support a wide range of 433mhz/315mhz devices, all the ones with SC5262 / SC5272, HX2262 / HX2272, PT2262 / PT2272, EV1527, RT1527, FP1527, HS1527 chipsets are supported by the RF gateway. Added to that RF2 support Kaku and Pilight an [huge list](https://wiki.pilight.org/devices). 
Note that for the moment RF, RF2 and Pilight can not be activated on the same boards together.



## For BLE devices 
OpenMQTTGateway is able to scan all the BLE devices that advertise their data so as to do presence detection. 
Added to that it retrieves the measures from the devices mentioned and linked to below. By default the data are read from the advertisements (no or very little impact on device battery life). For some devices we may connect briefly only to retrieve one or several parameters.

OpenMQTTGateway currently supports the decoding of [more than 100 Bluetooth devices](https://decoder.theengs.io/devices/devices.html), which include popular devices like Mi Flora, Xiaomi scales, Inkbird, Govee and ThermoPro thermo-hygrometers and BBQ thermometers, SwitchBot devices status and many more.

::: tip
- (2) See https://github.com/atc1441/ATC_MiThermometer
- (3) See https://github.com/pvvx/ATC_MiThermometer
:::

## For infrared IR devices 
The list of supported devices for ESP is [here](https://github.com/crankyoldgit/IRremoteESP8266/blob/master/SupportedProtocols.md), and [here](https://github.com/1technophile/OpenMQTTGateway/blob/6f73160d1421bebf2c1bbc9b8017978ff5b16520/main/config_IR.h#L123) for Arduino boards, as there is also the possibility of using raw and global cache (ESP) sending possibilities of this gateway is huge!

## LORA
LoRa is more dedicated at this moment for tinkering and DIY and there is no Off the shelves devices compatible to my knowledge with this gateway.
