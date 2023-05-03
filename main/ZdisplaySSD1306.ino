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

#if defined(ZdisplaySSD1306)

#  include <ArduinoJson.h>

#  include "ArduinoLog.h"
#  include "User_config.h"
#  include "config_SSD1306.h"

SemaphoreHandle_t semaphoreOLEDOperation;

boolean logToOLEDDisplay = LOG_TO_OLED;
boolean jsonDisplay = JSON_TO_OLED;
boolean displayFlip = DISPLAY_FLIP;
boolean displayState = DISPLAY_STATE;
boolean idlelogo = DISPLAY_IDLE_LOGO;
uint8_t displayBrightness = DISPLAY_BRIGHTNESS;

/*
Toogle log display
*/
void logToOLED(bool display) {
  logToOLEDDisplay = display;
  display ? Log.begin(LOG_LEVEL_OLED, &Oled) : Log.begin(LOG_LEVEL, &Serial); // Log on OLED following LOG_LEVEL_OLED
}

/*
module setup, for use in Arduino setup
*/
void setupSSD1306() {
  SSD1306Config_init();
  SSD1306Config_load();
  Log.trace(F("Setup SSD1306 Display" CR));
  Log.trace(F("ZdisplaySSD1306 command topic: %s" CR), subjectMQTTtoSSD1306set);
  Log.trace(F("ZdisplaySSD1306 log-oled: %T" CR), logToOLEDDisplay);
  Log.trace(F("ZdisplaySSD1306 json-oled: %T" CR), jsonDisplay);
  Log.trace(F("ZdisplaySSD1306 DISPLAY_PAGE_INTERVAL: %d" CR), DISPLAY_PAGE_INTERVAL);
  Log.trace(F("ZdisplaySSD1306 DISPLAY_IDLE_LOGO: %T" CR), idlelogo);
  Log.trace(F("ZdisplaySSD1306 DISPLAY_FLIP: %T" CR), displayFlip);
  Oled.begin();
  Log.notice(F("Setup SSD1306 Display end" CR));

#  if LOG_TO_OLED
  Log.begin(LOG_LEVEL_OLED, &Oled); // Log on OLED following LOG_LEVEL_OLED
  jsonDisplay = false;
#  else
  jsonDisplay = true;
#  endif
}

boolean logoDisplayed = false;
unsigned long nextDisplayPage = uptime() + DISPLAY_PAGE_INTERVAL;

/*
module loop, for use in Arduino loop
*/
void loopSSD1306() {
  /*
  Function to check if json messages are in the queue and send them for display

  long enough since the last message and display not being used and a queue message waiting
  */
  if (jsonDisplay && displayState) {
    if (uptime() >= nextDisplayPage && uxSemaphoreGetCount(semaphoreOLEDOperation) && currentWebUIMessage && newSSD1306Message) {
      if (!Oled.displayPage(currentWebUIMessage)) {
        Log.warning(F("[ssd1306] displayPage failed: %s" CR), currentWebUIMessage->title);
      }
      nextDisplayPage = uptime() + DISPLAY_PAGE_INTERVAL;
      logoDisplayed = false;
      newSSD1306Message = false;
    }
  }
  /*
  Display logo if it has been more than DISPLAY_PAGE_INTERVAL
  */
  if (uptime() > nextDisplayPage + 1 && !logoDisplayed && idlelogo && displayState) {
    Oled.display->normalDisplay();
    Oled.fillScreen(BLACK);
    Oled.drawLogo(rand() % 13 - 5, rand() % 32 - 13);
    logoDisplayed = true;
  }
}

