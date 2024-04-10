# Boards

OpenMQTTGateway is not closed to one board or type of board, by using the power of the Arduino framework and libraries that are cross compatibles it let you many choice of hardware, from an ESP8266 to an ESP32.

You can take a look to the [OpenMQTTGateway compatible website](https://compatible.openmqttgateway.com) to have a view of the [supported boards](https://compatible.openmqttgateway.com/index.php/boards/).

Moreover the gateways capacities can be extended with sensors; DHT, HC SR501, ADC, I2C bus, INA226, MQ2, TEMT6000, TSL2561, BME280/BMP280, HTU21D, AHTx0, DS1820
or actuators; LED, relays, PWM.

::: tip Running on a computer
If you want to use the BLE decoding capabilities of OpenMQTTGateway with a Raspberry Pi, Windows or Unix PC you can use [Theengs Gateway](https://gateway.theengs.io/).
:::

::: tip Running on a tablet or phone
If you want to use the BLE decoding capabilities of OpenMQTTGateway with a tablet or smartphone you can use [Theengs App](https://app.theengs.io/).
:::

[Theengs bridge](https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna) is a powerful BLE to MQTT gateway for over [100 sensors](https://decoder.theengs.io/devices/devices.html). Equipped with an Ethernet port, and external antenna, ensuring an enhanced range for your BLE sensors. It supports also WiFi connectivity.

<div style="text-align: center;">
    <a href="https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna" target="_blank" rel="noopener noreferrer">
    <img src="/img/Theengs-Bridge-ble-gateway.png" alt="Theengs bridge view" style="max-width: 100%; height: auto;">
    </a>
</div>

[Theengs plug](https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption) is available flashed with OpenMQTTGateway, and brings the functions below:
* BLE to MQTT gateway, tens of Bluetooth devices supported thanks to Theengs Decoder library. The plug uses an ESP32 acting as a BLE to Wifi gateway to scan, decode and forward the data of the nearby sensors,
* Smart plug that can be controlled remotely,
* Energy consumption monitoring,
* Device tracker,
* Presence detection (beta),
* Local connectivity first.

<div style="text-align: center;">
    <a href="https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption" target="_blank" rel="noopener noreferrer">
    <img src="/img/Theengs-Plug-OpenMQTTGateway.png" alt="Theengs plug view" style="max-width: 100%; height: auto;">
    </a>
</div>

Support the project by purchasing the [Theengs bridge](https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna) or the [Theengs plug](https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption)
The plug is available in North America only, other regions are planned.

Choosing your board depends heavily on the technologies you want to use with it.
To have a good overview of the compatibilities per board you can refer to the compatible modules attributes of each [board](https://compatible.openmqttgateway.com/index.php/boards/).

The choice between these boards will depend on your knowledge and your requirements in terms of reliability, situation, modules wanted and devices you already have. The table below present those (auto-generated)
