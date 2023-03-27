# BLE gateway

The manufacturer agnostic BLE gateway acts as a powerful BLE scanner of devices and decoder, allowing you to visualize and analyze information from a wide range of BLE devices. 
It can also act as a presence detection gateway by reading the nearby BLE tags or tracker devices.

Data are transmitted to an MQTT broker, where it can be used to trigger events and rules, as well as displayed, stored and processed in your favorite controller. 

With the ability to monitor and analyze data such as temperature, humidity, moisture, luminance, weight, pressure, fine particles, and more, the BLE gateway provides a flexible and customizable solution for integrating BLE technology into your control and monitoring systems.

![](../img/OpenMQTTGateway-sensors-ble.png)


## Receiving signals from [compatible BLE sensors](https://decoder.theengs.io/devices/devices_by_brand.html) to publish it to an MQTT broker.
To receive data from BLE sensors you can use an ESP32-based device with a programming USB port or use a Serial adapter.

OpenMQTTGateway is also available preloaded and configured with the [Theengs plug](https://shop.theengs.io/products/theengs-plug-smart-plug-ble-gateway-and-energy-consumption), a smart plug that acts as a BLE gateway and energy monitoring device.

1. Follow the [Upload](../upload/web-install.md) and [Configuration](../upload/portal.md) steps.

1. Download an MQTT client like MQTT explorer.

1. You should see data coming in your broker.

![](../img/OpenMQTTGateway-mqtt-explorer-lywsd03mmc-atc.png)

Once the data has been transmitted to the MQTT broker, it can be easily integrated with your preferred controller. For example, the data can be automatically discovered and made available within popular controllers, example below with Home Assistant.

![](../img/OpenMQTTGateway-home-assistant-chart.png)

Examples of compatible sensors among [our list](https://decoder.theengs.io/devices/devices_by_brand.html: Mi Flora, Mi jia, LYWDS02, LYWSD03MMC, ClearGrass, Mi scale, iBBQ, TPMS

## Receiving signals from BLE tracker devices for Presence detection
The gateway can detect the BLE trackers from Tile, NUT, TAGIT, ITAG, MiBand, Amazfit and RuuviTag and create automaticaly a device tracker entity following the Home Assistant discovery convention (if the auto discovery is activated).
To do this activate the "BT: Publish HASS presence" switch in your controller or send the followng MQTT command to your broker:
`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"hasspresence":true}'`

The entity created can be attached to a person to leverage presence detection. The `away` or `not home` state is triggered if the BLE tracker is not detected during the timer defined by `presenceawaytimer`.

![](../img/OpenMQTTGateway-BLE-tracker-Home-Assistant.png)

If you have multiple gateways, your BLE trackers may not be detected temporary by one gateway but still by the others. In this case you will see the tracker appears offline briefly and online again once it is detected by the others gateways.

By default `presenceawaytimer` is set to 120s, you can change it from the slider in your controller or with the following command (ms)

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"presenceawaytimer":66000}'`

Generally BLE devices will not broadcast if they are paired so you may need to ensure your beacons is unpaired/disconnected before it will be seen by the gateway.

Consider the distance estimation as a beta feature.

Note that you can find apps to simulate beacons and do some tests like [Beacon simulator](https://play.google.com/store/apps/details?id=net.alea.beaconsimulator)

iOS version >=10 devices advertise without an extra app MAC address, nevertheless this address [changes randomly](https://github.com/1technophile/OpenMQTTGateway/issues/71) and cannot be used for presence detection. You must install an app to advertise a fixed MAC address.

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
For certain devices like LYWSD03MMC OpenMQTTGateway use a connection (due to the fact that the advertized data are encrypted), this connection mechanism is launched after every `TimeBtwConnect` per default, you can modify it by following the procedure below.
:::

## Setting the time between connection attempts

If you want to change the time between BLE connect you can change it by MQTT, if you want the BLE connect time to be every 300s:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"intervalcnct":300000}'`

## Setting if the gateway publishes all the BLE devices scanned or only the detected sensors (default: false)

If you want to change this characteristic:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"onlysensors":true}'`

::: tip
With Home Assistant, this command is directly available through MQTT auto discovery as a switch into the HASS OpenMQTTGateway device entities list.
:::

The gateway will publish only the detected sensors like Mi Flora, Mi jia, LYWSD03MMC... and not the other BLE devices. This is useful if you don't use the gateway for presence detection but only to retrieve sensors data.

## Setting if the gateway publishes known devices which randomly change their MAC address

The default is false, as such changing MAC addresses cannot be related to specific devices.

If you want to change this characteristic:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"randommacs":true}'`

## Setting if the gateway use adaptive scanning

