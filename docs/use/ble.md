# BLE gateway

::: warning
We strongly encourage the use of a white-list (see below) so as to collect data from your devices only and not from other MAC addresses.
By default the gateway scans the advertizing BLE devices nearby with their MAC addresses. Depending on your country, it may be illegal to monitor networks for MAC addresses, especially on networks that you do not own. Please check your country's laws (for US Section 18 U.S. Code § 2511) - [discussion here](https://github.com/schollz/howmanypeoplearearound/issues/4).
:::

## Receiving signals from BLE beacon devices for Presence detection

Subscribe to all the messages with mosquitto or open your MQTT client software:

`    sudo mosquitto_sub -t +/# -v`

_NOTE: HM-10 or HM-11 module needed if you are not using ESP32; configure in `User_config.h`_

The BT gateway module for OpenMQTTGateway enables the detection of BLE beacons and their signal strength.  Generally BLE devices will not broadcast if they are paired so you may need to ensure your beacons is unpaired before it will be seen by the gateway.

If beacons are detected the gateway will periodically publish messages to MQTT (beacons must not be paired, see above):

```
home/OpenMQTTGateway/BTtoMQTT/45E174126E00 {"id":"45:e1:74:12:6e:00","rssi":-89,"distance":21.51847,"servicedata":"fe0000000000000000000000000000000000000000"}
```
```
home/OpenMQTTGateway/BTtoMQTT/C7FaaD132C00 {"id":"c7:fa:ad:13:2c:00","rssi":-68,"distance":2.799256,"servicedata":"drfgdrgdsrgesrdgdrgdregesrgtrhtyhtfyhdtyhh"}
```

The subtopic after `home/BTtoMQTT/` is the MAC address of the Bluetooth low energy beacon.  The rssi value is the [RSSI signal level](https://www.metageek.com/training/resources/understanding-rssi.html) from which you may deduce the relative distance to the device.
Consider the distance as a beta feature as currently we are not retrieving the emitting power of the beacon to make it more accurate.

Note that you can find apps to simulate beacons and do some tests like [Beacon simulator](https://play.google.com/store/apps/details?id=net.alea.beaconsimulator)

iOS version >=10 devices advertise without an extra app MAC address, nevertheless this address [changes randomly](https://github.com/1technophile/OpenMQTTGateway/issues/71) and cannot be used for presence detection. You must install an app to advertise a fixed MAC address.


## Receiving signals from BLE devices Mi Flora, Mi jia, LYWDS02, LYWSD03MMC, ClearGrass, Mi scale and [many more](https://compatible.openmqttgateway.com/index.php/devices/ble-devices/)
So as to receive BLE sensors data you need a simple ESP32
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
* Battery
* Voltage
* Open
* Presence

The infos will appear like this on your MQTT broker:

`home/OpenMQTTGateway/BTtoMQTT/4C33A6603C79 {"hum":"52.6","tempc":"19.2","tempf":"66.56"}`

More info are available on [my blog](https://1technophile.blogspot.fr/2017/11/mi-flora-integration-to-openmqttgateway.html) 

::: tip
OpenMQTTGateway publish the servicedata field of your BLE devices.
:::

## Setting a white or black list
A black list is a list of MAC addresses that will never be published by OMG
to set black list
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"black-list":["01:23:14:55:16:15","4C:65:77:88:9C:79","4C:65:A6:66:3C:79"]}'`

A white list is a list of MAC addresses permitted to be published by OMG
to set white list
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"white-list":["01:23:14:55:16:15","4C:65:77:88:9C:79","4C:65:A6:66:3C:79"]}'`

Note: if you want to filter (white or black list) on BLE sensors that are auto discovered, you need to wait for the discovery before applying the white or black list, or temporarily disable it:

to temporarily disable white/black list
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"ignoreWBlist":true}'`

to enable white/black list back
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"ignoreWBlist":false}'`

::: tip
So as to keep your white/black list persistent you can publish it with the retain option of MQTT (-r with mosquitto_pub or retain check box of MQTT Explorer)
`mosquitto_pub -r -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"white-list":["01:23:14:55:16:15","4C:65:77:88:9C:79","4C:65:A6:66:3C:79"]}'`
:::

## Setting the time between BLE scans and force a scan

If you want to change the time between readings you can change the interval by MQTT.
For example, if you want the BLE to scan every 66 seconds:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"interval":66000}'`

you can also force a scan to be done by the following command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"interval":0}'`

::: tip
With Home Assistant, this command is directly available through MQTT auto discovery as a switch into the HASS OpenMQTTGateway device entities list.
:::

Once the forced scan has completed, the previous scan interval value will be restored. Forcing a scan command trigger also a BLE connect process after the scan (see below).

The default value `TimeBtwRead` is set into config_BT.h or into your .ini file for platformio users.

If you want to scan continuously for BLE devices, for example for beacon location you can set the interval to 1ms:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"interval":1}'`

In this case you should deactivate the BLE connection mechanism to avoid concurrency between scan and connections (see chapter below, bleconnect).

