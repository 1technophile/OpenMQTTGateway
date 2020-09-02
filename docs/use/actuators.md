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

## PWM
This module allows control over PWM outputs.
It's primary use is for controlling LEDs, but it should be equally at home controlling anything that's controlled using PWM.
E.g. LEDs, servos, PC fans.
You would typically connect a PWM output to a transistor or MOSFET to allow control over higher power devices.

* JSON message format allows you to set any or all channels in a single message.
* Each channel can be set to smoothly transition from its current setting to the new setting over a specified number of seconds.
* Each channel can be calibrated with min and max settings, as well as a gamma curve.

### Configuration
In order to use the PWM actuator, you need to configure which pins the PWM output channels will be connected to.
There are a couple of `#define`s that achieve this.
They can be defined in the `build_flags` section of the env, or by directly editing `config_PWM.h`.

```c
#define PWM_CHANNEL_NAMES {"r", "g", "b", "w0", "w1"}
#define PWM_CHANNEL_PINS  { 25,  33,  32,   23,   22}
```

`PWM_CHANNEL_NAMES` lists the names that you would like to assign to each channel, and determines the number of channels.
`PWM_CHANNEL_PINS` lists the corresponding output pins that the channels will be connected to.
The number of entries in `PWM_CHANNEL_PINS` must exactly match the number of entries in `PWM_CHANNEL_NAMES`.

### Usage

#### Set
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoPWM/set -m '{"r":0.5,"g":0.2,"b":1,"fade":10.0}'`

This example sets new values for the channels named `r`, `g`, and `b`.
These channels will transition from their current values to the new values over 10s.

#### Calibrate
Calibration allows that min and max levels to be configured for each channel, so that the full 0-1 range of values
that can be specified with the `set` command actually do things.

`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoPWM/calibrate -m '{"min-r":0.01,"max-r":1.0,"gamma-r":2.5}'`

This example calibrates the channel named `r`.
After this calibration, if you set the `r` channel to 0.0, it will be remapped to 0.01 internally.
Also, the gamma curve for this channel will be set to 2.5.
This means that input values are raised to the power 2.5 internally.
This can be used to improve the linearity of inputs.
