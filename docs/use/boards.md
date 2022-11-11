# Boards

## ESP

### Erasing the flash

So as to erase the flash memory on ESP boards you may do a long press to TRIGGER_GPIO button or connect the pin TRIGGER_GPIO to the ground during several seconds.

On M5Stack boards you may do a long press to these buttons in low power mode 0 (see below how to go to low power mode 0):
* Button B on M5StickC and M5StickC Plus (GPIO 37)
* Button C on M5Stack (GPIO 37)
* Button lateral on M5stick (GPIO 35)

### Low power mode for ESP32
OpenMQTTGateway support a low power mode for ESP32, this mode can be set by MQTT:

* Normal mode (per default)

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoBT/config" -m '{"lowpowermode":0}'`

* Low Power mode

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoBT/config" -m '{"lowpowermode":2}'`

::: tip
When coming back from mode 2 to mode 0 you may publish the command with a retain flag so as to enable the gateway to retrieve it when reconnecting.
A low power mode switch is automatically created by discovery with Home Assistant, you may experience a delay between the command and the state update due to the fact that the update is published every 2 minutes.
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

Per default the log of the M5 boards is going to the LCD display with Errors and Warnings only, if you want to change the ouput at build time you can do it in [config_M5.h](https://github.com/1technophile/OpenMQTTGateway/blob/development/main/config_M5.h).

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
