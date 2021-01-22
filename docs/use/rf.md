
# RF gateways  (433mhz/315mhz)

Note that for the moment RF, RF2 and Pilight can not be activated on the same boards together.

## RCSwitch based gateway

### Receiving data from RF signal

Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

Generate your RF signals by pressing a remote button or other and you should see :

`home/OpenMQTTGateway/433toMQTT {"value":1315156,"protocol":1,"length":24,"delay":317}`

### Disabling Transmit function to safe a PIN

To disable transmit functions to allow the use of another pin, add the following to the config_rf.h file :

`#define RF_DISABLE_TRANSMIT`

### Send data by MQTT to convert it on RF signal 

`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTto433" -m '{"value":1315156}'`

This command will send by RF the code 1315156 and use the default parameters (protocol 1, delay 350)

Arduino IDE serial data received when publishing data by MQTT

![](../img/OpenMQTTGateway_serial1.jpg)

We see that the Arduino receive the value 1315156 on the MQTT subject "MQTTto433" and send the data by RF

Arduino IDE serial data received when receiving data by 433Mhz

![](../img/OpenMQTTGateway_serial2.jpg)

### Send data by MQTT with advanced RF parameters

RF sending support three advanced parameters; bits length, RF protocol and RF pulselength
if you want to use a different RCswitch protocol put inside your payload the protocol number 2, "protocol":2.

if you want to use a pulselength 315 put inside your topic "delay":315

if you want to use a bits number different than 24 put inside your topic "length":24 for example

Example:
`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTto433" -m '{"value":1315156,"protocol":2,"length":24,"delay":315}'`
will make RCSwitch use the protocol 2 with a pulselength of 315ms and a bits number of 24

### Repeat the RF signal OpenMQTTGateway receive
So as to repeat the RF signal received by the gateway once set the following parameter to true in config_RF.h

`#define repeatRFwMQTT true`

### Repeat the RF signal several times
You can add a "repeat" key/value to the MQTTto433 JSON message to override the default number of repeats.

Example:
`home/OpenMQTTGateway/commands/MQTTto433 {"value":1315156,"protocol":1,"length":24,"delay":317, "repeat":10}`

### Set Transmit and Receive Frequency of CC1101 Transceiver Module

Default transmit frequency of the CC1101 module is 433.92 Mhz, and this can be can changed by including the frequency in the transmit message.  Parameter is `mhz` and valid values are 300-348 Mhz, 387-464Mhz and 779-928Mhz.  Actual frequency support will depend on your CC1101 board.

`home/OpenMQTTGateway/commands/MQTTto433 {"value":1150,"protocol":6,"length":12,"delay":450,"repeat":8,"mhz":303.732}`

Default receive frequency of the CC1101 module is 433.92 Mhz, and this can be can changed by sending a message with the frequency.  Parameter is `mhz` and valid values are 300-348 Mhz, 387-464Mhz and 779-928Mhz.  Actual frequency support will depend on your CC1101 board

`home/OpenMQTTGateway/commands/MQTTto433 {"mhz":315.026}`

Messages received will include the frequency, and when transmitting on a different frequency the module return to the receive frequency afterwards.  ie transmit messages on 303.732 Mhz then receive messages on 433.92 Mhz 

`{"value":4534142,"protocol":6,"length":26,"delay":356,"mhz":315.026}`

## Pilight gateway

### Receiving data from RF signal

Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

Generate your RF signals by pressing a remote button or other and you will see :

![](../img/OpenMQTTGateway_Pilight_Digoo-DG-R8S.png)

### Send data by MQTT to transmit a RF signal

#### Using a known protocol
**ON**
`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoPilight" -m '{"message":"{\"systemcode\":12,\"unitcode\":22,\"on\":1}","protocol":"elro_400_switch"}'`

**OFF**
`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoPilight" -m '{"message":"{\"systemcode\":12,\"unitcode\":22,\"off\":1}","protocol":"elro_400_switch"}'`

These commands will transmit by RF the signals to actuate an elro_400 switch.

#### Using a raw signal
You can transmit raw signal data by using the "raw" protocol. This uses the Pilight pulse train string format. One such example string, representing a transmission for Nexus protocol weather stations, looks like this: `c:03020202010102020102010101010101010202020201020102020202020101010201010202;p:500,1000,2000,4000;r:12@`. This string represents pulses and gaps directly.

Each number in the list after `p:` that ends with `;` stands for **p**ulse and gap lengths in microseconds (µs). In this example, we have a list containing lengths of 500µs, 1000µs, 2000µs, and 4000µs.

