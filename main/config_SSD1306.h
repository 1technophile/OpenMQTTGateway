/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

    Supported boards with displays

    HELTEC ESP32 LORA - SSD1306 / Onboard 0.96-inch 128*64 dot matrix OLED display
    LILYGO® LoRa32 V2.1_1.6.1 433 Mhz / https://www.lilygo.cc/products/lora3?variant=42476923879605

    Copyright: (c)Florian ROBERT

    Contributors:
    - 1technophile
    - NorthernMan54

    This file is part of OpenMQTTGateway.

    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef config_SSD1306_h
#define config_SSD1306_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

#include "SSD1306Wire.h"

/*-------------------DEFINE LOG LEVEL----------------------*/

#ifndef LOG_LEVEL_LCD
#  define LOG_LEVEL_LCD LOG_LEVEL_WARNING // Default to only display Warning level messages
#endif

#ifndef LOG_TO_LCD
#  define LOG_TO_LCD false // Default to not display log messages on display
#endif

#ifndef JSON_TO_LCD
#  define JSON_TO_LCD true // Default to displaying JSON messages on the display
#endif

#ifndef DISPLAY_PAGE_INTERVAL
#  define DISPLAY_PAGE_INTERVAL 3 // Number of seconds between json message displays
#endif

#ifndef DISPLAY_IDLE_LOGO
#  define DISPLAY_IDLE_LOGO true // Display the OMG logo when idle
#endif

#ifndef DISPLAY_METRIC
#  define DISPLAY_METRIC true // Units used for display of sensor data
#endif

#ifndef DISPLAY_FLIP
#  define DISPLAY_FLIP true // Flip display orientation
#endif

#ifndef DISPLAY_STATE
#  define DISPLAY_STATE true // set to false if you don't want to use the display
#endif

#ifndef DISPLAY_BRIGHTNESS
#  define DISPLAY_BRIGHTNESS 50 // 0-100; 50 % brightness as default
#endif

/*------------------- DEFAULT DISPLAY GEOMETRY ----------------------*/

#define OLED_TEXT_BUFFER 1000
#define OLED_TEXT_ROWS   5
#define OLED_WIDTH       128
#define OLED_HEIGHT      64

// Display layout definitions based on board definition

#if defined(WIFI_LoRa_32_V2) // from ~/.platformio/packages/framework-arduinoespressif32/variants/.../pins_arduino.h
#  define OLED_WIDTH  DISPLAY_WIDTH
#  define OLED_HEIGHT DISPLAY_HEIGHT
#elif defined(ARDUINO_TTGO_LoRa32_v21new)
#  define OLED_WIDTH  128
#  define OLED_HEIGHT 64
#endif

#define OLED_TEXT_WIDTH OLED_WIDTH / 4 // This is an approx amount

/*-------------------DEFINE MQTT TOPIC FOR CONFIG----------------------*/

#define subjectMQTTtoSSD1306set "/commands/MQTTtoSSD1306/config"
#define subjectSSD1306toMQTT    "/SSD1306toMQTT"

/*-------------------EXTERNAL FUNCTIONS----------------------*/

extern void setupSSD1306();
extern void loopSSD1306();
extern void MQTTtoSSD1306(char*, JsonObject&);

// Simple construct for displaying message in lcd and oled displays

#define displayPrint(...) \
  if (lowpowermode < 2) ssd1306Print(__VA_ARGS__) // only print if not in low power mode
#define lpDisplayPrint(...) ssd1306Print(__VA_ARGS__) // print in low power mode

void ssd1306Print(char*, char*, char*);
void ssd1306Print(char*, char*);
void ssd1306Print(char*);

#define pubOled(...) ssd1306PubPrint(__VA_ARGS__)
void ssd1306PubPrint(const char*, JsonObject&);

// Structure for queueing OMG messages to the display

/*
Structure for queueing OMG messages to the display.
Length of each line is OLED_TEXT_WIDTH
- title
- line1
- line2
- line3
- line4
*/
struct displayQueueMessage {
  char title[OLED_TEXT_WIDTH];
  char line1[OLED_TEXT_WIDTH];
  char line2[OLED_TEXT_WIDTH];
  char line3[OLED_TEXT_WIDTH];
  char line4[OLED_TEXT_WIDTH];
};

// This pattern was borrowed from HardwareSerial and modified to support the ssd1306 display

class OledSerial : public Stream {
public:
  OledSerial(int);
  void begin();
  void drawLogo(int xshift, int yshift);
  boolean displayPage(displayQueueMessage*);

  SSD1306Wire* display;

  int available(void); // Dummy functions
  int peek(void); // Dummy functions
  int read(void); // Dummy functions
  void flush(void); // Dummy functions

  void fillScreen(OLEDDISPLAY_COLOR); // fillScreen display and set color

  // This is a bit of lazy programmer simplification for the semaphore and core detecting code.  Not sure if it is truly space efficient.

  inline size_t write(uint8_t x) {
    return write(&x, 1);
  }

  size_t write(const uint8_t* buffer, size_t size);
  inline size_t write(const char* buffer, size_t size) {
    return write((uint8_t*)buffer, size);
  }
  inline size_t write(const char* s) {
    return write((uint8_t*)s, strlen(s));
  }
  inline size_t write(unsigned long n) {
    return write((uint8_t)n);
  }
  inline size_t write(long n) {
    return write((uint8_t)n);
  }
  inline size_t write(unsigned int n) {
    return write((uint8_t)n);
  }
  inline size_t write(int n) {
    return write((uint8_t)n);
  }

protected:
};

extern OledSerial Oled;

/*------------------- Unit Conversion Functions ----------------------*/

#define convert_kmph2mph(kmph) (kmph * (1.0f / 1.609344f))

#define convert_mph2kmph(mph) (mph * 1.609344f)

#define convert_mm2inch(mm) (mm * 0.039370f)

#define convert_inch2mm(inch) (inch * 25.4f)

#define convert_kpa2psi(kpa) (kpa * (1.0f / 6.89475729f))

#define convert_psi2kpa(psi) (psi * 6.89475729f)

#define convert_hpa2inhg(hpa) (hpa * (1.0f / 33.8639f))

#define convert_inhg2hpa(inhg) (inhg * 33.8639f)

#endif
