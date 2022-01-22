# (Option 1) Upload from the web
You can upload the firmware to your ESP device directly from here.
1. Plug in your ESP to a USB port.
2. Select the firmware in the box below.
3. Click the install button and choose the port that the ESP is connected to.
4. Wait until the process is complete.

<web-uploader/>

The table below describes the libraries and the modules of each board configuration.

|Firmware|Boards compatible|Description|RF|IR|BLE|LORA|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|**ESP32**|
|esp32-lolin32lite-ble|ESP32 Lolin lite|Suitable for low power with BLE gateway, [tutorial](https://1technophile.blogspot.com/2021/04/low-power-esp32-ble-gateway.html)|-|-|X|-|
|esp32-m5atom|M5Atom|Compact enclosure ESP32 with BLE gateway|-|(Emit)|X|-|
|esp32-m5stack-ble|M5Stack Core|Expandable mobule with BLE gateway, display, and little IR emitter|-|-|X|-|
|esp32-m5stick-ble|M5Stick Grey|Expandable mobule with BLE gateway and little IR emitter|-|(Emit)|X|-|
|esp32-m5stick-c-ble|M5Stick C Orange|Expandable mobule with BLE gateway, display, and little IR emitter|-|(Emit)|X|-|
|esp32-m5stick-cp-ble|M5Stick C Plus Orange|Expandable mobule with BLE gateway, display, and little IR emitter|-|(Emit)|X|-|
|esp32-m5tough-ble|M5Tough rugged|Expandable mobule with BLE gateway and display, suitable for outdoor|-|-|X|-|
|esp32-olimex-gtw-ble-eth|Olimex Ethernet Gateway (non POE) using ethernet|BLE gateway with external antenna as an option|-|-|X|-|
|esp32-olimex-gtw-ble-wifi|Olimex Ethernet Gateway (non POE) using wifi| BLE gateway|-|-|X|-|
|esp32dev-ble-cont|ESP32 dev board|BLE gateway with continuous scanning, suitable in particular for door and window BLE sensors (and all the others)|-|-|X|-|
|esp32dev-ble|ESP32 dev board|BLE gateway with one scan every 55s per default|-|-|X|-|
|esp32dev-gf-sun-inverter|ESP32 dev board|RS232 reading of GridFree Sun Inverter|-|-|-|-|
|esp32dev-ir|ESP32 dev board|Infrared (Emitting and receiving) using [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)|-|X|-|-|
|esp32dev-multi_receiver|ESP32 dev board|Multi RF library with the possibility to switch between [ESPilight](https://github.com/puuu/ESPiLight), [RTL_433_ESP](https://github.com/NorthernMan54/rtl_433_ESP), [NewRemoteSwitch](https://github.com/1technophile/NewRemoteSwitch) and [RCSwitch](https://github.com/1technophile/rc-switch), need CC1101|X|-|-|-|
|esp32dev-pilight-CC1101|ESP32 dev board|Gateway using [ESPilight](https://github.com/puuu/ESPiLight) library only, need CC1101|X|-|-|-|
|esp32dev-pilight|ESP32 dev board|Gateway using [ESPilight](https://github.com/puuu/ESPiLight) without the need of CC1101|X|-|-|-|
|esp32dev-pilight-somfy-CC1101|ESP32 dev board|Gateway using [Somfy Remote](https://github.com/Legion2/Somfy_Remote_Lib) and [ESPilight](https://github.com/puuu/ESPiLight) library, need CC1101|X|-|-|-|
|esp32dev-rf|ESP32 dev board|RF gateway using [RCSwitch](https://github.com/1technophile/rc-switch) library|X|-|-|-|
|esp32dev-rtl_433|ESP32 dev board|Gateway using [RTL_433_ESP](https://github.com/NorthernMan54/rtl_433_ESP) library, need CC1101|X|-|-|-|
|esp32dev-somfy-CC1101|ESP32 dev board|Gateway using [Somfy Remote](https://github.com/Legion2/Somfy_Remote_Lib) library, need CC1101|X|-|-|-|
|esp32dev-weatherstation|ESP32 dev board|Gateway to retrieve weather station data Ventus W174/W132 (tested), Auriol H13726, Hama EWS 1500, Meteoscan W155/W160 using [WeatherStationDataRx](https://github.com/Zwer2k/WeatherStationDataRx)|X|-|-|-|
|esp32feather-ble|ESP32 Feather Adafruit|BLE for the Adafruit Feather board|-|-|X|-|
|heltec-wifi-lora32-868|ESP32 HELTEC LORA V2|LORA communication 868Mhz  using [arduino-LoRA](https://github.com/sandeepmistry/arduino-LoRa) |-|-|-|X|
|heltec-wifi-lora32-915|ESP32 HELTEC LORA V2|LORA communication 915Mhz using [arduino-LoRA](https://github.com/sandeepmistry/arduino-LoRa)|-|-|-|X|
|ttgo-lora32-v1-868|ESP32 TTGO LORA V1|LORA communication 868Mhz using [arduino-LoRA](https://github.com/sandeepmistry/arduino-LoRa)|-|-|-|X|
|ttgo-lora32-v1-915|ESP32 TTGO LORA V1|LORA communication 915Mhz using [arduino-LoRA](https://github.com/sandeepmistry/arduino-LoRa)|-|-|-|X|
|ttgo-tbeam|ESP32 TTGO T BEAM|BLE gateway with battery holder|-|-|X|-|
|**ESP8266**|
|nodemcuv2-2g|ESP8266 board|SMS gateway, need A6/A7 GSM module|-|-|-|-|
|nodemcuv2-ble|ESP8266 board|BLE gateway, need HM10 Module|-|-|X|-|
|nodemcuv2-ir|ESP8266 board|Infrared gateway using [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)|-|X|-|-|
|nodemcuv2-rf-cc1101|ESP8266 board|RF gateway using [RCSwitch](https://github.com/1technophile/rc-switch) library, need CC1101|X|-|-|-|
|nodemcuv2-rf|ESP8266 board|RF gateway using [RCSwitch](https://github.com/1technophile/rc-switch) library|X|-|-|-|
|nodemcuv2-rf2-cc1101|ESP8266 board|KAKU RF gateway using [NewRemoteSwitch](https://github.com/1technophile/NewRemoteSwitch) library, need CC1101|X|-|-|
|nodemcuv2-rf2|ESP8266 board|KAKU RF gateway using [NewRemoteSwitch](https://github.com/1technophile/NewRemoteSwitch) library|X|-|-|-|
|nodemcuv2-pilight|ESP8266 board|Gateway using [ESPilight](https://github.com/puuu/ESPiLight) without the need of CC1101|X|-|-|-|
|nodemcuv2-RS232|ESP8266 board|RS232 gateway|-|-|-|-|
|nodemcuv2-somfy-cc1101|ESP8266 board|Somfy RF gateway using [Somfy Remote](https://github.com/Legion2/Somfy_Remote_Lib) library, need CC1101|X|-|-|-|
|nodemcuv2-weatherstation|ESP8266 board|RF gateway using [WeatherStationDataRx](https://github.com/Zwer2k/WeatherStationDataRx) library|X|-|-|-|
|rf-wifi-gateway|ESP8266 board|RF gateway for USB stick using [RCSwitch](https://github.com/1technophile/rc-switch) library|X|-|-|-|
|rfbridge|ESP8255 board|RF gateway for the Sonoff RF Bridge relying on the internal decoder|X|-|-|-|
|sonoff-rfbridge-direct|ESP8255 board|RF gateway for the Sonoff RF Bridge requiring direct hack, relying on [ESPilight](https://github.com/puuu/ESPiLight) library, [tutorial](https://1technophile.blogspot.com/2019/04/sonoff-rf-bridge-pilight-or-how-to.html)|X|-|-|-|
|sonoff-basic|ESP8255 board|Wifi relay|-|-|-|-|
|sonoff-basic-rfr3|ESP8255 board|Wifi relay and RF receiver using [RCSwitch](https://github.com/1technophile/rc-switch) library, [tutorial](https://1technophile.blogspot.com/2019/08/new-sonoff-rfr3-as-433tomqtt-gateway.html)|Rec|-|-|-|

[![Hits](https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fdocs.openmqttgateway.com%2Fupload%2Fweb-install.html&count_bg=%2379C83D&title_bg=%23555555&icon=&icon_color=%23E7E7E7&title=hits&edge_flat=false)](https://hits.seeyoufarm.com)