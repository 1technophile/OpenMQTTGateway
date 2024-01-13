import os
import requests
import json
import argparse
import shutil

from common_wu import mf_temp32, mf_temp32c3, mf_temp8266, wu_temp_opt, wu_temp_p1, wu_temp_p2, wu_temp_p3, wu_temp_p4, wu_temp_end, vue_path, manif_path, cors_proxy, esp32_boot, mf_temp32s3

parser = argparse.ArgumentParser()
parser.add_argument('--dev', action='store_true')
parser.add_argument('repo', nargs='?', default='1technophile/OpenMQTTGateway')
args = parser.parse_args()
repo = args.repo
dev = args.dev

bin_path = 'toDeploy/'
manif_folder = "/firmware_build/"

if not os.path.exists(manif_path):
    os.makedirs(manif_path)

if dev:
    print('Generate Web Upload in dev mode')
    manif_folder = "/dev" + manif_folder
    # copy OTA latest version definition
    shutil.copy("scripts/latest_version_dev.json",
                manif_path + "latest_version_dev.json")
    # copy the binaries frombin_path to manif_path
    for name in os.listdir(bin_path):
        if '.bin' in name:
            shutil.copyfile(bin_path + name, (manif_path + name))
else:
    print('Generate Web Upload in release mode')
    # copy OTA latest version definition
    shutil.copy("scripts/latest_version.json", manif_path + "latest_version.json")
    release = requests.get('https://api.github.com/repos/' +
                        repo + '/releases/latest')
    rel_data = json.loads(release.text)
    if 'assets' in rel_data:
        assets = rel_data['assets']
        # Download assets into manif_path
        for item in range(len(assets)):
            name = assets[item]['name']
            if 'firmware.bin' in name:
                fw_bin = requests.get(assets[item]['browser_download_url'])
                filename = assets[item]['browser_download_url'].split('/')[-1]
                with open(manif_path + filename, 'wb') as output_file:
                    output_file.write(fw_bin.content)
                print('Downloaded: ' + filename)
            if 'partitions.bin' in name:
                part_bin = requests.get(assets[item]['browser_download_url'])
                filename = assets[item]['browser_download_url'].split('/')[-1]
                with open(manif_path + filename, 'wb') as output_file:
                    output_file.write(part_bin.content)
                print('Downloaded: ' + filename)
            if 'bootloader.bin' in name:
                bl_bin = requests.get(assets[item]['browser_download_url'])
                filename = assets[item]['browser_download_url'].split('/')[-1]
                with open(manif_path + filename, 'wb') as output_file:
                    output_file.write(bl_bin.content)
                print('Downloaded: ' + filename)
    else:
        print('Assets not found')
        os._exit(1)

if not os.path.exists(vue_path):
    os.makedirs(vue_path)

boot_bin = requests.get(esp32_boot)
filename = esp32_boot.split('/')[-1]
with open(manif_path + filename, 'wb') as output_file:
    output_file.write(boot_bin.content)

wu_file = open(vue_path + 'web-uploader.vue', 'w')
wu_file.write(wu_temp_p1)

for name in os.listdir(manif_path):
    if 'firmware.bin' in name  and ('esp32c3' not in name ) and ('esp32s3' not in name ) and ('esp32' in name or 'ttgo' in name or 'heltec' in name or 'thingpulse' in name or 'theengs' in name or 'lilygo' in name or 'shelly' in name or 'tinypico' in name):
        fw = name.split('-firmware')[0]
        man_file = fw + '.manifest.json'
        fwp_name = name.split('-firmware')[0] + '-partitions.bin'                           
        fwb_name = name.split('-firmware')[0] + '-bootloader.bin'
        mani_str = mf_temp32.substitute({'cp': cors_proxy, 'part': manif_folder + fwp_name.split('/')[-1], 'bin': manif_folder + name, 'bl': manif_folder + fwb_name, 'boot': manif_folder + esp32_boot.split('/')[-1]})

        with open(manif_path + man_file, 'w') as nf:
            nf.write(mani_str)

        wu_file.write(wu_temp_opt.substitute(
            {'mff': manif_folder + man_file, 'mfn': fw}))

        print('Created: ' + man_file)

wu_file.write(wu_temp_p2)

for name in os.listdir(manif_path):
    if 'firmware.bin' in name and ('esp32c3' in name ):
        fw = name.split('-firmware')[0]
        man_file = fw + '.manifest.json'
        fwp_name = name.split('-firmware')[0] + '-partitions.bin'
        fwb_name = name.split('-firmware')[0] + '-bootloader.bin'
        mani_str = mf_temp32c3.substitute({'cp': cors_proxy, 'part': manif_folder + fwp_name, 'bin': manif_folder + name, 'bl': manif_folder + fwb_name, 'boot': manif_folder + esp32_boot.split('/')[-1]})

        with open(manif_path + man_file, 'w') as nf:
            nf.write(mani_str)

        wu_file.write(wu_temp_opt.substitute(
            {'mff': manif_folder + man_file, 'mfn': fw}))

        print('Created: ' + man_file)

wu_file.write(wu_temp_p3)

for name in os.listdir(manif_path):
    if 'firmware.bin' in name and ('esp32s3' in name ):
        fw = name.split('-firmware')[0]
        man_file = fw + '.manifest.json'
        fwp_name = name.split('-firmware')[0] + '-partitions.bin'
        fwb_name = name.split('-firmware')[0] + '-bootloader.bin'
        mani_str = mf_temp32s3.substitute({'cp': cors_proxy, 'part': manif_folder + fwp_name, 'bin': manif_folder + name, 'bl': manif_folder + fwb_name, 'boot': manif_folder + esp32_boot.split('/')[-1]})

        with open(manif_path + man_file, 'w') as nf:
            nf.write(mani_str)

        wu_file.write(wu_temp_opt.substitute(
            {'mff': manif_folder + man_file, 'mfn': fw}))

        print('Created: ' + man_file)

wu_file.write(wu_temp_p4)

for name in os.listdir(manif_path):
    if 'firmware.bin' in name and ('nodemcu' in name or 'sonoff' in name or 'rf-wifi-gateway' in name or 'manual-wifi-test' in name or 'rfbridge' in name):
        fw = name.split('-firmware')[0]
        man_file = fw + '.manifest.json'
        mani_str = mf_temp8266.substitute(
            {'cp': cors_proxy, 'bin': manif_folder + name})

        with open(manif_path + man_file, 'w') as nf:
            nf.write(mani_str)

        wu_file.write(
            manif_folder + wu_temp_opt.substitute({'mff': manif_folder + man_file, 'mfn': fw}))

        print('Created: ' + man_file)

wu_file.write(wu_temp_end)
wu_file.close()
