# Actuators
## ON OFF
This module enables to actuate things by giving to a PIN a HIGH or LOW value corresponding to an MQTT topic.
Example usage: Connect a transistor to power a relay, connect a led...

So as to pilot the pin use the following commands with [simple receiving](../upload/pio.md#api):

OFF command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF/setOFF -m 15`

ON command
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF/setON -m 15`

or with [json receiving](../upload/pio.md#api)

OFF command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m '{"pin":15,"state":0}'`

ON command
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m '{"pin":15,"state":1}'`

## FASTLED



