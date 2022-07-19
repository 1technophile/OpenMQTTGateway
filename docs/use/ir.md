# IR gateway
There is two methods for finding the IR codes you want to replicate:
1. Point the remote control to an IR receiver see [Receiving data from IR signal](#receiving-data-from-ir-signal) & [Send data by MQTT to convert it on IR signal](#send-data-by-mqtt-to-convert-it-on-ir-signal)
2. Find the code corresponding to your devices with IR global cache database https://irdb.globalcache.com/ see [Send data by MQTT from Global cache database to convert it on IR signal](#send-data-by-mqtt-from-global-cach%C3%A9-database-to-convert-it-on-ir-signal)

Here is the [List of protocols](https://docs.google.com/spreadsheets/d/1rTDZIG8rm0dSf4vP7HdTdM4-aRY1nDWH4jK28_WRwHQ/edit?usp=sharing) supported by OMG, even if your device brand is not in the list you can still use raw IR data or GlobalCache database.

## Receiving data from IR signal
Subscribe to all the messages with mosquitto or open your MQTT client software:
`mosquitto_sub -t +/# -v`
And press your IR remote control in front of the receiver led you should see the following messages for example:

```json
home/OpenMQTTGateway/IRtoMQTT {"value":875849879,"protocol":7,"protocol_name":SAMSUNG,"bits":32,"raw":"4534,4432,612,518,614,516,616,1618,618,1616,618,512,618,1618,608,524,612,518,616,514,618,512,616,1618,616,1618,618,514,616,1618,616,514,616,514,618,512,616,1618,618,1618,618,514,610,1622,616,514,618,514,614,516,616,1618,618,512,618,512,618,1616,550,580,618,1616,612,1624,618,1616,618"}
```

With an hexadecimal value:
```json
{"value":9938405643,"protocol":55,"bits":35,"hex":"0x25060090B","protocol_name":"TECO"}
```

To receive big dump of raw data you need first to modify the [config_IR.h](https://github.com/1technophile/OpenMQTTGateway/blob/091b317660fd201a30e2cd0e15424a13c5a6bd71/config_IR.h#L41) and uncomment DumpMode true

Unknown protocols are filtered by default, if you want to see the unknown protocols set into [config_IR.h](https://github.com/1technophile/OpenMQTTGateway/blob/master/config_IR.h)
`#define pubIRunknownPrtcl true` instead of false

![](https://github.com/1technophile/OpenMQTTGateway/blob/master/img/OpenMQTTGateway_serial3.jpg)

You can take this code and try to reproduce it with the gateway either by using [decimal value](#send-data-by-mqtt-to-convert-it-on-ir-signal) or the [raw value](#send-raw-ir-data-by-mqtt).

## Send data by MQTT to convert it on IR signal 
With the IR gateway you need to put on the topic the protocol_name you want to use to send the signal, the different protocols implemented are [here](https://github.com/crankyoldgit/IRremoteESP8266/blob/f9d7e5c622670132731e3f9c64d9132128eb320c/src/IRremoteESP8266.h#L299)

Exhaustive list [here](https://docs.google.com/spreadsheets/d/1_5fQjAixzRtepkykmL-3uN3G5bLfQ0zMajM9OBZ1bx0/edit#gid=1910001295)

For example if I want to send a command to a Sony TV you can use the following command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoIR -m  '{"value":551489775,"protocol_name":"SONY"}'`

The code after the -m represent the payload you want to send.

You could alternatively use an hex value (bits is the number of hexadecimal values):
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoIR -m  '{"hex":"0x250600090B","bits":5,"protocol_name":"TECO"}'`

or

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoIR -m  '{"hex":"0xA6BCF20040600020000000000519","bits":14,"protocol_name":"HAIER_AC_YRW02"}'`

If you don’t want to use special parameters for IR just use value key, the protocol per default is NEC

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoIR -m  '{"value":551489775}'`

NOTE: on Arduino Uno most of the protocols are not enable per default due to memory constraints (it is not the case for MEGA), to enable them go to [User_config.h](https://github.com/1technophile/OpenMQTTGateway/blob/master/main/User_config.h) and uncomment the #define corresponding the protocols you want:

```cpp
//#define IR_COOLIX
//#define IR_Whynter
//#define IR_LG
//#define IR_Sony
//#define IR_DISH
//#define IR_RC5
//#define IR_Sharp
#define IR_SAMSUNG
```
## Send data by MQTT from Global Caché database to convert it on IR signal 

The website https://irdb.globalcache.com/ contains an important database of IR codes of various devices brand. By registering and asking the code you will receive a code as the example below:

```
code1: sendir,1:1,1,38000,1,69,340,169,20,20,20,20,20,64,20,20,20,20,20,20,20,20,20,20,20,64,20,64,20,20,20,64,20,64,20,64,20,64,20,64,20,64,20,64,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,64,20,64,20,64,20,64,20,64,20,64,20,1544,340,85,20,3663
```

Extract this part of the code:
```
38000,1,69,340,169,20,20,20,20,20,64,20,20,20,20,20,20,20,20,20,20,20,64,20,64,20,20,20,64,20,64,20,64,20,64,20,64,20,64,20,64,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,64,20,64,20,64,20,64,20,64,20,64,20,1544,340,85,20,3663
```

and publish it to MQTT with the subject containing IR_GC:
```
mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoIR -m '{"raw":"38000,1,1,171,171,21,64,21,21,21,21,21,21,21,21,21,21,21,21,21,64,21,64,21,21,21,21,21,21,21,21,21,21,21,21,21,64,21,21,21,21,21,21,21,64,21,21,21,21,21,21,21,21,21,64,21,64,21,64,21,21,21,64,21,64,21,64,21,64,21,1114","protocol_name":"GC"}'
```

You should be able to command your devices without having listened with the IR receiver or if your protocol is unknown by the IRremote library

## Send data by MQTT with advanced IR parameters
IR sending support two advanced parameters; bits length and repeat number.

The example below will send the following advanced parameters bits: 14 and repeat:4 times for a Sony protocol:
```
mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoIR -m '{"value":551489775,"protocol_name":"NEC","repeat":4,"bits":14}'
```

## Send raw IR data by MQTT

2) If you use an Arduino UNO enable `IR_Raw` by uncommenting the line 129 in User_config.h
`#define IR_Raw`
If you are using the uno you will have to comment other gateway like ZgatewayRF, ZgatewayBT and ZgatewayIR to keep enough memory

3) publish your code like below
```
mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoIR -m '{"raw":"8850,4450,600,550,550,550,600,1600,600,550,600,500,600,500,600,550,600,500,600,1650,600,1600,600,550,600,1600,600,1650,600,1600,600,1650,600,1600,600,550,600,500,600,550,550,1650,600,500,600,550,600,500,600,550,550,1650,600,1650,550,1650,600,550,550,1650,600,1650,550,1650,600,1650,600","protocol_name":"Raw"}'
```

With big raw array you may cross the limit of default payload size. In this case the gateway will not receive the message or will not send it to the broker.
In this case the best way is to use hex values instead, but if you can't you may change the parameters below:
In User_config.h replace:
```cpp
# define JSON_MSG_BUFFER 512
# define mqtt_max_packet_size 1024
```
by
```cpp
# define JSON_MSG_BUFFER 1280
# define mqtt_max_packet_size 1280
```

## Repeat the IR signal OpenMQTTGateway receive
So as to repeat the IR signal received by the gateway once set the following parameter to true in [config_IR.h](https://github.com/1technophile/OpenMQTTGateway/blob/091b317660fd201a30e2cd0e15424a13c5a6bd71/config_IR.h#L37)
`#define repeatIRwMQTT true`

## Raw IR signal forwarding
So as to repeat the raw IR signal received by the gateway, uncomment and set the following parameter to true in [config_IR.h](https://github.com/1technophile/OpenMQTTGateway/blob/091b317660fd201a30e2cd0e15424a13c5a6bd71/config_IR.h#L39)
`#define RawDirectForward true`