/*
Handler for mqtt commands sent to the module
- log-oled: boolean
  Enable / Disable display of log messages on display
*/
void MQTTtoSSD1306(char* topicOri, JsonObject& SSD1306data) { // json object decoding
  bool success = false;
  if (cmpToMainTopic(topicOri, subjectMQTTtoSSD1306set)) {
    Log.trace(F("MQTTtoSSD1306 json set" CR));
    // properties
    if (SSD1306data.containsKey("onstate")) {
      displayState = SSD1306data["onstate"].as<bool>();
      Log.notice(F("Set display state: %T" CR), displayState);
      success = true;
    }
    if (SSD1306data.containsKey("brightness")) {
      displayBrightness = SSD1306data["brightness"].as<int>();
      Log.notice(F("Set brightness: %d" CR), displayBrightness);
      success = true;
    }
    if (SSD1306data.containsKey("log-oled")) {
      logToOLEDDisplay = SSD1306data["log-oled"].as<bool>();
      Log.notice(F("Set OLED log: %T" CR), logToOLEDDisplay);
      logToOLED(logToOLEDDisplay);
      if (logToOLEDDisplay) {
        jsonDisplay = false;
      }
      success = true;
    } else if (SSD1306data.containsKey("json-oled")) {
      jsonDisplay = SSD1306data["json-oled"].as<bool>();
      if (jsonDisplay) {
        logToOLEDDisplay = false;
        logToOLED(logToOLEDDisplay);
      }
      Log.notice(F("Set json-oled: %T" CR), jsonDisplay);
      success = true;
    }
    if (SSD1306data.containsKey("idlelogo")) {
      idlelogo = SSD1306data["idlelogo"].as<bool>();
      success = true;
    }
    if (SSD1306data.containsKey("display-flip")) {
      displayFlip = SSD1306data["display-flip"].as<bool>();
      Log.notice(F("Set display-flip: %T" CR), displayFlip);
      success = true;
    }
    // save, load, init, erase
    if (SSD1306data.containsKey("save") && SSD1306data["save"]) {
      SSD1306Config_save();
      success = true;
    } else if (SSD1306data.containsKey("load") && SSD1306data["load"]) {
      success = SSD1306Config_load();
      if (success) {
        Log.notice(F("SSD1306 config loaded" CR));
      }
    } else if (SSD1306data.containsKey("init") && SSD1306data["init"]) {
      SSD1306Config_init();
      success = true;
      if (success) {
        Log.notice(F("SSD1306 config initialised" CR));
      }
    } else if (SSD1306data.containsKey("erase") && SSD1306data["erase"]) {
      // Erase config from NVS (non-volatile storage)
      preferences.begin(Gateway_Short_Name, false);
      if (preferences.isKey("SSD1306Config")) {
        success = preferences.remove("SSD1306Config");
        preferences.end();
      }
      if (success) {
        Log.notice(F("SSD1306 config erased" CR));
      }
    }
    if (success) {
      stateSSD1306Display();
    } else {
      // pub(subjectSSD1306toMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("[ SSD1306 ] MQTTtoSSD1306 Fail json" CR), SSD1306data);
    }
  }
}

void SSD1306Config_save() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jo = jsonBuffer.to<JsonObject>();
  jo["onstate"] = displayState;
  jo["brightness"] = displayBrightness;
  jo["log-oled"] = logToOLEDDisplay;
  jo["json-oled"] = jsonDisplay;
  jo["idlelogo"] = idlelogo;
  jo["display-flip"] = displayFlip;
  // Save config into NVS (non-volatile storage)
  String conf = "";
  serializeJson(jsonBuffer, conf);
  preferences.begin(Gateway_Short_Name, false);
  preferences.putString("SSD1306Config", conf);
  preferences.end();
  Log.notice(F("SSD1306 config saved" CR));
}

void SSD1306Config_init() {
  displayState = DISPLAY_STATE;
  displayBrightness = DISPLAY_BRIGHTNESS;
  logToOLEDDisplay = LOG_TO_OLED;
  jsonDisplay = JSON_TO_OLED;
  idlelogo = DISPLAY_IDLE_LOGO;
  displayFlip = DISPLAY_FLIP;
  Log.notice(F("SSD1306 config initialised" CR));
}

