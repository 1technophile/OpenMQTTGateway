# Actuators
## ON OFF
This module enables to actuate things by giving to a PIN a HIGH or LOW value corresponding to an MQTT topic.
Example usage: Connect a transistor to power a relay, connect a led...

So as to pilot the GPIO use the following commands with [simple receiving](../upload/pio.md#api):

OFF command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF/setOFF -m 15`

ON command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF/setON -m 15`

or with [json receiving](../upload/pio.md#api)

OFF command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m '{"gpio":15,"cmd":0}'`

ON command:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m '{"gpio":15,"cmd":1}'`

Since v0.9.9, you could ask for a short activation, the PIN will change state for only half a second.

For example you could use it with a relay board to activate an existing step relay. So your home automation act as a supplementary switch and do not interfere with the existing switch in your house.

It's available only with [json receiving](../upload/pio.md#api):

Goes ON for half a second, then back to OFF:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m '{"gpio":15,"cmd":"high_pulse"}'`

Goes OFF for half a second, then back to ON:
`mosquitto_pub -t home/OpenMQTTGateway_MEGA/commands/MQTTtoONOFF -m '{"gpio":15,"cmd":"low_pulse}"'`

Be aware that outputs are OFF by default when the board first start.

## FASTLED
### The FASTLED module support 2 different operation modes
1. control one specific RGB LED
* Set color
* Set blink

2. Start fire animation (Fire2012)

### Hardware wiring
Theoretically it should be possible to use every free IO pin. But after some tests only pin D2 works at WEMOS D1. Other platforms can work.
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

## Somfy RTS
This actuator allows to control Somfy RTS devices.

### Setup
Before the module can be used, virtual Somfy RTS remotes must be created.
This is done in `config_Somfy.h`.

`SOMFY_REMOTE_NUM` must be set to the number of virtual Somfy RTS remotes you want to have.
Then create for each of the virtual Somfy RTS remotes a unique 3-byte code and add them to `somfyRemotes`.
After a remote is setup, the order and codes should not be changed, else the setup process for all remotes have to be repeated.
Adding new codes at the end of the list is no problem.
Example of three virtual Somfy RTS remote codes:
```C
const uint32_t somfyRemotes[SOMFY_REMOTE_NUM] = {0x5184c8, 0xba24d0, 0xb77753};
```

Next the virtual Somfy RTS remotes must be paired with the Somfy RTS devices you want to control.
The next section describes how the PROG command/button of the virtual remote can be used.
Use the manual of the device you want to control for instructions on how to pair the virtual remote with the device.

### Commands
Commands must be send to the `commands/MQTTtoSomfy` subtopic.
Only json messages are supported.
The json message must contain two properties:
* remote: the index of the remote which is used to send the command (index start at zero)
* command: the command which should be send with the remote as string, see [table of command names](https://github.com/Legion2/Somfy_Remote_Lib#available-commands).

Optionally it can contain the following property:
* repeat: the number how often the command is repeated, default 4. Should be used to simulate long button presses, by increasing the repeat number, e.g. 20.

::: tip
The middle button on physical Somfy RTS Remote controls is called "My".
:::

Send PROG command with remote 0:

`mosquitto_pub -t home/OpenMQTTGateway_Somfy/commands/MQTTtoSomfy -m '{"remote":0,"command":"Prog"}'`

Send Up command with remote 1:

`mosquitto_pub -t home/OpenMQTTGateway_Somfy/commands/MQTTtoSomfy -m '{"remote":1,"command":"Up"}'`
