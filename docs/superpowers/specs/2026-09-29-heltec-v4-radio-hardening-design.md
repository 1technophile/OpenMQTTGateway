# Heltec WiFi LoRa 32 V4.3 Radio Hardening Design

## Goal

Complete production-grade OpenMQTTGateway support for the Heltec WiFi LoRa 32
V4.3 board with ESP32-S3R2, SX1262, and KCT8103L. Correct the board-specific RF
initialization, expose the optional SX1262 receive boost through MQTT and the
WebUI, tighten configuration validation, and retain existing SX127x behavior.

## Scope

This design targets the 16 MB flash, 2 MB PSRAM Heltec V4.3 high-frequency
board with the KCT8103L front end. The supported RF range is 863-928 MHz, which
covers the 868 MHz and 915 MHz uses of this hardware.

Heltec V4.2 boards with the GC1109 front end and the newer V4 R8 board are not
covered. Documentation and environment descriptions must identify V4.3 and
KCT8103L explicitly instead of promising support for every V4 revision.

The private offline Xiaomi BLE firmware and its display screens are not part of
this upstream change.

## RF Initialization

RadioLib remains responsible for the standard SX1262 reset, BUSY handling,
TCXO setup, calibration, regulator selection, packet-engine setup, and LoRa
modulation configuration.

OpenMQTTGateway adds the V4.3-specific initialization that RadioLib cannot
infer from the board:

1. Keep SX1262 DIO2 RF-switch control enabled. DIO2 is wired to the KCT8103L
   `PA_CPS` input and must not be disabled after `radio.begin()`.
2. Set the SX1262 current limit to 140 mA so the configured 4-28 dBm board
   output range remains usable at the high end.
3. Apply the Heltec receive compatibility patch by setting bit 0 of SX1262
   register `0x08B5` before reception. This is mandatory board initialization,
   not a user-visible boost setting.
4. Keep the KCT8103L front-end sequence and measured PA gain conversion table.
   GPIO7 controls FEM power, GPIO2 controls CSD, GPIO5 controls CTX, and the
   existing `LORA_RX_LNA=0` value selects the normal V4.3 receive/LNA path.

Every RadioLib operation in the mandatory initialization is checked. A failure
prevents the radio from being marked ready and is reported through the existing
radio error path.

## Optional Receive Boost

Add `bool rxBoostedGain` to `LORAConfig_s`, represented in JSON and MQTT as
`rxboostedgain`.

- Default: `false`.
- MQTT configuration: accepted on the existing LoRa configuration and transmit
  command paths.
- MQTT state: included in the existing LoRa state object.
- WebUI: exposed as a V4.3/SX1262-only checkbox.
- Persistence: included in NVS save/load, reset by `init`, and removed through
  the existing `erase` behavior.

Applying a configuration calls RadioLib `setRxBoostedGainMode()` before the
radio returns to continuous receive. Old stored configurations that do not
contain `rxboostedgain` retain the initialized default `false`.

The external KCT8103L LNA remains in its normal enabled receive state. The new
setting controls only the optional boosted-gain mode inside the SX1262.

## Configuration Validation

SX1262/V4.3 validation accepts only:

- frequency: 863000000 through 928000000 Hz;
- requested board TX power: 4 through 28 dBm;
- spreading factor: 5 through 12;
- coding-rate denominator: 5 through 8;
- preamble length: 6 through 65535;
- signal bandwidth: 7800, 10400, 15500, 15600, 20800, 31250, 41700, 62500,
  125000, 250000, or 500000 Hz.

Both 15500 and 15600 Hz are accepted because existing OMG WebUI and
documentation use 15500 while RadioLib and SX1262 documentation commonly use
15.6 kHz. They select the same SX1262 bandwidth setting.

Validation remains conditional on `LORA_RADIO_SX1262`; legacy SX127x profiles
keep their existing ranges and behavior.

Configuration updates remain transactional. A candidate is validated first,
then applied. If hardware application fails, the previous configuration is
reapplied and the candidate is neither published as active nor persisted.

## WebUI and MQTT Behavior

For the V4.3 build, the WebUI presents:

- frequency choices suitable for 868 and 915 MHz operation;
- TX power choices from 4 through 28 dBm;
- the existing modulation controls;
- an `RX boosted gain` checkbox.

Legacy builds retain their current WebUI frequency and 0-14 dBm power choices.
The MQTT field name is `rxboostedgain` in commands, persisted JSON, and state
output. No topic names change.

## Board Metadata and Compatibility Cleanup

The board metadata will identify V4.3/KCT8103L precisely and include the normal
PlatformIO `platforms` declaration. Existing flash, PSRAM, USB, partition,
OLED, LED, and pin values remain unchanged.

The ESP32-S3 internal-temperature compatibility change will be scoped to newer
ESP32 targets so the original ESP32 implementation retains its historical
runtime behavior. This is a backward-compatibility cleanup, not a V4.3 feature.

## Verification

Host tests must prove:

- DIO2 remains enabled;
- the current limit is set to 140 mA;
- register `0x08B5` bit 0 is applied without destroying other bits;
- receive boost defaults off and can be enabled and disabled;
- every accepted boundary and bandwidth is valid and nearby invalid values are
  rejected;
- a hardware-application failure reports an error and preserves readiness or
  rollback semantics as appropriate;
- SX127x transmit, receive, and configuration behavior remains intact.

Build verification covers `heltec-wifi-lora-32-v4`, both TTGO LoRa profiles,
the original Heltec LoRa profile, and a representative ESP32 BLE profile.

Hardware verification uses both physical V4.3 boards and checks boot, PSRAM,
OLED, radio initialization logs, bidirectional packets, RSSI/SNR reporting, and
normal versus boosted receive settings. MQTT and WebUI are verified at build
and handler level while offline; a real broker round trip remains a later
acceptance step when Wi-Fi is deliberately enabled.

## Acceptance Criteria

The change is ready to leave draft state when:

1. all host tests and compatibility builds pass;
2. the upstream CI V4.3 job passes;
3. both boards initialize without RadioLib errors;
4. bidirectional LoRa communication works with DIO2 enabled;
5. MQTT and WebUI expose the same persisted `rxboostedgain` value;
6. documentation no longer overstates support for GC1109 or V4 R8 hardware.
