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
    }
  }
  /*
  Display logo if it has been more than DISPLAY_PAGE_INTERVAL
  */
#  if DISPLAY_IDLE_LOGO
  if (uptime() > nextDisplayPage + 1 && !logoDisplayed) {
    Oled.fillScreen(WHITE);
    Oled.drawLogo();
    logoDisplayed = true;
  } else {
    logoDisplayed = false;
  }
#  endif
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
    if (SSD1306data.containsKey("log-lcd")) {
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
      pub(subjectSSD1306toMQTTset, SSD1306data);
    } else {
      pub(subjectSSD1306toMQTTset, "{\"Status\": \"Error\"}"); // Fail feedback
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
          // {"id":"AA:BB:CC:DD:EE:FF","mac_type":0,"adv_type":0,"name":"sps","manufacturerdata":"de071f1000b1612908","rssi":-70,"brand":"Inkbird","model":"T(H) Sensor","model_id":"IBS-TH1/TH2/P01B","type":"THBX","cidc":false,"acts":true,"tempc":20.14,"tempf":68.252,"hum":41.27,"batt":41}

          if (data["model_id"] != "MS-CDP" && data["model_id"] != "GAEN" && data["model_id"] != "IBEACON") {
            // Line 1
            strlcpy(message->line1, data["model"], OLED_TEXT_WIDTH);

            // Line 2, 3, 4
            String line2 = "";
            String line3 = "";
            String line4 = "";

            // Properties
            String properties[6] = {"", "", "", "", "", ""};
            int property = -1;

            if (data["type"] == "THB" || data["type"] == "THBX" || data["type"] == "PLANT" || data["type"] == "AIR" || data["type"] == "BATT" || (data["type"] == "UNIQ" && data["model_id"] == "SDLS")) {
              if (data.containsKey("tempc")) {
                property++;
                char temp[5];
                if (displayMetric) {
                  float temperature = data["tempc"];
                  dtostrf(temperature, 3, 1, temp);
                  properties[property] = "temp: " + (String)temp + "°C ";
                } else {
                  float temperature = data["tempf"];
                  dtostrf(temperature, 3, 1, temp);
                  properties[property] = "temp: " + (String)temp + "°F ";
                }
              }

              if (data.containsKey("tempc2_dp")) {
                property++;
                char tempdp[5];
                if (displayMetric) {
                  float temperature = data["tempc2_dp"];
                  dtostrf(temperature, 3, 1, tempdp);
                  properties[property] = "dewp: " + (String)tempdp + "°C ";
                } else {
                  float temperature = data["tempf2_dp"];
                  dtostrf(temperature, 3, 1, tempdp);
                  properties[property] = "dewp: " + (String)tempdp + "°F ";
                }
              }

              if (data.containsKey("extprobe")) {
                property++;
                properties[property] = " ext. probe";
              }

              if (data.containsKey("hum")) {
                property++;
                float humidity = data["hum"];
                char hum[5];

                dtostrf(humidity, 3, 1, hum);
                properties[property] = "hum: " + (String)hum + "% ";
              }

              if (data.containsKey("pm25")) {
                property++;
                int pm25int = data["pm25"];
                char pm25[3];
                itoa(pm25int, pm25, 10);
                if ((data.containsKey("pm10"))) {
                  properties[property] = "PM 2.5: " + (String)pm25 + " ";

                } else {
                  properties[property] = "pm2.5: " + (String)pm25 + "μg/m³ ";
                }
              }

              if (data.containsKey("pm10")) {
                property++;
                int pm10int = data["pm10"];
                char pm10[3];
                itoa(pm10int, pm10, 10);
                if ((data.containsKey("pm25"))) {
                  properties[property] = "/ 10: " + (String)pm10 + "μg/m³ ";

                } else {
                  properties[property] = "pm10: " + (String)pm10 + "μg/m³ ";
                }
              }

              if (data.containsKey("for")) {
                property++;
                int formint = data["for"];
                char form[3];
                itoa(formint, form, 10);
                properties[property] = "CH₂O: " + (String)form + "mg/m³ ";
              }

              if (data.containsKey("co2")) {
                property++;
                int co2int = data["co2"];
                char co2[4];
                itoa(co2int, co2, 10);
                properties[property] = "co2: " + (String)co2 + "ppm ";
              }

              if (data.containsKey("moi")) {
                property++;
                int moiint = data["moi"];
                char moi[4];
                itoa(moiint, moi, 10);
                properties[property] = "moi: " + (String)moi + "% ";
              }

              if (data.containsKey("lux")) {
                property++;
                int luxint = data["lux"];
                char lux[5];
                itoa(luxint, lux, 10);
                properties[property] = "lux: " + (String)lux + "lx ";
              }

              if (data.containsKey("fer")) {
                property++;
                int ferint = data["fer"];
                char fer[7];
                itoa(ferint, fer, 10);
                properties[property] = "fer: " + (String)fer + "µS/cm ";
              }

              if (data.containsKey("pres")) {
                property++;
                int presint = data["pres"];
                char pres[4];
                itoa(presint, pres, 10);
                properties[property] = "pres: " + (String)pres + "hPa ";
              }

              if (data.containsKey("batt")) {
                property++;
                int battery = data["batt"];
                char batt[5];
                itoa(battery, batt, 10);
                properties[property] = "batt: " + (String)batt + "% ";
              }

              if (data.containsKey("shake")) {
                property++;
                int shakeint = data["shake"];
                char shake[3];
                itoa(shakeint, shake, 10);
                properties[property] = "shake: " + (String)shake + " ";
              }

              if (data.containsKey("volt")) {
                property++;
                float voltf = data["volt"];
                char volt[5];
                dtostrf(voltf, 3, 1, volt);
                properties[property] = "volt: " + (String)volt + "V ";
              }

              if (data.containsKey("wake")) {
                property++;
                String wakestr = data["wake"];
                properties[property] = "wake: " + wakestr + " ";
              }

            } else if (data["type"] == "BBQ") {
              String tempcstr = "";
              int j = 7;
              if (data["model_id"] == "IBT-2X(S)") {
                j = 3;
              } else if (data["model_id"] == "IBT-4X(S/C)") {
                j = 5;
              }

              for (int i = 0; i < j; i++) {
                if (i == 0) {
                  if (displayMetric) {
                    tempcstr = "tempc";
                  } else {
                    tempcstr = "tempf";
                  }
                  i++;
                } else {
                  if (displayMetric) {
                    tempcstr = "tempc" + (String)i;
                  } else {
                    tempcstr = "tempf" + (String)i;
                  }
                }

                if (data.containsKey(tempcstr)) {
                  char temp[5];
                  float temperature = data[tempcstr];
                  dtostrf(temperature, 3, 1, temp);
                  properties[i - 1] = "tp" + (String)i + ": " + (String)temp;
                  if (displayMetric) {
                    properties[i - 1] += "°C ";
                  } else {
                    properties[i - 1] += "°F ";
                  }
                } else {
                  properties[i - 1] = "tp" + (String)i + ": " + "off ";
                }
              }
            } else if (data["type"] == "BODY") {
              if (data.containsKey("steps")) {
                property++;
                int stepsint = data["steps"];
                char steps[5];
                itoa(stepsint, steps, 10);
                properties[property] = "steps: " + (String)steps + " ";
                // next line
                property++;
              }

              if (data.containsKey("act_bpm")) {
                property++;
                int actbpmint = data["act_bpm"];
                char actbpm[3];
                itoa(actbpmint, actbpm, 10);
                properties[property] = "activity bpm: " + (String)actbpm + " ";
              }

              if (data.containsKey("bpm")) {
                property++;
                int bpmint = data["bpm"];
                char bpm[3];
                itoa(bpmint, bpm, 10);
                properties[property] = "bpm: " + (String)bpm + " ";
              }
            } else if (data["type"] == "SCALE") {
              if (data.containsKey("weighing_mode")) {
                property++;
                String mode = data["weighing_mode"];
                properties[property] = mode + " ";
                // next line
                property++;
              }

              if (data.containsKey("weight")) {
                property++;
                float weightf = data["weight"];
                char weight[7];
                dtostrf(weightf, 3, 1, weight);
                if (data.containsKey("unit")) {
                  String unit = data["unit"];
                  properties[property] = "weight: " + (String)weight + unit + " ";
                } else {
                  properties[property] = "weight: " + (String)weight;
                }
                // next line
                property++;
              }

              if (data.containsKey("impedance")) {
                property++;
                int impint = data["impedance"];
                char imp[3];
                itoa(impint, imp, 10);
                properties[property] = "impedance: " + (String)imp + "ohm ";
              }
            } else if (data["type"] == "UNIQ") {
              if (data["model_id"] == "M1017") {
                if (data.containsKey("lvl_cm")) {
                  property++;
                  char lvl[5];
                  if (displayMetric) {
                    float lvlf = data["lvl_cm"];
                    dtostrf(lvlf, 3, 1, lvl);
                    properties[property] = "level: " + (String)lvl + "cm ";
                  } else {
                    float lvlf = data["lvl_in"];
                    dtostrf(lvlf, 3, 1, lvl);
                    properties[property] = "level: " + (String)lvl + "\" ";
                  }
                }

                if (data.containsKey("quality")) {
                  property++;
                  int qualint = data["quality"];
                  char qual[3];
                  itoa(qualint, qual, 10);
                  properties[property] = "qy: " + (String)qual + " ";
                }

                if (data.containsKey("batt")) {
                  property++;
                  int battery = data["batt"];
                  char batt[5];
                  itoa(battery, batt, 10);
                  properties[property] = "batt: " + (String)batt + "% ";
                }
              }
            }

            line2 = properties[0] + properties[1];
            line3 = properties[2] + properties[3];
            line4 = properties[4] + properties[5];

            line2.toCharArray(message->line2, OLED_TEXT_WIDTH);
            line3.toCharArray(message->line3, OLED_TEXT_WIDTH);
            line4.toCharArray(message->line4, OLED_TEXT_WIDTH);

            if (xQueueSend(displayQueue, (void*)&message, 0) != pdTRUE) {
              Log.error(F("[ SSD1306 ] displayQueue full, discarding signal %s" CR), message->title);
              free(message);
            } else {
              // Log.notice(F("Queued %s" CR), message->title);
            }
            break;
          }
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
  if (displayFlip) {
    display->flipScreenVertically();
  } else {
    display->resetOrientation();
  }
  display->setFont(ArialMT_Plain_10);
  display->setColor(WHITE);
  display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
  display->display();
  drawLogo();
  display->setLogBuffer(OLED_TEXT_ROWS, OLED_TEXT_BUFFER);
  delay(1000);
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
void OledSerial::drawLogo() {
  if (xSemaphoreTake(semaphoreOLEDOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
    display->setColor(BLACK);

    // line 1
    display->drawLine(15, 28, 20, 31);
    display->drawLine(15, 29, 20, 32);
    // line 2
    display->drawLine(25, 29, 22, 21);
    display->drawLine(26, 29, 23, 21);
    // circle 1
    display->fillCircle(25, 35, 7);
    display->setColor(WHITE);
    display->fillCircle(25, 35, 5);
    // circle 2
    display->setColor(BLACK);
    display->fillCircle(23, 18, 4);
    display->setColor(WHITE);
    display->fillCircle(23, 18, 2);
    // circle 3
    display->setColor(BLACK);
    display->fillCircle(11, 25, 5);
    display->setColor(WHITE);
    display->fillCircle(11, 25, 3);
    // name
    display->setColor(BLACK);
    display->drawString(32, 32, "penMQTTGateway");

    display->display();
    delay(50);
    xSemaphoreGive(semaphoreOLEDOperation);
  }
}

#endif
