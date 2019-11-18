# Upload Binaries
This section is usefull if you want to directly flash your ESP from your desktop. Once flashed you can change  wifi and broker settings.
Nevertheless you will not be able to change advanced parameters, if you want to do so refer to [Upload from PlatformIO][pio] section.

Download the binary corresponding to your board and gateway [here](https://github.com/1technophile/OpenMQTTGateway/releases) from github and uncompress it.

## ESP32
* Download the bootloader [here](https://github.com/espressif/arduino-esp32/raw/master/tools/sdk/bin/bootloader_dio_80m.bin)
* Download the boot_app0 from [here](https://github.com/espressif/arduino-esp32/raw/master/tools/partitions/boot_app0.bin)
* Download the flash tool utility from espressif:
https://www.espressif.com/en/products/hardware/esp32/resources
* Uncompress the package
* Execute `flash_download_tools`
* Choose ESP32 DownloadTool
* Set the files and the adress as below:
![Flash download tool](../img/OpenMQTTgateway_ESP32_binary_flash.png)
And set the parameters used by arduino IDE, we are able to upload to ESP32 a binary file containing OpenMQTTGateway.
* Set the config as above
* Connect your ESP32 board and select the COM port 
* Click on start
The upload details appears in the rear shell windows, you can see also the progress bar changing
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

Note that to reset the wifi and mqtt settings you can check *yes, wipes all data*

Once loaded into your board you have to set your network parameters with wifi manager portal
From your smartphone search for your OpenMQTTGateway wifi network and connect to it, a web page will appear
* Select your wifi
* Set your wifi password
* Set your MQTT Server IP
* Set your MQTT Server username (not compulsory)
* Set your MQTT Server password (not compulsory)

The ESP restart and connect to your network. Note that your credentials are saved into the ESP memory, if you want to redo the configuration you have to erase the ESP memory with the flash download tool.

_The default password for wifi manager is "your_password"_

Once done the gateway should connect to your network and your broker, you should see it into the broker in the form of the following messages:
```
home/OpenMQTTGateway/LWT Online 
home/OpenMQTTGateway/version
```
