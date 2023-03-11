import os
import requests
import json
import string
import argparse
import shutil

mf_temp32 = string.Template('''{
  "name": "OpenMQTTGateway",
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

mf_temp8266 = string.Template('''{
  "name": "OpenMQTTGateway",
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
      <optgroup label="ESP8266">'''

wu_temp_end = '''
      </optgroup>
    </select><br><br>
    <input type="checkbox" id="erase" checked>
    <label for="erase"> Erase Flash (erases all saved data)</label><br><br>
    <esp-web-install-button erase-first></esp-web-install-button>
  </div>
</template>

<script>
export default {
  mounted () {
    const espWebInstallButton = document.querySelector("esp-web-install-button");
    const eraseCheck = document.getElementById("erase");
    espWebInstallButton.addEventListener("state-changed", (ev) => { console.log(ev.detail) });
    const selectFW = document.querySelector("select");
    espWebInstallButton.manifest = selectFW.value;
    selectFW.addEventListener("change", () => {
      espWebInstallButton.manifest = selectFW.value;
    });
    eraseCheck.addEventListener("change", (ev) => {
      if (eraseCheck.checked) {
        espWebInstallButton.setAttribute('erase-first','');
      } else {
        espWebInstallButton.removeAttribute('erase-first');
      }
    });
  }
}
</script>'''

manif_folder = "/dev/firmware_build/"
manif_path = 'docs/.vuepress/public/firmware_build/'
vue_path = 'docs/.vuepress/components/'
bin_path = 'toDeploy/'
cors_proxy = ''  # 'https://cors.bridged.cc/'
esp32_blurl = 'https://github.com/espressif/arduino-esp32/raw/2.0.5/tools/sdk/esp32/bin/bootloader_dio_80m.bin'
esp32_boot = 'https://github.com/espressif/arduino-esp32/raw/2.0.5/tools/partitions/boot_app0.bin'

if not os.path.exists(manif_path):
    os.makedirs(manif_path)

# copy OTA latest version definition
shutil.move("scripts/latest_version_dev.json",
            manif_path + "latest_version_dev.json")

if not os.path.exists(vue_path):
    os.makedirs(vue_path)

wu_file = open(vue_path + 'web-uploader.vue', 'w')
wu_file.write(wu_temp_p1)

bl_bin = requests.get(esp32_blurl)
filename = esp32_blurl.split('/')[-1]
with open(manif_path + filename, 'wb') as output_file:
    output_file.write(bl_bin.content)

boot_bin = requests.get(esp32_boot)
filename = esp32_boot.split('/')[-1]
with open(manif_path + filename, 'wb') as output_file:
    output_file.write(boot_bin.content)

for name in os.listdir(bin_path):
    if 'firmware.bin' in name and ('esp32' in name or 'ttgo' in name or 'heltec' in name or 'thingpulse' in name or 'lilygo' in name or 'shelly' in name):
        fw = name.split('-firmware')[0]
        man_file = fw + '.manifest.json'
        print('Bin name:' + name)
        part_name = name.split('-firmware')[0] + '-partitions.bin'
        print('Partition name:' + part_name)
        mani_str = mf_temp32.substitute({'cp': cors_proxy, 'part': manif_folder + part_name.split('/')[-1], 'bin': manif_folder + name.split(
            '/')[-1], 'bl': manif_folder + esp32_blurl.split('/')[-1], 'boot': manif_folder + esp32_boot.split('/')[-1]})

        with open(manif_path + man_file, 'w') as nf:
            nf.write(mani_str)

        wu_file.write(wu_temp_opt.substitute(
            {'mff': manif_folder + man_file, 'mfn': fw}))

        shutil.copyfile(bin_path + name, (manif_path + name))
        shutil.copyfile(bin_path + part_name, (manif_path + part_name))

        print('Created: ' + os.path.abspath(man_file))

wu_file.write(wu_temp_p2)

for name in os.listdir(bin_path):
    if 'firmware.bin' in name and ('nodemcu' in name or 'sonoff' in name or 'rf-wifi-gateway' in name or 'manual-wifi-test' in name or 'rfbridge' in name):
        fw = name.split('-firmware')[0]
        man_file = fw + '.manifest.json'
        print('Bin name:' + name)
        part_name = name.split('-firmware')[0] + '-partitions.bin'
        print('Partition name:' + part_name)
        mani_str = mf_temp8266.substitute(
            {'cp': cors_proxy, 'bin': manif_folder + name.split('/')[-1]})

        with open(manif_path + man_file, 'w') as nf:
            nf.write(mani_str)

        wu_file.write(
            manif_folder + wu_temp_opt.substitute({'mff': manif_folder + man_file, 'mfn': fw}))

        shutil.copyfile(bin_path + name, (manif_path + name))

        print('Created: ' + os.path.abspath(man_file))

wu_file.write(wu_temp_end)
wu_file.close()
