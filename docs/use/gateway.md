# System commands (ESP only)

## Restart the ESP

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"cmd":"restart"}'`

## Erase the ESP settings

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"cmd":"erase"}'`

# State LED usage

The gateway can support up to 3 LED to display its operating state:
* LED_INFO 
switched ON when network and MQTT connection are OK
5s ON, 5s OFF when WIFI is disconnected
1s ON, 4s OFF when MQTT is disconnected

* LED_RECEIVE
Blink for `TimeLedON` 0.5s when the gateway receive a signal from one of its module so as to send to MQTT

* LED_SEND
Blink for `TimeLedON` 0.5s when the gateway send a signal with one of its module from an MQTT command

