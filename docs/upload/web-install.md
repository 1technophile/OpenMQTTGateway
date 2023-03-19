# (Option 1) Upload from the web

[![Hits](https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fdocs.openmqttgateway.com%2Fupload%2Fweb-install.html&count_bg=%2379C83D&title_bg=%23555555&icon=&icon_color=%23E7E7E7&title=hits&edge_flat=false)](https://hits.seeyoufarm.com)

::: tip Running on a tablet or phone
If you want to use the BLE decoding capabilities of OpenMQTTGateway with a tablet or smartphone you can use [Theengs App](https://app.theengs.io/).
:::

::: warning Note
If you are on macOS and have a LilyGo LoRa32 V2.1 board, make sure you have the [correct driver for the CH9102 Serial Chip](https://github.com/WCHSoftGroup/ch34xser_macos) installed and selected in the popup when initiating the web install.
To finalise the driver installation don't forget the confirmation in the **Security** section of **System Preferences** after the restart.
The correct driver to then select in the popup of this web install is
`/dev/cu.wchusbserialXXXXXXXXXXX`
:::

You can upload the firmware to your ESP device directly from here.
1. Plug in your ESP to a USB port.
2. Select the firmware in the box below.
3. Click the install button and choose the port that the ESP is connected to.
4. Wait until the process is complete.
5. Once completed you can configure your [Wifi and MQTT credentials](portal.md)

<web-uploader/>

The auto-generated table below describes the libraries and the modules of each board configuration.
