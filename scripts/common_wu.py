import string

mf_temp32 = string.Template('''{
  "name": "OpenMQTTGateway",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32",
      "improv": false,
      "parts": [
        { "path": "$cp$bl", "offset": 4096 },
        { "path": "$cp$part", "offset": 32768 },
        { "path": "$cp$boot", "offset": 57344 },
        { "path": "$cp$bin", "offset": 65536 }
      ]
    }
  ]
}''')

mf_temp32c3 = string.Template('''{
  "name": "OpenMQTTGateway",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-C3",
      "improv": false,
      "parts": [
        { "path": "$cp$bl", "offset": 0 },
        { "path": "$cp$part", "offset": 32768 },
        { "path": "$cp$boot", "offset": 57344 },
        { "path": "$cp$bin", "offset": 65536 }
      ]
    }
  ]
}''')

mf_temp32s3 = string.Template('''{
  "name": "OpenMQTTGateway",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "improv": false,
      "parts": [
        { "path": "$cp$bl", "offset": 0 },
        { "path": "$cp$part", "offset": 32768 },
        { "path": "$cp$boot", "offset": 57344 },
        { "path": "$cp$bin", "offset": 65536 }
      ]
    }
  ]
}''')

mf_temp8266 = string.Template('''{
  "name": "OpenMQTTGateway",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP8266",
      "parts": [{ "path": "$cp$bin", "offset": 0 }]
    }
  ]
}''')

wu_temp_opt = string.Template('''
        <option
          value="$mff"
        >
        $mfn
        </option>
        ''')

wu_temp_p1 = '''<template>
  <div align="center">
    <select>
      <optgroup label="ESP32">'''

wu_temp_p2 = '''
      </optgroup>
      <optgroup label="ESP32C3">'''

wu_temp_p3 = '''
      </optgroup>
      <optgroup label="ESP32S3">'''

wu_temp_p4 = '''
      </optgroup>
      <optgroup label="ESP8266">'''

wu_temp_end = '''
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
</script>'''

manif_path = 'docs/.vuepress/public/firmware_build/'
vue_path = 'docs/.vuepress/components/'
cors_proxy = ''  # 'https://cors.bridged.cc/'
esp32_boot = 'https://github.com/espressif/arduino-esp32/raw/2.0.7/tools/partitions/boot_app0.bin'
