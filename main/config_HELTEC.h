/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

    HELTEC ESP32 LORA - SSD1306 / Onboard 0.96-inch 128*64 dot matrix OLED display

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

#ifndef config_HELTEC_h
#define config_HELTEC_h

#include <Arduino.h>
#include <Wire.h>

#include "SSD1306Wire.h"

#ifdef ZboardHELTEC
#  define OLED_WIDTH       128
#  define OLED_HEIGHT      64
#  define OLED_TEXT_BUFFER 1000
#  define OLED_TEXT_ROWS   5
#endif

extern void setupHELTEC();
extern void loopHELTEC();
size_t OledPrint(String msg);

/*-------------------DEFINE LOG LEVEL----------------------*/
#ifndef LOG_LEVEL_LCD
#  define LOG_LEVEL_LCD LOG_LEVEL_WARNING // Default to only display Notice level messages
#endif
#ifndef LOG_TO_LCD
#  define LOG_TO_LCD false // Default to not display log messages on display
#endif
/*-------------------DEFINE MQTT TOPIC FOR CONFIG----------------------*/
#define subjectMQTTtoHELTECset "/commands/MQTTtoHELTEC/config"

// Simple construct for displaying message in lcd and oled displays

#define displayPrint(...) \
  if (lowpowermode < 2) heltecPrint(__VA_ARGS__) // only print if not in low power mode
#define lpDisplayPrint(...) heltecPrint(__VA_ARGS__) // print in low power mode

void heltecPrint(char*, char*, char*);
void heltecPrint(char*, char*);
void heltecPrint(char*);

// This pattern was borrowed from HardwareSerial and modified to support the ssd1306 display

class OledSerial : public Stream {
private:
  void displayIntro(int scale, int displayWidth, int displayHeight);

public:
  OledSerial(int);
  void begin();
  void drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool line1, bool line2, bool name);

  SSD1306Wire* display;

  int available(void); // Dummy functions
  int peek(void); // Dummy functions
  int read(void); // Dummy functions
  void flush(void); // Dummy functions

  void fillScreen(OLEDDISPLAY_COLOR); // fillScreen display and set color

  // This is a bit of lazy programmer simplifacation for the semapore and core detecting code.  Not sure if it is truly space efficient.

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

#endif
