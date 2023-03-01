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
boolean logToLCDDisplay = LOG_TO_LCD;
boolean jsonDisplay = JSON_TO_LCD;
boolean displayMetric = DISPLAY_METRIC;
boolean displayFlip = DISPLAY_FLIP;
boolean displayState = DISPLAY_STATE;
boolean idlelogo = DISPLAY_IDLE_LOGO;
uint8_t displayBrightness = round(DISPLAY_BRIGHTNESS * 2.55);

/*
Toogle log display
*/
void logToLCD(bool display) {
  logToLCDDisplay = display;
  display ? Log.begin(LOG_LEVEL_LCD, &Oled) : Log.begin(LOG_LEVEL, &Serial); // Log on LCD following LOG_LEVEL_LCD
}

/*
module setup, for use in Arduino setup
*/
void setupSSD1306() {
  Log.trace(F("Setup SSD1306 Display" CR));
  Log.trace(F("ZdisplaySSD1306 command topic: %s" CR), subjectMQTTtoSSD1306set);
  Log.trace(F("ZdisplaySSD1306 log-lcd: %T" CR), logToLCDDisplay);
  Log.trace(F("ZdisplaySSD1306 json-lcd: %T" CR), jsonDisplay);
  Log.trace(F("ZdisplaySSD1306 DISPLAY_PAGE_INTERVAL: %d" CR), DISPLAY_PAGE_INTERVAL);
  Log.trace(F("ZdisplaySSD1306 DISPLAY_IDLE_LOGO: %T" CR), DISPLAY_IDLE_LOGO);
  Log.trace(F("ZdisplaySSD1306 DISPLAY_METRIC: %T" CR), displayMetric);
  Log.trace(F("ZdisplaySSD1306 DISPLAY_FLIP: %T" CR), displayFlip);

  Oled.begin();
  Log.notice(F("Setup SSD1306 Display end" CR));

#  if LOG_TO_LCD
  Log.begin(LOG_LEVEL_LCD, &Oled); // Log on LCD following LOG_LEVEL_LCD
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
  if (jsonDisplay) {
    if (uptime() >= nextDisplayPage && uxSemaphoreGetCount(semaphoreOLEDOperation) && uxQueueMessagesWaiting(displayQueue)) {
      displayQueueMessage* message = nullptr;
      xQueueReceive(displayQueue, &message, portMAX_DELAY);
      if (!Oled.displayPage(message)) {
        Log.warning(F("[ssd1306] displayPage failed: %s" CR), message->title);
      }
      free(message);
      nextDisplayPage = uptime() + DISPLAY_PAGE_INTERVAL;
      logoDisplayed = false;
    }
  }
  /*
  Display logo if it has been more than DISPLAY_PAGE_INTERVAL
  */
  if (uptime() > nextDisplayPage + 1 && !logoDisplayed && idlelogo) {
    Oled.fillScreen(BLACK);
    Oled.drawLogo(rand() % 13 - 5, rand() % 32 - 13);
    logoDisplayed = true;
  }
}

