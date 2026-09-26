# LoRa gateway

## Compatible parts

An ESP32 board with a LoRa module, such as a TTGO LoRa32 or Heltec WiFi LoRa
32 board.

With this kind of board there is no hardware modification needed.

The `heltec-wifi-lora-32-v4` environment supports the Heltec WiFi LoRa 32
V4/V4.3 hardware with its onboard SX1262, KCT8103L RF front end, and OLED.
The available hardware validation covers the 868 MHz V4.3 revision.

Build the V4 firmware with:

```bash
pio run -e heltec-wifi-lora-32-v4
```
