/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
    M5 boards management
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - 1technophile
  
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

#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
#  include "TheengsCommon.h"
#  include "config_M5.h" // pulls in <M5Unified.h> for every M5 board

void wakeScreen(int brightness);
void displayIntro(int i, int X, int Y);
void drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool circle4, bool circle5, bool circle6);
void sleepScreen();
void setBrightness(int brightness);

void logToLCD(bool display) {
  display ? Log.begin(LOG_LEVEL_LCD, &M5.Lcd) : Log.begin(LOG_LEVEL, &Serial); // Log on LCD following LOG_LEVEL_LCD
}

void setBrightness(int brightness) {
  // M5Unified drives the backlight uniformly across Core (direct control) and
  // the AXP-powered boards (Stick C/CP, Tough), replacing the old per-board
  // M5.Lcd.setBrightness / M5.Axp.ScreenBreath split. Map the 0..100 OMG scale
  // onto the panel's 0..255 range (100 -> 200, sleep 2 -> 4).
  M5.Display.setBrightness(brightness * 2);
}

void setupM5() {
  THEENGS_LOG_NOTICE(F("Setup M5" CR));
  M5.begin(); // auto-detects the board, initialises the display and power
  pinMode(SLEEP_BUTTON, INPUT);
  // M5 stack 320*240
  // M5StickC 160*80
  // M5Stick LCD not supported
  wakeScreen(NORMAL_LCD_BRIGHTNESS);
  M5.Lcd.fillScreen(TFT_WHITE);
  displayIntro(M5.Lcd.width() * 0.25, (M5.Lcd.width() / 2) + M5.Lcd.width() * 0.12, (M5.Lcd.height() / 2) + M5.Lcd.height() * 0.2);
#  if LOG_TO_LCD
  Log.begin(LOG_LEVEL_LCD, &M5.Lcd); // Log on LCD following LOG_LEVEL_LCD
#  endif

  THEENGS_LOG_NOTICE(F("Setup M5 end" CR));
}

void sleepScreen() {
  THEENGS_LOG_TRACE(F("Screen going to sleep" CR));
  // Backlight off + panel sleep. M5Unified routes this to the AXP LDO on
  // Stick/Tough and to the Core's backlight control, so the old M5.begin()
  // re-init workaround and the M5.Axp calls are no longer needed.
  M5.Display.sleep();
}

void wakeScreen(int brightness) {
  THEENGS_LOG_TRACE(F("Screen wake up" CR));
  M5.Display.wakeup(); // undo sleepScreen(); harmless if already awake
  M5.Lcd.setCursor(0, 0, (M5.Lcd.height() > 200) ? 4 : 2);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setRotation(1);
  setBrightness(brightness);
}

void loopM5() {
  static int previousLogLevel;
  int currentLogLevel = Log.getLastMsgLevel();
  if (previousLogLevel != currentLogLevel) {
    switch (currentLogLevel) {
      case 1:
      case 2:
        wakeScreen(NORMAL_LCD_BRIGHTNESS);
        M5.Lcd.fillScreen(TFT_RED); // FATAL, ERROR
        M5.Lcd.setTextColor(TFT_BLACK, TFT_RED);
        break;
      case 3:
        wakeScreen(NORMAL_LCD_BRIGHTNESS);
        M5.Lcd.fillScreen(TFT_ORANGE); // WARNING
        M5.Lcd.setTextColor(TFT_BLACK, TFT_ORANGE);
        break;
      default:
        wakeScreen(SLEEP_LCD_BRIGHTNESS);
        M5.Lcd.fillScreen(TFT_WHITE);
        drawLogo(M5.Lcd.width() * 0.1875, (M5.Lcd.width() / 2) - M5.Lcd.width() * 0.24, M5.Lcd.height() * 0.5, true, true, true, true, true, true);
        break;
    }
  }
  previousLogLevel = currentLogLevel;
}

void XtoM5(const char* topicOri, JsonObject& M5data) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoM5set)) {
    THEENGS_LOG_TRACE(F("MQTTtoM5 json set" CR));
    // Log display set between M5 lcd (true) and serial monitor (false)
    if (M5data.containsKey("log-lcd")) {
      bool displayOnLCD = M5data["log-lcd"];
      THEENGS_LOG_NOTICE(F("Set lcd log: %T" CR), displayOnLCD);
      logToLCD(displayOnLCD);
    }
  }
}

