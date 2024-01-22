# OpenMQTTGateway LoRa Node Example
This repository contains an example of a LoRa node program designed for the ESP32 platform. The program reads the internal temperature of the ESP32, packages the data into a JSON format, and sends it over LoRa.
It also sends a constant raw string simulating a Makerfab Soil sensor payload.

## Features:
* Uses an SX12XX LoRa module.
* Displays packet sending status and temperature data on an SSD1306 OLED display.
* Sends the ESP32's MAC address as the node ID.
* Sends temperature data in Celsius.

## Hardware Requirements:
* ESP32 development board.
* SX12XX LoRa module.
* SSD1306 OLED display.

## Pin Configuration:
* SCK  - GPIO5
* MISO - GPIO19
* MOSI - GPIO27
* SS   - GPIO18
* RST  - GPIO14
* DI0  - GPIO26

## Setup:
### Hardware Setup:

Connect the SX1278 LoRa module and the SSD1306 OLED display to the ESP32 according to the pin configuration.
Ensure that the OLED display is powered correctly.

### Software Setup:

* Clone this repository.
* Open the provided node program with PlatformIO
* Upload the program to your ESP32.

## Usage:
Power on the ESP32.
The OLED display will show the status of the packet being sent and the current temperature reading.
The built-in LED on the ESP32 will blink once every time a packet is sent.
Monitor the serial output (at 115200 baud rate) to see the JSON formatted data being sent.

## Data Format:
The data is sent in the following JSON format:

```json
{
  "model": "ESP32TEMP",
  "id": "ESP32_MAC_ADDRESS",
  "tempc": "TEMPERATURE_IN_CELSIUS"
}
```

## Troubleshooting:
LoRa Initialization Failed: Ensure that the SX1278 LoRa module is connected correctly and powered on.
OLED Display Not Working: Check the connections and ensure that the display is powered correctly.
No Temperature Data: Ensure that the ESP32's internal temperature sensor is functional.
Contributing:
Feel free to contribute to this example by opening issues or submitting pull requests. Any feedback or improvements are welcome!

I hope this README helps users understand and use your program! Adjustments can be made as necessary to fit any additional details or changes.