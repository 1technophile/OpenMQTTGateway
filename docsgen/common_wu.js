// Common templates and constants for web installer manifest generation
// Used by: scripts/gen_wu.js

const mf_temp32 = (vars) => `{
  "name": "OpenMQTTGateway",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "improv": false,
      "parts": [
        { "path": "${vars.cp}${vars.bl}", "offset": 4096 },
        { "path": "${vars.cp}${vars.part}", "offset": 32768 },
        { "path": "${vars.cp}${vars.boot}", "offset": 57344 },
        { "path": "${vars.cp}${vars.bin}", "offset": 65536 }
      ]
    }
  ]
}`;

const mf_temp32c3 = (vars) => `{
  "name": "OpenMQTTGateway",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-C3",
      "improv": false,
      "parts": [
        { "path": "${vars.cp}${vars.bl}", "offset": 0 },
        { "path": "${vars.cp}${vars.part}", "offset": 32768 },
        { "path": "${vars.cp}${vars.boot}", "offset": 57344 },
        { "path": "${vars.cp}${vars.bin}", "offset": 65536 }
      ]
    }
  ]
}`;

const mf_temp32s3 = (vars) => `{
  "name": "OpenMQTTGateway",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "improv": false,
      "parts": [
        { "path": "${vars.cp}${vars.bl}", "offset": 0 },
        { "path": "${vars.cp}${vars.part}", "offset": 32768 },
        { "path": "${vars.cp}${vars.boot}", "offset": 57344 },
        { "path": "${vars.cp}${vars.bin}", "offset": 65536 }
      ]
    }
  ]
}`;

const mf_temp8266 = (vars) => `{
  "name": "OpenMQTTGateway",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP8266",
      "parts": [{ "path": "${vars.cp}${vars.bin}", "offset": 0 }]
    }
  ]
}`;

const wu_temp_opt = (vars) => `
        <option
          value="${vars.mff}"
        >
        ${vars.mfn}
        </option>
        `;

const wu_temp_p1 = `<template>
  <div align="center">
    <select>
      <optgroup label="ESP32">`;

const wu_temp_p2 = `
      </optgroup>
      <optgroup label="ESP32C3">`;

const wu_temp_p3 = `
      </optgroup>
      <optgroup label="ESP32S3">`;

const wu_temp_p4 = `
      </optgroup>
      <optgroup label="ESP8266">`;

const wu_temp_end = `
      </optgroup>
    </select><br><br>
    <esp-web-install-button erase-first></esp-web-install-button>
  </div>
</template>

<script>
export default {
  mounted () {
    const espWebInstallButton = document.querySelector("esp-web-install-button");
    espWebInstallButton.addEventListener("state-changed", (ev) => { console.log(ev.detail) });
    const selectFW = document.querySelector("select");
    espWebInstallButton.manifest = selectFW.value;
    selectFW.addEventListener("change", () => {
      espWebInstallButton.manifest = selectFW.value;
    });
  }
}
</script>`;

const manif_path = 'docs/.vuepress/public/firmware_build/';
const vue_path = 'docs/.vuepress/components/';
const cors_proxy = ''; // 'https://cors.bridged.cc/'
const esp32_boot = 'https://github.com/espressif/arduino-esp32/raw/2.0.7/tools/partitions/boot_app0.bin';

module.exports = {
  mf_temp32,
  mf_temp32c3,
  mf_temp32s3,
  mf_temp8266,
  wu_temp_opt,
  wu_temp_p1,
  wu_temp_p2,
  wu_temp_p3,
  wu_temp_p4,
  wu_temp_end,
  manif_path,
  vue_path,
  cors_proxy,
  esp32_boot
};
