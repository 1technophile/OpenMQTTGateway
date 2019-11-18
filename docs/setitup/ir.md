# IR gateway
## Compatible parts
|Module|Purpose|Where to Buy|
|-|-|-|
|IR diode|Emitting|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|IR receiver|Receiving|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|transistor 2N2222|Amplify uC signal for the IR diode|-|
|330 ohms resistor|-|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|
|220 ohms resistor|limit current to LED|[compatible parts list](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1323184277)|

The IR setup can work with bc547 and a 4x3 LED-Matrix.

## Pinout
|Board| Receiver Pin| Emitter Pin|
|-|:-:|:-:|
|Arduino UNO|D2|D9|
|ESP8266|D4|D0|
|ESP32|27/**26**|14|

Connect the Emitter and Receiver to a 5V supply source, and the ground of your supply source to the ground of your board.

## Arduino Hardware setup
![IR](../img/OpenMQTTgateway_Arduino_Addon_IR.png)

## ESP8266 Hardware setup
![IR](../img/OpenMQTTgateway_ESP8266_Addon_IR.png)

## ESP32 Hardware setup
![IR](../img/OpenMQTTgateway_ESP32_Addon_IR.png)
