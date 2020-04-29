# Sensors
## Compatible sensors
|Module|Purpose|Where to Buy|
|-|-|-|
|DHT11|Temperature and Humidity|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|DHT22|Temperature and Humidity|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|HCSR501|PIR|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|BH1750|Digital light|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|BME280|Temperature, Humidity and pressure|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|HTU21|Temperature, Humidity|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|GPIO Input|Inputs|-|
|GPIO KeyCode|Keycode|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|INA226|Current and voltage|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|TSL2561|Luminiosity|[parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|

## Pinout
|Module|Arduino Pin| ESP8266 Pin|ESP32 Pin|
|-|-|-|-|
|DHT11/22|<a href="img/OpenMQTTgateway_Arduino_Addon_DHT.png" target="_blank">D8</a>|<a href="img/OpenMQTTgateway_ESP8266_Addon_DHT.png" target="_blank">D1</a>|16|
|HC-SR501/HC-SR505|7|D5|5|
|Analog reading|A0|A0|A0|
|BH1750 SDA|A4|D2|21|
|BH1750 SCL|A5|D1|22|
|BME280 SDA|A4|D2|21|
|BME280 SCL|A5|D1|22|
|HTU21 SDA|A4|D2|21|
|HTU21 SCL|A5|D1|22|
|INA226 SDA|A4|D2|21|
|INA226 SCL|A5|D1|22|
|TSL2561 SDA|A4|D2|21|
|TSL2561 SCL|A5|D1|22|

Vcc pin of the board and the Module to a 5V supply source
Ground pins of the board and the Module to the ground of the supply source.
