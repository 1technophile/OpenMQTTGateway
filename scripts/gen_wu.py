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

parser = argparse.ArgumentParser()
parser.add_argument('repo')
args = parser.parse_args()
repo = args.repo

manif_folder = "/firmware_build/"
manif_path = 'docs/.vuepress/public/firmware_build/'
vue_path = 'docs/.vuepress/components/'
cors_proxy = ''  # 'https://cors.bridged.cc/'
esp32_blurl = 'https://github.com/espressif/arduino-esp32/raw/2.0.5/tools/sdk/esp32/bin/bootloader_dio_80m.bin'
esp32_boot = 'https://github.com/espressif/arduino-esp32/raw/2.0.5/tools/partitions/boot_app0.bin'
release = requests.get('https://api.github.com/repos/' +
                       repo + '/releases/latest')
rel_data = json.loads(release.text)

if 'assets' in rel_data:
    assets = rel_data['assets']
else:
    print('Assets not found')
    os._exit(1)

if not os.path.exists(manif_path):
    os.makedirs(manif_path)

# copy OTA latest version definition
shutil.move("scripts/latest_version.json", manif_path + "latest_version.json")

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

for item in range(len(assets)):
    name = assets[item]['name']
    if 'firmware.bin' in name and ('esp32' in name or 'ttgo' in name or 'heltec' in name or 'thingpulse' in name or 'lilygo' in name or 'shelly' in name):
        fw = name.split('-firmware')[0]
        man_file = fw + '.manifest.json'
        fw_url = assets[item]['browser_download_url']
        fwp_url = fw_url.split('-firmware')[0] + '-partitions.bin'
        mani_str = mf_temp32.substitute({'cp': cors_proxy, 'part': manif_folder + fwp_url.split('/')[-1], 'bin': manif_folder + fw_url.split(
            '/')[-1], 'bl': manif_folder + esp32_blurl.split('/')[-1], 'boot': manif_folder + esp32_boot.split('/')[-1]})

        with open(manif_path + man_file, 'w') as nf:
            nf.write(mani_str)

        wu_file.write(wu_temp_opt.substitute(
            {'mff': manif_folder + man_file, 'mfn': fw}))

        fw_bin = requests.get(fw_url)
        filename = fw_url.split('/')[-1]
        with open(manif_path + filename, 'wb') as output_file:
            output_file.write(fw_bin.content)

        part_bin = requests.get(fwp_url)
        filename = fwp_url.split('/')[-1]
        with open(manif_path + filename, 'wb') as output_file:
            output_file.write(part_bin.content)

        print('Created: ' + os.path.abspath(man_file))

wu_file.write(wu_temp_p2)

for item in range(len(assets)):
    name = assets[item]['name']
    if 'firmware.bin' in name and ('nodemcu' in name or 'sonoff' in name or 'rf-wifi-gateway' in name or 'manual-wifi-test' in name or 'rfbridge' in name):
        fw = name.split('-firmware')[0]
        man_file = fw + '.manifest.json'
        fw_url = assets[item]['browser_download_url']
        mani_str = mf_temp8266.substitute(
            {'cp': cors_proxy, 'bin': manif_folder + fw_url.split('/')[-1]})

        with open(manif_path + man_file, 'w') as nf:
            nf.write(mani_str)

        wu_file.write(
            manif_folder + wu_temp_opt.substitute({'mff': manif_folder + man_file, 'mfn': fw}))
        fw_bin = requests.get(fw_url)
        filename = fw_url.split('/')[-1]
        with open(manif_path + filename, 'wb') as output_file:
            output_file.write(fw_bin.content)

        part_bin = requests.get(fwp_url)
        filename = fwp_url.split('/')[-1]
        with open(manif_path + filename, 'wb') as output_file:
            output_file.write(part_bin.content)
        print('Created: ' + os.path.abspath(man_file))

wu_file.write(wu_temp_end)
wu_file.close()
