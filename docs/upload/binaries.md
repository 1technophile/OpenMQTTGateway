# Upload Binaries
This section is useful if you want to directly flash your ESP from your desktop. Once flashed you can change  wifi and broker settings.
Nevertheless you will not be able to change advanced parameters, if you want to do so refer to [Upload from PlatformIO][pio] section.

Download the binary corresponding to your board and gateway [here](https://github.com/1technophile/OpenMQTTGateway/releases) from github and uncompress it.

## ESP32
* Download the bootloader [here](https://github.com/espressif/arduino-esp32/raw/master/tools/sdk/bin/bootloader_dio_80m.bin)
* Download the boot_app0 from [here](https://github.com/espressif/arduino-esp32/raw/master/tools/partitions/boot_app0.bin)
* Download the flash tool utility from espressif:
https://www.espressif.com/en/products/hardware/esp32/resources
* Uncompress the package
* Execute `flash_download_tools`
* Choose Developper mode
* Choose ESP32 DownloadTool
* Set the files and the address as below:
![Flash download tool](../img/OpenMQTTgateway_ESP32_binary_flash.png)
And set the parameters used by arduino IDE, we are able to upload to ESP32 a binary file containing OpenMQTTGateway.
* Set the config as above
* Connect your ESP32 board and select the COM port 
* Click on erase if it is your first upload
* If you see SYNC (above the START button) for more than 5 seconds, press the boot button of your ESP32
* Once done you can click on START button
* If you see SYNC (above the START button) for more than 5 seconds, press the boot button of your ESP32
The upload details appears in the rear shell windows, you can see also the progress bar changing.
* Once done the flash tool display "FINISH" like below
![Flash download tool 2](../img/OpenMQTTgateway_ESP32_binary_flash2.png)

## ESP8266
* Download the NodeMCU Py Flasher tool :
https://github.com/marcelstoer/nodemcu-pyflasher/releases
* Execute `NodeMCU-PyFlasher`
* Set the files and parameters as below:
![](../img/OpenMQTTgateway_NodeMCU_PyFlasher.png)
* Connect your board and select the COM port 
* Click on *FlashNodeMCU*
The upload details appears.

With an ESP if you did not set your network and mqtt parameters manually you can now open the [web portal configuration](portal.md).

Note that to reset the wifi and mqtt settings you can check *yes, wipes all data*
