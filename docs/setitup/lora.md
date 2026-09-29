# LoRa gateway

## Compatible parts

An ESP32 board with a LoRa module, such as a TTGO LoRa32 or Heltec WiFi LoRa
32 board.

With this kind of board there is no hardware modification needed.

The `heltec-wifi-lora-32-v4` environment supports the Heltec WiFi LoRa 32
V4.3 high-frequency hardware with its onboard SX1262, KCT8103L RF front end,
16 MB flash, 2 MB QSPI PSRAM, and OLED. The supported RF range is 863-928 MHz.
V4.2 with the GC1109 front end and V4 R8 are not covered by this profile.
The available hardware validation covers the 868 MHz V4.3 revision.

Build the V4 firmware with:

```bash
pio run -e heltec-wifi-lora-32-v4
```
