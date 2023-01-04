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
QueueHandle_t displayQueue;

void logToLCD(bool display) {
  display ? Log.begin(LOG_LEVEL_LCD, &Oled) : Log.begin(LOG_LEVEL, &Serial); // Log on LCD following LOG_LEVEL_LCD
}

void setupSSD1306() {
  Log.trace(F("Setup SSD1306 Display" CR));
  Log.trace(F("ZdisplaySSD1306 command topic: %s" CR), subjectMQTTtoSSD1306set);
  Oled.begin();
  Log.notice(F("Setup SSD1306 Display end" CR));

#  if LOG_TO_LCD
  Log.begin(LOG_LEVEL_LCD, &Oled); // Log on LCD following LOG_LEVEL_LCD
#  endif
}

static int previousLogLevel = 0;
unsigned long nextDisplayPage = uptime() + DISPLAYPAGEINTERVAL;

void loopSSD1306() {
  /*
  int currentLogLevel = Log.getLastMsgLevel();
  if (previousLogLevel != currentLogLevel && lowpowermode != 2) {
    switch (currentLogLevel) {
      case 1:
      case 2:
        //        wakeScreen(NORMAL_LCD_BRIGHTNESS);
        //        M5.Lcd.fillScreen(TFT_RED); // FATAL, ERROR
        //        M5.Lcd.setTextColor(TFT_BLACK, TFT_RED);
        break;
      case 3:
        //        wakeScreen(NORMAL_LCD_BRIGHTNESS);
        //        M5.Lcd.fillScreen(TFT_ORANGE); // WARNING
        //        M5.Lcd.setTextColor(TFT_BLACK, TFT_ORANGE);
        break;
      default:
        // TODO: Display splash screen x seconds after last message displayed on OLED
        Oled.fillScreen(WHITE);
        Oled.drawLogo((int)OLED_WIDTH * 0.24, (int)(OLED_WIDTH / 2) - OLED_WIDTH * 0.2, (int)(OLED_HEIGHT / 2) + OLED_HEIGHT * 0.2, true, true, true, true, true, true); // Name
        break;
    }
  }
  previousLogLevel = currentLogLevel;
  */

  // long enough since the last message and display not being used and a queue message waiting

  if (uptime() > nextDisplayPage && uxSemaphoreGetCount(semaphoreOLEDOperation) && uxQueueMessagesWaiting(displayQueue)) {
    displayQueueMessage* message = nullptr;
    xQueueReceive(displayQueue, &message, portMAX_DELAY);
    Oled.displayPage(message);
    nextDisplayPage = uptime() + DISPLAYPAGEINTERVAL;
  }
}

void MQTTtoSSD1306(char* topicOri, JsonObject& SSD1306data) { // json object decoding
  bool success = false;
  if (cmpToMainTopic(topicOri, subjectMQTTtoSSD1306set)) {
    Log.trace(F("MQTTtoSSD1306 json set" CR));
    // Log display set between SSD1306 lcd (true) and serial monitor (false)
    if (SSD1306data.containsKey("log-lcd")) {
      bool displayOnLCD = SSD1306data["log-lcd"];
      Log.notice(F("Set lcd log: %T" CR), displayOnLCD);
      logToLCD(displayOnLCD);
      success = true;
    }
    if (success) {
      pub(subjectSSD1306toMQTTset, SSD1306data);
    } else {
      pub(subjectSSD1306toMQTTset, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("MQTTtoSSD1306 Fail json" CR), SSD1306data);
    }
  }
}

constexpr unsigned int hash(const char* s, int off = 0) { // workaround for switching on a string https://stackoverflow.com/a/46711735/18643696
  return !s[off] ? 5381 : (hash(s, off + 1) * 33) ^ s[off];
}

