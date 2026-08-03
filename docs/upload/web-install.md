---
aside: false
---

# (Option 1) Upload from the web

This is the easiest way to install OpenMQTTGateway: no software to download, no development environment.

## Before you flash

* Use a **data** USB cable — many cables are charge-only and the board won't show up with one.
* Use **Chrome, Edge or Opera** — Firefox and Safari don't support web flashing.
* Most boards need a **USB serial driver**. Recent operating systems often include it; if your board doesn't appear in the port list, see below.

::: details My board doesn't show up in the port list
1. Try another USB cable (data, not charge-only) and another USB port.
2. Install the driver matching your board's USB chip (check the board's product page if unsure):
   * **CP210x** (Silicon Labs): [driver download](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers)
   * **CH340 / CH341** (WCH): [driver download](https://www.wch-ic.com/downloads/CH341SER_ZIP.html)
   * **CH9102** (WCH): [macOS driver](https://github.com/WCHSoftGroup/ch34xser_macos), [Windows driver](https://www.wch-ic.com/downloads/CH343SER_ZIP.html)
3. On macOS, complete the driver installation by confirming it in the **Security** section of **System Preferences**, then restart. For a LilyGo LoRa32 V2.1 board, the correct port to select is `/dev/cu.wchusbserialXXXXXXXXXXX`.
4. Some boards need the **BOOT** button held down while plugging in the cable to enter flash mode.
5. On Linux, add your user to the `dialout` group (`sudo usermod -a -G dialout $USER`) and log in again.
:::

## Select your firmware
To upload firmware to your ESP device directly from this page, first connect your ESP device to a USB port on your computer. Then, choose the appropriate firmware from the available options. Next, click the **Connect** button and select the USB port where your ESP is plugged in. Wait for the upload process to finish. After the upload completes, you can set up your [WiFi and MQTT credentials](portal.md).

<BoardEnvironmentTable 
      boardsUrl="/boards-info.json"
      selectorPath="/upload/board-selector.html"
/>

<small style="color: var(--vp-c-text-3);">Board photos courtesy of their manufacturers. ESP32 Feather photo © <a href="https://www.adafruit.com/product/3405">Adafruit Industries</a>, <a href="https://creativecommons.org/licenses/by-sa/4.0/">CC BY-SA</a>.</small>


::: tip Running on a tablet or phone
If you want to use the BLE decoding capabilities of OpenMQTTGateway with a tablet or smartphone you can use [Theengs App](https://app.theengs.io/).
:::


## Next step

Once the upload is complete, connect the gateway to your WiFi network and MQTT broker through the [configuration portal](portal.md).

