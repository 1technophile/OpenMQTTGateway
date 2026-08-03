# Getting Started

This guide walks you through the two most popular setups from start to finish, flashed **from your browser** and integrated with **Home Assistant**. No soldering, no development environment, about 15 minutes:

* **Bluetooth sensors** — any ESP32 board reading thermometers, plant sensors, scales...
* **RF devices (433/315/868/915MHz)** — a LILYGO LoRa32 board receiving weather stations, door sensors, PIR sensors...

Using another controller (OpenHAB, Node-RED...) or another technology (IR, LoRa)? Follow the same steps and take the detours linked along the way.

## How the pieces fit together

OpenMQTTGateway (OMG) is a firmware for your board that translates signals from your devices (Bluetooth sensors, RF remotes, weather stations...) into MQTT messages, and the other way around:

<OmgPipeline />

The **broker** is the message hub; the **controller** is the home automation software where you see and use the data.

## What you need

* **A board** — for **Bluetooth**, any ESP32 development board. For **RF**, a LILYGO® LoRa32 V2.1 or a Heltec LoRa V2 matching your devices' frequency — 433MHz for most European devices, 915MHz in North America (no soldering needed). See the [board guide](prerequisites/board.md) for other cases, or get a pre-flashed [Theengs Bridge](https://shop.theengs.io/) and skip the flashing step.
* **A data USB cable** — some cables are charge-only and won't work.
* **An MQTT broker** — see Step 1.
* **A device to observe** — e.g. a [compatible Bluetooth sensor](https://decoder.theengs.io/devices/devices.html) such as a Mi Flora, LYWSD03MMC, Govee or Inkbird thermometer, or any RF sensor around you (weather station, door or PIR sensor...).

## Step 1 — Set up the MQTT broker

If you use **Home Assistant**, the simplest option is the Mosquitto add-on:

1. In Home Assistant go to **Settings → Add-ons → Add-on store**, install **Mosquitto broker** and start it.
2. Install the **MQTT integration** (**Settings → Devices & services → Add integration → MQTT**) if it isn't set up already.
3. Create a dedicated user for the gateway in **Settings → People → Users** (e.g. `omg` — it doesn't need to be an administrator). The gateway will log in with it.

No Home Assistant? Install [Mosquitto](https://mosquitto.org/) on any always-on machine, or see the [broker page](prerequisites/broker.md) for alternatives.

## Step 2 — Flash the firmware from your browser

1. Open the [web installer](upload/web-install.md) in **Chrome, Edge or Opera** (Firefox and Safari don't support web flashing).
2. Connect the board to your computer with the USB cable.
3. Pick the firmware matching your board and use case — the two most flashed:
   * **Bluetooth** on a generic ESP32: <a href="./upload/board-selector.html?env=esp32dev-ble"><strong>esp32dev-ble</strong></a>
   * **RF** on a LILYGO LoRa32: <a href="./upload/board-selector.html?env=lilygo-rtl_433"><strong>lilygo-rtl_433</strong></a> (the radio frequency is configurable afterwards)
4. Click **Connect**, select the board's USB port, and let the installer finish.

Prefer a desktop tool, or need a custom configuration? See the other [upload options](upload/index.md).

## Step 3 — Connect the gateway to your WiFi and broker

After flashing, the gateway starts its own WiFi access point:

1. From your phone or computer, connect to the WiFi network named **OpenMQTTGateway** or starting with **OMG_** (no password, except for Theengs devices — see the [portal page](upload/portal.md)).
2. A configuration page opens (on Android, accept "stay connected without internet"). Click **Configure WiFi**.
3. Select your WiFi network and enter its password.
4. Enter your broker's address as MQTT server (for the Home Assistant add-on, `homeassistant.local` or the server IP), keep port `1883`, and fill in the MQTT username and password from Step 1.
5. Click **Save**. The gateway restarts and joins your network.

The full list of portal fields is described on the [configuration portal page](upload/portal.md).

## Step 4 — Check that it works

* In Home Assistant, go to **Settings → Devices & services → MQTT**: after a minute you should see a device named **OpenMQTTGateway**, created automatically through MQTT discovery.
* **Bluetooth**: bring a compatible sensor nearby — it appears as its own device with its measurements as entities. Some devices only advertise on an event: press a button or trigger a reading if nothing shows up.
* **RF**: supported devices appear automatically as soon as they transmit — weather stations send every minute or two, door and PIR sensors on an event, so trigger one to see it arrive.
* You can also watch the raw messages with [MQTT Explorer](https://mqtt-explorer.com/): connect to your broker and look for `home/OpenMQTTGateway/LWT` = `online` and messages under `home/OpenMQTTGateway/`.

Something not right? Head to the [troubleshooting page](upload/troubleshoot.md) or ask on the [community forum](https://community.openmqttgateway.com).

## Where to go next

* **More RF options** — other <a href="./upload/web-install.html?bridge=rf433">compatible boards and firmwares</a>, transmitting, frequency tuning, and protocol details in [RF hardware setup](setitup/rf.md) and [RF usage](use/rf.md).
* **Infrared** — wire an IR emitter/receiver following the [IR hardware setup](setitup/ir.md).
* **Tune the gateway** — the [WebUI](use/webui.md) and [gateway commands](use/gateway.md) let you configure it at runtime over MQTT.
* **Other controllers** — [OpenHAB](integrate/openhab3.md), [Node-RED](integrate/node_red.md), [Jeedom](integrate/jeedom.md), [AWS IoT](integrate/aws_iot.md).
* **Full Home Assistant details** — [Home Assistant integration](integrate/home_assistant.md).
