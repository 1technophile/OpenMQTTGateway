# Adding protocols

Adding your device protocol to OpenMQTTGateway increases interoperability and creates new use cases. Use the steps below to pick the right path and upstream your changes.

::: tip Before you open a PR
Read the [Development contributions guide](./development.md) for naming rules, QA, and PR steps. It keeps your protocol addition reviewable.
:::

## RF or IR
1. Implement or extend the protocol in the upstream libs first: [RCSwitch](https://github.com/1technophile/rc-switch) or [Pilight](https://github.com/pilight/pilight) for RF, and [IRRemoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) for IR.
2. Once merged (or while your PR is pending), wire it in OMG as a new decoder/mapping.
3. Add a short test sniff/build locally with the relevant environment (for example `esp32dev-all-test`).

## BLE
1. For BLE message decoding, OpenMQTTGateway uses [Theengs Decoder](https://decoder.theengs.io/). Submit new device decoders directly to the [GitHub repository](https://github.com/theengs/decoder).
2. After the decoder exists, ensure the device is reported correctly through OMG (advertising devices are supported without connection).
3. Build locally for a BLE-enabled environment and validate the payload.

Notes:
- We support reading **advertising** BLE devices (they broadcast regularly without a connection).
- Keep payloads compact and follow existing JSON fields for consistency.