void displayIntro(int i, int X, int Y) {
  THEENGS_LOG_TRACE(F("Intro display on screen" CR));
  drawLogo(i, X, Y, false, true, false, false, false, false);
  delay(50);
  drawLogo(i, X, Y, false, false, true, false, false, false);
  delay(50);
  drawLogo(i, X, Y, false, true, false, false, false, false);
  delay(50);
  drawLogo(i, X, Y, false, true, true, false, false, false);
  delay(50);
  drawLogo(i, X, Y, false, true, true, true, false, false);
  delay(50);
  drawLogo(i, X, Y, false, true, true, false, true, false);
  delay(50);
  drawLogo(i, X, Y, false, true, true, true, true, false);
  delay(50);
  drawLogo(i, X, Y, true, true, true, true, true, false);
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
    M5.Lcd.drawLine(circle1X - 2, circle1Y, circle2X - 2, circle2Y, TFT_BLUE);
    M5.Lcd.drawLine(circle1X - 1, circle1Y, circle2X - 1, circle2Y, TFT_BLUE);
    M5.Lcd.drawLine(circle1X, circle1Y, circle2X, circle2Y, TFT_BLUE);
    M5.Lcd.drawLine(circle1X + 1, circle1Y, circle2X + 1, circle2Y, TFT_BLUE);
    M5.Lcd.drawLine(circle1X + 2, circle1Y, circle2X + 2, circle2Y, TFT_BLUE);
    M5.Lcd.fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2, TFT_WHITE);
  }
  if (line2) {
    M5.Lcd.drawLine(circle1X - 2, circle1Y, circle3X - 2, circle3Y, TFT_BLUE);
    M5.Lcd.drawLine(circle1X - 1, circle1Y, circle3X - 1, circle3Y, TFT_BLUE);
    M5.Lcd.drawLine(circle1X, circle1Y, circle3X, circle3Y, TFT_BLUE);
    M5.Lcd.drawLine(circle1X + 1, circle1Y, circle3X + 1, circle3Y, TFT_BLUE);
    M5.Lcd.fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2, TFT_WHITE);
  }
  M5.Lcd.fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T * 2, TFT_WHITE);
  if (circle1) {
    M5.Lcd.fillCircle(circle1X, circle1Y, logoSize / 2, TFT_WHITE);
    M5.Lcd.fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T, TFT_GREEN);
    M5.Lcd.fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T * 2, TFT_WHITE);
  }
  if (circle2) {
    M5.Lcd.fillCircle(circle2X, circle2Y, logoSize / 3, TFT_WHITE);
    M5.Lcd.fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T, TFT_ORANGE);
    M5.Lcd.fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2, TFT_WHITE);
  }
  if (circle3) {
    M5.Lcd.fillCircle(circle3X, circle3Y, logoSize / 4, TFT_WHITE);
    M5.Lcd.fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T, TFT_PINK);
    M5.Lcd.fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2, TFT_WHITE);
  }
  if (name) {
    M5.Lcd.setTextColor(TFT_BLUE);
    M5.Lcd.drawString("penMQTTGateway", circle1X + (circle1X * 0.27), circle1Y, (M5.Lcd.height() > 200) ? 4 : 2);
  }
}

void M5Print(char* line1, char* line2, char* line3) {
  wakeScreen(NORMAL_LCD_BRIGHTNESS);
  M5.Lcd.fillScreen(TFT_WHITE);
  drawLogo(M5.Lcd.width() * 0.1875, (M5.Lcd.width() / 2) - M5.Lcd.width() * 0.24, M5.Lcd.height() * 0.5, true, true, true, true, true, true);
  M5.Lcd.setTextColor(TFT_BLUE);
  M5.Lcd.drawString(line1, 5, M5.Lcd.height() * 0.7, 1);
  M5.Lcd.drawString(line2, 5, M5.Lcd.height() * 0.8, 1);
  M5.Lcd.drawString(line3, 5, M5.Lcd.height() * 0.9, 1);
  delay(2000);
}
#endif
