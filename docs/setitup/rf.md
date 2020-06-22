# RF gateway (433mhz/315mhz)
## Compatible parts
|Module|Purpose|Where to Buy|
|-|-|-|
|SRX882 (recommended)|433Mhz Receiver|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|STX882 (recommended)|433Mhz Transmitter|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|XD RF 5V|433Mhz Receiver|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|FS1000A|433Mhz Transmitter|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|CC1101|433Mhz Transceiver|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|

## Pinout
|Board| Receiver Pin| Emitter Pin|
|-|:-:|:-:|
|Arduino UNO|D3|D4|
|ESP8266|D2/**D3**/D1/D8|**RX**/D2|
|ESP32|**27**/26|12|
|RF BRIDGE|-|-|
|RF BRIDGE [DIRECT HACK](https://github.com/xoseperez/espurna/wiki/Hardware-Itead-Sonoff-RF-Bridge---Direct-Hack)|4|5|
|SONOFF RFR3|4|-|
|RF WIFI GATEWAY|5|-|

Connect the Emitter and Receiver to a 5V (**3.3V** for CC1101) supply source, and the ground of your supply source to the ground of your board.

### CC1101 Pinout
|Board|Receiver Pin(GDO2)|Emitter Pin(GDO0)|SCK|VCC|MOSI|MISO|CSN|GND
|-|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|ESP8266|D2/**D3**/D1/D8|**RX**/D2|D5|**3V3**|D7|D6|D8|GND
|ESP32|**27**/26|12|D18|**3V3**|D23|D19|D5|GND

More information about the [CC1101 wiring](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib#wiring).


## Arduino Hardware setup
![RF](../img/OpenMQTTgateway_Arduino_Addon_RF.png)

## ESP8266 Hardware setup
If the gateway works only when serial monitor is connected don't use D3 use D2 instead (gpio 4) and modify config_RF.h accordingly.

With SRX882 some users reported that D3 is not working use D1 instead in this case and modify config_RF.h accordingly.

![Addon_RF](../img/OpenMQTTgateway_ESP8266_Addon_RF.png)

## ESP32 Hardware setup
![Addon_RF](../img/OpenMQTTgateway_ESP32_Addon_RF.png)

## SONOFF RF Bridge Hardware setup
Per default there is no need on modifying the RF Bridge hardware, unless you don't want to use the provided RF controller (EFM8BB1). Indeed if you want to extend the protocols supported by the bridge you can [bypass this controller](https://github.com/xoseperez/espurna/wiki/Hardware-Itead-Sonoff-RF-Bridge---Direct-Hack) and use the ESP8255 capacities to decode RF Signal.
The RF processing can be achieved after the modification by either RF, RF2 or Pilight gateways.

## SONOFF RFR3 Hardware setup
[Connect GPIO4 of the ESP8255 to the pin D0 of SYN470](https://1technophile.blogspot.com/2019/08/new-sonoff-rfr3-as-433tomqtt-gateway.html)

## WIFI RF GATEWAY Hardware setup
This board doesn't require any hardware modifications.
