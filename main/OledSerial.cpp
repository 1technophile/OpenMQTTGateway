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


#include "OledSerial.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config_HELTEC.h"
#include "heltec.h"
#include "pins_arduino.h"

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SERIAL)
OledSerial Oled(0);
// OledSerial Oled1(1);
// OledSerial Oled2(2);
#endif

OledSerial::OledSerial(int uart_nr) : _uart_nr(uart_nr), _uart(NULL) {}


void OledSerial::begin() {
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, false /*Serial Enable*/); // User OMG serial support
  Heltec.display->setFont(ArialMT_Plain_10);

  Heltec.display->setColor(WHITE);
  Heltec.display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
  Heltec.display->display();
  displayIntro(OLED_WIDTH * 0.24, (OLED_WIDTH / 2) - OLED_WIDTH * 0.2, (OLED_HEIGHT / 2) + OLED_HEIGHT * 0.2);
  Heltec.display->setLogBuffer(5, 40);
}

int OledSerial::available(void) {
  // return uartAvailable(_uart);
  return 1;
}

int OledSerial::peek(void) {
  if (available()) {
    return 1;
  }
  return -1;
}

int OledSerial::read(void) {
  if (available()) {
    return 1;
  }
  return -1;
}


void OledSerial::flush(void) {
  // uartFlush(_uart);
}

size_t OledSerial::write(uint8_t c) {
  Heltec.display->clear();
  Heltec.display->setColor(WHITE);
  Heltec.display->setFont(ArialMT_Plain_10);

  Heltec.display->print((char)c);
  Heltec.display->drawLogBuffer(0, 0);
  Heltec.display->display();
  return 1;
}

size_t OledSerial::write(const uint8_t* buffer, size_t size) {
  // uartWriteBuf(_uart, buffer, size);
  Serial.print("!");
  Heltec.display->clear();
  Heltec.display->setColor(WHITE);
  Heltec.display->setFont(ArialMT_Plain_10);

  // Heltec.display->print(buffer, size);
  Heltec.display->drawLogBuffer(0, 0);
  Heltec.display->display();
  return size;
}

void displayIntro(int i, int X, int Y) {
  drawLogo(i, X, Y, false, true, false, false, false, false); // Circle 2
  drawLogo(i, X, Y, false, false, true, false, false, false); // Circle 3
  drawLogo(i, X, Y, false, true, true, true, false, false); // Line 1
  drawLogo(i, X, Y, false, true, true, false, true, false); // Line 2
  drawLogo(i, X, Y, true, true, true, true, true, false); // Circle 1
  drawLogo(i, X, Y, true, true, true, true, true, true); // Name
}

void drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool line1, bool line2, bool name) {
  int circle1T = logoSize / 15;
  int circle2T = logoSize / 25;
  int circle3T = logoSize / 30;

  int circle3Y = circle1Y - (logoSize * 1.2);
  int circle3X = circle1X - (logoSize * 0.13);
  int circle2X = circle1X - (logoSize * 1.05);
  int circle2Y = circle1Y - (logoSize * 0.8);

  if (line1) {
    Heltec.display->setColor(BLACK);
    Heltec.display->drawLine(circle1X - 2, circle1Y, circle2X - 2, circle2Y);
    Heltec.display->drawLine(circle1X - 1, circle1Y, circle2X - 1, circle2Y);
    Heltec.display->drawLine(circle1X, circle1Y, circle2X, circle2Y);
    Heltec.display->drawLine(circle1X + 1, circle1Y, circle2X + 1, circle2Y);
    Heltec.display->drawLine(circle1X + 2, circle1Y, circle2X + 2, circle2Y);
    Heltec.display->setColor(WHITE);
    Heltec.display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2); // , WHITE);
  }
  if (line2) {
    Heltec.display->setColor(BLACK);
    Heltec.display->drawLine(circle1X - 2, circle1Y, circle3X - 2, circle3Y);
    Heltec.display->drawLine(circle1X - 1, circle1Y, circle3X - 1, circle3Y);
    Heltec.display->drawLine(circle1X, circle1Y, circle3X, circle3Y);
    Heltec.display->drawLine(circle1X + 1, circle1Y, circle3X + 1, circle3Y);
    Heltec.display->setColor(WHITE);
    Heltec.display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2); // , WHITE);
  }
  if (circle1) {
    Heltec.display->setColor(WHITE);
    Heltec.display->fillCircle(circle1X, circle1Y, logoSize / 2); // , WHITE);
    Heltec.display->setColor(BLACK);
    Heltec.display->fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T); // , TFT_GREEN);
    Heltec.display->setColor(WHITE);
    Heltec.display->fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T * 2); // , WHITE);
  }
  if (circle2) {
    Heltec.display->setColor(WHITE);
    Heltec.display->fillCircle(circle2X, circle2Y, logoSize / 3); // , WHITE);
    Heltec.display->setColor(BLACK);
    Heltec.display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T); // , TFT_ORANGE);
    Heltec.display->setColor(WHITE);
    Heltec.display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2); // , WHITE);
  }
  if (circle3) {
    Heltec.display->setColor(WHITE);
    Heltec.display->fillCircle(circle3X, circle3Y, logoSize / 4); // , WHITE);
    Heltec.display->setColor(BLACK);
    Heltec.display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T); // , TFT_PINK);
    Heltec.display->setColor(WHITE);
    Heltec.display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2); // , WHITE);
  }
  if (name) {
    Heltec.display->setColor(BLACK);
    Heltec.display->drawString(circle1X + (circle1X * 0.27), circle1Y, "penMQTTGateway");
  }
  Heltec.display->display();
  delay(50);
}