/*
Handler for mqtt commands sent to the module
- log-lcd: boolean
  Enable / Disable display of log messages on display
*/
void MQTTtoSSD1306(char* topicOri, JsonObject& SSD1306data) { // json object decoding
  bool success = false;
  if (cmpToMainTopic(topicOri, subjectMQTTtoSSD1306set)) {
    Log.trace(F("MQTTtoSSD1306 json set" CR));
    // Log display set between SSD1306 lcd (true) and serial monitor (false)
    if (SSD1306data.containsKey("onstate")) {
      if (displayState != SSD1306data["onstate"]) {
        displayState = SSD1306data["onstate"];
        Oled.begin();
      }
      displayState = SSD1306data["onstate"];
      Log.notice(F("Set display state: %T" CR), logToLCDDisplay);
      success = true;
    } else if (SSD1306data.containsKey("brightness")) {
      displayBrightness = SSD1306data["brightness"];
      displayBrightness = round(displayBrightness * 2.55);
      Oled.display->setBrightness(displayBrightness);
      Log.notice(F("Set brightness: %d" CR), displayBrightness);
      success = true;
    } else if (SSD1306data.containsKey("log-lcd")) {
      logToLCDDisplay = SSD1306data["log-lcd"];
      Log.notice(F("Set lcd log: %T" CR), logToLCDDisplay);
      logToLCD(logToLCDDisplay);
      if (logToLCDDisplay) {
        jsonDisplay = false;
      }
      success = true;
    } else if (SSD1306data.containsKey("json-lcd")) {
      jsonDisplay = SSD1306data["json-lcd"];
      if (jsonDisplay) {
        logToLCDDisplay = false;
        logToLCD(logToLCDDisplay);
      }
      Log.notice(F("Set json-lcd: %T" CR), jsonDisplay);
      success = true;
    } else if (SSD1306data.containsKey("display-metric")) {
      displayMetric = SSD1306data["display-metric"];
      Log.notice(F("Set display-metric: %T" CR), displayMetric);
      success = true;
    } else if (SSD1306data.containsKey("idlelogo")) {
      idlelogo = SSD1306data["idlelogo"];
      success = true;
    } else if (SSD1306data.containsKey("display-flip")) {
      displayFlip = SSD1306data["display-flip"];
      Log.notice(F("Set display-flip: %T" CR), displayFlip);
      if (displayFlip) {
        Oled.display->flipScreenVertically();
      } else {
        Oled.display->resetOrientation();
      }
      success = true;
    }
    if (success) {
      stateSSD1306Display();
    } else {
      pub(subjectSSD1306toMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("[ SSD1306 ] MQTTtoSSD1306 Fail json" CR), SSD1306data);
    }
  }
}

/*
Workaround for c not having a string based switch/case function
*/
constexpr unsigned int hash(const char* s, int off = 0) { // workaround for switching on a string https://stackoverflow.com/a/46711735/18643696
  return !s[off] ? 5381 : (hash(s, off + 1) * 33) ^ s[off];
}

