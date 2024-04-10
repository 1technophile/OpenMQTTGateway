OpenMQTTGateway aims to unify various technologies and protocols into a single firmware. This reduces the need for multiple physical bridges and streamlines diverse technologies under the widely-used [MQTT](http://mqtt.org/) protocol.

<div style="text-align: center;">
<img src="img/OpenMQTTGateway.png" alt="Overview of the protocols and compatible controllers" style="max-width: 100%; height: auto;">
</div>

# What is an MQTT gateway or bridge ?

MQTT, short for Message Queuing Telemetry Transport, is a lightweight messaging protocol ideal for IoT devices.

An MQTT gateway or bridge plays a pivotal role in the MQTT ecosystem:
* Protocol Translation: It converts non-MQTT protocols (like LoRa or Bluetooth) into MQTT, enabling broader network communication.
* Data Aggregation: Combines data from multiple devices into single messages, optimizing network use.
* Security: Incorporates features like SSL/TLS encryption to safeguard data during transmission.
* Device Management: Handles tasks like firmware updates and remote configuration changes.

In essence, an MQTT gateway ensures smooth communication between devices and MQTT brokers, enhancing the efficiency and security of IoT systems.

# What OpenMQTTGateway can do ?

OpenMQTTGateway integrates with established technologies, such as 433mhz/315mhz protocols and infrared (IR), allowing you to upgrade and repurpose older devices. Additionally, OMG is compatible with modern technologies like Bluetooth Low Energy (BLE) and LoRa.

To have an overview of the supported PIR, door, water, temperature, smoke sensors, sirens, rings, beacons, switches & weather stations you can take a look to the 
[compatible devices list](https://compatible.openmqttgateway.com/index.php/devices)

You can run OpenMQTTGateway on a wide variety of [boards](https://compatible.openmqttgateway.com/index.php/boards/), ESP32, ESP8266, ESP32S3, ESP32C3.
BLE to MQTT gateway can also run on Raspberry Pi, Windows or Unix computers thanks to [Theengs Gateway](https://theengs.github.io/gateway/).

Using MQTT, you can seamlessly integrate with home automation platforms such as OpenHAB, Home Assistant, and others, or with IoT software like Node-Red.

# Use cases
Leveraging OpenMQTTGateway with a controller allows you to:
* Monitor a garden with a Mi Flora BLE sensor and control an irrigation valve depending on the soil moisture,
* Trigger a fan depending on the temperature and humidity thanks to a Mi Jia/LYWSD03MMC BLE sensor,
* Follow your meat temperature when cooking with an Inkbird IBBQ
* Alert yourself by a controller notification if the temperature of a fridge or freezer is too high,
* Detect a beacon/smartwatch so as to trigger a special scenario when you come home,
* [Lose weight with the help of a complete log system](https://www.youtube.com/watch?v=noUROhtf0E0&t=18s), video from [@Andreas Spiess](https://www.youtube.com/channel/UCu7_D0o48KbfhpEohoP7YSQ)
* Detect opened door or windows through 433mhz or BLE and alert yourself when leaving
* Detect water leakage or smoke remotely
* Actionate a siren if something is going wrong
* Detect if your far mailbox has been opened by the postman with LoRa
* Make smart your old TV or AC system through infrared control
* Monitor vehicle tire pressure

The limit is your imagination 😀

# Functions
Under the hood, OpenMQTTGateway offers features such as:
* Deduplication
* Simple and lite API
* Strong integrations with libraries used
* Signal forward/repeat
* Wifi web portal onboarding
* Web portal configuration
* Whitelist & Blacklist management
* Secure connections
* Over the air updates
* Local or cloud, your choice

## Using OpenMQTTGateway ?
Support open-source development through sponsorship and gain exclusive access to our private forum. Your questions, issues, and feature requests will receive priority attention, plus you'll gain insider access to our roadmap.

<div style="text-align: center;">
    <iframe src="https://github.com/sponsors/theengs/button" title="Sponsor Theengs" height="32" width="228" style="border: 0; border-radius: 6px;"></iframe>
</div>

## Products powered by OpenMQTTGateway

### Theengs Bridge, BLE gateway with external antenna

[Theengs bridge](https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna) is a powerfull BLE to MQTT gateway for over [100 sensors](https://decoder.theengs.io/devices/devices.html). Equipped with an Ethernet port, and external antenna, ensuring an enhanced range for your BLE sensors. It supports also WiFi connectivity.

<div style="text-align: center;">
    <a href="https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna" target="_blank" rel="noopener noreferrer">
    <img src="img/Theengs-Bridge-ble-gateway.png" alt="Theengs bridge view" style="max-width: 100%; height: auto;">
    </a>
</div>

### Theengs Plug, BLE gateway and Smart Plug

[Theengs plug](https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption) brings the following features:
* BLE to MQTT gateway, tens of [Bluetooth devices](https://compatible.openmqttgateway.com/index.php/devices/ble-devices/) supported thanks to Theengs Decoder library. The plug uses an ESP32 acting as a BLE to Wifi gateway to scan, decode and forward the data of the nearby sensors,
* Smart plug that can be controlled remotely,
* Energy consumption monitoring,
* Device tracker,
* Presence detection (beta),
* Local connectivity first.

<div style="text-align: center;">
    <a href="https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption" target="_blank" rel="noopener noreferrer">
    <img src="img/Theengs-Plug-OpenMQTTGateway.png" alt="Theengs plug view" style="max-width: 100%; height: auto;">
    </a>
</div>

Support the project by purchasing the [Theengs bridge](https://shop.theengs.io/products/theengs-bridge-esp32-ble-mqtt-gateway-with-ethernet-and-external-antenna) or the [Theengs plug](https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption)

## Media

* [Hackaday - ARDUINO LIBRARY BRINGS RTL_433 TO THE ESP32](https://hackaday.com/2023/01/13/arduino-library-brings-rtl_433-to-the-esp32)
* [CNX Software - 433 MHz is not dead! Using an ESP32 board with LoRa module to talk to 433 MHz sensors](https://www.cnx-software.com/2023/01/14/esp32-board-with-lora-433-mhz-sensors/)
* [RTL_433 PORTED TO ESP32 MICROCONTROLLERS WITH CC1101 OR SX127X TRANSCEIVER CHIPS](https://www.rtl-sdr.com/rtl_433-ported-to-esp32-microcontrollers-with-cc1101-or-sx127x-transceiver-chips/)
* [Using low-cost wireless sensors in the unlicensed bands](https://lwn.net/Articles/921497/)
* [SMART PLUG ESP32 OPENMQTTGATEWAY SERVING AS AN BLE MQTT GATEWAY AND A POWER METER](https://www.electronics-lab.com/smart-plug-esp32-openmqttgateway-serving-as-an-ble-mqtt-gateway-and-a-power-meter/)

### Theengs Plug
[![Theengs Plug video ElektroMaker](https://img.youtube.com/vi/nUwMt9p2U7o/0.jpg)](https://www.youtube.com/watch?v=nUwMt9p2U7o&t=427s)

### 433Mhz and BLE
[![433Mhz and BLE gateway video by Andreas Spiess](https://img.youtube.com/vi/_gdXR1uklaY/0.jpg)](https://www.youtube.com/watch?v=_gdXR1uklaY)

### BLE
[![BLE gateway video by Andreas Spiess](https://img.youtube.com/vi/noUROhtf0E0/0.jpg)](https://www.youtube.com/watch?v=noUROhtf0E0)

### 433Mhz
[![RTL_433 video by TECH MIND](https://img.youtube.com/vi/H-JXWbWjJYE/0.jpg)](https://www.youtube.com/watch?v=H-JXWbWjJYE)

### LORA
[![LORA video by Priceless Toolkit](https://img.youtube.com/vi/6DftaHxDawM/0.jpg)](https://www.youtube.com/watch?v=6DftaHxDawM)

::: warning Note
The material and information contained in this documentation is for general information purposes only. You should not rely upon the material or information on this documentation as a basis for making any business, legal or any other decisions. There is no warranty given on this documentation content. If you decide to follow the information and materials given it is at your own risk. I will not be liable for any false, inaccurate, inappropriate or incomplete information presented on this website.
:::
