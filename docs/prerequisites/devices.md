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
OpenMQTTGateway is able to scan all the BLE devices that advertise their data so as to do presence detection. 
Added to that it retrieves the measures from the devices below. By default the data are read from the advertisements (no impact on device battery life). When a (c) is present after the model name, this means that the gateway connects to it so as to retrieve data or control the device. For some devices we may connect only to retrieve one or several parameters (the rest being advertised), in this case the (c) is placed with the parameter.

|Devices|Model|Measurements|
|-|:-:|:-:|
| Amazfit|Smart Watch/Band|steps, activity heart rate (when activated in the Zepp Life settings)|
| April Brother|ABTemp|uuid/mfid/major/txpower @ 1 m/temperature/battery|
| April Brother|N03|temperature/humidity/luminance/battery|
| ATorch Battery Capacity Monitor (c)|DT24|voltage/amp/watt|
| BLE watches with fixed MAC||rssi for presence detection|
| BLE beacons keychains||rssi for presence detection|
| BlueCharm|BC08|acceleration x/y/z-axis/voltage/temperature|
| BlueMaestro|TempoDisc 1 in 1|temperature/battery|
| BlueMaestro|TempoDisc 3 in 1|temperature/humidity/dew point/battery|
| BlueMaestro|TempoDisc 4 in 1|temperature/humidity/pressure/battery|
| BM2 Battery Monitor|BM2|battery/volt(c)|
| ClearGrass|CGG1|temperature/humidity/battery/voltage (depending on which CGG1 firmware is installed)|
| ClearGrass alarm clock|CGD1|temperature/humidity/battery|
| ClearGrass alarm clock|CGC1|temperature/humidity/battery|
| ClearGrass with atmospheric pressure|CGP1W|temperature/humidity/air pressure|
| ClearGrass Clock|LYWSD02|temperature/humidity/battery|
| GOVEE|H5055|temperature1/temperature2/temperature3/temperature4/temperature5/temperature6/battery|
| GOVEE|H5074|temperature/humidity/battery|
| GOVEE|H5075|temperature/humidity/battery|
| GOVEE|H5072|temperature/humidity/battery|
| GOVEE|H5101|temperature/humidity/battery|
| GOVEE|H5102|temperature/humidity/battery|
| GOVEE|H5106|PM2.5/temperature/humidity/battery|
| GOVEE|H5174|temperature/humidity/battery|
| GOVEE|H5177|temperature/humidity/battery|
| HONEYWELL|JQJCY01YM|formaldehyde/temperature/humidity/battery|
| Hydractiva Digital | Amphiro/Oras|sessions/time/litres/temperature/energy|
| iBeacon|protocol|uuid/mfid/major/minor/txpower @ 1 m/voltage|
| Jaalee|JHT F525|temperature/humidity/battery|
| INKBIRD|IBS-TH1|temperature/humidity/battery|
| INKBIRD|IBS-TH2/P01B|temperature/battery|
| INKBIRD|IBT-2X|temperature1/temperature2|
| INKBIRD|IBT-4X(S/C)|temperature1/temperature2/temperature3/temperature4|
| INKBIRD|IBT-6XS|temperature1/temperature2/temperature3/temperature4/temperature5/temperature6|
| iNode|Energy Meter|Current average and aggregate kW(h)/m³/battery|
| Oria/Brifit/SigmaWit/SensorPro|TH Sensor T201|temperature/humidity/battery|
| Oria/Brifit/SigmaWit/SensorPro|TH Sensor T301|temperature/humidity/battery|
| Mokosmart|M1|acceleration x/y/z-axis/battery|
| Mokosmart|H4|temperature/humidity/voltage|
| Mopeka|Pro|temperature/level/sync status/voltage/battery/reading quality|
| Otio/BeeWi|Door & Window Sensor|contact/battery|
| Polar|H10 Chest strap|activity heart rate|
| Qingping|CGDK2|temperature/humidity|
| Qingping|CGH1|open|
| Qingping|CGPR1|presence/luminance/battery|
| Qingping|CGDN1|temperature/humidity/PM2.5/PM10/carbon dioxide|
| RDL52832||mfid/uuid/minor/major/txpower @ 1 m/temperature/humidity/acceleration x/y/z-axis|
| RBaron|b-parasite|moisture/temperature/humidity/luminance (v1.1.0+)/voltage|
| RuuviTag Raw V1|RuuviTag|temperature/humidity/pressure/acceleration x/y/z-axis/voltage|
| RuuviTag Raw V2|RuuviTag|temperature/humidity/pressure/acceleration x/y/z-axis/voltage/TX power/movement/counter/sequence number|
| SmartDry|Laundry Sensor|temperature/humidity/shake/voltage/wake|
| Sensirion|MyCO₂/CO₂ Gadget|temperature/humidity/carbon dioxide|
| Sensirion|SHT4X TH sensor|temperature/humidity|
| Switchbot|Bot (c)|mode/state/battery|
| Switchbot|Motion Sensor|movement/light level/sensing distance/led/scope tested/battery|
| Switchbot|Contact Sensor|contact/movement/scope tested/light level/in count/out count/push count/battery|
| Switchbot|Curtain|motion state/position/light level/battery/calibration state|
| Switchbot|Meter (Plus)|temperature/humidity/battery|
| Thermobeacon|WS02|temperature/humidity/voltage/timestamp/maximum temperature/maximum temperature timestamp/minimum temperature/minimum temperature timestamp|
| Thermobeacon|WS08|temperature/humidity/voltage/timestamp/maximum temperature/maximum temperature timestamp/minimum temperature/minimum temperature timestamp|
| ThermoPro|TP357|temperature/humidity|
| ThermoPro|TP358|temperature/humidity|
| ThermoPro|TP359|temperature/humidity|
| ThermoPro|TP393|temperature/humidity|
| TPMS|TPMS|temperature/pressure/battery/alarm/count|
| Vegtrug||temperature/moisture/luminance/fertility|
| XIAOMI Mi Flora|HHCCJCY01HHCC|temperature/moisture/luminance/fertility/battery(c) firmware >3.1.8|
| XIAOMI Ropot|HHCCPOT002|temperature/moisture/fertility|
| XIAOMI Mi Jia|LYWSDCGO|temperature/humidity/battery|
| XIAOMI Mi Jia|LYWSD02|temperature/humidity/battery|
| XIAOMI Mi Jia 2(c)|LYWSD03MMC|temperature/humidity/battery/voltage|
| XIAOMI Mi Jia 2 custom firmware (2)|LYWSD03MMC ATC|temperature/humidity/battery/voltage|
| XIAOMI Mi Jia 2 custom firmware (3)|LYWSD03MMC PVVX|temperature/humidity/battery/voltage|
| XIAOMI Mi Lamp|MUE4094RT|presence|
| XIAOMI Mi Smart Scale|XMTZC01HM/XMTZC04HM|weighing mode/unit/weight|
| XIAOMI Mi Body Composition Scale|XMTZC02HM/XMTZC05HM|weighing mode/unit/weight/impedance|
| XIAOMI Mi Temp/Humidity v1(c)|MHO-C401|temperature/humidity/battery/voltage|
| XIAOMI Mi Temp/Humidity v2(c)|XMWSDJ04MMC|temperature/humidity/battery/voltage|
| XIAOMI|Mi band|steps/activity heart rate (when activated in the Zepp Life settings)|

Exhaustive list [here](https://compatible.openmqttgateway.com/index.php/devices/ble-devices/)

::: tip
- (2) See https://github.com/atc1441/ATC_MiThermometer
- (3) See https://github.com/pvvx/ATC_MiThermometer
:::

![devices](../img/OpenMQTTGateway_devices_ble.png ':size=250%')

## For infrared IR devices 
The list of supported devices for ESP is [here](https://github.com/crankyoldgit/IRremoteESP8266/blob/master/SupportedProtocols.md), and [here](https://github.com/1technophile/OpenMQTTGateway/blob/6f73160d1421bebf2c1bbc9b8017978ff5b16520/main/config_IR.h#L123) for Arduino boards, as there is also the possibility of using raw and global cache (ESP) sending possibilities of this gateway is huge!

## LORA
LORA is more dedicated at this moment for tinkering and DIY and there is no Off the shelves devices compatible to my knowledge with this gateway.
