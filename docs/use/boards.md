# Boards

## ESP

### Erasing the flash

So as to erase the flash memory on ESP boards you may do a long press to TRIGGER_GPIO button or connect the pin TRIGGER_GPIO to the ground during several seconds.

On M5Stack boards you may do a long press to these buttons in low power mode 0 (see below how to go to low power mode 0):
* Button B on M5StickC and M5StickC Plus (GPIO 37)
* Button C on M5Stack (GPIO 37)
* Button lateral on M5stick (GPIO 35)

You can also do a long press when powering the board to reset it, this press must be done during the first 5 seconds after the start.

### Wifi interference on ESP32 ###
Certain sensors like HC-SR501 is prone to generate false signals / triggers when used on a ESP32 with Wifi enabled. To reduce or elimate the effect the board must be put into Wifi B/G with lower TX power.

This can be achieved with the following macro, `WifiGMode` defined true and `WifiPower` to e.g. WIFI_POWER_11dBm.  

Since the WiFi protocol is persisted in the flash of the ESP32 you have to run at least once with `WiFiGMode` defined **false** to get Band N back.

### Low power mode for ESP32
OpenMQTTGateway support a low power mode for ESP32, this mode is available per default on boards with batteries. The other boards needs to have the macro `DEFAULT_LOW_POWER_MODE` defined at 0, 1 or 2 to use it. More information about the modes is available into User_config.h.

When available this mode can be set by MQTT:

* Normal mode (per default)

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoBT/config" -m '{"lowpowermode":0}'`

* Low Power mode

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoBT/config" -m '{"lowpowermode":2}'`

The interval between the ESP32 wake up is defined at build time by the macro `TimeBtwRead`, a change of the `interval` through MQTT will not impact the time between wake up.

::: tip
When coming back from mode 2 to mode 0 you may publish the command with a retain flag so as to enable the gateway to retrieve it when reconnecting.
A low power mode switch is automatically created by discovery with Home Assistant, you may experience a delay between the command and the state update due to the fact that the update will be revceived and acknowledged when the device woke up.
In low power mode you should use ESPWifiManualSetup so as to rely on the credentials entered into User_config.h.
So as to do that uncomment the line below in User_config.h
``` c
//#  define ESPWifiManualSetup true
```
For platformio and example of environment is available into [prod_env.ini.example](https://github.com/1technophile/OpenMQTTGateway/blob/development/prod_env.ini.example)
:::

::: warning
If you change the default low power mode in config_BT.h to 2 and your credential are not set or not correct, the ESP32 will not connect to the broker and the only way to change the low power mode will be a new erase/upload.
:::

## M5StickC, M5StickC Plus or M5Stack

### Behaviour

If the connection of the board to WIFI and MQTT is successful you will see the logo with text like below:

![boards](../img/OpenMQTTgateway_M5_Stack_Board_Display_Text.png)

The same behaviour apply to M5StickC and M5StickC Plus

![boards](../img/OpenMQTTgateway_M5_StickC_Board_Display_Text.png)

### Setting the log output

Per default the log of the M5 boards is going to the LCD display with Errors and Warnings only, if you want to change the output at build time you can do it in [config_M5.h](https://github.com/1technophile/OpenMQTTGateway/blob/development/main/config_M5.h).

You can also change it by MQTT. For example if you want to set to LCD

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoM5/config -m '{"log-display":true}'`

you can also revert it to the serial monitor:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoM5/config -m '{"log-display":false}'`

### Low power mode for M5 boards
OpenMQTTGateway support a low power mode for ESP32, this mode can be set by MQTT or a button on M5 boards:

* Normal mode (per default), screen ON

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoBT/config" -m '{"lowpowermode":0}'`

* Low Power mode, screen ON when processing only

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoBT/config" -m '{"lowpowermode":1}'`

* Low Power mode, screen OFF, LED ON when processing on M5StickC or M5stickC Plus

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoBT/config" -m '{"lowpowermode":2}'`

The low power mode can be changed also with a push to button B when the board is processing (top button on M5stickC, M5stickC Plus and middle button of M5stack).
If you are already in low power mode 1 or 2 with M5Stack you can wake up the board by pressing the red button.

### Low power mode (deepSleep) for ESP8266 & ESP32 boards
In certain use cases where power is severly limited you can use the ESP8266 or ESP32 deep sleep capability.

* e.g. measuring a pool temperature every 5 minutes using an ESP8266 and DS18B20 probe where the ESP8266 is powered by very limited battery backed solar power.
* e.g. as a water/leak detector which wake-up based on sensor state an ESP32 and C-37 YL-83 HM-RD sensor where the ESP32 is powered by very limited battery power.

During deep sleep everything is off and (almost) all execution state is lost. 

Consumption is about 20 µA on an ESP8266.

Use this when you want the device to sleep for minutes,  hours woken by external sensor state.

You only have to define the macro `DEEP_SLEEP_IN_US` with the number of microseconds, this works for both ESP8266 and ESP32.

For an ESP8266 a hardware jumper is required connecting RST to a GPIO (not to CH_PD) defined by the macro `ESP8266_DEEP_SLEEP_WAKE_PIN` and defaulted to D0.

On an ESP32 we can also use an external sensor state to wake-up the ESP and this is defined by macro `ESP32_EXT0_WAKE_PIN` and which state it must toggle to by macro `ESP32_EXT0_WAKE_PIN_STATE` defaulted to 1 (high).

And the sensor code must set variable `ready_to_sleep` to true after publishing the measurement to MQTT and the main loop will then enter deep sleep.