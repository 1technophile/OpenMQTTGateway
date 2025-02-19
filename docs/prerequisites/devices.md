# Supported Devices

OpenMQTTGateway supports a wide range of devices across multiple communication protocols. Below are the categories and details of supported devices.

## Bluetooth (BLE) Devices

OpenMQTTGateway can scan all Bluetooth Low Energy (BLE) devices that advertise their data, making it ideal for presence detection. Additionally, it retrieves measurements from supported devices with minimal impact on battery life by reading advertisements. In some cases, the gateway briefly connects to retrieve specific parameters or control the device.

Currently, OpenMQTTGateway supports [more than 110 Bluetooth devices](https://decoder.theengs.io/devices/devices.html), including:

- **Mi Flora** (plant monitoring)
- **Xiaomi scales**
- **Inkbird, Govee, and ThermoPro** thermo-hygrometers and BBQ thermometers
- **SwitchBot** devices (monitoring and control)
- **And many more!**

For additional BLE modifications and custom firmware, check out:
- [ATC_MiThermometer (2)](https://github.com/atc1441/ATC_MiThermometer)
- [ATC_MiThermometer (3)](https://github.com/pvvx/ATC_MiThermometer)

![BLE Devices](../img/OpenMQTTGateway_devices_ble.png ':size=250%')

---

## Radio Frequency (RF) Devices

OpenMQTTGateway supports a broad range of **315MHz, 433MHz, and 815MHz** devices, primarily through [RTL_433_ESP](https://github.com/NorthernMan54/rtl_433_ESP). Additionally, it supports devices using the following chipsets:

- **SC5262 / SC5272**
- **HX2262 / HX2272**
- **PT2262 / PT2272**
- **EV1527 / RT1527 / FP1527 / HS1527**

For **RF2**, OpenMQTTGateway also supports **KaKu and Pilight**, covering an extensive range of devices listed in the [Pilight wiki](https://wiki.pilight.org/devices).

⚠ **Note:** RF, RF2, RTL_433 and Pilight cannot be enabled on the same board simultaneously.

![RF Boards](../img/OpenMQTTGateway_devices_rf1.png ':size=250%')  
![RF Boards](../img/OpenMQTTGateway_devices_rf2.png ':size=250%')  
![RF Boards](../img/OpenMQTTGateway_devices_rf3.png ':size=250%')

---

## Infrared (IR) Devices

OpenMQTTGateway supports infrared (IR) communication, enabling control of IR-based devices.

- Supported **ESP-based devices**: [List here](https://github.com/crankyoldgit/IRremoteESP8266/blob/master/SupportedProtocols.md)
- Supported **Arduino-based devices**: [List here](https://github.com/1technophile/OpenMQTTGateway/blob/6f73160d1421bebf2c1bbc9b8017978ff5b16520/main/config_IR.h#L123)

The gateway also supports **raw IR signal transmission** and **Global Cache IR sending**, greatly expanding its compatibility.

---

## LoRa Devices

LoRa support in OpenMQTTGateway is currently focused on **DIY and experimentation**. At this time, a few LoRa devices are known to be compatible with the gateway.