/*
Parse json message from module into a format for displaying on screen, and queue for display
*/
void ssd1306PubPrint(const char* topicori, JsonObject& data) {
  if (jsonDisplay) {
    displayQueueMessage* message = (displayQueueMessage*)malloc(sizeof(displayQueueMessage));
    if (message != NULL) {
      char* topic = strdup(topicori);
      strlcpy(message->title, strtok(topic, "/"), OLED_TEXT_WIDTH);
      free(topic);

      Oled.display->normalDisplay();
      // Oled.display->normalDisplay();

      switch (hash(message->title)) {
        case hash("SYStoMQTT"): {
          // {"uptime":456356,"version":"lilygo-rtl_433-test-A-v1.1.1-25-g574177d[lily-cloud]","freemem":125488,"mqttport":"1883","mqttsecure":false,"freestack":3752,"rssi":-36,"SSID":"The_Beach","BSSID":"64:A5:C3:69:C3:38","ip":"192.168.1.239","mac":"4C:75:25:A8:D5:D8","actRec":3,"mhz":433.92,"RTLRssiThresh":-98,"RTLRssi":-108,"RTLAVGRssi":-107,"RTLCnt":121707,"RTLOOKThresh":90,"modules":["LILYGO_OLED","CLOUD","rtl_433"]}

          // Line 1

          strlcpy(message->line1, data["version"], OLED_TEXT_WIDTH);

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
            Log.error(F("[ SSD1306 ] ERROR: displayQueue full, discarding signal %s" CR), message->title);
          } else {
            // Log.notice(F("Queued %s" CR), message->title);
          }
          break;
        }

#  ifdef ZgatewayRTL_433
        case hash("RTL_433toMQTT"): {
          // {"model":"Acurite-Tower","id":2043,"channel":"B","battery_ok":1,"temperature_C":5.3,"humidity":81,"mic":"CHECKSUM","protocol":"Acurite 592TXR Temp/Humidity, 5n1 Weather Station, 6045 Lightning, 3N1, Atlas","rssi":-81,"duration":121060}

          // Line 1

          strlcpy(message->line1, data["model"], OLED_TEXT_WIDTH);

          // Line 2

          String id = data["id"];
          String channel = data["channel"];
          String line2 = "id: " + id + " channel: " + channel;
          line2.toCharArray(message->line2, OLED_TEXT_WIDTH);

          // Line 3

          String line3 = "";

          if (data.containsKey("temperature_C")) {
            float temperature_C = data["temperature_C"];
            char temp[5];

            if (displayMetric) {
              dtostrf(temperature_C, 3, 1, temp);
              line3 = "temp: " + (String)temp + "°C ";
            } else {
              dtostrf(convertTemp_CtoF(temperature_C), 3, 1, temp);
              line3 = "temp: " + (String)temp + "°F ";
            }
          }

          float humidity = data["humidity"];
          if (data.containsKey("humidity") && humidity <= 100 && humidity >= 0) {
            char hum[5];
            dtostrf(humidity, 3, 1, hum);
            line3 += "hum: " + (String)hum + "% ";
          }
          if (data.containsKey("wind_avg_km_h")) {
            float wind_avg_km_h = data["wind_avg_km_h"];
            char wind[6];

            if (displayMetric) {
              dtostrf(wind_avg_km_h, 3, 1, wind);
              line3 += "wind: " + (String)wind + "km/h ";
            } else {
              dtostrf(convert_kmph2mph(wind_avg_km_h), 3, 1, wind);
              line3 += "wind: " + (String)wind + "mp/h ";
            }
          }

          line3.toCharArray(message->line3, OLED_TEXT_WIDTH);

          // Line 4

          String rssi = data["rssi"];
          String battery_ok = data["battery_ok"];

          String line4 = "batt: " + battery_ok + " rssi: " + rssi;
          line4.toCharArray(message->line4, OLED_TEXT_WIDTH);

          // Queue completed message

          if (xQueueSend(displayQueue, (void*)&message, 0) != pdTRUE) {
            Log.error(F("[ SSD1306 ] displayQueue full, discarding signal %s" CR), message->title);
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

          strlcpy(message->line1, "bme280", OLED_TEXT_WIDTH);

          // Line 2

          String line2 = "";
          if (data.containsKey("tempc")) {
            char temp[5];
            float temperature_C = data["tempc"];

            if (displayMetric) {
              dtostrf(temperature_C, 3, 1, temp);
              line2 = "temp: " + (String)temp + "°C ";
            } else {
              dtostrf(convertTemp_CtoF(temperature_C), 3, 1, temp);
              line2 = "temp: " + (String)temp + "°F ";
            }
          }
          line2.toCharArray(message->line2, OLED_TEXT_WIDTH);

          // Line 3

          String line3 = "";
          float humidity = data["hum"];
          if (data.containsKey("hum") && humidity <= 100 && humidity >= 0) {
            char hum[5];
            dtostrf(humidity, 3, 1, hum);
            line3 += "hum: " + (String)hum + "% ";
          }
          line3.toCharArray(message->line3, OLED_TEXT_WIDTH);

          // Line 4

          float pa = (int)data["pa"] / 100;
          char pressure[6];

          String line4 = "";
          if (displayMetric) {
            dtostrf(pa, 3, 1, pressure);
            line4 = "pressure: " + (String)pressure + " hPa";
          } else {
            dtostrf(convert_hpa2inhg(pa), 3, 1, pressure);
            line4 = "pressure: " + (String)pressure + " inHg";
          }
          line4.toCharArray(message->line4, OLED_TEXT_WIDTH);

          // Queue completed message

          if (xQueueSend(displayQueue, (void*)&message, 0) != pdTRUE) {
            Log.error(F("[ SSD1306 ] displayQueue full, discarding signal %s" CR), message->title);
            free(message);
          } else {
            // Log.notice(F("Queued %s" CR), message->title);
          }
          break;
        }
#  endif
#  ifdef ZgatewayBT
        case hash("BTtoMQTT"): {
          // {"id":"49:22:07:07:1C:F6","mac_type":0,"name":"sps","rssi":-87,"brand":"Inkbird","model":"TH Sensor","model_id":"IBS-TH1","cidc":false,"tempc":0.8,"tempf":33.44,"hum":29.61,"batt":100}

          // Line 1

          strlcpy(message->line1, data["brand"], OLED_TEXT_WIDTH);

          // Line 2

          strlcpy(message->line2, data["model_id"], OLED_TEXT_WIDTH);

          // Line 3

          String line3 = "";

          if (data.containsKey("tempc")) {
            float temperature_C = data["tempc"];
            char temp[5];

            if (displayMetric) {
              dtostrf(temperature_C, 3, 1, temp);
              line3 = "temp: " + (String)temp + "°C ";
            } else {
              dtostrf(convertTemp_CtoF(temperature_C), 3, 1, temp);
              line3 = "temp: " + (String)temp + "°F ";
            }
          }

          float humidity = data["hum"];
          if (data.containsKey("hum") && humidity <= 100 && humidity >= 0) {
            char hum[5];
            dtostrf(humidity, 3, 1, hum);
            line3 += "hum: " + (String)hum + "% ";
          }

          line3.toCharArray(message->line3, OLED_TEXT_WIDTH);

          // Line 4

          String rssi = data["rssi"];
          String battery_ok = data["batt"];

          String line4;
          if (data.containsKey("batt"))
            line4 += "batt: " + battery_ok + " ";

          line4 += "rssi: " + rssi;
          line4.toCharArray(message->line4, OLED_TEXT_WIDTH);

          // Queue completed message

          if (xQueueSend(displayQueue, (void*)&message, 0) != pdTRUE) {
            Log.error(F("[ SSD1306 ] displayQueue full, discarding signal %s" CR), message->title);
            free(message);
          } else {
            // Log.notice(F("Queued %s" CR), message->title);
          }
          break;
        }
#  endif
        default:
          Log.error(F("[ SSD1306 ] unhandled topic %s" CR), message->title);
      }
    } else {
      Log.error(F("[ SSD1306 ] insufficent memory " CR));
    }
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

/*
Initialize ssd1306 oled display for use, and display animated OMG logo
*/
void OledSerial::begin() {
  // SSD1306.begin(); // User OMG serial support

  semaphoreOLEDOperation = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreOLEDOperation);

  display->init();
  if (displayState) {
    if (displayFlip) {
      display->flipScreenVertically();
    } else {
      display->resetOrientation();
    }
    display->setFont(ArialMT_Plain_10);
    display->setBrightness(displayBrightness);
    drawLogo(0, 0);
    display->invertDisplay();
    display->setLogBuffer(OLED_TEXT_ROWS, OLED_TEXT_BUFFER);
    delay(1000);
  } else {
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
boolean OledSerial::displayPage(displayQueueMessage* message) {
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

void stateSSD1306Display() {
  //Publish display state
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject DISPLAYdata = jsonBuffer.to<JsonObject>();
  DISPLAYdata["onstate"] = (bool)displayState;
  DISPLAYdata["brightness"] = (int)round(displayBrightness / 2.55);
  DISPLAYdata["display-metric"] = (bool)displayMetric;
  DISPLAYdata["display-flip"] = (bool)displayFlip;
  DISPLAYdata["idlelogo"] = (bool)idlelogo;
  DISPLAYdata["log-lcd"] = (bool)logToLCDDisplay;
  DISPLAYdata["json-lcd"] = (bool)jsonDisplay;
  pub(subjectSSD1306toMQTT, DISPLAYdata);
}

#endif
