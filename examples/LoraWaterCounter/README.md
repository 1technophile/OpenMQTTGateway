# OpenMQTTGateway LoRa Node Example
This repository contains an example of a LoRa node program designed for the ESP32 platform. 
The program reads the DHT22 temperature and humidity, the battery level and  water consumption via reed switch, packages the data into a JSON format, and sends it over LoRa.

## Features:
* Uses an SX12XX LoRa module.
* Displays packet sending status, counter and battery level data on an SSD1306 OLED display.
* Sends the ESP32's MAC address as the node ID.
* Sends temperature data in Celsius.

## Hardware Requirements:
* ESP32 development board.
* SX12XX LoRa module.
* SSD1306 OLED display.

## Pin Configuration: (Lilygo LoRa32 V2.1_1.6)
* SCK  - GPIO5
* MISO - GPIO19
* MOSI - GPIO27
* SS   - GPIO18
* RST  - GPIO23
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

If you use arduino load ino file.

## Data Format:
The data is sent in the following JSON format:

```json
{
  "model": "ESP32CNT",
  "id": "ESP32_MAC_ADDRESS",
  "count": "COUNTER_IN_LITER",
  "tempc": "TEMPERATURE_IN_CELSIUS",
  "hum": "HUMIDITY_IN_PERCENTAGE",
  "batt": "BATTERY_IN_PERCENTAGE"
}
```

## Troubleshooting:
LoRa Initialization Failed: Ensure that the SX1278 LoRa module is connected correctly and powered on.
OLED Display Not Working: Check the connections and ensure that the display is powered correctly.
No Temperature Data: Ensure that the ESP32's internal temperature sensor is functional.
Contributing:
Feel free to contribute to this example by opening issues or submitting pull requests. Any feedback or improvements are welcome!

I hope this README helps users understand and use your program! Adjustments can be made as necessary to fit any additional details or changes.
