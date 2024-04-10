# Troubleshooting

## Compilation/build error
This badge [![Build Status](https://github.com/1technophile/OpenMQTTGateway/workflows/Build/badge.svg?branch=master)](https://github.com/1technophile/OpenMQTTGateway/actions?query=branch%3Amaster+workflow%3ABuild) show you the state of the compilation of the master and this one [![Build Status](https://github.com/1technophile/OpenMQTTGateway/workflows/Build/badge.svg?branch=development)](https://github.com/1technophile/OpenMQTTGateway/actions?query=branch%3Adevelopment+workflow%3ABuild) for the development branch.
If you see a green badge this means that the code compilation is OK with the configuration given in the `docs/platformio.ini`.
Check your environment, boards , libraries before submitting an issue or a question.

## ESP32 compilation errors related to WiFi
If you get one or several of the following errors:

`error: 'WIFI_STA' was not declared in this scope`

`error: 'class WiFiClass' has no member named 'mode'`

`error: no matching function for call to 'WiFiClass::macAddress()`

You have a conflict between Arduino default WiFi library and ESP32 one. So as to resolve this issue you should move or remove the Arduino WiFi library (Arduino Sketchbook folder\libraries\WiFi) in order to enable the IDE to take the one from ESP32 (Arduino Sketchbook folder\hardware\espressif\arduino-esp32\libraries\WiFi)
More info on [this topic](https://community.openmqttgateway.com/t/esp32-compilation-error/144/5?u=1technophile)

## Not able to send or receive RF or IR 
→ Verify your power supply voltage with a multimeter, it should be 5V (can be 12V for FS1000A emitter),  please note that on NodeMCU V3 the Vin does not supply 5V contrary to NodeMCU V1

→ Verify your wiring

→ To eliminate issues of OpenMQTTGateway or you home controller try uploading basic examples from the libraries directly (like [SendDemo](https://github.com/sui77/rc-switch/tree/master/examples/SendDemo) for RF or IRSendDemo for IR) and execute them. If it doesn't work this means that you have mostly an issue related with your hardware or due to IDE/library version used.

Regarding the IR led emitter you can replace it with a normal led and see if it lights up when you send an MQTT command

→ If you are only unable to receive RF on nodemcu (or if it only works when a serial connection is active):

try with D2 instead of D3
and put
`#define RF_RECEIVER_GPIO 4 // D2 on nodemcu`
in config_rf.h
instead of
`#define RF_RECEIVER_GPIO 0 // D3 on nodemcu`

## Repetitive MQTT disconnections or/and commands sent to the gateway not taken into account
Most probably a network issue, don't use a guest network and if going through a firewall check its rules. To put aside gateway issue, try to connect to a local broker on the same network.

## You don't see the messages appearing on your broker but they appears on the serial monitor
This is due to a too small MQTT packet size, open User_config.h and set:
`#define mqtt_max_packet_size 1024`

## ESP Continuous restart or strange behaviour:
This can be due to corruption of the ESP flash memory, try to erase flash and upload OMG on it again.

If you didn't find your answer here post a question to the forum:
[![Community forum](https://img.shields.io/badge/community-forum-brightgreen.svg)](https://community.openmqttgateway.com)

## ESP does not connect to broker with TLS enabled
If you get the following error:
`W: failed, ssl error code=54` ("Certificate is expired or not yet valid.")

This is most probable caused by the time of the esp is not correct/synchronized.
The esp uses the Network Time Protocol (NTP) to get the current time from a time server.
If you get this error ntp is not configured correctly in the gateway.
Uncomment `//#    define NTP_SERVER "pool.ntp.org"` to set the `pool.ntp.org` as the time server.
You can also choose any other ntp time server you like.

It is normal that the time synchronization process takes some time and the MQTT connection will not be successful the first time.
If you set the ntp server for the gateway and keep getting the errors you should check your certificate validity duration.
