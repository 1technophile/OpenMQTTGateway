# BLE gateway
## Receiving signals from BLE beacon devices for Presence detection

Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

_NOTE: HM-10 or HM-11 module needed if you are not using ESP32; configure in `user_config.h`_

The BT gateway module for OpenMQTTGateway enables the detection of BLE beacons and their signal strength.  Generally BLE devices will not broadcast if they are paired so you may need to ensure your beacons is unpaired before it will be seen by the gateway.

If beacons are detected the gateway will periodically publish messages to MQTT (beacons must not be paired, see above):

```
home/OpenMQTTGateway/BTtoMQTT/45E174126E00 {"id":"45:e1:74:12:6e:00","rssi":-89,"distance":21.51847,"servicedata":"fe0000000000000000000000000000000000000000"}
```
```
home/OpenMQTTGateway/BTtoMQTT/C7FaaD132C00 {"id":"c7:fa:ad:13:2c:00","rssi":-68,"distance":2.799256,"servicedata":"drfgdrgdsrgesrdgdrgdregesrgtrhtyhtfyhdtyhh"}
```

The subtopic after `home/BTtoMQTT/` is the MAC address of the Bluetooth low energy beacon.  The rssi value is the [RSSI signal level](https://www.metageek.com/training/resources/understanding-rssi.html) from which you may deduce the relative distance to the device.
Consider the distance as a beta featuer as currently we are not retrieving the emitting power of the beacon to make it more accurate.

Note that you can find apps to simulate beacons and do some tests like [Beacon simulator](https://play.google.com/store/apps/details?id=net.alea.beaconsimulator)

IOS version >=10 devices advertise without an extra app a mac address, nevertheless this address [changes randomly](https://github.com/1technophile/OpenMQTTGateway/issues/71) and cannot be used for presence detection. You must install an app to advertise a fixed MAC address.


## Receiving signals Mi Flora/ Mi jia device/ LYWDS02, ClearGrass or Mi scale
So as to receive BLE sensors data you need either a simple ESP32 either an ESP8266/arduino + HM10/11 with firmware >= v601
The mi flora supported firmware is >3.1.8

Verify that your sensor is working with the App and update it with the last software version.
You should see in your MQTT broker the following data:
![](../img/OpenMQTTgateway_miflora_results.png)

Note that the gateway return one or two measurement value each time. The different measures depending on the devices are:
* Lux
* Temperature
* Moisture
* Fertilization
* Humidity
* Pressure
* Steps
* Weight
* Impedance
* Battery (mi jia only)

The infos will appear like this on your MQTT broker:

`home/OpenMQTTGateway/BTtoMQTT/4C33A6603C79 {"hum":"52.6","tem":"19.2"}`

More info are available on [my blog](https://1technophile.blogspot.fr/2017/11/mi-flora-integration-to-openmqttgateway.html)  (especially about how it was implemented with HM10)

::: tip
The HM10 module doesn't read enough information (servicedata UUID is missing) to support Mi Scale and Mi Band. They are supported nevertheless with ESP32.
OpenMQTTGateway publish the servicedata field of your BLE devices, with HM10 this field can be longer compared to ESP32 if the device is not recognised.
:::

## Setting a white or black list
A black list is a list of mac adresses that will never be published by OMG
to set black list
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"black-list":["012314551615","4C6577889C79","4C65A6663C79"]}'`

A white list is a list of mac adresses permitted to be published by OMG
to set white list
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"white-list":["012314551615","4C65A5553C79","4C65A6663C79"]}'`

Note: if you want to filter (white or black list) on BLE sensors that are auto discovered, you need to wait for the discovery before applying the white or black list

::: tip
So as to keep your white/black list persistent you can publish it with the retain option of MQTT (-r with mosquitto_pub or retain check box of MQTT Explorer)
`mosquitto_pub -r -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"white-list":["012314551615","4C65A5553C79","4C65A6663C79"]}'`
:::

## Setting the time between scans and force a scan

If you want to change the time between readings you can change the interval by MQTT, if you want the BLE scan every 66seconds:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"interval":66000}'`

you can also force a scan to be done by the following command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"interval":0}'`

Once done the previous value of interval will be recovered.

The default value is set into config_BT.h

## Setting the minimum RSSI accepted to publish device data

If you want to change the minimum RSSI value accepted for a device to be published, you can change it by MQTT. For example if you want to set -80

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"minrssi":-80}'`

you can also accept all the devices by the following command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"minrssi":0}'`

The default value is set into config_BT.h

## Other

To check your hm10 firmware version upload a serial sketch to the nodemcu (this will enable communication directly with the hm10) and launch the command:
`AT+VERR?`

More info about HM-10 is [available here](http://www.martyncurrey.com/hm-10-bluetooth-4ble-modules/)
