# System commands (ESP only)

## Restart the ESP
`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"cmd":"restart"}'`

::: tip
With Home Assistant, this command is directly avalaible through MQTT auto discovery as a switch into the HASS OpenMQTTGateway device entities list.
:::

## Erase the ESP settings

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"cmd":"erase"}'`

::: tip
With Home Assistant, this command is directly avalaible through MQTT auto discovery as a switch into the HASS OpenMQTTGateway device entities list.
:::

## Retrieve current status of the ESP

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"cmd":"status"}'`

## Auto discovery
You can deactivate the MQTT auto discovery function, this function enable to create automaticaly devices/entities with Home Assistant convention.
### Deactivate
`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"discovery":false}'`

### Activate
`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"discovery":true}'`

If you want the settings to be kept upon gateway restart, you can publish the command with the retain flag.

::: tip
Auto discovery is enable by default on release binaries, on platformio (except for UNO). With Arduino IDE please read the [advanced configuration section](../upload/advanced-configuration#auto-discovery) of the documentation.
:::

# State LED usage

The gateway can support up to 3 LED to display its operating state:
* LED_INFO 
switched ON when network and MQTT connection are OK
5s ON, 5s OFF when WIFI is disconnected
1s ON, 4s OFF when MQTT is disconnected

* LED_RECEIVE
Blink for `TimeLedON` 1s when the gateway receive a signal from one of its module so as to send to MQTT

* LED_SEND
Blink for `TimeLedON` 1s when the gateway send a signal with one of its module from an MQTT command

