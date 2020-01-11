# Upload with PlatformIO
This section is usefull if you want to do advanced configuration of your project or if you choose an Arduino. Indeed the ESP family can be loaded directly without any configuration from your desktop. 
Advanced configurations means changing the default used pins, the MQTT subjects and all the expert parameters that you can find in [user_config.h](https://github.com/1technophile/OpenMQTTGateway/blob/development/main/User_config.h) and in all [config_XX.h](https://github.com/1technophile/OpenMQTTGateway/tree/development/main).
If you don't have to change the default parameters except wifi and broker setting you can go directly to the [Load](load) section.

Download the [CODE](https://github.com/1technophile/OpenMQTTGateway/releases) from github.

So as to configure OpenMQTTGateway you have the choice between 2 development environments:
* [Arduino IDE](https://www.arduino.cc/en/Main/Software)
* [PlatformIO](https://platformio.org/)

I advise to use PlatformIO, this way you will not have to search for all the necessary libraries and adequate forks/revisions. If you really want to use Arduino you need to download the libraries listed [here](https://github.com/1technophile/OpenMQTTGateway/blob/d2dd6138558909b71cc44f69665340247bd5f356/platformio.ini#L55) at the version or revision specified.

With PlatformIO 
* Open the folder named "OpenMQTTGateway"
* Open the platformio.ini file and uncomment the default_envs line corresponding to your board and choosen module like below.

``` ini
;default_envs = sonoff-basic-rfr3
;default_envs = rfbridge
;default_envs = esp32dev-all
default_envs = esp32dev-rf
;default_envs = esp32dev-ir
;default_envs = esp32dev-ble
;default_envs = ttgo-lora32-v1
```

If you don't know which env to activate you can refer to [devices](../prerequisites/devices).

The different listed configurations represents some standard environments, to overload them with special parameters or modules you can modify the config files. The definitions coming from [platformio.ini](https://github.com/1technophile/OpenMQTTGateway/blob/development/platformio.ini) file and config files are cumulative.
The main config file is [user_config.h](https://github.com/1technophile/OpenMQTTGateway/blob/development/main/User_config.h), added to it you have one config file per gateway, sensor or actuator, you will find them in the [main](https://github.com/1technophile/OpenMQTTGateway/tree/development/main) folder.

If you want to add more sensors or gateways to one `default_envs` you can uncomment them into [user_config.h](https://github.com/1technophile/OpenMQTTGateway/blob/d2dd6138558909b71cc44f69665340247bd5f356/main/User_config.h#L84) or add the modules directly into your environment definition of your .ini files.

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

You can define your environment into an additionnal `production_env.ini` file than `platformio.ini`, when building PIO will scan for all the 
``` ini
*_env.ini
```
This way when updating the code your environments definition will not be overwritten.

Once your configuration is done you can upload the program to your board by clicking on the white arrow at the blue bottom bar of your PIO editor or with the following command:
`pio run --target upload`

PIO will download the necessaries platform and libraries with the correct versions, build the code and upload it.

If you encounter errors the first thing to do is to clean your environment by using the white dust bin in the blue bottom bar.

With some ESP it could be necessary to push the reset button when the upload begin.

If you want to erase the settings stored in the ESP memory use:
`pio run --target erase`

Once done the gateway should connect to your network and your broker, you should see it into the broker in the form of the following messages:
```
home/OpenMQTTGateway/LWT Online 
home/OpenMQTTGateway/version
```

# API
With the V0.9 we added the support of json for receiving and publishing.
Per default Json reception and Json publication is activated, the previous simple reception mode is also activated to avoid regression on commands.

You can deactivate Json or simple mode following theses instructions:
```C++
#define jsonPublishing true //comment if you don't want to use Json  publishing  (one topic for all the parameters)
//example home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4 {"rssi":-63,"servicedata":"fe0000000000000000000000000000000000000000"}
//#define simplePublishing true //comment if you don't want to use simple publishing (one topic for one parameter)
//example 
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/rssi -63.0
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/servicedata fe0000000000000000000000000000000000000000
#define simpleReceiving true //comment if you don't want to use old way reception analysis
#define jsonReceiving true //comment if you don't want to use Json  reception analysis
```

If you are using platformio you can also comment the definitions above and define your parameters into platformio.ini file by setting the following `build_flags`:
```C++
  '-DjsonPublishing=true'
  '-DjsonReceiving=true'
  '-DsimpleReceiving=true'
  '-DsimplePublishing=true'
```

Note that depending on the environment the default platformio.ini has common option defined see sections:
```
[com-arduino]
[com-esp]
```

If you want to use HASS MQTT discovery you need to have 
`#define jsonPublishing true`
&
`#define ZmqttDiscovery "HADiscovery"`
uncommented.
Added to that auto discovery box should be selected into your Home Assistant MQTT integration configuration.

With an ESP if you did not set your network and mqtt parameters manualy you can now open the [web portal configuration](portal.md).
