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

## Change the WiFi credentials

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"wifi_ssid":"ssid", "wifi_pass":"password"}'`

::: tip
If the new connection fails the gateway will fallback to the previous connection.
:::

## Change the MQTT broker credentials
```
mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m
'{
  "mqtt_user": "user_name",
  "mqtt_pass": "password",
  "mqtt_server": "host",
  "mqtt_port": "port",
  "mqtt_secure": "false"
}'
```
::: tip
Server, port, and secure_flag are only required if changing connection to another broker.  
If the new connection fails the gateway will fallback to the previous connection.
:::


# Firmware update from MQTT (ESP only)

The gateway can be updated through an MQTT message by providing a JSON formatted message with a version number, OTA password (optional, see below), and URL to fetch the update from.  

To enable this functionality, `MQTT_HTTPS_FW_UPDATE` will need to be defined or the line that defines in in user_config.h will need to be uncommented.

::: tip
If using an unsecure MQTT broker it is **highly recommended** to disable the password checking by setting the macro `MQTT_HTTPS_FW_UPDATE_USE_PASSWORD` to 0 (default is 1 (enabled)), otherwise a clear text password may be sent over the network.
:::

### Example firmware update message:
```
mosquitto_pub -t "home/<gateway_name>/commands/firmware_update" -m
'{
  "version": "test",
  "password": "OTAPASSWORD",
  "url": "https://github.com/1technophile/OpenMQTTGateway/releases/download/v0.9.6/esp32-m5stack-ble-firmware.bin"
}'
```

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