void ssd1306PubPrint(const char* topicori, JsonObject& data) {
  displayQueueMessage* message = (displayQueueMessage*)malloc(sizeof(displayQueueMessage));

  Log.error(F("ERROR: displayQueue full, discarding signal %s" CR), topicori);
  strcpy(message->title, strtok((char*)topicori, "/"));
  Log.error(F("ERROR: displayQueue full, discarding signal %s" CR), message->title);

  switch (hash(message->title)) {
    case hash("SYStoMQTT"): {
      // {"uptime":456356,"version":"lilygo-rtl_433-test-A-v1.1.1-25-g574177d[lily-cloud]","freemem":125488,"mqttport":"1883","mqttsecure":false,"freestack":3752,"rssi":-36,"SSID":"The_Beach","BSSID":"64:A5:C3:69:C3:38","ip":"192.168.1.239","mac":"4C:75:25:A8:D5:D8","actRec":3,"mhz":433.92,"RTLRssiThresh":-98,"RTLRssi":-108,"RTLAVGRssi":-107,"RTLCnt":121707,"RTLOOKThresh":90,"modules":["LILYGO_OLED","CLOUD","rtl_433"]}

      // Line 1

      strncpy(message->line1, data["version"], OLED_TEXT_WIDTH);

      // Line 2

      String uptime = data["uptime"];
      String line2 = "uptime: " + uptime;
      line2.toCharArray(message->line2, OLED_TEXT_WIDTH);

      // Line 3

      String freemem = data["freemem"];
      String line3 = "freemem: " + freemem;
      line3.toCharArray(message->line3, OLED_TEXT_WIDTH);

      // Line 4

      String ip = data["ip"];
      String line4 = "ip: " + ip;
      line4.toCharArray(message->line4, OLED_TEXT_WIDTH);

      // Queue completed message

      if (xQueueSend(displayQueue, (void*)&message, 0) != pdTRUE) {
        Log.error(F("ERROR: displayQueue full, discarding signal %s" CR), message->title);
      } else {
        // Log.notice(F("Queued %s" CR), message->title);
      }
      break;
    }

#  ifdef ZgatewayRTL_433
    case hash("RTL_433toMQTT"): {
      // {"model":"Acurite-Tower","id":2043,"channel":"B","battery_ok":1,"temperature_C":5.3,"humidity":81,"mic":"CHECKSUM","protocol":"Acurite 592TXR Temp/Humidity, 5n1 Weather Station, 6045 Lightning, 3N1, Atlas","rssi":-81,"duration":121060}

      // Line 1

      strcpy(message->line1, data["model"]);

      // Line 2

      String id = data["id"];
      String channel = data["channel"];
      String line2 = "id: " + id + " channel: " + channel;
      line2.toCharArray(message->line2, OLED_TEXT_WIDTH);

      // Line 3

      char temperature_C[5];
      dtostrf(data["temperature_C"], 3, 1, temperature_C);
      int humidity = data["humidity"];
      String wind_avg_km_h = data["wind_avg_km_h"];

      String line3 = "";
      if (data.containsKey("temperature_C")) {
        line3 = "temp: " + (String)temperature_C + "°C ";
      }
      if (data.containsKey("humidity") && humidity <= 100 && humidity >= 0) {
        line3 += "hum: " + (String)humidity + "% ";
      }
      if (data.containsKey("wind_avg_km_h")) {
        line3 += "wind: " + wind_avg_km_h + " ";
      }
      line3.toCharArray(message->line3, OLED_TEXT_WIDTH);

      // Line 4

      String rssi = data["rssi"];
      String battery_ok = data["battery_ok"];

      String line4 = "batt: " + battery_ok + " rssi: " + rssi;
      line4.toCharArray(message->line4, OLED_TEXT_WIDTH);

      // Queue completed message

      if (xQueueSend(displayQueue, (void*)&message, 0) != pdTRUE) {
        Log.error(F("ERROR: displayQueue full, discarding signal %s" CR), message->title);
      } else {
        // Log.notice(F("Queued %s" CR), message->title);
      }
      break;
    }
#  endif
#  ifdef ZsensorBME280
    case hash("CLIMAtoMQTT"): {
      // {"tempc":17.06,"tempf":62.708,"hum":50.0752,"pa":98876.14,"altim":205.8725,"altift":675.4348}

      // Line 1

      strcpy(message->line1, "bme280");

      // Line 2

      strcpy(message->line2, "");

      // Line 3

      char temperature_C[5];
      dtostrf(data["tempc"], 3, 1, temperature_C);
      int humidity = data["hum"];

      String line3 = "";
      if (data.containsKey("tempc")) {
        line3 = "temp: " + (String)temperature_C + "°C ";
      }
      if (data.containsKey("hum") && humidity <= 100 && humidity >= 0) {
        line3 += "hum: " + (String)humidity + "% ";
      }
      line3.toCharArray(message->line3, OLED_TEXT_WIDTH);

      // Line 4

      int pressure = (int)data["pa"] / 100;

      String line4 = "pressure: " + (String)pressure + " hPa";
      line4.toCharArray(message->line4, OLED_TEXT_WIDTH);

      // Queue completed message

      if (xQueueSend(displayQueue, (void*)&message, 0) != pdTRUE) {
        Log.error(F("ERROR: displayQueue full, discarding signal %s" CR), message->title);
      } else {
        // Log.notice(F("Queued %s" CR), message->title);
      }
      break;
    }
#  endif
    default:
      Log.error(F("ERROR: unhandled topic %s" CR), message->title);
  }
}

