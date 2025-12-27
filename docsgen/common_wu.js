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


const cors_proxy = ''; // 'https://cors.bridged.cc/'
const esp32_boot = 'https://github.com/espressif/arduino-esp32/raw/2.0.7/tools/partitions/boot_app0.bin';

module.exports = {
  mf_temp32,
  mf_temp32c3,
  mf_temp32s3,
  mf_temp8266,
  cors_proxy,
  esp32_boot
};