bool SSD1306Config_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (preferences.isKey("SSD1306Config")) {
    auto error = deserializeJson(jsonBuffer, preferences.getString("SSD1306Config", "{}"));
    preferences.end();
    if (error) {
      Log.error(F("SSD1306 config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      return false;
    }
    if (jsonBuffer.isNull()) {
      Log.warning(F("SSD1306 config is null" CR));
      return false;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    displayState = jo["onstate"].as<bool>();
    displayBrightness = jo["brightness"].as<int>();
    logToOLEDDisplay = jo["log-oled"].as<bool>();
    jsonDisplay = jo["json-oled"].as<bool>();
    idlelogo = jo["idlelogo"].as<bool>();
    displayFlip = jo["display-flip"].as<bool>();
    Log.notice(F("Saved SSD1306 config loaded" CR));
    return true;
  } else {
    preferences.end();
    Log.notice(F("No SSD1306 config to load" CR));
    return false;
  }
}

// Simple print methonds

/*
Display three lines of text on display, scroll if needed
*/
void ssd1306Print(char* line1, char* line2, char* line3) {
  Oled.println(line1);
  Oled.println(line2);
  Oled.println(line3);
  delay(2000);
}

/*
Display two lines of text on display, scroll if needed
*/
void ssd1306Print(char* line1, char* line2) {
  Oled.println(line1);
  Oled.println(line2);
  delay(2000);
}

/*
Display single line of text on display, scroll if needed
*/
void ssd1306Print(char* line1) {
  Oled.println(line1);
  delay(2000);
}

// This pattern was borrowed from HardwareSerial and modified to support the ssd1306 display

OledSerial Oled(0); // Not sure about this, came from Hardwareserial
OledSerial::OledSerial(int x) {
#  if defined(WIFI_Kit_32) || defined(WIFI_LoRa_32) || defined(WIFI_LoRa_32_V2)
  pinMode(RST_OLED, OUTPUT); // https://github.com/espressif/arduino-esp32/issues/4278
  digitalWrite(RST_OLED, LOW);
  delay(50);
  digitalWrite(RST_OLED, HIGH);
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);
#  elif defined(Wireless_Stick)
  // pinMode(RST_OLED, OUTPUT); // https://github.com/espressif/arduino-esp32/issues/4278
  // digitalWrite(RST_OLED, LOW);
  // delay(50);
  // digitalWrite(RST_OLED, HIGH);
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_64_32);
#  elif defined(ARDUINO_TTGO_LoRa32_v21new) // LILYGO® Disaster-Radio LoRa V2.1_1.6.1
  // pinMode(OLED_RST, OUTPUT);   // https://github.com/espressif/arduino-esp32/issues/4278
  // digitalWrite(OLED_RST, LOW);
  // delay(50);
  // digitalWrite(OLED_RST, HIGH);
  display = new SSD1306Wire(0x3c, OLED_SDA, OLED_SCL, GEOMETRY_128_64);
#  endif
}

/*
Initialize ssd1306 oled display for use, and display OMG logo
*/
void OledSerial::begin() {
  // SSD1306.begin(); // User OMG serial support

  semaphoreOLEDOperation = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreOLEDOperation);

  display->init();
  if (displayFlip) {
    display->flipScreenVertically();
  } else {
    display->resetOrientation();
  }
  display->setFont(ArialMT_Plain_10);
  display->setBrightness(round(displayBrightness * 2.55));
  drawLogo(0, 0);
  display->invertDisplay();
  display->setLogBuffer(OLED_TEXT_ROWS, OLED_TEXT_BUFFER);
  delay(1000);

  if (!displayState) {
    display->displayOff();
  }
}

/*
Dummy virtual functions carried over from Serial
*/
int OledSerial::available(void) {
}

/*
Dummy virtual functions carried over from Serial
*/
int OledSerial::peek(void) {
}

/*
Dummy virtual functions carried over from Serial
*/
int OledSerial::read(void) {
}

/*
Dummy virtual functions carried over from Serial
*/
void OledSerial::flush(void) {
}

/*
Erase display and paint it with the color.  Used to 
*/
void OledSerial::fillScreen(OLEDDISPLAY_COLOR color) {
  if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
    display->clear();
    display->setColor(color);
    display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
    xSemaphoreGive(semaphoreOLEDOperation);
  }
}

