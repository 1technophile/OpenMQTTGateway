# RFM69 gateway

The RFM69 gateway enables to send and receive signal from/to another RFM69 module. It is based on the work of @bbx10 https://github.com/bbx10/nanohab

### Receiving data from RFM signal

Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

Generate your RF signals with a Moteino on other RFM69 based devices, you will receive :

`home/OpenMQTTGateway/RFM69toMQTT 60,-98,0`

In this case "60,-98,0" is the signal sent by another RFM69 + a wemos D1 with the sample sketch [rfm69send](https://github.com/1technophile/rfm69send/blob/master/rfm69send.ino) loaded in it.

### Send data by MQTT to convert it on RFM69 signal 
`mosquitto_pub -t "home/OpenMQTTGateway/commands/MQTTtoPilight" -m '{"data":"test"}'`

This command will send by RFM69 the string *test* to the default receiver id 99

### Send data by MQTT with advanced RFM69 parameters

RFM69 sending support one advanced parameters the target receiver ID.

Example:
SimplePublishing
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoRFM69/RCV_34 -m 33151562`
will make the gateway send to the receiver ID 34 (node number) instead of 99

JsonPublishing
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoRFM69 -m '{"data":1315156,"receiverid":34}'`
This command will send by RFM69 the string *test* to the receiver id 34 instead of 99

### RFM69 acknowledgment
Unlike RF or IR RFM69 as a complete acknowledgment mechanism. When a signal is sent the RFM69 can acknowledge the fact that the recipient received the message.
In this case the gateway will publish the sent message to the topic defined by subjectGTWRFM69toMQTT macro.
