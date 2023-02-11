# Boards

## ESP

### Erasing the flash

So as to erase the flash memory on ESP boards you may do a long press to TRIGGER_GPIO button or connect the pin TRIGGER_GPIO to the ground during several seconds.

On M5Stack boards you may do a long press to these buttons in low power mode 0 (see below how to go to low power mode 0):
* Button B on M5StickC and M5StickC Plus (GPIO 37)
* Button C on M5Stack (GPIO 37)
* Button lateral on M5stick (GPIO 35)

You can also do a long press when powering the board to reset it, this press must be done during the first 5 seconds after the start.

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

## SSD1306 Display boards ( Heltec SX127X 433Mhz boards and LILYGO® LoRa32 V2.1_1.6.1 433 Mhz )

Several options are available for the display of information on the SSD1306 Display.  These options include display of the OMG logo and setup messages, redirecting of the log output to the display, and display of various module messages on the display.  These options are exclusive to each other, and when a different option is enabled, the current option is disabled.

### Setting the log output

The display of serial log messages to the display can be enabled via compiler directive `-DLOG_TO_LCD=true` or via MQTT commands.

For example if you want to set the serial log to LCD

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306 -m '{"log-lcd":true}'`

you can also revert it back to the serial monitor:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306 -m '{"log-lcd":false}'`

The log level of the messages displayed is Errors and Warnings, and this can only be changed via the compiler directive `-DLOG_LEVEL_LCD=LOG_LEVEL_NOTICE`.  

### Displaying Module json messages ( default )

The display of messages from various modules is also supported.  Currently supported modules include `ZgatewayRTL_433` and `ZsensorBME280`.

This can be enabled with the compiler directive `-DJSON_TO_LCD=true`.

You can also change it by MQTT. For example if you want to display module json messages:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306 -m '{"json-lcd":true}'`

And to disable the display of module json messages:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306 -m '{"display-json":false}'`

### Units for display, Metric or Imperial

By default the display uses metric units, and this can be changed either by compiler directive or mqtt command.

The compiler directive is `-DDISPLAY_METRIC=true`

The mqtt command to change the units is:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306 -m '{"display-metric":false}'`

Please note that it may take several seconds/display updates for the units to change.  This is due to the queueing of messages for display.

### Flip Display 180 degrees

This allows you to rotate the display 180 degrees.  Can be set at compile time or during use, defaults to true.

The compiler directive is `-DDISPLAY_FLIP=false`

The mqtt command to change display orientation is:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306 -m '{"display-flip":false}'`

Please note that it may take several seconds/display updates for the display to change.  This is due to the queueing of messages for display.

### Display OpenMQTTGateway Logo when Idle

When this option is enabled the OLED display will show the OpenMQTTGateway Logo when it is idle.  Defaults to false

The compiler directive is `-DDISPLAY_IDLE_LOGO=true`