Each number after `c:` and ended by `;` represents a **c**ode that references the `p:` list by index. In this example, the first 4 numbers after `c:` are 0, 3, 0, and 2, which reference `p:`[0] = 500, `p:`[3] = 4000, `p:`[0] = 500, and `p:`[2] = 2000, respectively. In the language of digital radio transceiving, the most basic unit is usually a pulse and gap pair; in other words, 0s and 1s are represented by a pulse followed by a gap (lack of pulse) and the time lengths of these pulses and gaps. Different protocols have different pulse lengths and gap lengths representing 0, and a different one representing 1. Because of this pulse-gap nature, the codes in `c:` must be taken as pairs; the first number in a pair represents the length of the pulse, and the second number the subsequent gap. In this example, the first pair, 03, represents a pulse of 500µs followed by a gap of 4000µs. The next pair, 02, represents a pulse of 500µs followed by a gap of 2000µs.

The number after `r:` represents how many times the message in the string is to be **r**epeated. The `r:` block is optional. The default number of repeats if `r:` is not specified is 10. Greater than about 100 repeats will cause a crash due to memory usage. If this example were written without specifying repeats, it would look like this: `{"raw":"c:03020202010102020102010101010101010202020201020102020202020101010201010202;p:500,1000,2000,4000@"}`

The entire string must end in a `@`. Each block must end in a `;`, but if it is the last block in the string, the `@` replaces the `;`. Since the `r:` block is optional, this last block could be either `p:` or `r:`.

The JSON for the MQTT message to `home/OpenMQTTGateway/commands/MQTTtoPilight` should specify the pulse train string as the value for the "raw" key: `{"raw":"c:03020202010102020102010101010101010202020201020102020202020101010201010202;p:500,1000,2000,4000;r:12@"}`.

e.g. `mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoPilight" -m '{"raw":"c:03020202010102020102010101010101010202020201020102020202020101010201010202;p:500,1000,2000,4000;r:12@"}'`

## RF with SONOFF RF BRIDGE
### Receiving data from RF signal

Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

Generate your RF signals by pressing a remote button or other and you will see:
```
home/OpenMQTTGateway/SRFBtoMQTT {"raw":"2B660186042E00E7E5","value":"59365","delay":"1111","val_Thigh":"390","val_Tlow":"1070"}
```

The first parameter is the raw value extracted from the RF module of the Sonoff bridge. The data are in hexadecimal and correspond to the details below:
https://www.itead.cc/wiki/images/5/5e/RF_Universal_Transeceive_Module_Serial_Protocol_v1.0.pdf
OpenMQTTGateway process the raw value to extract the other decimal values that can be reused to reproduce a signal (raw value can also be reused).

NOTE: currently the device doesn't receive correct values from Switches remote control

### Send data by MQTT to convert it on RF signal 
`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoSRFB" -m '{"value":1315156}'`

This command will send by RF the code 1315156 and use the default parameters:
Repeat = 1
Low time= 320
High time= 900
SYNC = 9500

### Send data by MQTT with advanced RF parameters

RF bridge sending support four advanced parameters; Repeat, Low time, High time & Sync
if you want to repeat your signal sending put into your json payload "repeat":2, 2 means 2 repetitions of signal

if you want to use a low time of 315 put inside your json payload "Tlow":315

if you want to use a high time of 845 put inside your json payload "Thigh":845

if you want to use a sync time of 9123 put inside your json payload "Tsyn":9123 

Example:
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSRFB/Tlow_315/Thigh_845/Tsyn_9123 -m '{"value":"33151562","delay":"9123","val_Thigh":"845","val_Tlow":"315"}'`
will make RF Bridge send a signal with the use of listed parameters 315, 845, 9123...

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSRFB/Raw -m '{"raw":"267A013603B6140551"}'`
will make RF Bridge send a signal with the use of advanced parameters defined in the raw string

## RF2 gateway KAKU
RF2 gateway enables to send command to RF devices with the KAKU protocol. DIO chacon devices are an example.
It uses the same pinout as the RF gateway and both gateways can be used on the same setup.

Receiving RF codes with the KAKU protocol is not compatible with ZgatewayRF , so as to get the code of your remotes you should comment ZgatewayRF in User_config.h.
Transmitting can be done with both ZgatewayRF and ZgatewayRF2

### Receiving data from KAKU signal

Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

Generate your RF signals by pressing a remote button or other and you will see :

`home/OpenMQTTGateway/RF2toMQTT {"unit":0,"groupBit":0,"period":273,"address":8233228,"switchType":0}`

### Send data by MQTT to convert it on KAKU signal 

Once you get the infos publish the parameters with mqtt like that for off:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoRF2 -m "{"unit":0,"groupBit":0,"period":273,"address":8233228,"switchType":0}"`

for on:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoRF2 -m "{"unit":0,"groupBit":0,"period":273,"adress":8233228,"switchType":1}"`
