# System commands (ESP only)

## Restart the ESP

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"cmd":"restart"}'`

## Erase the ESP settings

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSYS/config" -m '{"cmd":"erase"}'`
