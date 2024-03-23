
import shutil
import os
import hashlib
Import("env")

def calculate_md5(filepath):
    md5_hash = hashlib.md5()
    with open(filepath, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            md5_hash.update(byte_block)
    return md5_hash.hexdigest()

def main():
    print(f"Pre build BLE library replacement script")
    source_file = 'lib/esp32-bt-lib/esp32/libbtdm_app.a'
    destination_file = os.path.join(env.GetProjectConfig().get("platformio", "packages_dir"), 'framework-arduinoespressif32', 'tools', 'sdk', 'esp32', 'ld', 'libbtdm_app.a')

    try:
        shutil.copyfile(source_file, destination_file)
        md5_hash = calculate_md5(destination_file)
        print(f"Successfully copied {source_file} to {destination_file}")
        print(f"MD5: {md5_hash}")
    except Exception as e:
        print(f"Error occurred: {e}")

main()