::: tip
For certain devices like LYWSD03MMC OpenMQTTGateway use a connection (due to the fact that the advertized data are encrypted), this connection mechanism is launched after every `ScanBeforeConnect` per default, you can modify it by following the procedure below.
:::

## Setting the number of scans between connection attempts

If you want to change the number of BLE scans that are done before a BLE connect you can change it by MQTT, if you want the BLE connect to be every 30 scans:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"scanbcnct":30}'`

The BLE connect will be done every 30 * (`TimeBtwRead` + `Scan_duration`), 30 * (55000 + 10000) = 1950000ms

## Setting if the gateway publishes all the BLE devices scanned or only the detected sensors

If you want to change this characteristic:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"onlysensors":true}'`

::: tip
With Home Assistant, this command is directly available through MQTT auto discovery as a switch into the HASS OpenMQTTGateway device entities list.
:::

The gateway will publish only the detected sensors like Mi Flora, Mi jia, LYWSD03MMC... and not the other BLE devices. This is usefull if you don't use the gateway for presence detection but only to retrieve sensors data.

## Setting if the gateway connects to BLE devices eligibles on ESP32

If you want to change this characteristic:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"bleconnect":false}'`

::: tip
With Home Assistant, this command is directly available through MQTT auto discovery as a switch into the HASS OpenMQTTGateway device entities list.
:::

## Setting if the gateway publish into Home Assistant Home presence topic

If you want to publish to Home Assistant presence topic, you can activate this function by the HASS interface (this command is auto discovered), [here is a yaml example](../integrate/home_assistant.md#mqtt-room-presence).
Or by an MQTT command.

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"hasspresence":true}'`

To change presence publication topic, use this MQTT command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"presenceTopic":"presence/"}'`

To use iBeacon UUID for presence, instead of sender (random) MAC address, use this MQTT command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"presenceUseBeaconUuid":true}'`

This will change usual payload for iBeacon from:
`{"id":"60:87:57:4C:9B:C2","mac_type":1,"rssi":-78,"distance":7.85288,"brand":"GENERIC","model":"iBeacon","model_id":"IBEACON","mfid":"4c00","uuid":"1de4b189115e45f6b44e509352269977","major":0,"minor":0,"txpower":-66}`
To:
`{"id":"1de4b189115e45f6b44e509352269977","mac_type":1,"rssi":-78,"distance":7.85288,"brand":"GENERIC","model":"iBeacon","model_id":"IBEACON","mfid":"4c00","uuid":"1de4b189115e45f6b44e509352269977","major":0,"minor":0,"txpower":-66,"mac":"60:87:57:4C:9B:C2"}`
Note: the MAC address is put in "mac" field.

## Setting if the gateway uses iBeacon UUID as topic, instead of (random) MAC address

By default, iBeacon are published like other devices, using a topic based on the MAC address of the sender.
But modern phones randomize their Bluetooth MAC address making it difficult to track iBeacon.

For example, the 2 following messages corresponds to the same iBeacon, but with different MAC and topics:
```
home/OpenMQTTGateway/BTtoMQTT/58782076BC24 {"id":"58:78:20:76:BC:24","mac_type":1,"rssi":-79,"brand":"GENERIC","model":"iBeacon","model_id":"IBEACON","mfid":"4c00","uuid":"1de4b189115e45f6b44e509352269977","major":0,"minor":0,"txpower":-66}
home/OpenMQTTGateway/BTtoMQTT/5210A84690AC {"id":"52:10:A8:46:90:AC","mac_type":1,"rssi":-77,"brand":"GENERIC","model":"iBeacon","model_id":"IBEACON","mfid":"4c00","uuid":"1de4b189115e45f6b44e509352269977","major":0,"minor":0,"txpower":-66}
```

To use iBeacon UUID as topic, use this MQTT command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"pubBeaconUuidForTopic":true}'`

Resulting in such messages (for the same iBeacon as previously):
```
home/OpenMQTTGateway/BTtoMQTT/1de4b189115e45f6b44e509352269977 {"id":"52:10:A8:46:90:AC","mac_type":1,"rssi":-76,"brand":"GENERIC","model":"iBeacon","model_id":"IBEACON","mfid":"4c00","uuid":"1de4b189115e45f6b44e509352269977","major":0,"minor":0,"txpower":-66}
home/OpenMQTTGateway/BTtoMQTT/1de4b189115e45f6b44e509352269977 {"id":"7B:63:C6:82:DC:57","mac_type":1,"rssi":-83,"brand":"GENERIC","model":"iBeacon","model_id":"IBEACON","mfid":"4c00","uuid":"1de4b189115e45f6b44e509352269977","major":0,"minor":0,"txpower":-66}
```

## Setting the minimum RSSI accepted to publish device data

If you want to change the minimum RSSI value accepted for a device to be published, you can change it by MQTT. For example if you want to set -80

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"minrssi":-80}'`

you can also accept all the devices by the following command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"minrssi":-200}'`

