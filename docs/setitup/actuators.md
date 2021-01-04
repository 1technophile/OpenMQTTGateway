# Actuators
## Compatible parts 
|Module|Purpose|Where to Buy|
|-|-|-|
|LED|Basic led|[parts list](https://compatible.openmqttgateway.com/index.php/parts)|
|FASTLED|RGB Leds management|[parts list](https://compatible.openmqttgateway.com/index.php/parts)|
|BUZZER|-|[parts list](https://compatible.openmqttgateway.com/index.php/parts)|
|RELAY|Switch power circuit|[parts list](https://compatible.openmqttgateway.com/index.php/parts)|

## Pinout
|Module| Boards|
|-|-|
|RELAY|all output compatible pins|
|FASTLED|all output compatible pins|

Vcc pin of the board and the Module to a 5V supply source
Ground pins of the board and the Module to the ground of the supply source.

## Somfy RTS
For this actuator a 433.42 MHz RF transmitter is required.
The standard 433.92 MHz transmitter don't work.
The CC1101 Transceiver supports both 433.42 MHz and 433.92 MHz and can be used with the Somfy RTS actor.
The wiring of the hardware is described in the [RF gateway](rf).
