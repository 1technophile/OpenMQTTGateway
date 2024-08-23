# Boards

## ESP

### Erasing the flash

So as to erase the flash memory on ESP boards you may do a long press to TRIGGER_GPIO button or connect the pin TRIGGER_GPIO to the ground during several seconds.

On M5Stack boards you may do a long press to these buttons in low power mode 0 (see below how to go to low power mode 0):
* Button B on M5StickC and M5StickC Plus (GPIO 37)
* Button C on M5Stack (GPIO 37)
* Button lateral on M5stick (GPIO 35)

You can also do a long press when powering the board to reset it, this press must be done during the first 5 seconds after the start.

### Wifi interference on sensors when using an ESP ###
Certain sensors like HC-SR501 is prone to generate false signals / triggers when used on a ESP with Wifi enabled. To reduce or eliminate the effect the board must be put into Wifi B/G with lower TX power.

This can be achieved with the following macro, `WifiGMode` defined true and `WifiPower` to e.g. WIFI_POWER_11dBm (ESP32) or 11 (ESP8266).  

Since the WiFi protocol is persisted in the flash of the ESP you have to run at least once with `WiFiGMode` defined **false** to get Band N back.

### Low power mode for ESP32
OpenMQTTGateway support a low power mode for ESP32, the boards needs to have the macro `DEFAULT_LOW_POWER_MODE` defined at `ALWAYS_ON`, `INTERVAL` or `ACTION` to use it. More information about the modes is available into User_config.h.

When available this mode can be set by MQTT:

* Normal mode (per default)

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"powermode":0, "save":true}'`

* Low Power mode wake up from interval defined by DEEP_SLEEP_IN_US and pins defined by ESP32_EXT0_WAKE_PIN and/or ESP32_EXT1_WAKE_PIN

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"powermode":1, "save":true}'`

* Low Power mode wake up from pins defined by ESP32_EXT0_WAKE_PIN and/or ESP32_EXT1_WAKE_PIN

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"powermode":2, "save":true}'`

::: tip
A low power mode switch is automatically created by discovery with Home Assistant, you may experience a delay between the command and the state update due to the fact that the update will be received and acknowledged when the device woke up.
If you are publishing the state change while the device is asleep use the retain flag. This way the device will retrieve the powermode command at wake up.
:::

The default sleep time is defined by the macro `DEEP_SLEEP_IN_US` (default 60s)

We can also use an external sensor state to wake-up the ESP, this is defined by macro `ESP32_EXT0_WAKE_PIN/ESP32_EXT1_WAKE_PIN`, the level is defined by the macro `ESP32_EXT0_WAKE_PIN_STATE/ESP32_EXT0_WAKE_PIN_STATE` (default to HIGH).

::: warning
If you change the default low power mode to INTERVAL or ACTION and your credential are not set or not correct, the ESP32 will not connect to the broker and the only way to change the low power mode will be a new erase/upload.
:::

## M5StickC, M5StickC Plus or M5Stack

### Behaviour

If the connection of the board to WiFi and MQTT is successful you will see the logo with text like below:

![boards](../img/OpenMQTTgateway_M5_Stack_Board_Display_Text.png)

The same behaviour apply to M5StickC and M5StickC Plus

![boards](../img/OpenMQTTgateway_M5_StickC_Board_Display_Text.png)

### Setting the log output

Per default the log of the M5 boards is going to the LCD display with Errors and Warnings only, if you want to change the output at build time you can do it in [config_M5.h](https://github.com/1technophile/OpenMQTTGateway/blob/development/main/config_M5.h).

You can also change it by MQTT. For example if you want to set to LCD

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoM5/config -m '{"log-display":true}'`

you can also revert it to the serial monitor:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoM5/config -m '{"log-display":false}'`
