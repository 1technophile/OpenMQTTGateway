---
title: Dimplex Connex gateway
description: Control Dimplex Connex electric heaters from MQTT and Home Assistant with an ESP32 and an nRF24L01 module. Wiring, pairing heaters to the gateway, and setting zone temperatures.
---
# Dimplex Connex gateway (2.4 GHz)

The Connex gateway replaces or works alongside the Dimplex Connex wall controller. It talks to Connex heaters directly over their 2.4 GHz link, so each heater zone can be set from MQTT or Home Assistant.

## What you need

|Part|Notes|
|-|-|
|ESP32 dev board|Any ESP32 with the usual VSPI pins free|
|nRF24L01 or nRF24L01+ module|The PA+LNA version with an external antenna gives more range|

The Connex link runs at 250 kbps on channel 40 (2.440 GHz). There's no modification to the heaters.

## Wiring

|nRF24L01|ESP32|Build flag (default)|
|-|-|-|
|GND|GND|-|
|VCC|3V3|-|
|CE|GPIO25|`CONNEX_CE_GPIO` (25)|
|CSN|GPIO5|`CONNEX_CSN_GPIO` (5)|
|SCK|GPIO18|`CONNEX_SCK_GPIO` (18)|
|MOSI|GPIO23|`CONNEX_MOSI_GPIO` (23)|
|MISO|GPIO19|`CONNEX_MISO_GPIO` (19)|
|IRQ|not used|-|

Set a flag in your environment's `build_flags` to move a pin, for example `'-DCONNEX_CE_GPIO=26'`.

:::tip
nRF24L01 modules are sensitive to supply noise. A 10 µF capacitor across VCC and GND, soldered close to the module, solves most "radio not responding" and range problems.
:::

## Firmware

Build and upload the `esp32dev-connex` environment, see [Build from source](../upload/builds). Once the gateway is online, check the radio:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoCONNEXset -m '{}'`

The gateway answers on `home/OpenMQTTGateway/CONNEXtoMQTT/gateway`. Everything the gateway reports goes to this one retained topic:

```json
{"radio":"ok","controllerid":"","learning":false,"burst":200,"interval":3,"acks":10,"power":"low","zones":{}}
```

If `radio` reads `not responding`, check the wiring and the capacitor.

## Learning an existing wall controller

If you have a working Connex wall controller, the gateway can share it instead of pairing the heaters again. Have the gateway learn the wall controller's ID:

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoCONNEXset -m '{"learn":true}'`

Then change any zone on the wall controller within 2 minutes. The gateway saves the ID it hears and sends its own commands with that same ID. A heater only ever stores one controller ID, so it sees a single controller and follows both the wall controller and the gateway. From then on the gateway also reports changes made on the wall controller, with `"source":"controller"`. You can also set an ID by hand with `{"controllerid":"A1B2","save":true}`.

Skip this section if you have no wall controller, or want the gateway to use its own ID: pair the heaters instead.

## Pairing a heater

Each heater listens for one controller ID and one zone. Pairing gives a heater the gateway's controller ID and the zone you choose. The first pairing creates a random controller ID and saves it on the gateway.

1. On the heater, hold **V** for 3 seconds, then press **-**, **+**, **V**. The heater is now waiting for a controller for about 10 seconds.
2. Within those 10 seconds, send the zone (1-4) and a starting temperature:

   `mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoCONNEXset -m '{"pair":1,"temperature":20}'`

3. The heater display changes to the temperature you sent. If it doesn't, the 10 seconds probably ran out: repeat both steps.

Several heaters can share a zone: pair each one to the same zone number, one at a time. Heaters already paired in that zone follow the pairing temperature too.

:::warning
A heater that was paired before, to a wall controller or to another gateway, ignores a new pairing until it is unpaired. To unpair, hold **V** for 3 seconds, then press **V**, **+**, **-**. Then pair it again.
:::

## Setting a temperature

`mosquitto_pub -t home/OpenMQTTGateway/commands/MQTTtoCONNEX -m '{"zone":1,"temperature":20.5}'`

The temperature range is 0 to 30 °C in 0.5 °C steps, the same as the heater's own controls. Each command is sent as a burst of identical frames. The gateway then republishes its status, with the zone under `zones`:

```json
{"radio":"ok","controllerid":"4D06","learning":false,"burst":200,"interval":3,"acks":10,"power":"low",
 "zones":{"1":{"setpoint":20.5,"source":"gateway","sent":11,"acked":10,"status":"ok","age":0}}}
```

If no frame is acked (heater out of range or unpowered), the zone keeps its previous `setpoint` and the attempt is reported as `"status":"failed"` with `"failed_setpoint"`. A rejected command (invalid zone or temperature, no controller ID, radio not responding) publishes a non-retained status with an `"error"` field and sends nothing.

The protocol has no off command, and the wall controller doesn't have one either. Setting 0 °C (or 5 °C for frost protection) has the same effect.

With MQTT discovery enabled, each zone appears in Home Assistant as a climate entity, after a controller ID exists. Its setpoint is read from that zone's entry in the gateway status.

## What the gateway can and can't know

The heaters never transmit their own state. Keep this in mind when reading the gateway's messages:

- **`acked`** counts radio-level acknowledgements. They prove a heater's radio heard the frame, not that a heater accepted it. A heater that isn't paired still acks. The heater display is the real check.
- **Last setpoint, not current setpoint.** The gateway status lists the last setpoint sent or heard per zone, with `age` in seconds since it changed. A change made on the heater's own buttons isn't visible to the gateway.
- **After a reboot** the last setpoints are restored from flash and marked `"restored":true`, with `"age":null` because when they changed is unknown. The gateway doesn't resend them, so it never overrides a change made on a heater while the gateway was off.

## Configuration commands

All sent to `home/OpenMQTTGateway/commands/MQTTtoCONNEXset`:

|Payload|Effect|
|-|-|
|`{}`|Publish the gateway status|
|`{"pair":1,"temperature":20}`|Pair a heater in sync mode to zone 1|
|`{"learn":true}` / `{"learn":false}`|Start / stop learning a wall controller's ID|
|`{"controllerid":"A1B2"}`|Set the controller ID|
|`{"burst":200,"interval":3,"acks":10}`|Most frames per command, milliseconds between them, and how many acks end the burst early (0 = always send them all)|
|`{"power":"max"}`|Transmit power: `min`, `low`, `high` or `max` (or 0-3)|
|`{"save":true}`|Save controller ID, burst and power settings to flash|
|`{"erase":true}`|Erase the saved settings and last setpoints|

A single frame was enough for heaters close to the gateway. By default a command stops after 10 acked frames, and only sends up to 200 frames when acks are scarce, so a weak link gets more attempts. Acks come from whichever heater's radio answers, so with several heaters in a zone set `acks` higher, or to 0, if one of them misses commands.

## Build flags

Pin flags are in [Wiring](#wiring).

|Flag|Default|Purpose|
|-|-|-|
|`CONNEX_BURST_COUNT`|200|Frames per command|
|`CONNEX_BURST_INTERVAL_MS`|3|Milliseconds between frames|
|`CONNEX_BURST_ACKS`|10|Acked frames that end a burst early, 0 = full burst|
|`CONNEX_PA_LEVEL`|1 (low)|Transmit power, 0 = min to 3 = max|
|`CONNEX_MIN_TEMP`|0.0|Lowest accepted setpoint (°C)|
|`CONNEX_MAX_TEMP`|30.0|Highest accepted setpoint (°C)|
