# Heltec V4 Custom-Frequency WebUI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Preserve an active custom SX1262 frequency, including 869525000 Hz, when users save LoRa TX power through WebUI.

**Architecture:** Replace only the SX1262 frequency preset selector with an exact integer-Hz input. Parse its submitted value strictly, then reuse the existing transactional `LORAConfig_update`/NVS path; the WebUI reports rejected updates. Keep legacy SX127x rendering and MQTT partial updates unchanged.

**Tech Stack:** C++17, ArduinoJson, OpenMQTTGateway WebUI, RadioLib SX1262, native host harness, PlatformIO.

**Spec:** `docs/superpowers/specs/2026-10-04-heltec-v4-custom-frequency-webui-design.md`

## Global Constraints

- Public Heltec V4 support branch only; no private WALK/BASE weather protocol or paired retune.
- SX1262 WebUI must round-trip exact integer Hz; retain legacy SX127x selector behavior.
- Frequency input must reject malformed, fractional, empty, overflowing, and hardware-out-of-range values.
- Failed validation or radio apply must preserve prior runtime and NVS settings and show a visible WebUI error.
- MQTT `txpower` partial updates and `save:true` semantics remain unchanged.
- No flash of the private field pair as part of this correction.

## Review Focus

- Custom 869525000 Hz plus a TX-only form change must stay at 869525000 Hz: Task 1 full-form round-trip test.
- Empty/fractional/non-numeric/overflowing frequency must not become 0 or a prefix: Task 1 invalid-input test.
- SX1262-invalid 433000000 Hz must leave hardware and NVS untouched: Task 1 range-rejection test.
- Radio apply failure after partial hardware changes must restore the old settings and report failure: Task 1 injected-failure test.
- Legacy SX127x frequencies and power options must render and save as before: Task 1 legacy host run.

---

### Task 1: Exact SX1262 WebUI frequency round trip

**Files:**
- Modify: `main/config_WebContent.h` (LoRa frequency control and error slot)
- Modify: `main/webUI.cpp` (`handleLA` parsing, update result, SX1262 rendering)
- Test: `test/host/lora_gateway_config_test.cpp`

**Interfaces:**
- Consumes: `LORAConfig.frequency`, `LORAConfig_update(JsonObject&) -> bool`, existing `validateSX1262Config` through that update path.
- Produces: SX1262 `lf` numeric Hz input and visible rejected-save message; no new persisted field or MQTT topic.

- [ ] **Step 1: Write failing host assertions.** Set SX1262 config to 869525000 Hz, render `handleLA()`, and require the `lf` numeric input to contain `869525000`. Submit a full form with `lf=869525000`, `lt=18`, and unchanged other fields; require frequency/radio frequency to remain 869525000 Hz, power 18 dBm, and saved NVS JSON to contain both values. Reset defaults, load NVS, and require the same pair again. Add `{"txpower":18,"save":true}` partial-update regression with custom frequency. Assert that `lf` values `""`, `"869.525"`, `"869525000x"`, `"999999999999"`, and `"433000000"` leave config/NVS unchanged and render `LoRa configuration rejected`. Inject the existing fake RadioLib one-shot bandwidth failure during a submitted form and assert rollback plus the same error. Retain SX127x selector assertions.
- [ ] **Step 2: Run red test.** Run `bash test/host/run_heltec_v4_radio_tests.sh`; expect the new SX1262 custom-frequency assertion to fail on the current preset-only UI.
- [ ] **Step 3: Implement the WebUI change.** Under `LORA_RADIO_SX1262`, render an `<input type='number' name='lf'>` with exact `LORAConfig.frequency`, integer step, and the existing SX1262 hardware bounds. In `handleLA`, parse decimal digits without accepting signs, whitespace, fractions, trailing text, or overflow; store a numeric JSON value. Use `LORAConfig_update(WEBtoLORA)`'s bool result, call `stateLORAMeasures()` only after success, and place the static `LoRa configuration rejected` message inside the rendered page after failure. Keep the legacy selector and update behavior under the non-SX1262 branch.
- [ ] **Step 4: Run green host suite.** Run `bash test/host/run_heltec_v4_radio_tests.sh`; expect every SX1262 and SX127x host test to pass.
- [ ] **Step 5: Build the public board environment.** Run `pio run -e heltec-wifi-lora-32-v4`; expect exit 0 and inspect any warnings involving `handleLA` or `config_lora_body`.
- [ ] **Step 6: Review and commit this one task.** Inspect `git diff --check` and `git diff` for only the three listed source/test paths. Commit them with `fix(heltec): preserve custom LoRa frequency in WebUI`. Do not include unrelated or ignored files.
