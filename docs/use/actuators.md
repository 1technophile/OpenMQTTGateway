# Actuators
## ON OFF
This module enables to actuate things by giving to a PIN a HIGH or LOW value corresponding to an MQTT topic.
Example usage: Connect a transistor to power a relay, connect a led...

So as to pilot the pin use the following commands:

OFF command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m OFF`

ON command
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m ON`

## FASTLED



