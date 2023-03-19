# (Option 3) Upload your configurations

## Introduction

This section is useful if you want to do advanced configuration of your project or if you choose an Arduino. Indeed the ESP hardware family can be loaded directly without any configuration from your desktop.

Advanced configuration means changing the default pins, the MQTT topics, and all the expert parameters that you can find in [`User_config.h`](https://github.com/1technophile/OpenMQTTGateway/blob/development/main/User_config.h) and in all [`config_XX.h`](https://github.com/1technophile/OpenMQTTGateway/tree/development/main) files. If you don't have to change the default parameters except Wi-Fi and MQTT broker settings, you don't need advanced configuration; you can go directly to the [Upload Binaries](binaries.md) section instead.

To make advanced configurations to OpenMQTTGateway, you have the choice between two development environments:
* [PlatformIO](https://platformio.org/)
* [Arduino IDE](https://www.arduino.cc/en/Main/Software)

I recommend using PlatformIO; this way you will not have to search for all the necessary libraries and adequate forks/revisions. If you really want to use Arduino IDE, you need to download the libraries listed [here](https://github.com/1technophile/OpenMQTTGateway/blob/d2dd6138558909b71cc44f69665340247bd5f356/platformio.ini#L55) at the version or revision specified.

## Configure & Upload with PlatformIO

* Download the [CODE](https://github.com/1technophile/OpenMQTTGateway/releases) from github.
* Open the `OpenMQTTGateway` folder

You will find inside the folder a `platformio.ini` config file. PlatformIO uses this file to define how to build OMG for different kinds of hardware. Not just that, but it also specifies which modules to turn on and off. And there's more: it also lets you configure the settings of those modules.

PlatformIO config files work on the concept of overriding. At first, a very simple base "environment" is specified that specifies common variables shared by all situations:
```ini
[env]
framework = arduino
lib_deps =
  ${libraries.pubsubclient}
  ${libraries.arduinojson}
  ${libraries.arduinolog}
build_flags =
  -w ; supress all warnings
;  '-DLOG_LEVEL=LOG_LEVEL_TRACE'  ; Enable trace level logging
monitor_speed = 115200
```
Later "environments" get more specific, but inherit everything that was defined in this common environment:
```ini
[com-esp]
lib_deps =
  ${env.lib_deps}           ; Inherit all the library dependencies from [env]
  ${libraries.wifimanager}  ; Add another library dependency on top of them
build_flags =
  ${env.build_flags}        ; Inherit all the build flags from [env]
  '-DsimpleReceiving=true'  ; Add some of our own build flags
  '-DZmqttDiscovery="HADiscovery"'
  '-DTRACE=1'
  ;'-DCORE_DEBUG_LEVEL=4'
```
Here, build flags starting with "-D" let us set configuration values you would normally find in `User_config.h` and `config_xx.h` files by specifying them here, overriding the default values set in those files. To include special characters, you can triple escape them with a backslash like so:
```ini
  '-Dwifi_password="Cato\\\'sYounger\\\$on"' ; Cato'sYounger$on
```

The different listed configurations in `platformio.ini` represent some standard environments and boards. For example, the environment
```ini
[env:nodemcuv2-pilight]
```
sets the default settings for ESP8266 (NodeMCU v2) devices using the Plight module.

### *(Option A)* Creating a portable config file
You could make your configuration changes directly by editing the values in `User_config.h` (for main OMG settings) and `config_XX.h` (for module-specific settings). You could also make most of those changes by instead writing some -D build flags in `platformio.ini`. But for maximum portability, a feature of PlatformIO allows you to make your configurations by creating a new file and listing all of your overrides there. This way, you can pull the latest OMG code changes without losing your configurations, or having to re-enter them manually.

To do this, create a file with a name ending in `_env.ini`, such as `production_env.ini`, into the root folder next to `platformio.ini`. PlatformIO will scan for all files ending in `_env.ini` and use it to override the default values in `platformio.ini`.

At the top of your `*_env.ini` file, for example, put the following to enable the ESP8266 (NodeMCU v2) with the Pilight module.
```ini
[platformio]
default_envs = nodemcuv2-pilight
```
This will make this environment the default environment for this PlatformIO project. If another environment isn't specified when building, it will default to this one.

For the rest of your config file, you can override the default configurations or add new configurations to existing environments in `platformio.ini`, or create a new environment. For example, if want to use both Pilight module and the BME280 module with an ESP8266, we can create a new environment. This is an example `my_env.ini` file that creates two new environments:

```ini
[platformio]
default_envs = nodemcuv2-pilight-bme280-ota

[env:nodemcuv2-pilight-bme280]
extends = env:nodemcuv2-pilight
lib_deps =
  ${env:nodemcuv2-pilight.lib_deps}
  ${libraries.bme280}
build_flags =
  ${env:nodemcuv2-pilight.build_flags}
  '-DGateway_Name="OpenMQTTGateway"'
  '-DZsensorBME280="BME280"'
  '-DBase_Topic="rf/"'
  '-DESPWifiManualSetup=true'
  '-Dwifi_ssid="mynetwork"'
  '-Dwifi_password="Cato\\\'sYounger\\\$on"' ; Cato'sYounger$on
  '-DMQTT_USER="mqttusername"'
  '-DMQTT_PASS="mqttpassword"'
  '-DMQTT_SERVER="mqttserver.local"'
  '-Dota_password="otapassword"'
  '-DLED_RECEIVE=LED_BUILTIN'        ; Comment 1
  '-DLED_RECEIVE_ON=LOW'             ; Comment 2
  '-DRF_RECEIVER_GPIO=13'
  '-DRF_EMITTER_GPIO=15'
  '-DsimpleReceiving=false'          
  '-UZmqttDiscovery'                 ; Disable HA discovery
monitor_speed = 115200

[env:nodemcuv2-pilight-bme280-ota]
extends = env:nodemcuv2-pilight-bme280
upload_protocol = espota
upload_port = OpenMQTTGateway.local
upload_flags =
  --auth=otapassword
  --port=8266
```

::: warning Note
Adding manual WiFi and MQTT credentials to an environment also requires to define
`'-DESPWifiManualSetup=true'`
for the credetials to be registered correctly.
:::

The first new environment we create, `nodemcuv2-pilight-bme280`, inherits the default `nodemcuv2-pilight` environment in `platformio.ini` with the `extends = env:nodemcuv2-pilight` line. In the `lib_deps` section, it imports all the `lib_deps` of `nodemcuv2-pilight` with the `${env:nodemcuv2-pilight.lib_deps}` line, and adds BME280 on top of it. (Since the environment we're extending already has this `lib_deps` attribute, specifying it again would normally replace it completely with our new attribute; instead, to keep its value but simply append to it, we import the original in the beginning of our `lib_deps` attribute.) In the `build_flags` section, it again imports all the `build_flags` of `nodemcuv2-pilight` and many of its own overrides, e.g. changing `Base_Topic` found in `User_config.h` from the default to "rf/" by using the `'-DBase_Topic="rf/"'` line. It also unsets previously set configurations (i.e. `mqttDiscovery`) by using `'-UZmqttDiscovery'`. This environment will work over serial upload.

The second new environment, `nodemcuv2-pilight-bme280-ota`, inherits everything we specified in the first environment (with the line `extends = env:nodemcuv2-pilight-bme280`), but modifies it to upload over OTA (Wi-Fi). We also specified this as the `default_env` in the beginning of the file, so PlatformIO will choose this environment to build and upload if we don't specify otherwise.

The first time we're flashing OMG to the board, we can use the command `pio run --target upload --environment nodemcuv2-pilight-bme280` to specify that we want to build and run the `nodemcuv2-pilight-bme280` environment (over USB serial). Afterwards, we don't have to specify `--environment` (e.g. just run `pio run --target upload`) to run the default `nodemcuv2-pilight-bme280-ota` environment and update the code over the air.

### *(Option B)* Editing files directly

You can also modify the `User_config.h` file and your modules' `config_XX.h` files to your liking, and then edit the `platformio.ini` file to uncomment the `default_envs` lines corresponding to your board and chosen modules, like below:

``` ini
;default_envs = sonoff-basic-rfr3
;default_envs = rfbridge
;default_envs = esp32dev-all
default_envs = esp32dev-rf
;default_envs = esp32dev-ir
;default_envs = esp32dev-ble
;default_envs = ttgo-lora32-v1
```

If you don't know which `env` to activate, you can refer to [devices](../prerequisites/devices).

If you want to add more sensors or gateways to one `default_envs` you can add the modules directly into your environment definition of your `.ini` files or uncomment them in [`User_config.h`](https://github.com/1technophile/OpenMQTTGateway/blob/d2dd6138558909b71cc44f69665340247bd5f356/main/User_config.h#L84).

Example to add IR to `esp32dev-rf` add the `build_flags` below to the env definition:
``` ini
  '-DZgatewayIR="IR"'
```

``` ini
[env:esp32dev-rf]
platform = ${com.esp32_platform}
board = esp32dev
lib_deps =
  ${com-esp.lib_deps}
  ${libraries.rc-switch}
build_flags = 
  ${com-esp.build_flags}
  '-DZgatewayRF="RF"'
  '-DZgatewayIR="IR"'
  '-DGateway_Name="OpenMQTTGateway_ESP32_RF_IR"'
```

Once your configuration is done you can upload the program to your board by clicking on the white arrow at the blue bottom bar of your PIO editor or with the following command:
`pio run --target upload`

PIO will download the necessaries platform and libraries with the correct versions, build the code and upload it.

If you encounter errors the first thing to do is to clean your environment by using the white dust bin in the blue bottom bar of your PIO editor or with the following command:
`pio run --target clean`

With some ESP it could be necessary to push the reset button when the upload begin.

If you want to erase the settings stored in the ESP memory use:
`pio run --target erase`
This can be useful especially before the first upload or when you change the board partitions sizing.

Once done the gateway should connect to your network and your broker, you should see it into the broker in the form of the following messages:
```
home/OpenMQTTGateway/LWT Online 
home/OpenMQTTGateway/version
```

With PIO you can also upload the firmware through Over the Air, so as to do that you can add the upload options flags used below, `upload_port` is the IP address of your ESP:

``` ini
[env:esp32-ble]
platform = ${com.esp32_platform}
board = esp32dev
board_build.partitions = min_spiffs.csv
lib_deps =
  ${com-esp.lib_deps}
  ${libraries.ble}
build_flags =
  ${com-esp.build_flags}
  '-DZgatewayBT="BT"'
  '-DGateway_Name="OpenMQTTGateway_ESP32"'
upload_protocol = espota
upload_port = 192.168.1.22
upload_flags =
  --auth=OTAPASSWORD
  --port=8266
```

## Configure & Upload with Arduino IDE

* Download the [CODE](https://github.com/1technophile/OpenMQTTGateway/releases) from github
* First download the last version of the Arduino IDE from the Arduino [website](https://www.arduino.cc/en/Main/Software)
* Add ESP32 boards by following this [tutorial](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-arduino-ide)
* Add ESP8266 boards by following this [tutorial](https://github.com/esp8266/Arduino#installing-with-boards-manager)
* Download the libraries package corresponding to your board and module wished into the same page (example esp32-m5stick-c-ble-libraries.zip)
* Unzip the libraries into your Arduino libraries folder (example D:/Users/XXXX/Documents/Arduino/libraries)
* If necessary replace the spaces into each library folder by _: example rename “ESP32 BLE Arduino” folder to “ESP32_BLE_Arduino”
* Open the file main.ino from OpenMQTTGateway/main folder with the Arduino IDE
* Change the settings and the desired gateways into `User_config.h` (uncomment the modules you want)

*Example for the use of RF gateway:*

```cpp
#define ZgatewayRF "RF" //ESP8266, Arduino, ESP32
//#define ZgatewayIR "IR" //ESP8266, Arduino, Sonoff RF Bridge
//#define ZgatewayLORA "LORA" //ESP8266, Arduino, ESP32
//#define ZgatewayPilight "Pilight" //ESP8266, Arduino, ESP32
//#define ZgatewayBT "BT" //ESP8266, ESP32
```

* Change the pins or parameters corresponding to the modules chosen, for RF you can change the pins in config_RF.h
* Choose the board on the Arduino IDE
* Select the port corresponding to the board
* Note that for using BLE on ESP32 you will need to select *Minimal SPIFFS* into Tools->Partition Scheme
* Open the serial monitor and set 115200 bauds
* Upload ➡️
* You should see the logs in the serial monitor

With an ESP if you did not set your network and MQTT parameters manually you can now open the [web portal configuration](portal.md).

## API
With the V0.9 we added the support of json for receiving and publishing.
Per default Json reception and Json publication is activated, the previous simple reception mode is also activated to avoid regression on commands.

You can deactivate Json or simple mode following theses instructions:
```cpp
#define jsonPublishing true //define false if you don't want to use Json publishing (one topic for all the parameters)
//example home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4 {"rssi":-63,"servicedata":"fe0000000000000000000000000000000000000000"}
#define simplePublishing false //define true if you want to use simple publishing (one topic for one parameter)
//example 
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/rssi -63.0
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/servicedata fe0000000000000000000000000000000000000000
#define simpleReceiving true //define false if you don't want to use old way reception analysis
#define jsonReceiving true //define false if you don't want to use Json  reception analysis
```

If you are using platformio you can also comment the definitions above and define your parameters into platformio.ini file by setting the following `build_flags`:
```cpp
  '-DjsonPublishing=true'
  '-DjsonReceiving=true'
  '-DsimpleReceiving=true'
  '-DsimplePublishing=true'
```

Note that depending on the environment the default platformio.ini has common option defined see sections:
``` ini
[com-arduino]
[com-esp]
```

If you want to use HASS MQTT discovery you need to have 
`#define jsonPublishing true`
&
`#define ZmqttDiscovery "HADiscovery"`
uncommented.
Added to that auto discovery box should be selected into your Home Assistant MQTT integration configuration.

With an ESP if you did not set your network and MQTT parameters manually you can now open the [web portal configuration](portal.md).

::: warning Note
simpleReceiving on Arduino boards doesn't accept 64 bits MQTT values, you can only send 32bits values from the MQTT broker.
:::

[![Hits](https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fdocs.openmqttgateway.com%2Fupload%2Fbuilds.html&count_bg=%2379C83D&title_bg=%23555555&icon=&icon_color=%23E7E7E7&title=hits&edge_flat=false)](https://hits.seeyoufarm.com)
