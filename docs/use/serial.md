# RS232/Serial gateway

The SERIAL gateway can be used to send and receive data from the serial connection to and from MQTT. Both softwareSerial as hardwareSerial are supported. HardwareSerial can be used for higher baud rates, but is limited to specific pins on most platforms.

## Sending an SERIAL message

Simply publish the message you wish to transmit, minus the prefix and postfix. For example, to send the "Turn On" signal for a Mitsubishi XD221U projector, the code is simply '!' so you would use the command

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoSERIAL -m  '{"value": "!"}'`

It will automatically add the prefix and postfix you set in [config_SERIAL.h](https://github.com/1technophile/OpenMQTTGateway/blob/master/main/config_SERIAL.h).


## Receiving an SERIAL message

Two modes are available for receiving SERIAL messages.

### Single MQTT message mode (default)
To receive a message, subscribe to all with `mosquitto_sub -t +/# -v`
and perform an action that should get a response from the device. For example, If I were to send the "Turn On" signal from earlier, I would receive back

```json
home/OpenMQTTGateway/SERIALtoMQTT {"value":"1"}
```

Because this projector echoes back a received command to acknowledge. Some devices will send a NACK, or Negative Acknowledge, to confirm that they received your message but could not comply. That would look like

```json
home/OpenMQTTGateway/SERIALtoMQTT {"value":"!:N"}
```

### JSON mode
This mode can be used if the received message on the serial link is JSON. The JSON keys are used as separate MQTT sub-topics. For nested JSON this will be repeated for sub-keys up to the specified nesting level.

For example:

input received at serial link:
```json
{temperature: {sens1: 22, sens2: 23}, humidity: {sens1: 80, sens2: 60}}
```


output in case of max nesting level 1:
```json
home/OpenMQTTGateway/SERIALtoMQTT/temperature  "{sens1: 22, sens2: 23}"
home/OpenMQTTGateway/SERIALtoMQTT/humidity     "{sens1: 80, sens2: 60}"
```

output in case of max nesting level 2 (or higher):
```json
home/OpenMQTTGateway/SERIALtoMQTT/temperature/sens1  22
home/OpenMQTTGateway/SERIALtoMQTT/temperature/sens2  23
home/OpenMQTTGateway/SERIALtoMQTT/humidity/sens1  80
home/OpenMQTTGateway/SERIALtoMQTT/humidity/sens2  60
```

