# Devices

You can take a look to the [OpenMQTTGateway compatible website](https://compatible.openmqttgateway.com) to have a view of the [supported devices](https://compatible.openmqttgateway.com/index.php/devices/).

Added to that is an overview of devices supported by OpenMQTTGateway:

## For radio frequency devices 
OpenMQTTGateway can support a wide range of 433mhz/315mhz devices, all the ones with SC5262 / SC5272, HX2262 / HX2272, PT2262 / PT2272, EV1527, RT1527, FP1527, HS1527 chipsets are supported by the RF gateway. Added to that RF2 support Kaku and Pilight an [huge list](https://wiki.pilight.org/devices). 
Note that for the moment RF, RF2 and Pilight can not be activated on the same boards together.

![boards](../img/OpenMQTTGateway_devices_rf1.png ':size=250%')
![boards](../img/OpenMQTTGateway_devices_rf2.png ':size=250%')
![boards](../img/OpenMQTTGateway_devices_rf3.png ':size=250%')

## For BLE devices 
OpenMQTTGateway is able to scan all the BLE devices that advertise their data so as to do presence detection. Added to that it can retrieve the measures from the devices below.

|Devices|Model|Measurements|
|-|:-:|:-:|
| BLE watches with fixed mac||rssi for presence detection|
| BLE beacons keychains||rssi for presence detection|
| Vegtrug ||temperature/moisture/luminance/fertility|
| XIAOMI Mi Flora |HHCCJCY01HHCC|temperature/moisture/luminance/fertility|
| XIAOMI Mi Jia |LYWSDCGO|temperature/humidity/battery|
| XIAOMI Mi Jia 2 (1)|LYWSD03MMC|temperature/humidity/battery/volt|
| XIAOMI Mi Jia 2 custom firmware (2)|LYWSD03MMC ATC|temperature/humidity/battery/volt|
| XIAOMI Mi Jia 2 custom firmware (3)|LYWSD03MMC PVVX|temperature/humidity/battery/volt|
| XIAOMI MHO-C401 (1)|MHO-C401|temperature/humidity/battery/volt|
| XIAOMI Mi Lamp |MUE4094RT|presence|
| HONEYWELL |JQJCY01YM|formaldehyde/temperature/humidity/battery|
| INKBIRD (1)|IBS-TH1|temperature/humidity/battery|
| ClearGrass |CGG1|temperature/humidity/battery|
| Qingping |CGDK2|temperature/humidity|
| Qingping |CGH1|open|
| Qingping |CGPR1|presence/luminance|
| ClearGrass alarm clock|CGD1|temperature/humidity|
| ClearGrass with atmospheric pressure |CGP1W|temperature/humidity/air pressure|
| Clock |LYWDS02|temperature/humidity|
| XIAOMI Mi Scale v1 (1)|XMTZC04HM|weight|
| XIAOMI Mi Scale v2 (1)|XMTZC05HM|weight|
| XIAOMI Mi band (1)||steps|
| iNode Energy Meter (1)||power/energy/battery|
| Thermobeacon|WS02|temperature/humidity/volt|

Exhaustive list [here](https://compatible.openmqttgateway.com/index.php/devices/ble-devices/)

::: tip
- (1) Not supported with HM10.
- (2) See https://github.com/atc1441/ATC_MiThermometer
- (3) See https://github.com/pvvx/ATC_MiThermometer
:::

![devices](../img/OpenMQTTGateway_devices_ble.png ':size=250%')

## For infrared IR devices 
The list of supported devices for ESP is [here](https://github.com/crankyoldgit/IRremoteESP8266/blob/master/SupportedProtocols.md), and [here](https://github.com/1technophile/OpenMQTTGateway/blob/6f73160d1421bebf2c1bbc9b8017978ff5b16520/main/config_IR.h#L123) for Arduino boards, as there is also the possibility of using raw and global caché (ESP)  sending possibilities of this gateway is huge!

## LORA
LORA is more dedicated at this moment for tinkering and DIY and there is no Off the shelves devices compatible to my knowledge with this gateway.
