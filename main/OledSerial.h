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

#ifndef OledSerial_h
#define OledSerial_h

// #include <inttypes.h>

#include "Stream.h"
#include "esp32-hal.h"

class OledSerial : public Stream {
private:
  void drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool line1, bool line2, bool name);

/*
Display OpenMQTTGateway logo
*/

  void displayIntro(int scale, int displayWidth, int displayHeight);

public:
  OledSerial(int uart_nr);

  void begin();

  int available(void);
  int peek(void);
  int read(void);

  void flush(void);

  size_t write(uint8_t);
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
  int _uart_nr;
  uart_t* _uart;
};

extern OledSerial Oled;

#endif // OledSerial_h
