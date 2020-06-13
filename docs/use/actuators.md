# Actuators
## ON OFF
This module enables to actuate things by giving to a PIN a HIGH or LOW value corresponding to an MQTT topic.
Example usage: Connect a transistor to power a relay, connect a led...

So as to pilot the GPIO use the following commands with [simple receiving](../upload/pio.md#api):

OFF command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF/setOFF -m 15`

ON command
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF/setON -m 15`

or with [json receiving](../upload/pio.md#api)

OFF command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m '{"gpio":15,"cmd":0}'`

ON command
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m '{"gpio":15,"cmd":1}'`

## FASTLED
### The FASTLED module support 2 different operation modes
1. control one specific RGB LED
* Set color
* Set blink

2. Start fire animation (Fire2012)

### Hardware wiring
Theoreticaly it should be possible to use every free IO pin. But after some tests only pin D2 works at WEMOS D1. Other platforms can work.
The default setting use NEOPIXEL (WS2812B). The simplest wiring is direct connect D2 to data pin of LED stripe and connect VCC/GND to power source. You should also add an capacitor.





