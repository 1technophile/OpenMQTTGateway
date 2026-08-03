---
aside: false
---

# Boards

OpenMQTTGateway is not closed to one board or type of board, by using the power of the Arduino framework and libraries that are cross compatibles it let you many choice of hardware, from an ESP8266 to an ESP32.

## Which board do I need?

For the most common use cases, the choice is simple:

|I want to...|Recommended board|Extra hardware|
|-|-|:-:|
|Bring Bluetooth sensors into my home automation|Any ESP32 development board|None|
|Receive RF devices — 433/315/868/915MHz (weather stations, door/PIR sensors...)|LILYGO® LoRa32 V2.1 or Heltec LoRa V2, matching your devices' frequency|None|
|Send and receive RF remotes and switches (433/315MHz)|ESP32 + SRX882/STX882 or CC1101 modules|[Wiring](../setitup/rf.md)|
|Control my TV or AC through infrared|ESP32 + IR emitter/receiver|[Wiring](../setitup/ir.md)|
|Reach far-away sensors with LoRa|LILYGO® LoRa32 or Heltec LoRa board|None|
|Skip flashing and wiring entirely|[Theengs Bridge](https://shop.theengs.io/), a pre-flashed BLE gateway|None|

::: tip Running on a computer
If you want to use the BLE decoding capabilities of OpenMQTTGateway with a Raspberry Pi, Windows or Unix PC you can use [Theengs Gateway](https://gateway.theengs.io/), and on a tablet or smartphone the [Theengs App](https://app.theengs.io/).
:::

## Exploring further

Moreover the gateways capacities can be extended with sensors; DHT, HC SR501, ADC, I2C bus, INA226, MQ2, TEMT6000, TSL2561, BME280/BMP280, HTU21D, AHTx0, DS1820
or actuators; LED, relays, PWM.

Choosing your board depends heavily on the technologies you want to use with it.
The choice between these boards will depend on your knowledge and your requirements in terms of reliability, situation, modules wanted and devices you already have. Use the table below to explore the latest environments.

<BoardEnvironmentTable 
      boardsUrl="/boards-info.json"
      selectorPath="/upload/board-selector.html"
/>
