OpenMQTTGateway project goal is to concentrate in one gateway different technologies, decreasing by the way the number of proprietary gateways needed, and hiding the different technologies singularity behind a simple & wide spread communication protocol; [MQTT](http://mqtt.org/).

![Overview](./img/OpenMQTTGateway.png)

OpenMQTTGateway support very mature technologies like basic 433mhz/315mhz protocols & infrared (IR) so as to make your old dumb devices "smart" and avoid you to throw then away. These devices have also the advantages of having a lower cost compared to Zwave or more sophisticated protocols.
OMG support also up to date technologies like Bluetooth Low Energy (BLE) or LORA.

To have an overview of the supported PIR, door, water, smoke sensors, sirens, rings, beacons, switchs & weather stations you can take a look to the 
[compatible devices list](https://compatible.openmqttgateway.com/index.php/devices)

With MQTT you can connect the compatible software you want, it can be an home automation controller (OpenHAB, Home Assistant, Jeedom, FHEM, Domoticz...) or another software like Node-Red.

# Use cases
With OpenMQTTGateway and a controller you can for example:
* Monitor your garden with a Mi Flora sensor and control an irrigation valve depending on the soil moisture,
* Trigger a fan depending on the temperature and humidity thanks to a Mi Jia sensor,
* Alert yourself by a controller notification if the temperature of your fridge is too high,
* Detect your beacon/smartwatch so as to trigger a special scenario when you come home,
* [Lose weight with the help of a complete log system](https://www.youtube.com/watch?v=noUROhtf0E0&t=18s), video from [@Andreas Spiess](https://www.youtube.com/channel/UCu7_D0o48KbfhpEohoP7YSQ)
* Detect opened door or windows and alert yourself when leaving home
* Detect water leakage or smoke
* Actionate a siren if something is going wrong
The limit is your imagination 😀

# Functions
Behind the scene you will find functionnalities dedicated to gateways like:
* Deduplication
* Simple and lite API
* Strong integrations with libraries used
* Signal forward/repeat
* First configuration with web portal
* Whitelist & Blacklist management

::: warning Note
The material and information contained in this documentation is for general information purposes only. You should not rely upon the material or information on this documentation as a basis for making any business, legal or any other decisions. There is no warranty given on this documentation content. If you decide to follow the informations and materials given it is at your own risk.I will not be liable for any false, inaccurate, inappropriate or incomplete information presented on this website.
:::