Adaptive scanning lets the gateway decide for you the best passive `interval` and active `intervalacts` scan interval, depending on the characteristics of your devices.
The gateway retrieves your devices' information from [Theengs Decoder](https://decoder.theengs.io) and adapts its parameters accordingly if a device that requires it is detected.
For example a door or a PIR sensor will require continuous scanning, so if detected the gateway is going to reduce its time between scans to the minimum. Or your devices may also require active scanning to retrieve data, in this case the gateway will also trigger active scans at regular intervals.

If you want to change this characteristic (default:true):

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"adaptivescan":false}'`

Setting Adaptive scanning to `false` will automatically put the gateway to continuous active scanning if no additional manual changes have already been applied.

::: tip
With Home Assistant, this command is directly available through MQTT auto discovery as a switch into the HASS OpenMQTTGateway device entities list.
:::

An overview with background information to better understand the different setting used:

**Passive scanning:** With this scanning mode the gateway picks up any freely available broadcasts sent out by devices, without any interaction with the devices. The interval for this is set with [{"interval":66000}](#setting-the-time-between-ble-scans-and-force-a-scan)

**Active scanning:** With this scanning mode the gateway sends out requests for sensor broadcasts first, before then picking up the broadcast advertisement data. Some devices require this request before they send out all data in their broadcasts. The interval for this active scanning with request first is set by [{"intervalacts":300000}](#setting-the-time-between-active-scanning)

If adaptive scanning is set to false and you want to manually set these intervals, setting [Publishing advertisement and advanced data](#advanced-publishing-advertisement-and-advanced-data-default-false) to true will show you additional data about which of your devices require active scanning and/or continuous scanning, so that you can tune these setting to your devices and your individual requirements of their data.

**"cont":true** - the device requires continuous scanning. If passive ({"interval":100}) or active ({"intervalacts":100}) depends on the additional device specification.

**"acts":true** - the device requires active scanning to broadcast all of it's data for decoding.

## Setting the time between active scanning

If you have passive scanning activated, but also have some devices which require active scanning, this defines the time interval between two intermittent active scans.

If you want to change the time between active scans you can change it by MQTT. For setting the active scan interval time to every 5 minutes:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"intervalacts":300000}'`

## Setting the duration of a scan

If you want to change the default 10 sec duration of each scan cycle to 5 seconds

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"scanduration":5000}'`

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

## Store BLE configuration into the gateway

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

## Read/write BLE characteristics over MQTT

The gateway can read and write BLE characteristics from devices and provide the results in an MQTT message.  
::: tip
These actions will be taken on the next BLE connection, which occurs after scanning and after the scan count is reached, [see above to set this](#setting-the-number-of-scans-between-connection-attempts).
This can be overridden by providing an (optional) parameter `"immediate": true` within the command. This will cause the BLE scan to stop if currently in progress, allowing the command to be immediately processed. All other connection commands in queue will also be processed for the same device, commands for other devices will be deferred until the next normally scheduled connection.

**Note** Some devices need to have the MAC address type specified. You can find this type by checking the log/MQTT data and looking for "mac_type". The mac_type of your device can be seen by setting `pubadvdata` to `true` with an MQTT command (see Publishing advertisement data), or with the macro `pubBLEAdvData true`. By default the type is 0 but some devices use different type values. You must specify the correct type to connect successfully.  
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

## SwitchBot Bot control

SwitchBot Bot devices are automatically discovered and available as a device in the configuration menu of home assistant.

::: tip 
If the SwitchBot mode is changed the ESP32 must be restarted. 
:::

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

## ADVANCED: Advertisement and advanced data (default: false)

If you want to enable this feature:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"pubadvdata":true}'`

This will publish extensive information about the device:
```json
{"id":"11:22:33:44:55:66","mac_type":0,"adv_type":0,"name":"Qingping Motion & Light","rssi":-93,"servicedata":"88121122334455660201520f0126090403000000","servicedatauuid":"0xfdcd","brand":"Qingping","model":"Motion & Light","model_id":"CGPR1","lux":3,"batt":82}
```

To stop publishing advertisement data:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoBT/config -m '{"pubadvdata":false}'`

::: warning Note
All product and company names are trademarks or registered trademarks of their respective holders. Use of them does not imply any affiliation with or endorsement by them.
:::

::: warning
We strongly encourage the use of a white-list (see below) so as to collect data from your devices only and not from other MAC addresses.
By default the gateway scans the advertizing BLE devices nearby with their MAC addresses. Depending on your country, it may be illegal to monitor networks for MAC addresses, especially on networks that you do not own. Please check your country's laws (for US Section 18 U.S. Code § 2511) - [discussion here](https://github.com/schollz/howmanypeoplearearound/issues/4).
:::
