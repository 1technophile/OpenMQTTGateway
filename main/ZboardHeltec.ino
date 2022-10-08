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

#include "User_config.h"
#if defined(ZboardHELTEC)
#  include "ArduinoLog.h"
#  include "config_HELTEC.h"
// #  include "heltec.h"

void logToLCD(bool display) {
  display ? Log.begin(LOG_LEVEL_LCD, &Oled) : Log.begin(LOG_LEVEL, &Serial); // Log on LCD following LOG_LEVEL_LCD
}

void setupHELTEC() {
  Log.trace(F("Setup HELTEC Display" CR));
  Oled.begin();
  Log.notice(F("Setup HELTEC Display end" CR));

#  if LOG_TO_LCD
  Log.begin(LOG_LEVEL_LCD, &Oled); // Log on LCD following LOG_LEVEL_LCD
#  endif
}

void loopHELTEC() {
}

void MQTTtoHELTEC(char* topicOri, JsonObject& HELTECdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoHELTECset)) {
    Log.trace(F("MQTTtoHELTEC json set" CR));
    // Log display set between HELTEC lcd (true) and serial monitor (false)
    if (HELTECdata.containsKey("log-lcd")) {
      bool displayOnLCD = HELTECdata["log-lcd"];
      Log.notice(F("Set lcd log: %T" CR), displayOnLCD);
      logToLCD(displayOnLCD);
    }
  }
}

// Simple method to display message on oled

size_t OledPrint(String msg) {
  return Oled.print(msg);
}

// This pattern was borrowed from HardwareSerial and modified to support the ssd1306 display

OledSerial Oled(0); // Not sure about this, came from Hardwareserial
OledSerial::OledSerial(int x) {} // Not sure about this, came from Hardwareserial

void OledSerial::begin() {
  Heltec.begin(); // User OMG serial support
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);

  Heltec.display->setColor(WHITE);
  Heltec.display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
  Heltec.display->display();
  displayIntro(OLED_WIDTH * 0.24, (OLED_WIDTH / 2) - OLED_WIDTH * 0.2, (OLED_HEIGHT / 2) + OLED_HEIGHT * 0.2);
  Heltec.display->setLogBuffer(OLED_TEXT_HEIGHT, OLED_TEXT_WIDTH);
  delay(1000);
}

// Dummy virtual's from Serial

int OledSerial::available(void) {
}

int OledSerial::peek(void) {
}

int OledSerial::read(void) {
}

void OledSerial::flush(void) {
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
  Heltec.display->clear();
  Heltec.display->setColor(WHITE);
  Heltec.display->setFont(ArialMT_Plain_10);
  if (size > OLED_TEXT_WIDTH)
    size = OLED_TEXT_WIDTH; // Too wide for the display
  while (size) {
    Heltec.display->print((char)*buffer++);
    size--;
  }
  Heltec.display->drawLogBuffer(0, 0);
  Heltec.display->display();
  return size;
}

/*
Display OpenMQTTGateway logo - borrowed from ZboardM5.ino and tweaked for ssd1306 display ( removed color and tweaked size/location )
*/

void OledSerial::displayIntro(int scale, int displayWidth, int displayHeight) {
  drawLogo(scale, displayWidth, displayHeight, false, true, false, false, false, false); // Circle 2
  drawLogo(scale, displayWidth, displayHeight, false, false, true, false, false, false); // Circle 3
  drawLogo(scale, displayWidth, displayHeight, false, true, true, true, false, false); // Line 1
  drawLogo(scale, displayWidth, displayHeight, false, true, true, false, true, false); // Line 2
  drawLogo(scale, displayWidth, displayHeight, true, true, true, true, true, false); // Circle 1
  drawLogo(scale, displayWidth, displayHeight, true, true, true, true, true, true); // Name
}

void OledSerial::drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool line1, bool line2, bool name) {
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

// Copy and paste from the heltecautomation/Heltec ESP32 Dev-Boards@^1.1.1 package, and removed the lora components

Heltec_ESP32::Heltec_ESP32() {
  // Need to reset display prior to initial configuration
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, LOW);
  delay(50);
  digitalWrite(RST_OLED, HIGH);

#  if defined(WIFI_Kit_32) || defined(WIFI_LoRa_32) || defined(WIFI_LoRa_32_V2)
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);
#  elif defined(Wireless_Stick)
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_64_32);
#  endif
}

Heltec_ESP32::~Heltec_ESP32() {
  delete display;
}

void Heltec_ESP32::begin() {
  VextON();
  display->init();
}

void Heltec_ESP32::VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void Heltec_ESP32::VextOFF(void) //Vext default OFF
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

Heltec_ESP32 Heltec;

#endif