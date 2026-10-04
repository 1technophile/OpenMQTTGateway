# Preserve custom LoRa frequency in Heltec V4 WebUI

## Goal and scope

The Heltec V4 upstream-support branch already accepts partial LoRa
configuration updates through MQTT and offers requested TX power from
4 through 28 dBm in WebUI. Its WebUI frequency selector offers only
868000000 and 915000000 Hz. When the running configuration is a custom
frequency such as 869525000 Hz, neither option is selected, so saving
the form to change TX power can submit 868000000 Hz as a side effect.

Fix this generic WebUI round trip in the clean Heltec V4 support branch.
Do not add the private WALK/BASE weather protocol, remote node control,
or a paired frequency-switch procedure to the upstream PR.

## Chosen design and alternatives

For SX1262 builds, show a numeric frequency field in integer Hz whose
value is the exact active `LORAConfig.frequency`; keep a short hint for
common 868/915 MHz values. A numeric field avoids silently replacing
arbitrary valid values with one of two presets. Retain the existing
selector for legacy SX127x builds to avoid unrelated UI changes.

Adding a temporary "custom" option to the old selector could preserve
the active value, but would not let users edit it. Requiring MQTT for
every custom frequency is functional yet leaves a misleading WebUI.

## Update behavior

Strictly parse the SX1262 WebUI frequency as an integer number of Hz;
reject empty, fractional, malformed, overflowing, or out-of-range
values. Pass a numeric JSON value to the existing transactional
`LORAConfig_update` path and its `validateSX1262Config` check. A failed
apply must leave the prior runtime/NVS configuration unchanged and show
a visible error rather than displaying the old state as a successful
save. A successful save persists the complete effective configuration
using the existing NVS path. Changing only TX power in WebUI must keep
the frequency and all other radio settings unchanged.

Keep the existing MQTT partial-update semantics: a command containing
only `txpower` changes only that field; `save:true` makes it persistent.
Do not add a second configuration source or change the wire topic.
The WebUI's hardware-valid frequency range is not a declaration that
every value is legal with every antenna or in every jurisdiction.

## Verification

Extend the production-handler host test using a running 869525000 Hz
SX1262 configuration. Verify that rendering shows exactly 869525000,
a full form submission changing only TX power to 18 dBm leaves frequency
at 869525000, updates the radio, and persists both settings. Verify a
reboot/load round trip, malformed and out-of-range input rejection,
radio-apply failure rollback, visible error feedback, and unchanged
legacy SX127x rendering. Run the complete Heltec V4 host-radio suite
and build the public V4 environment. Do not flash the private field
pair as part of this upstream UI correction.
