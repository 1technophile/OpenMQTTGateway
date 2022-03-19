OpenMQTTGateway project goal is to concentrate in one firmware different technologies and protocols, decreasing by the way the number of physical bridges needed, and hiding the different technologies singularity behind a simple & wide spread communication protocol; [MQTT](http://mqtt.org/).

![Overview](./img/OpenMQTTGateway.png)

OpenMQTTGateway supports very mature technologies like basic 433mhz/315mhz protocols & infrared (IR) so you can make your old dumb devices "smart" and avoid throwing them away. These devices also have the advantages of having a lower cost compared to Zwave or more sophisticated protocols.
OMG also supports up to date technologies like Bluetooth Low Energy (BLE) or LORA.

To have an overview of the supported PIR, door, water, temperature, smoke sensors, sirens, rings, beacons, switches & weather stations you can take a look to the 
[compatible devices list](https://compatible.openmqttgateway.com/index.php/devices)

You can run OpenMQTTGateway on a wide variety of [boards](https://compatible.openmqttgateway.com/index.php/boards/), ESP32, ESP8266, Arduino MEGA, UNO (with limitation).
BLE to MQTT gateway can also run on Raspberry Pi, Windows or Unix computers thanks to [Theengs Gateway](https://theengs.github.io/gateway/).

With MQTT you can connect the compatible software you want, integrating in home automation controllers like (OpenHAB, Home Assistant, Jeedom, FHEM, Domoticz...) or Internet of Things software like Node-Red.

# Use cases
With OpenMQTTGateway and a controller you can for example:
* Monitor a garden with a Mi Flora BLE sensor and control an irrigation valve depending on the soil moisture,
* Trigger a fan depending on the temperature and humidity thanks to a Mi Jia/LYWSD03MMC BLE sensor,
* Follow your meat temperature when cooking with an Inkbird IBBQ
* Alert yourself by a controller notification if the temperature of a fridge or freezer is too high,
* Detect a beacon/smartwatch so as to trigger a special scenario when you come home,
* [Lose weight with the help of a complete log system](https://www.youtube.com/watch?v=noUROhtf0E0&t=18s), video from [@Andreas Spiess](https://www.youtube.com/channel/UCu7_D0o48KbfhpEohoP7YSQ)
* Detect opened door or windows through 433mhz or BLE and alert yourself when leaving
* Detect water leakage or smoke remotely
* Actionate a siren if something is going wrong
* Detect if your far mailbox has been opened by the postman with LORA
* Make smart your old TV or AC system through infrared control
* Monitor vehicle tire pressure

The limit is your imagination 😀

# Functions
Behind the scene you will find functionalities dedicated to gateways like:
* Deduplication
* Simple and lite API
* Strong integrations with libraries used
* Signal forward/repeat
* First configuration with web portal
* Whitelist & Blacklist management
* Secure connections
* Over the air updates

::: warning Note
The material and information contained in this documentation is for general information purposes only. You should not rely upon the material or information on this documentation as a basis for making any business, legal or any other decisions. There is no warranty given on this documentation content. If you decide to follow the information and materials given it is at your own risk. I will not be liable for any false, inaccurate, inappropriate or incomplete information presented on this website.
:::