// Simple print methonds

void ssd1306Print(char* line1, char* line2, char* line3) {
  Oled.println(line1);
  Oled.println(line2);
  Oled.println(line3);
  delay(2000);
}

void ssd1306Print(char* line1, char* line2) {
  Oled.println(line1);
  Oled.println(line2);
  delay(2000);
}

void ssd1306Print(char* line1) {
  Oled.println(line1);
  delay(2000);
}

// This pattern was borrowed from HardwareSerial and modified to support the ssd1306 display

OledSerial Oled(0); // Not sure about this, came from Hardwareserial
OledSerial::OledSerial(int x) {
  displayQueue = xQueueCreate(5, sizeof(displayQueueMessage*));
#  if defined(WIFI_Kit_32) || defined(WIFI_LoRa_32) || defined(WIFI_LoRa_32_V2)
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, LOW);
  delay(50);
  digitalWrite(RST_OLED, HIGH);
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);
#  elif defined(Wireless_Stick)
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, LOW);
  delay(50);
  digitalWrite(RST_OLED, HIGH);
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_64_32);
#  elif defined(ARDUINO_TTGO_LoRa32_v21new) // LILYGO® Disaster-Radio LoRa V2.1_1.6.1
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(50);
  digitalWrite(OLED_RST, HIGH);
  display = new SSD1306Wire(0x3c, OLED_SDA, OLED_SCL, GEOMETRY_128_64);
#  endif
}

void OledSerial::begin() {
  // SSD1306.begin(); // User OMG serial support

  semaphoreOLEDOperation = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreOLEDOperation);

  display->init();
  display->flipScreenVertically();
  display->setFont(ArialMT_Plain_10);

  display->setColor(WHITE);
  display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
  display->display();
  ssd1306Intro(OLED_WIDTH * 0.24, (OLED_WIDTH / 2) - OLED_WIDTH * 0.2, (OLED_HEIGHT / 2) + OLED_HEIGHT * 0.2);
  display->setLogBuffer(OLED_TEXT_ROWS, OLED_TEXT_BUFFER);
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

void OledSerial::fillScreen(OLEDDISPLAY_COLOR color) {
  if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
    display->clear();
    display->setColor(color);
    display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
    xSemaphoreGive(semaphoreOLEDOperation);
  }
}