The default value is set into config_BT.h

## ADVANCED: Setting up an external decoder

This advanced option is used to publish raw radio frames on a specific topic to be decoded by an external decoder instead of the integrated one.

To enable external decoder:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"extDecoderEnable":true}'`

To change the default external decoder topic to "undecoded":

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"extDecoderTopic":"undecoded"}'`

## ADVANCED: Filtering out connectable devices

[With OpenHAB integration](../integrate/openhab2.md), this configuration is highly recommended, otherwise you may encounter incomplete data.

If you want to enable this feature:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"filterConnectable":true}'`

## ADVANCED: Publishing known service data

If you want to enable this feature:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"pubKnownServiceData":true}'`

## ADVANCED: Publishing unknown service data

If you want to change the default behaviour, in case you are having too heavy service data:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"pubUnknownServiceData":false}'`

## ADVANCED: Publishing known manufacturer's data

If you want to change the default behaviour:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"pubKnownManufData":true}'`

## ADVANCED: Publishing unknown manufacturer's data

If you want to change the default behaviour, in case you are having too heavy service data:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"pubUnknownManufData":false}'`

## ADVANCED: Publishing the service UUID data

If you want to change the default behaviour:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"pubServiceDataUUID":true}'`

## Store BLE configuration into the gateway (only with ESP32 boards)

Open MQTT Gateway has the capability to save the current configuration and reload it at startup.

To store the running configuration into the gateway, use the command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"save":true}'`

At any time, you can reload the stored configuration with the command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"load":true}'`

If you want to erase the stored configuration, use the command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"erase":true}'`
Note that it will not change the running configuration, only ensure default configuration is used at next startup.

By the way, if you want to load the default built-in configuration (on any board, not only ESP32), use the command:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"init":true}'`
Note that it will not change the stored configuration, `erase` or `save` is still needed to overwrite the saved configuration.

## Read/write BLE characteristics over MQTT (ESP32 only)

The gateway can read and write BLE characteristics from devices and provide the results in an MQTT message.  
::: tip
These actions will be taken on the next BLE connection, which occurs after scanning and after the scan count is reached, [see above to set this](#setting-the-number-of-scans-between-connection-attempts).
This can be overridden by providing an (optional) parameter `"immediate": true` within the command. This will cause the BLE scan to stop if currently in progress, allowing the command to be immediately processed. All other connection commands in queue will also be processed for the same device, commands for other devices will be deferred until the next normally scheduled connection.

**Note** Some devices need to have the MAC address type specified. You can find this type by checking the log/MQTT data and looking for "mac_type". By default the type is 0 but some devices use different type values. You must specify the correct type to connect successfully.  
To specify the MAC address type add the parameter `"mac_type"` to the command. For example `"mac_type": 1` to connect with a device with the MAC address type of 1.
:::

### Example write command
```
mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{
  "ble_write_address":"AA:BB:CC:DD:EE:FF",
  "ble_write_service":"cba20d00-224d-11e6-9fb8-0002a5d5c51b",
  "ble_write_char":"cba20002-224d-11e6-9fb8-0002a5d5c51b",
  "ble_write_value":"TEST",
  "value_type":"STRING",
  "ttl":4,
  "immediate":true }'
```
Response:
```
{
  "id":"AA:BB:CC:DD:EE:FF",
  "service":"cba20d00-224d-11e6-9fb8-0002a5d5c51b",
  "characteristic":"cba20002-224d-11e6-9fb8-0002a5d5c51b",
  "write":"TEST",
  "success":true
}
```
### Example read command
```
mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{
  "ble_read_address":"AA:BB:CC:DD:EE:FF",
  "ble_read_service":"cba20d00-224d-11e6-9fb8-0002a5d5c51b",
  "ble_read_char":"cba20002-224d-11e6-9fb8-0002a5d5c51b",
  "value_type":"STRING",
  "ttl": 2 }'
```
Response:
```
{
  "id":"AA:BB:CC:DD:EE:FF",
  "service":"cba20d00-224d-11e6-9fb8-0002a5d5c51b",
  "characteristic":"cba20002-224d-11e6-9fb8-0002a5d5c51b",
  "read":"TEST",
  "success":true
}
```

::: tip
The `ttl` parameter is the number of attempts to connect (defaults to 1), which occur after the BLE scan completes.  
`value_type` can be one of: STRING, HEX, INT, FLOAT. Default is STRING if omitted in the message.
:::

## SwitchBot Bot control (ESP32 only)

SwitchBot Bot devices are automatically discovered and available as a device in the configuration menu of home assistant.

::: tip If the SwitchBot mode is changed the ESP32 must be restarted. :::

The device can also be controlled over MQTT with a simplified BLE write command.

### Example command to set the SwitchBot state to ON:
```
mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{
  "SBS1":"on",
  "mac":"AA:BB:CC:DD:EE:FF"
}'
```
Response (assuming success):
```
{
  "id":"AA:BB:CC:DD:EE:FF",
  "state":"on"
}
```
