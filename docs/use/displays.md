# Displays

## SSD1306 Display (Heltec SX127X and LILYGO® LoRa32 boards)
Several options are available for the display of information on the SSD1306 display. Some options are exclusive to each other, and when a different option is enabled, the current option is disabled.

The current SSD1306 display states are being published to the `SSD1306toMQTT` topic, e.g.

`{"onstate":true,"brightness":50,"displaymetric":true,"display-flip":true,"idlelogo":true,"log-oled":false,"json-oled":true}`

### Display ON/OFF
To turn the SSD1306 display on or off.

This can be enabled with the compiler directive `-DDISPLAY_STATE=true`.

MQTT Display OFF command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m {"onstate":false}`

MQTT Display ON command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m {"onstate":true}`

### Brightness
The display brightness can be set between 0-100%.

It is recommended to set a value lower than 100 to extend the life of the OLED display. The default setting is 50.

This can be set with the compiler directive `-DDISPLAY_BRIGHTNESS=50`.

or with the runtime command

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m {"brightness":50}`

### Metric or Imperial property units
To have applicable device properties displayed in Imperial units, e.g. °F for temperature.

This can be set with the compiler directive `-DDISPLAY_METRIC=false`.

or with the runtime command

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m {"displaymetric":false}`

### Rotating the display by 180 degrees

This can be set with the compiler directive `-DDISPLAY_FLIP=false`.

or with the runtime command

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m {"display-flip":false}`

### Display idle Logo
To display the OpenMQTTGateway logo during device display idle time. This reduces the likelihood of burn-in.

This can be set with the compiler directive `-DDISPLAY_IDLE_LOGO=true`.

or at runtime with

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m {"idlelogo":true}`

### Setting the log output

The display of serial log messages to the display can be enabled via compiler directive `-DLOG_TO_OLED=true` or via MQTT commands.

For example if you want to set the serial log to OLED

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m '{"log-oled":true}'`

you can also revert it back with

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m '{"log-oled":false}'`

The log level of the messages displayed is Errors and Warnings, and this can only be changed via the compiler directive `-DLOG_LEVEL_OLED=LOG_LEVEL_NOTICE`.  

### Displaying Module json messages (default)

The display of messages from various modules is also supported. Currently supported modules include `ZgatewayRTL_433`, `ZgatewayBT` and `ZsensorBME280`.

This can be enabled with the compiler directive `-DJSON_TO_OLED=true`.

You can also change it by MQTT. For example if you want to display module json messages:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m '{"json-oled":true}'`

And to disable the display of module json messages:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m '{"display-json":false}'`

### Store the current display configuration in the gateway

To store the running display configuration into non-volatile storage on the gateway use the following command. This assures persistence across restarts.

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m '{"save":true}'`

At any time, you can reload the stored configuration with the command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m '{"load":true}'`

If you want to erase the stored configuration, use the command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m '{"erase":true}'` 

Note that this will not change the running configuration, it only ensures that the default configuration is used at next startup.

If you want to load the default configuration use the command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSSD1306/config -m '{"init":true}'` 

Note that this will not change the stored configuration, `erase` or `save` is still needed to overwrite the saved configuration.