/*
Write line of text to the display with vertical scrolling of screen
*/
size_t OledSerial::write(const uint8_t* buffer, size_t size) {
  if (xPortGetCoreID() == CONFIG_ARDUINO_RUNNING_CORE) {
    if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
      nextDisplayPage = uptime() + DISPLAY_PAGE_INTERVAL;
      display->normalDisplay();
      display->clear();
      display->setColor(WHITE);
      display->setFont(ArialMT_Plain_10);
      while (size) {
        display->write((char)*buffer++);
        size--;
      }
      display->drawLogBuffer(0, 0);
      display->display();
      xSemaphoreGive(semaphoreOLEDOperation);
      return size;
    }
  }
  // Default to Serial output if the display is not available
  return Serial.write(buffer, size);
}

/*
Display full page message on the display.
- Used to display JSON messages published from each gateway module
*/
boolean OledSerial::displayPage(webUIQueueMessage* message) {
  if (xPortGetCoreID() == CONFIG_ARDUINO_RUNNING_CORE) {
    if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
      display->normalDisplay();
      display->clear();
      display->setColor(WHITE);
      display->setFont(ArialMT_Plain_10);
      display->drawString(0, 0, message->title);
      display->drawLine(0, 12, OLED_WIDTH, 12);
      display->drawString(0, 13, message->line1);
      display->drawString(0, 26, message->line2);
      display->drawString(0, 39, message->line3);
      display->drawString(0, 52, message->line4);
      display->display();
      xSemaphoreGive(semaphoreOLEDOperation);
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

/*
Primitives behind OpenMQTTGateway logo
*/
void OledSerial::drawLogo(int xshift, int yshift) {
  if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
    display->setColor(WHITE);
    // line 1
    display->drawLine(15 + xshift, 28 + yshift, 20 + xshift, 31 + yshift);
    display->drawLine(15 + xshift, 29 + yshift, 20 + xshift, 32 + yshift);
    // line 2
    display->drawLine(25 + xshift, 29 + yshift, 22 + xshift, 21 + yshift);
    display->drawLine(26 + xshift, 29 + yshift, 23 + xshift, 21 + yshift);
    // circle 1
    display->fillCircle(25 + xshift, 35 + yshift, 7);
    display->setColor(BLACK);
    display->fillCircle(25 + xshift, 35 + yshift, 5);
    // circle 2
    display->setColor(WHITE);
    display->fillCircle(23 + xshift, 18 + yshift, 4);
    display->setColor(BLACK);
    display->fillCircle(23 + xshift, 18 + yshift, 2);
    // circle 3
    display->setColor(WHITE);
    display->fillCircle(11 + xshift, 25 + yshift, 5);
    display->setColor(BLACK);
    display->fillCircle(11 + xshift, 25 + yshift, 3);
    // name
    display->setColor(WHITE);
    display->drawString(32 + xshift, 32 + yshift, "penMQTTGateway");

    display->display();
    xSemaphoreGive(semaphoreOLEDOperation);
  }
}

String stateSSD1306Display() {
  //Publish display state
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject DISPLAYdata = jsonBuffer.to<JsonObject>();
  DISPLAYdata["onstate"] = (bool)displayState;
  DISPLAYdata["brightness"] = (int)displayBrightness;
  DISPLAYdata["display-flip"] = (bool)displayFlip;
  DISPLAYdata["idlelogo"] = (bool)idlelogo;
  DISPLAYdata["log-oled"] = (bool)logToOLEDDisplay;
  DISPLAYdata["json-oled"] = (bool)jsonDisplay;
  pub(subjectSSD1306toMQTT, DISPLAYdata);
  // apply
  Oled.display->setBrightness(round(displayBrightness * 2.55));

  if (!displayState) {
    Oled.display->displayOff();
  } else {
    Oled.display->displayOn();
  }

  if (displayFlip) {
    Oled.display->flipScreenVertically();
  } else {
    Oled.display->resetOrientation();
  }
  String output;
  serializeJson(DISPLAYdata, output);
  return output;
}

#endif
