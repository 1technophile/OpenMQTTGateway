--- 
pageClass: table-generated-page
---

## Select your firmware
To upload firmware to your ESP device directly from this page, first connect your ESP device to a USB port on your computer. Then, choose the appropriate firmware from the available options. Next, click the **Connect** button and select the USB port where your ESP is plugged in. Wait for the upload process to finish. After the upload completes, you can set up your [WiFi and MQTT credentials](portal.md).

<BoardEnvironmentTable 
      boardsUrl="/boards-info.json"
      selectorPath="/upload/board-selector.html"
/>


::: tip Running on a tablet or phone
If you want to use the BLE decoding capabilities of OpenMQTTGateway with a tablet or smartphone you can use [Theengs App](https://app.theengs.io/).
:::

::: warning Note
If you are on macOS and have a LilyGo LoRa32 V2.1 board, make sure you have the [correct driver for the CH9102 Serial Chip](https://github.com/WCHSoftGroup/ch34xser_macos) installed and selected in the popup when initiating the web install.
To finalise the driver installation don't forget the confirmation in the **Security** section of **System Preferences** after the restart.
The correct driver to then select in the popup of this web install is
`/dev/cu.wchusbserialXXXXXXXXXXX`
:::




## Using OpenMQTTGateway ?
Support open-source development through sponsorship and gain exclusive access to our private forum. Your questions, issues, and feature requests will receive priority attention, plus you'll gain insider access to our roadmap.

<div style="text-align: center;">
    <iframe src="https://github.com/sponsors/theengs/button" title="Sponsor Theengs" height="32" width="228" style="border: 0; border-radius: 6px;"></iframe>
</div>