size_t OledSerial::write(const uint8_t* buffer, size_t size) {
  if (xPortGetCoreID() == CONFIG_ARDUINO_RUNNING_CORE) {
    if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
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

void OledSerial::displayPage(displayQueueMessage* message) {
  if (xPortGetCoreID() == CONFIG_ARDUINO_RUNNING_CORE) {
    if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
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
      return;
    }
  }
  free(&message);
}

/*
Display OpenMQTTGateway logo - borrowed from ZboardM5.ino and tweaked for ssd1306 display ( removed color and tweaked size/location )
*/

void OledSerial::ssd1306Intro(int scale, int displayWidth, int displayHeight) {
  drawLogo(scale, displayWidth, displayHeight, false, true, false, false, false, false); // Circle 2
  drawLogo(scale, displayWidth, displayHeight, false, false, true, false, false, false); // Circle 3
  drawLogo(scale, displayWidth, displayHeight, false, true, true, true, false, false); // Line 1
  drawLogo(scale, displayWidth, displayHeight, false, true, true, false, true, false); // Line 2
  drawLogo(scale, displayWidth, displayHeight, true, true, true, true, true, false); // Circle 1
  drawLogo(scale, displayWidth, displayHeight, true, true, true, true, true, true); // Name
}

void OledSerial::drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool line1, bool line2, bool name) {
  if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
    int circle1T = logoSize / 15;
    int circle2T = logoSize / 25;
    int circle3T = logoSize / 30;

    int circle3Y = circle1Y - (logoSize * 1.2);
    int circle3X = circle1X - (logoSize * 0.13);
    int circle2X = circle1X - (logoSize * 1.05);
    int circle2Y = circle1Y - (logoSize * 0.8);

    if (line1) {
      display->setColor(BLACK);
      display->drawLine(circle1X - 2, circle1Y, circle2X - 2, circle2Y);
      display->drawLine(circle1X - 1, circle1Y, circle2X - 1, circle2Y);
      display->drawLine(circle1X, circle1Y, circle2X, circle2Y);
      display->drawLine(circle1X + 1, circle1Y, circle2X + 1, circle2Y);
      display->drawLine(circle1X + 2, circle1Y, circle2X + 2, circle2Y);
      display->setColor(WHITE);
      display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2); // , WHITE);
    }
    if (line2) {
      display->setColor(BLACK);
      display->drawLine(circle1X - 2, circle1Y, circle3X - 2, circle3Y);
      display->drawLine(circle1X - 1, circle1Y, circle3X - 1, circle3Y);
      display->drawLine(circle1X, circle1Y, circle3X, circle3Y);
      display->drawLine(circle1X + 1, circle1Y, circle3X + 1, circle3Y);
      display->setColor(WHITE);
      display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2); // , WHITE);
    }
    if (circle1) {
      display->setColor(WHITE);
      display->fillCircle(circle1X, circle1Y, logoSize / 2); // , WHITE);
      display->setColor(BLACK);
      display->fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T); // , TFT_GREEN);
      display->setColor(WHITE);
      display->fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T * 2); // , WHITE);
    }
    if (circle2) {
      display->setColor(WHITE);
      display->fillCircle(circle2X, circle2Y, logoSize / 3); // , WHITE);
      display->setColor(BLACK);
      display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T); // , TFT_ORANGE);
      display->setColor(WHITE);
      display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2); // , WHITE);
    }
    if (circle3) {
      display->setColor(WHITE);
      display->fillCircle(circle3X, circle3Y, logoSize / 4); // , WHITE);
      display->setColor(BLACK);
      display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T); // , TFT_PINK);
      display->setColor(WHITE);
      display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2); // , WHITE);
    }
    if (name) {
      display->setColor(BLACK);
      display->drawString(circle1X + (circle1X * 0.27), circle1Y, "penMQTTGateway");
    }
    display->display();
    delay(50);
    xSemaphoreGive(semaphoreOLEDOperation);
  }
}

#endif
