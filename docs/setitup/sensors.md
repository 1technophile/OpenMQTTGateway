# Sensors
## Compatible sensors
|Module|Purpose|Where to Buy|
|-|-|-|
|DHT11|Temperature, Humidity|-|
|DHT22|Temperature, Humidity|-|
|HCSR501|PIR|-|
|BH1750|Digital light|-|
|BME280|Temperature, Humidity, Pressure|-|
|BMP280|Temperature, Pressure|-|
|C-37, YL-83, HM-RD|Leak, Water|-|
|HTU21|Temperature, Humidity|-|
|GPIO Input|Inputs|-|
|GPIO KeyCode|Keycode|-|
|INA226|Current, Voltage|-|
|MQ2|Gas (flammable)|-|
|TEMT6000|Luminosity|-|
|TSL2561|Luminosity|-|

## Pinout
|Module|Arduino Pin| ESP8266 Pin|ESP32 Pin|
|-|-|-|-|
|Analog reading|A0|A0|A0|
|BH1750 SDA|A4|D2|21|
|BH1750 SCL|A5|D1|22|
|BME280/BMP280 SDA|A4|D2|21|
|BME280/BMP280 SCL|A5|D1|22|
|C-37, YL-83, HM-RD|A0 + D14|A0 + D14|A7 + D14|
|DHT11/22|<a href="img/OpenMQTTgateway_Arduino_Addon_DHT.png" target="_blank">D8</a>|<a href="img/OpenMQTTgateway_ESP8266_Addon_DHT.png" target="_blank">D1</a>|16|
|HC-SR501/HC-SR505|7|D5|5|
|HTU21 SDA|A4|D2|21|
|HTU21 SCL|A5|D1|22|
|INA226 SDA|A4|D2|21|
|INA226 SCL|A5|D1|22|
|MQ02 |A0 + D4|A0 + D4|A0 + D4|
|TEMT6000 |A0|A0|A0|
|TSL2561 SDA|A4|D2|21|
|TSL2561 SCL|A5|D1|22|

Vcc pin of the board and the Module to a 3.3V or 5V supply source depending on sensor voltage requirement.
Ground pins of the board and the Module to the ground of the supply source.
