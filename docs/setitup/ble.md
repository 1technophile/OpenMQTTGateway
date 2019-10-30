# BLE gateway
## Compatible parts
The ESP32 has an integrated BLE module, with this board the BLE gateway don't need any additional hardware. Making it the **advised board for BLE**.

For Arduino and ESP8266 you can use an HM10 or HM11 below.
|Module|Purpose|Where to Buy|
|-|-|-|
|HM 10 Keyes bluetooth module|Bluetooth|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|

## Pinout
|Module Pin|Board RX Pin|Board TX Pin|
|-|:-:|:-:|
|Arduino|D6 to HM10 TX|D5 to HM10 RX|
|ESP8266|D6 to HM10 TX|D7 to HM10 RX|

Vcc pin of the board and the HM10 Module to a 5V supply source.
Ground pins of the board and the HM10 Module to the ground of the supply source.
The HM10/11 firmware version must be  >= v601.
The baud rate of the HM10/11 module must be set to 9600 bauds.

## Arduino Hardware setup
![BLE Arduino](../img/OpenMQTTgateway_Arduino_Addon_BT.png)

## ESP8266 Hardware setup
![BLE ESP8266](../img/OpenMQTTgateway_ESP8266_Addon_BT.png)


