/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT

    Copyright: (c)Florian ROBERT
  
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
#if defined(ZwebUI) && defined(ESP32)
#  include <ArduinoJson.h>
#  include <SPIFFS.h>
#  include <WebServer.h> // Docs for this are here - https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer

#  include "ArduinoLog.h"
#  include "config_WebContent.h"
#  include "config_WebUI.h"

#  if defined(ZgatewayCloud)
#    include "config_Cloud.h"
#  endif

#  if defined(ZdisplaySSD1306)
#    include "config_SSD1306.h"
#  endif

uint32_t requestToken = 0;

QueueHandle_t webUIQueue;

WebServer server(80);

/*------------------- External functions ----------------------*/
extern void eraseAndRestart();
extern unsigned long uptime();

/*------------------- Web Console Globals ----------------------*/

#  define ROW_LENGTH 1024

const uint16_t LOG_BUFFER_SIZE = 6096;
uint32_t log_buffer_pointer;
void* log_buffer_mutex;
char log_buffer[LOG_BUFFER_SIZE]; // Log buffer in HEAP

const uint16_t MAX_LOGSZ = LOG_BUFFER_SIZE - 96;
const uint16_t TOPSZ = 151; // Max number of characters in topic string
uint8_t masterlog_level; // Master log level used to override set log level
bool reset_web_log_flag = false; // Reset web console log

const char* www_username = WEBUI_LOGIN;
String authFailResponse = "Authentication Failed";
bool webUISecure = WEBUI_AUTH;
boolean displayMetric = DISPLAY_METRIC;

/*********************************************************************************************\
 * ESP32 AutoMutex
\*********************************************************************************************/

//////////////////////////////////////////
// automutex.
// create a mute in your driver with:
// void *mutex = nullptr;
//
// then protect any function with
// TasAutoMutex m(&mutex, "somename");
// - mutex is automatically initialised if not already intialised.
// - it will be automagically released when the function is over.
// - the same thread can take multiple times (recursive).
// - advanced options m.give() and m.take() allow you fine control within a function.
// - if take=false at creat, it will not be initially taken.
// - name is used in serial log of mutex deadlock.
// - maxWait in ticks is how long it will wait before failing in a deadlock scenario (and then emitting on serial)
class TasAutoMutex {
  SemaphoreHandle_t mutex;
  bool taken;
  int maxWait;
  const char* name;

public:
  TasAutoMutex(SemaphoreHandle_t* mutex, const char* name = "", int maxWait = 40, bool take = true);
  ~TasAutoMutex();
  void give();
  void take();
  static void init(SemaphoreHandle_t* ptr);
};
//////////////////////////////////////////

TasAutoMutex::TasAutoMutex(SemaphoreHandle_t* mutex, const char* name, int maxWait, bool take) {
  if (mutex) {
    if (!(*mutex)) {
      TasAutoMutex::init(mutex);
    }
    this->mutex = *mutex;
    this->maxWait = maxWait;
    this->name = name;
    if (take) {
      this->taken = xSemaphoreTakeRecursive(this->mutex, this->maxWait);
      //      if (!this->taken){
      //        Serial.printf("\r\nMutexfail %s\r\n", this->name);
      //      }
    }
  } else {
    this->mutex = (SemaphoreHandle_t) nullptr;
  }
}

TasAutoMutex::~TasAutoMutex() {
  if (this->mutex) {
    if (this->taken) {
      xSemaphoreGiveRecursive(this->mutex);
      this->taken = false;
    }
  }
}

void TasAutoMutex::init(SemaphoreHandle_t* ptr) {
  SemaphoreHandle_t mutex = xSemaphoreCreateRecursiveMutex();
  (*ptr) = mutex;
  // needed, else for ESP8266 as we will initialis more than once in logging
  //  (*ptr) = (void *) 1;
}

void TasAutoMutex::give() {
  if (this->mutex) {
    if (this->taken) {
      xSemaphoreGiveRecursive(this->mutex);
      this->taken = false;
    }
  }
}

void TasAutoMutex::take() {
  if (this->mutex) {
    if (!this->taken) {
      this->taken = xSemaphoreTakeRecursive(this->mutex, this->maxWait);
      //      if (!this->taken){
      //        Serial.printf("\r\nMutexfail %s\r\n", this->name);
      //      }
    }
  }
}

// Get span until single character in string
size_t strchrspn(const char* str1, int character) {
  size_t ret = 0;
  char* start = (char*)str1;
  char* end = strchr(str1, character);
  if (end) ret = end - start;
  return ret;
}

int WifiGetRssiAsQuality(int rssi) {
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

char* GetTextIndexed(char* destination, size_t destination_size, uint32_t index, const char* haystack) {
  // Returns empty string if not found
  // Returns text of found
  char* write = destination;
  const char* read = haystack;

  index++;
  while (index--) {
    size_t size = destination_size - 1;
    write = destination;
    char ch = '.';
    while ((ch != '\0') && (ch != '|')) {
      ch = pgm_read_byte(read++);
      if (size && (ch != '|')) {
        *write++ = ch;
        size--;
      }
    }
    if (0 == ch) {
      if (index) {
        write = destination;
      }
      break;
    }
  }
  *write = '\0';
  return destination;
}

const char kUnescapeCode[] = "&><\"\'\\";
const char kEscapeCode[] PROGMEM = "&amp;|&gt;|&lt;|&quot;|&apos;|&#92;";

String HtmlEscape(const String unescaped) {
  char escaped[10];
  size_t ulen = unescaped.length();
  String result;
  result.reserve(ulen); // pre-reserve the required space to avoid mutiple reallocations
  for (size_t i = 0; i < ulen; i++) {
    char c = unescaped[i];
    char* p = strchr(kUnescapeCode, c);
    if (p != nullptr) {
      result += GetTextIndexed(escaped, sizeof(escaped), p - kUnescapeCode, kEscapeCode);
    } else {
      result += c;
    }
  }
  return result;
}

void AddLogData(uint32_t loglevel, const char* log_data, const char* log_data_payload = nullptr, const char* log_data_retained = nullptr) {
  // Store log_data in buffer
  // To lower heap usage log_data_payload may contain the payload data from MqttPublishPayload()
  //  and log_data_retained may contain optional retained message from MqttPublishPayload()
#  ifdef ESP32
  // this takes the mutex, and will be release when the class is destroyed -
  // i.e. when the functon leaves  You CAN call mutex.give() to leave early.
  TasAutoMutex mutex((SemaphoreHandle_t*)&log_buffer_mutex);
#  endif // ESP32

  char empty[2] = {0};
  if (!log_data_payload) {
    log_data_payload = empty;
  }
  if (!log_data_retained) {
    log_data_retained = empty;
  }

  if (!log_buffer) {
    return;
  } // Leave now if there is no buffer available

  // Delimited, zero-terminated buffer of log lines.
  // Each entry has this format: [index][loglevel][log data]['\1']

  // Truncate log messages longer than MAX_LOGSZ which is the log buffer size minus 64 spare
  uint32_t log_data_len = strlen(log_data) + strlen(log_data_payload) + strlen(log_data_retained);
  char too_long[TOPSZ];
  if (log_data_len > MAX_LOGSZ) {
    snprintf_P(too_long, sizeof(too_long) - 20, PSTR("%s%s"), log_data, log_data_payload); // 20 = strlen("... 123456 truncated")
    snprintf_P(too_long, sizeof(too_long), PSTR("%s... %d truncated"), too_long, log_data_len);
    log_data = too_long;
    log_data_payload = empty;
    log_data_retained = empty;
  }

  log_buffer_pointer &= 0xFF;
  if (!log_buffer_pointer) {
    log_buffer_pointer++; // Index 0 is not allowed as it is the end of char string
  }
  while (log_buffer_pointer == log_buffer[0] || // If log already holds the next index, remove it
         strlen(log_buffer) + strlen(log_data) + strlen(log_data_payload) + strlen(log_data_retained) + 4 > LOG_BUFFER_SIZE) // 4 = log_buffer_pointer + '\1' + '\0'
  {
    char* it = log_buffer;
    it++; // Skip log_buffer_pointer
    it += strchrspn(it, '\1'); // Skip log line
    it++; // Skip delimiting "\1"
    memmove(log_buffer, it, LOG_BUFFER_SIZE - (it - log_buffer)); // Move buffer forward to remove oldest log line
  }
  snprintf_P(log_buffer, LOG_BUFFER_SIZE, PSTR("%s%c%c%s%s%s%s\1"),
             log_buffer, log_buffer_pointer++, '0' + loglevel, "", log_data, log_data_payload, log_data_retained);
  log_buffer_pointer &= 0xFF;
  if (!log_buffer_pointer) {
    log_buffer_pointer++; // Index 0 is not allowed as it is the end of char string
  }
}

bool NeedLogRefresh(uint32_t req_loglevel, uint32_t index) {
  if (!log_buffer) {
    return false;
  } // Leave now if there is no buffer available

#  ifdef ESP32
  // this takes the mutex, and will be release when the class is destroyed -
  // i.e. when the functon leaves  You CAN call mutex.give() to leave early.
  TasAutoMutex mutex((SemaphoreHandle_t*)&log_buffer_mutex);
#  endif // ESP32

  // Skip initial buffer fill
  if (strlen(log_buffer) < LOG_BUFFER_SIZE / 2) {
    return false;
  }

  char* line;
  size_t len;
  if (!GetLog(req_loglevel, &index, &line, &len)) {
    return false;
  }
  return ((line - log_buffer) < LOG_BUFFER_SIZE / 4);
}

bool GetLog(uint32_t req_loglevel, uint32_t* index_p, char** entry_pp, size_t* len_p) {
  if (!log_buffer) {
    return false;
  } // Leave now if there is no buffer available
  if (uptime() < 3) {
    return false;
  } // Allow time to setup correct log level

  uint32_t index = *index_p;
  if (!req_loglevel || (index == log_buffer_pointer)) {
    return false;
  }

#  ifdef ESP32
  // this takes the mutex, and will be release when the class is destroyed -
  // i.e. when the functon leaves  You CAN call mutex.give() to leave early.
  TasAutoMutex mutex((SemaphoreHandle_t*)&log_buffer_mutex);
#  endif // ESP32

  if (!index) { // Dump all
    index = log_buffer[0];
  }

  do {
    size_t len = 0;
    uint32_t loglevel = 0;
    char* entry_p = log_buffer;
    do {
      uint32_t cur_idx = *entry_p;
      entry_p++;
      size_t tmp = strchrspn(entry_p, '\1');
      tmp++; // Skip terminating '\1'
      if (cur_idx == index) { // Found the requested entry
        loglevel = *entry_p - '0';
        entry_p++; // Skip loglevel
        len = tmp - 1;
        break;
      }
      entry_p += tmp;
    } while (entry_p < log_buffer + LOG_BUFFER_SIZE && *entry_p != '\0');
    index++;
    if (index > 255) {
      index = 1;
    } // Skip 0 as it is not allowed
    *index_p = index;
    if ((len > 0) &&
        (loglevel <= req_loglevel) &&
        (masterlog_level <= req_loglevel)) {
      *entry_pp = entry_p;
      *len_p = len;
      return true;
    }
    delay(0);
  } while (index != log_buffer_pointer);
  return false;
}

/*------------------- Local functions ----------------------*/

#  ifdef WEBUI_DEVELOPMENT
//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

bool exists(String path) {
  bool yes = false;
  File file = FILESYSTEM.open(path, "r");
  if (!file.isDirectory()) {
    yes = true;
  }
  file.close();
  return yes;
}
#  endif

/**
 * @brief / - Page
 * 
 */
void handleRoot() {
  WEBUI_TRACE_LOG(F("handleRoot: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    if (server.hasArg("m")) {
      if (currentWebUIMessage) {
        server.send(200, "application/json", "{t}{s}<b>" + String(currentWebUIMessage->title) + "</b>{e}{s}" + String(currentWebUIMessage->line1) + "{e}{s}" + String(currentWebUIMessage->line2) + "{e}{s}" + String(currentWebUIMessage->line3) + "{e}{s}" + String(currentWebUIMessage->line4) + "{e}</table>");
      } else {
        server.send(200, "application/json", "{t}{s}Uptime:{m}" + String(uptime()) + "{e}</table>");
      }
    } else if (server.hasArg("rst")) { // TODO: This should redirect to the RST page
      Log.warning(F("[WebUI] Restart" CR));
      char jsonChar[100];
      serializeJson(modules, jsonChar, measureJson(modules) + 1);
      char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Restart").c_str());
      String response = String(buffer);
      response += String(restart_script);
      response += String(script);
      response += String(style);
      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, reset_body, jsonChar, gateway_name, "Restart");
      response += String(buffer);
      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
      response += String(buffer);
      server.send(200, "text/html", response);

      delay(2000); // Wait for web page to be sent before

      ESPRestart(5);
    } else {
      // WEBUI_TRACE_LOG(F("Arguments %s" CR), message);
      server.send(200, "text/plain", "00:14:36.767 RSL: RESULT = {\"Topic\":\"topic\"}");
    }
  } else {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);

    char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Main Menu").c_str());
    String response = String(buffer);
    response += String(root_script);
    response += String(script);
    response += String(style);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, root_body, jsonChar, gateway_name);
    response += String(buffer);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
    response += String(buffer);
    server.send(200, "text/html", response);
  }
}

/**
 * @brief /CN - Configuration Page
 * 
 */
void handleCN() {
  WEBUI_SECURE
  WEBUI_TRACE_LOG(F("handleCN: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleCN Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  } else {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);

    char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Configuration").c_str());
    String response = String(buffer);
    response += String(script);
    response += String(style);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, config_body, jsonChar, gateway_name);
    response += String(buffer);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
    response += String(buffer);
    server.send(200, "text/html", response);
  }
}

/**
 * @brief /WU - Configuration Page
 * T: handleWU: uri: /wu, args: 3, method: 1
 * T: handleWU Arg: 0, dm=on - displayMetric
 * T: handleWU Arg: 1, sw=on - webUISecure
 * T: handleWU Arg: 2, save=
 */
void handleWU() {
  WEBUI_TRACE_LOG(F("handleWU: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleWU Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    bool update = false;

    if (displayMetric != server.hasArg("dm")) {
      update = true;
    }
    displayMetric = server.hasArg("dm");

    if (webUISecure != server.hasArg("sw")) {
      update = true;
    }
    webUISecure = server.hasArg("sw");

    if (server.hasArg("save") && update) {
      WebUIConfig_save();
    }
  }

  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);

  char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Configure WebUI").c_str());
  String response = String(buffer);
  response += String(script);
  response += String(style);
  int logLevel = Log.getLevel();
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, config_webui_body, jsonChar, gateway_name, (displayMetric ? "checked" : ""), (webUISecure ? "checked" : ""));
  response += String(buffer);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
  response += String(buffer);
  server.send(200, "text/html", response);
}

/**
 * @brief /WI - Configure WiFi Page
 * T: handleWI: uri: /wi, args: 4, method: 1
 * T: handleWI Arg: 0, s1=SSID
 * T: handleWI Arg: 1, p1=xxxxxx
 * T: handleWI Arg: 3, save=
 */
void handleWI() {
  WEBUI_TRACE_LOG(F("handleWI: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  String WiFiScan = "";
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleWI Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    if (server.hasArg("scan")) {
      bool limitScannedNetworks = true;
      int n = WiFi.scanNetworks();

      WEBUI_TRACE_LOG(F("handleWI scan: found %d" CR), n);
      if (0 == n) {
        // WSContentSend_P(PSTR(D_NO_NETWORKS_FOUND));
        // limitScannedNetworks = false; // in order to show D_SCAN_FOR_WIFI_NETWORKS
      } else {
        //sort networks
        int indices[n];
        for (uint32_t i = 0; i < n; i++) {
          indices[i] = i;
        }

        // RSSI SORT
        for (uint32_t i = 0; i < n; i++) {
          for (uint32_t j = i + 1; j < n; j++) {
            if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
              std::swap(indices[i], indices[j]);
            }
          }
        }

        uint32_t networksToShow = n;
        if ((limitScannedNetworks) && (networksToShow > MAX_WIFI_NETWORKS_TO_SHOW)) {
          networksToShow = MAX_WIFI_NETWORKS_TO_SHOW;
        }

        for (uint32_t i = 0; i < n; i++) {
          if (-1 == indices[i]) {
            continue;
          }
          String cssid = WiFi.SSID(indices[i]);
          uint32_t cschn = WiFi.channel(indices[i]);
          for (uint32_t j = i + 1; j < n; j++) {
            if ((cssid == WiFi.SSID(indices[j])) && (cschn == WiFi.channel(indices[j]))) {
              WEBUI_TRACE_LOG(F("handleWI scan: duplicate %s" CR), WiFi.SSID(indices[j]).c_str());
              indices[j] = -1; // set dup aps to index -1
            }
          }
        }

        //display networks in page
        for (uint32_t i = 0; i < networksToShow; i++) {
          if (-1 == indices[i]) {
            continue;
          } // skip dups
          int32_t rssi = WiFi.RSSI(indices[i]);
          WEBUI_TRACE_LOG(F("D_LOG_WIFI D_SSID  %s, D_BSSID  %s,  D_CHANNEL  %d,  D_RSSI  %d" CR),
                          WiFi.SSID(indices[i]).c_str(), WiFi.BSSIDstr(indices[i]).c_str(), WiFi.channel(indices[i]), rssi);
          int quality = WifiGetRssiAsQuality(rssi);
          String ssid_copy = WiFi.SSID(indices[i]);
          if (!ssid_copy.length()) {
            ssid_copy = F("no_name");
          }

          WiFiScan += "<div><a href='#p' onclick='c(this)'>" + HtmlEscape(ssid_copy) + "</a>&nbsp;(" + WiFi.channel(indices[i]) + ")&nbsp<span class='q'>" + quality + "% (" + rssi + " dBm)</span></div>";
        }
      }
      WEBUI_TRACE_LOG(F("handleWI scan: results %s" CR), WiFiScan.c_str());

      char jsonChar[100];
      serializeJson(modules, jsonChar, measureJson(modules) + 1);

      char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Configure WiFi").c_str());
      String response = String(buffer);
      response += String(wifi_script);
      response += String(script);
      response += String(style);
      // wifi_ssid, wifi_password, gateway_name
      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, config_wifi_body, jsonChar, gateway_name, WiFiScan.c_str(), WiFi.SSID(), WiFi.psk());
      response += String(buffer);
      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
      response += String(buffer);
      server.send(200, "text/html", response);
      return;

    } else if (server.hasArg("save")) {
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject WEBtoSYS = jsonBuffer.to<JsonObject>();
      bool update = false;
      if (server.hasArg("s1")) {
        WEBtoSYS["wifi_ssid"] = server.arg("s1");
        if (strncmp((char*)WiFi.SSID().c_str(), server.arg("s1").c_str(), parameters_size)) {
          update = true;
        }
      }
      if (server.hasArg("p1")) {
        WEBtoSYS["wifi_pass"] = server.arg("p1");
        if (strncmp((char*)WiFi.psk().c_str(), server.arg("p1").c_str(), parameters_size)) {
          update = true;
        }
      }
      if (update) {
        String topic = String(mqtt_topic) + String(gateway_name) + String(subjectMQTTtoSYSset);
        String output;
        serializeJson(WEBtoSYS, output);
        Log.notice(F("[WebUI] MQTTtoSYS %s" CR), output.c_str());
        Log.warning(F("[WebUI] Save WiFi and Restart" CR));
        char jsonChar[100];
        serializeJson(modules, jsonChar, measureJson(modules) + 1);
        char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

        snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Save WiFi and Restart").c_str());
        String response = String(buffer);
        response += String(restart_script);
        response += String(script);
        response += String(style);
        snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, reset_body, jsonChar, gateway_name, "Save WiFi and Restart");
        response += String(buffer);
        snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
        response += String(buffer);
        server.send(200, "text/html", response);

        delay(2000); // Wait for web page to be sent before
        MQTTtoSYS((char*)topic.c_str(), WEBtoSYS);
        return;
      } else {
        Log.warning(F("[WebUI] No changes" CR));
      }
    }
  }
  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);

  char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Configure WiFi").c_str());
  String response = String(buffer);
  response += String(wifi_script);
  response += String(script);
  response += String(style);
  // wifi_ssid, wifi_password, gateway_name
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, config_wifi_body, jsonChar, gateway_name, WiFiScan.c_str(), WiFi.SSID(), WiFi.psk());
  response += String(buffer);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
  response += String(buffer);
  server.send(200, "text/html", response);
}

/**
 * @brief /MQ - Configure MQTT Page
 * T: handleMQ: uri: /mq, args: 8, method: 1
 * T: handleMQ Arg: 0, mh=192.168.1.11
 * T: handleMQ Arg: 1, ml=1883
 * T: handleMQ Arg: 2, mu=1234
 * T: handleMQ Arg: 3, mp= 
 * T: handleMQ Arg: 4, sc=on
 * T: handleMQ Arg: 5, h=
 * T: handleMQ Arg: 6, mt=home/
 * T: handleMQ Arg: 7, save=
 */
void handleMQ() {
  WEBUI_TRACE_LOG(F("handleMQ: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleMQ Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    if (server.hasArg("save")) {
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject WEBtoSYS = jsonBuffer.to<JsonObject>();
      bool update = false;

      if (server.hasArg("mh")) {
        WEBtoSYS["mqtt_server"] = server.arg("mh");
        if (strncmp(mqtt_server, server.arg("mh").c_str(), parameters_size)) {
          update = true;
        }
      }

      if (server.hasArg("ml")) {
        WEBtoSYS["mqtt_port"] = server.arg("ml");
        if (strncmp(mqtt_port, server.arg("ml").c_str(), 6)) {
          update = true;
        }
      }

      if (server.hasArg("mu")) {
        WEBtoSYS["mqtt_user"] = server.arg("mu");
        if (strncmp(mqtt_user, server.arg("mu").c_str(), parameters_size)) {
          update = true;
        }
      }

      if (server.hasArg("mp")) {
        WEBtoSYS["mqtt_pass"] = server.arg("mp");
        if (strncmp(mqtt_pass, server.arg("mp").c_str(), parameters_size)) {
          update = true;
        }
      }

      // SC - Secure Connection argument is only present when true
      if (mqtt_secure != server.hasArg("sc")) {
        update = true;
      }
      WEBtoSYS["mqtt_secure"] = server.hasArg("sc");

      if (!update) {
        Log.warning(F("[WebUI] clearing" CR));
        for (JsonObject::iterator it = WEBtoSYS.begin(); it != WEBtoSYS.end(); ++it) {
          WEBtoSYS.remove(it);
        }
      }

      if (server.hasArg("h")) {
        WEBtoSYS["gateway_name"] = server.arg("h");
        if (strncmp(gateway_name, server.arg("h").c_str(), parameters_size)) {
          update = true;
        }
      }

      if (server.hasArg("mt")) {
        WEBtoSYS["mqtt_topic"] = server.arg("mt");
        if (strncmp(mqtt_topic, server.arg("mt").c_str(), parameters_size)) {
          update = true;
        }
      }

#  ifndef ESPWifiManualSetup
      if (update) {
        Log.warning(F("[WebUI] Save MQTT and Restart" CR));

        char jsonChar[100];
        serializeJson(modules, jsonChar, measureJson(modules) + 1);
        char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

        snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Save MQTT and Restart").c_str());
        String response = String(buffer);
        response += String(restart_script);
        response += String(script);
        response += String(style);
        snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, reset_body, jsonChar, gateway_name, "Save MQTT and Restart");
        response += String(buffer);
        snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
        response += String(buffer);
        server.send(200, "text/html", response);

        delay(2000); // Wait for web page to be sent before
        String topic = String(mqtt_topic) + String(gateway_name) + String(subjectMQTTtoSYSset);
        String output;
        serializeJson(WEBtoSYS, output);
        Log.notice(F("[WebUI] MQTTtoSYS %s" CR), output.c_str());
        MQTTtoSYS((char*)topic.c_str(), WEBtoSYS);
        return;
      } else {
        Log.warning(F("[WebUI] No changes" CR));
      }
#  endif
    }
  }

  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);

  char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Configure MQTT").c_str());
  String response = String(buffer);
  response += String(script);
  response += String(style);
  // mqtt server (mh), mqtt port (ml), mqtt username (mu), mqtt password (mp), secure connection (sc), server certificate (msc), topic (mt)
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, config_mqtt_body, jsonChar, gateway_name, mqtt_server, mqtt_port, mqtt_user, mqtt_pass, (mqtt_secure ? "checked" : ""), gateway_name, mqtt_topic);
  response += String(buffer);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
  response += String(buffer);
  server.send(200, "text/html", response);
}

/**
 * @brief /LO - Configure Logging Page
 * T: handleLO: uri: /lo, args: 2, method: 1
 * T: handleLO Arg: 0, lo=5
 * T: handleLO Arg: 1, save=
 */
void handleLO() {
  WEBUI_TRACE_LOG(F("handleLO: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleLO Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    if (server.hasArg("save") && server.hasArg("lo") && server.arg("lo").toInt() != Log.getLevel()) {
      Log.fatal(F("[WebUI] Log level changed to: %d" CR), server.arg("lo").toInt());
      Log.setLevel(server.arg("lo").toInt());
    }
  }

  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);

  char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Configure Logging").c_str());
  String response = String(buffer);
  response += String(script);
  response += String(style);
  int logLevel = Log.getLevel();
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, config_logging_body, jsonChar, gateway_name, (logLevel == 0 ? "selected" : ""), (logLevel == 1 ? "selected" : ""), (logLevel == 2 ? "selected" : ""), (logLevel == 3 ? "selected" : ""), (logLevel == 4 ? "selected" : ""), (logLevel == 5 ? "selected" : ""), (logLevel == 6 ? "selected" : ""));
  response += String(buffer);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
  response += String(buffer);
  server.send(200, "text/html", response);
}

/**
 * @brief /RT - Reset configuration ( Erase and Restart ) from Configuration menu
 * 
 */
void handleRT() {
  WEBUI_TRACE_LOG(F("handleRT: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleRT Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  }
  if (server.hasArg("non")) {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);
    Log.warning(F("[WebUI] Erase and Restart" CR));

    char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Erase and Restart").c_str());
    String response = String(buffer);
    response += String(restart_script);
    response += String(script);
    response += String(style);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, reset_body, jsonChar, gateway_name, "Erase and Restart");
    response += String(buffer);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
    response += String(buffer);
    server.send(200, "text/html", response);

    eraseAndRestart();
  } else {
    handleCN();
  }
}

#  if defined(ZgatewayCloud)
/**
 * @brief /CL - Cloud Configuration
 * 
 */
void handleCL() {
  WEBUI_TRACE_LOG(F("handleCL: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleCL Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  }

  if (server.hasArg("save")) {
    // T: handleCL: uri: /cl, args: 2, method: 1
    // T: handleCL Arg: 0, cl-en=on
    // T: handleCL Arg: 1, save=
    if (server.hasArg("save") && server.method() == 1) {
      setCloudEnabled(server.hasArg("cl-en"));
    }
  }

  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);

  char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Configure Cloud").c_str());
  String response = String(buffer);
  response += String(script);
  response += String(style);

  char cloudEnabled[8] = {0};
  if (isCloudEnabled()) {
    strncpy(cloudEnabled, "checked", 8);
  }

  char deviceToken[5] = {0};
  if (!isCloudDeviceTokenSupplied()) {
    strncpy(deviceToken, " Not", 4);
  }

  requestToken = esp_random();
#    ifdef ESP32_ETHERNET
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, config_cloud_body, jsonChar, gateway_name, " cloud checked", " Not", (String(CLOUDGATEWAY) + "token/start").c_str(), (char*)ETH.macAddress().c_str(), ("http://" + String(ip2CharArray(ETH.localIP())) + "/").c_str(), gateway_name, uptime(), requestToken);
#    else
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, config_cloud_body, jsonChar, gateway_name, cloudEnabled, deviceToken, (String(CLOUDGATEWAY) + "token/start").c_str(), (char*)WiFi.macAddress().c_str(), ("http://" + String(ip2CharArray(WiFi.localIP())) + "/").c_str(), gateway_name, uptime(), requestToken);
#    endif
  response += String(buffer);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
  response += String(buffer);
  server.send(200, "text/html", response);
}

/**
 * @brief /TK - Receive Cloud Device Token
 * 
 */
void handleTK() {
  WEBUI_TRACE_LOG(F("handleTK: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleTK Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  }

  if (server.hasArg("deviceToken") && server.hasArg("uptime") && server.hasArg("RT")) {
    String deviceToken = server.arg("deviceToken");

    if (setCloudDeviceToken(deviceToken) && server.arg("RT").toInt() == requestToken && server.arg("uptime").toInt() + 600 > uptime()) {
      setCloudEnabled(true);
      char jsonChar[100];
      serializeJson(modules, jsonChar, measureJson(modules) + 1);

      char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Received Device Token").c_str());
      String response = String(buffer);
      response += String(script);
      response += String(style);
      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, token_body, jsonChar, gateway_name);
      response += String(buffer);
      snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
      response += String(buffer);
      server.send(200, "text/html", response);
    } else {
      WEBUI_TRACE_LOG(F("handleTK: uptime: %u, uptime: %u, ok: %T" CR), server.arg("uptime").toInt(), uptime(), server.arg("uptime").toInt() + 600 > uptime());
      WEBUI_TRACE_LOG(F("handleTK: RT: %d, RT: %d, ok: %T " CR), server.arg("RT").toInt(), requestToken, server.arg("RT").toInt() == requestToken);
      Log.error(F("[WebUI] Invalid Token Response: RT: %T, uptime: %T" CR), server.arg("RT").toInt() == requestToken, server.arg("uptime").toInt() + 600 > uptime());
      server.send(500, "text/html", "Internal ERROR - Invalid Token");
    }
  }
}

#  endif

/**
 * @brief /IN - Information Page
 * 
 */
void handleIN() {
  WEBUI_TRACE_LOG(F("handleCN: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleIN Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  } else {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);

    String informationDisplay = stateMeasures(); // .replace(",\"", "}1");  // .replace("\":", "=2")

// }1 json-oled }2 true } }1 Cloud }2 cloudEnabled}2true}1c
#  if defined(ZgatewayBT)
    informationDisplay += "1<BR>BT}2}1"; // }1 the bracket is not needed as the previous message ends with }
    informationDisplay += stateBTMeasures(false);
#  endif
#  if defined(ZdisplaySSD1306)
    informationDisplay += "1<BR>SSD1306}2}1"; // }1 the bracket is not needed as the previous message ends with }
    informationDisplay += stateSSD1306Display();
#  endif
#  if defined(ZgatewayCloud)
    informationDisplay += "1<BR>Cloud}2}1";
    informationDisplay += stateCLOUDStatus();
#  endif
    informationDisplay += "1<BR>WebUI}2}1";
    informationDisplay += stateWebUIStatus();

    // stateBTMeasures causes a Stack canary watchpoint triggered (loopTask)
    // WEBUI_TRACE_LOG(F("[WebUI] informationDisplay before %s" CR), informationDisplay.c_str());

    // TODO: need to fix display of modules array within SYStoMQTT

    informationDisplay += "1}2";
    informationDisplay.replace(",\"", "}1");
    informationDisplay.replace("\":", "}2");
    informationDisplay.replace("{\"", "");
    informationDisplay.replace("\"", "\\\"");

    // WEBUI_TRACE_LOG(F("[WebUI] informationDisplay after %s" CR), informationDisplay.c_str());

    if (informationDisplay.length() > WEB_TEMPLATE_BUFFER_MAX_SIZE) {
      Log.warning(F("[WebUI] informationDisplay content length ( %d ) greater than WEB_TEMPLATE_BUFFER_MAX_SIZE.  Display truncated" CR), informationDisplay.length());
    }

    char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Information").c_str());
    String response = String(buffer);

    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, information_script, informationDisplay.c_str());
    response += String(buffer);

    response += String(script);
    response += String(style);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, information_body, jsonChar, gateway_name);
    response += String(buffer);

    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
    response += String(buffer);

    server.send(200, "text/html", response);
  }
}

#  if defined(ESP32) && defined(MQTT_HTTPS_FW_UPDATE)
/**
 * @brief /UP - Firmware Upgrade Page
 * 
 */
void handleUP() {
  WEBUI_TRACE_LOG(F("handleUP: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleUP Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    DynamicJsonDocument jsonBuffer(JSON_MSG_BUFFER);
    JsonObject WEBtoSYS = jsonBuffer.to<JsonObject>();

    if (server.hasArg("o")) {
      WEBtoSYS["url"] = server.arg("o");
      WEBtoSYS["version"] = "test";
      WEBtoSYS["password"] = ota_pass;

      {
        sendRestartPage();

        String output;
        serializeJson(WEBtoSYS, output);
        Log.notice(F("[WebUI] MQTTtoSYSupdate %s" CR), output.c_str());
      }

      String topic = String(mqtt_topic) + String(gateway_name) + String(subjectMQTTtoSYSupdate);
      MQTTHttpsFWUpdate((char*)topic.c_str(), WEBtoSYS);
      return;
    } else if (server.hasArg("le")) {
      uint32_t le = server.arg("le").toInt();
      if (le != 0) {
        WEBtoSYS["version"] = (le == 1 ? "latest" : (le == 2 ? "dev" : "unknown"));
        WEBtoSYS["password"] = ota_pass;
        {
          sendRestartPage();

          String output;
          serializeJson(WEBtoSYS, output);
          Log.notice(F("[WebUI] MQTTtoSYSupdate %s" CR), output.c_str());
        }

        String topic = String(mqtt_topic) + String(gateway_name) + String(subjectMQTTtoSYSupdate);
        MQTTHttpsFWUpdate((char*)topic.c_str(), WEBtoSYS);
        return;
      }
    }
  }
  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);

  char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Firmware Upgrade").c_str());
  String response = String(buffer);
  response += String(script);
  response += String(style);
  String systemUrl = RELEASE_LINK + latestVersion + "/" + ENV_NAME + "-firmware.bin";
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, upgrade_body, jsonChar, gateway_name, systemUrl.c_str());
  response += String(buffer);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
  response += String(buffer);
  server.send(200, "text/html", response);
}
#  endif

void sendRestartPage() {
  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);
  char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Updating Firmware and Restart").c_str());
  String response = String(buffer);
  response += String(restart_script);
  response += String(script);
  response += String(style);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, reset_body, jsonChar, gateway_name, "Updating Firmware and Restart");
  response += String(buffer);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
  response += String(buffer);
  server.send(200, "text/html", response);

  delay(2000); // Wait for web page to be sent before
}

/**
 * @brief /CS - Serial Console and Command Line
 * 
 */
void handleCS() {
  WEBUI_TRACE_LOG(F("handleCS: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  WEBUI_SECURE
  if (server.args() && server.hasArg("c2")) {
    for (uint8_t i = 0; i < server.args(); i++) {
      WEBUI_TRACE_LOG(F("handleCS Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    if (server.hasArg("c1")) {
      String c1 = server.arg("c1");

      String cmdTopic = String(mqtt_topic) + String(gateway_name) + "/" + c1.substring(0, c1.indexOf(' '));
      String command = c1.substring(c1.indexOf(' ') + 1);
      if (command.length()) {
        WEBUI_TRACE_LOG(F("[WebUI] handleCS inject MQTT Command topic: '%s', command: '%s'" CR), cmdTopic.c_str(), command.c_str());
        receivingMQTT((char*)cmdTopic.c_str(), (char*)command.c_str());
      } else {
        Log.warning(F("[WebUI] Missing command: '%s', command: '%s'" CR), cmdTopic.c_str(), command.c_str());
      }
    }

    uint32_t index = server.arg("c2").toInt();

    String message = String(log_buffer_pointer) + "}1" + String(reset_web_log_flag) + "}1";
    if (!reset_web_log_flag) {
      index = 0;
      reset_web_log_flag = true;
    }

    bool cflg = (index);
    char* line;
    size_t len;
    while (GetLog(1, &index, &line, &len)) {
      if (cflg) {
        message += "\n";
      }
      for (int x = 0; x < len - 1; x++) {
        message += line[x];
      }
      cflg = true;
    }
    message += "}1";
    server.send(200, "text/plain", message);
  } else {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);

    char buffer[WEB_TEMPLATE_BUFFER_MAX_SIZE];

    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, header_html, (String(gateway_name) + " - Console").c_str());
    String response = String(buffer);
    response += String(console_script);
    response += String(script);
    response += String(style);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, console_body, jsonChar, gateway_name);
    response += String(buffer);
    snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
    response += String(buffer);
    server.send(200, "text/html", response);
  }
}

/**
 * @brief Page not found handler
 * 
 */
void notFound() {
  WEBUI_SECURE
#  ifdef WEBUI_DEVELOPMENT
  String path = server.uri();
  if (!exists(path)) {
    if (exists(path + ".html")) {
      path += ".html";
    } else {
#  endif
      Log.warning(F("[WebUI] notFound: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
      server.send(404, "text/plain", "Not found");
      return;
#  ifdef WEBUI_DEVELOPMENT
    }
  }
  WEBUI_TRACE_LOG(F("notFound returning: actual uri: %s, args: %d, method: %d" CR), path, server.args(), server.method());
  File file = FILESYSTEM.open(path, "r");
  server.streamFile(file, "text/html");
  file.close();
#  endif
}

void WebUISetup() {
  WEBUI_TRACE_LOG(F("ZwebUI setup start" CR));

  WebUIConfig_load();
  webUIQueue = xQueueCreate(5, sizeof(webUIQueueMessage*));

#  ifdef WEBUI_DEVELOPMENT
  FILESYSTEM.begin();
  {
    File root = FILESYSTEM.open("/");
    File file = root.openNextFile();
    while (file) {
      String fileName = file.name();
      size_t fileSize = file.size();
      WEBUI_TRACE_LOG(F("FS File: %s, size: %s" CR), fileName.c_str(), formatBytes(fileSize).c_str());
      file = root.openNextFile();
    }
  }
#  endif
  server.onNotFound(notFound);

  server.on("/", handleRoot); // Main Menu

  server.on("/in", handleIN); // Information
  server.on("/cs", handleCS); // Console
#  if defined(ESP32) && defined(MQTT_HTTPS_FW_UPDATE)
  server.on("/up", handleUP); // Firmware Upgrade
#  endif
  server.on("/cn", handleCN); // Configuration
  server.on("/wi", handleWI); // Configure Wifi
  server.on("/mq", handleMQ); // Configure MQTT
  server.on("/wu", handleWU); // Configure WebUI
#  if defined(ZgatewayCloud)
  server.on("/cl", handleCL); // Configure Cloud
  server.on("/tk", handleTK); // Store Device Token
#  endif
  server.on("/lo", handleLO); // Configure Logging

  server.on("/rt", handleRT); // Reset configuration ( Erase and Restart )
  server.begin();

  Log.begin(LOG_LEVEL, &WebLog);

  Log.trace(F("[WebUI] displayMetric %T" CR), displayMetric);
  Log.trace(F("[WebUI] WebUI Secure %T" CR), webUISecure);
  Log.notice(F("OpenMQTTGateway URL: http://%s/" CR), WiFi.localIP().toString().c_str());
  displayPrint("URL: http://", (char*)WiFi.localIP().toString().c_str());
  Log.notice(F("ZwebUI setup done" CR));
}

unsigned long nextWebUIMessage = uptime() + DISPLAY_WEBUI_INTERVAL;

void WebUILoop() {
  server.handleClient();

  if (uptime() >= nextWebUIMessage && uxQueueMessagesWaiting(webUIQueue)) {
    webUIQueueMessage* message = nullptr;
    xQueueReceive(webUIQueue, &message, portMAX_DELAY);
    newSSD1306Message = true;

    if (currentWebUIMessage) {
      free(currentWebUIMessage);
    }
    currentWebUIMessage = message;
    nextWebUIMessage = uptime() + DISPLAY_WEBUI_INTERVAL;
  }
}

void MQTTtoWebUI(char* topicOri, JsonObject& WebUIdata) { // json object decoding
  bool success = false;
  if (cmpToMainTopic(topicOri, subjectMQTTtoWebUIset)) {
    WEBUI_TRACE_LOG(F("MQTTtoWebUI json set" CR));
    // properties
    if (WebUIdata.containsKey("displaymetric")) {
      displayMetric = WebUIdata["displaymetric"].as<bool>();
      Log.notice(F("Set displaymetric: %T" CR), displayMetric);
      success = true;
    }
    // save, load, init, erase
    if (WebUIdata.containsKey("save") && WebUIdata["save"]) {
      success = WebUIConfig_save();
      if (success) {
        Log.notice(F("WebUI config saved" CR));
      }
    } else if (WebUIdata.containsKey("load") && WebUIdata["load"]) {
      success = WebUIConfig_load();
      if (success) {
        Log.notice(F("WebUI config loaded" CR));
      }
    } else if (WebUIdata.containsKey("init") && WebUIdata["init"]) {
      WebUIConfig_init();
      success = true;
      if (success) {
        Log.notice(F("WebUI config initialised" CR));
      }
    } else if (WebUIdata.containsKey("erase") && WebUIdata["erase"]) {
      // Erase config from NVS (non-volatile storage)
      preferences.begin(Gateway_Short_Name, false);
      success = preferences.remove("WebUIConfig");
      preferences.end();
      if (success) {
        Log.notice(F("WebUI config erased" CR));
      }
    }
    if (success) {
      stateWebUIStatus();
    } else {
      // pub(subjectWebUItoMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("[ WebUI ] MQTTtoWebUI Fail json" CR), WebUIdata);
    }
  }
}

String stateWebUIStatus() {
  //Publish display state
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject WebUIdata = jsonBuffer.to<JsonObject>();
  WebUIdata["displayMetric"] = (bool)displayMetric;
  WebUIdata["webUISecure"] = (bool)webUISecure;
  WebUIdata["displayQueue"] = uxQueueMessagesWaiting(webUIQueue);

  String output;
  serializeJson(WebUIdata, output);

  // WebUIdata["currentMessage"] = currentWebUIMessage;
  pub(subjectWebUItoMQTT, WebUIdata);
  return output;
}

bool WebUIConfig_save() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jo = jsonBuffer.to<JsonObject>();
  jo["displayMetric"] = (bool)displayMetric;
  jo["webUISecure"] = (bool)webUISecure;
  // Save config into NVS (non-volatile storage)
  String conf = "";
  serializeJson(jsonBuffer, conf);
  preferences.begin(Gateway_Short_Name, false);
  int result = preferences.putString("WebUIConfig", conf);
  preferences.end();
  Log.trace(F("[WebUI] WebUIConfig_save: %s, result: %d" CR), conf.c_str(), result);
  return true;
}

void WebUIConfig_init() {
  displayMetric = DISPLAY_METRIC;
  webUISecure = WEBUI_AUTH;
  Log.notice(F("WebUI config initialised" CR));
}

bool WebUIConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (preferences.isKey("WebUIConfig")) {
    auto error = deserializeJson(jsonBuffer, preferences.getString("WebUIConfig", "{}"));
    preferences.end();
    if (error) {
      Log.error(F("WebUI config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      return false;
    }
    if (jsonBuffer.isNull()) {
      Log.warning(F("WebUI config is null" CR));
      return false;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    displayMetric = jo["displayMetric"].as<bool>();
    webUISecure = jo["webUISecure"].as<bool>();
    return true;
  } else {
    preferences.end();
    Log.notice(F("No WebUI config to load" CR));
    return false;
  }
}

/*
Workaround for c not having a string based switch/case function
*/
constexpr unsigned int webUIHash(const char* s, int off = 0) { // workaround for switching on a string https://stackoverflow.com/a/46711735/18643696
  return !s[off] ? 5381 : (webUIHash(s, off + 1) * 33) ^ s[off];
}

/*
Parse json message from module into a format for display
*/
void webUIPubPrint(const char* topicori, JsonObject& data) {
  WEBUI_TRACE_LOG(F("[ webUIPubPrint ] pub %s " CR), topicori);
  if (webUIQueue) {
    webUIQueueMessage* message = (webUIQueueMessage*)heap_caps_calloc(1, sizeof(webUIQueueMessage), MALLOC_CAP_8BIT);
    if (message != NULL) {
      // Initalize message
      strlcpy(message->line1, "", WEBUI_TEXT_WIDTH);
      strlcpy(message->line2, "", WEBUI_TEXT_WIDTH);
      strlcpy(message->line3, "", WEBUI_TEXT_WIDTH);
      strlcpy(message->line4, "", WEBUI_TEXT_WIDTH);
      char* topic = strdup(topicori);
      strlcpy(message->title, strtok(topic, "/"), WEBUI_TEXT_WIDTH);
      free(topic);

      //  WEBUI_TRACE_LOG(F("[ webUIPubPrint ] switch %s " CR), message->title);
      switch (webUIHash(message->title)) {
        case webUIHash("SYStoMQTT"): {
          // {"uptime":456356,"version":"lilygo-rtl_433-test-A-v1.1.1-25-g574177d[lily-cloud]","freemem":125488,"mqttport":"1883","mqttsecure":false,"freestack":3752,"rssi":-36,"SSID":"The_Beach","BSSID":"64:A5:C3:69:C3:38","ip":"192.168.1.239","mac":"4C:75:25:A8:D5:D8","actRec":3,"mhz":433.92,"RTLRssiThresh":-98,"RTLRssi":-108,"RTLAVGRssi":-107,"RTLCnt":121707,"RTLOOKThresh":90,"modules":["LILYGO_OLED","CLOUD","rtl_433"]}

          // Line 1

          if (data["version"]) {
            strlcpy(message->line1, data["version"], WEBUI_TEXT_WIDTH);
          } else {
            strlcpy(message->line1, "", WEBUI_TEXT_WIDTH);
          }

          // Line 2

          String uptime = data["uptime"];
          String line = "uptime: " + uptime;
          line.toCharArray(message->line2, WEBUI_TEXT_WIDTH);

          // Line 3

          String freemem = data["freemem"];
          line = "freemem: " + freemem;
          line.toCharArray(message->line3, WEBUI_TEXT_WIDTH);

          // Line 4

          String ip = data["ip"];
          line = "ip: " + ip;
          line.toCharArray(message->line4, WEBUI_TEXT_WIDTH);

          // Queue completed message

          if (xQueueSend(webUIQueue, (void*)&message, 0) != pdTRUE) {
            Log.warning(F("[ WebUI ] ERROR: webUIQueue full, discarding %s" CR), message->title);
            free(message);
          } else {
            // Log.notice(F("[ WebUI ] Queued %s" CR), message->title);
          }
          break;
        }

#  ifdef ZgatewayRTL_433
        case webUIHash("RTL_433toMQTT"): {
          if (data["model"] && strncmp(data["model"], "status", 6)) { // Does not contain "status"
            // {"model":"Acurite-Tower","id":2043,"channel":"B","battery_ok":1,"temperature_C":5.3,"humidity":81,"mic":"CHECKSUM","protocol":"Acurite 592TXR Temp/Humidity, 5n1 Weather Station, 6045 Lightning, 3N1, Atlas","rssi":-81,"duration":121060}

            // Line 1

            strlcpy(message->line1, data["model"], WEBUI_TEXT_WIDTH);

            // Line 2

            String line2 = "";
            if (data["id"]) {
              String id = data["id"];
              line2 += "id: " + id + " ";
            }

            if (data["channel"]) {
              String channel = data["channel"];
              line2 += "channel: " + channel;
            }
            line2.toCharArray(message->line2, WEBUI_TEXT_WIDTH);
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

            line3.toCharArray(message->line3, WEBUI_TEXT_WIDTH);

            // Line 4

            String line4 = "";
            if (data["battery_ok"]) {
              line4 = "batt: " + data["battery_ok"].as<String>();
            } else {
              line4 = "pulses: " + data["pulses"].as<String>();
            }

            line4 += " rssi: " + data["rssi"].as<String>();
            line4.toCharArray(message->line4, WEBUI_TEXT_WIDTH);

            // Queue completed message

            if (xQueueSend(webUIQueue, (void*)&message, 0) != pdTRUE) {
              Log.error(F("[ WebUI ] webUIQueue full, discarding signal %s" CR), message->title);
              free(message);
            } else {
              // Log.notice(F("[ WebUI ] Queued %s" CR), message->title);
            }
          } else {
            Log.error(F("[ WebUI ] rtl_433 not displaying %s" CR), message->title);
            free(message);
          }
          break;
        }
#  endif
#  ifdef ZsensorBME280
        case webUIHash("CLIMAtoMQTT"): {
          // {"tempc":17.06,"tempf":62.708,"hum":50.0752,"pa":98876.14,"altim":205.8725,"altift":675.4348}

          // Line 1

          strlcpy(message->line1, "bme280", WEBUI_TEXT_WIDTH);

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
          line2.toCharArray(message->line2, WEBUI_TEXT_WIDTH);

          // Line 3

          String line3 = "";
          float humidity = data["hum"];
          if (data.containsKey("hum") && humidity <= 100 && humidity >= 0) {
            char hum[5];
            dtostrf(humidity, 3, 1, hum);
            line3 += "hum: " + (String)hum + "% ";
          }
          line3.toCharArray(message->line3, WEBUI_TEXT_WIDTH);

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
          line4.toCharArray(message->line4, WEBUI_TEXT_WIDTH);

          // Queue completed message

          if (xQueueSend(webUIQueue, (void*)&message, 0) != pdTRUE) {
            Log.error(F("[ WebUI ] webUIQueue full, discarding signal %s" CR), message->title);
            free(message);
          } else {
            // Log.notice(F("[ WebUI ] Queued %s" CR), message->title);
          }
          break;
        }
#  endif
#  ifdef ZgatewayBT
        case webUIHash("BTtoMQTT"): {
          // {"id":"AA:BB:CC:DD:EE:FF","mac_type":0,"adv_type":0,"name":"sps","manufacturerdata":"de071f1000b1612908","rssi":-70,"brand":"Inkbird","model":"T(H) Sensor","model_id":"IBS-TH1/TH2/P01B","type":"THBX","cidc":false,"acts":true,"tempc":20.14,"tempf":68.252,"hum":41.27,"batt":41}

          if (data["model_id"] != "MS-CDP" && data["model_id"] != "GAEN" && data["model_id"] != "APPLE_CONT" && data["model_id"] != "IBEACON") {
            // Line 2, 3, 4
            String line2 = "";
            String line3 = "";
            String line4 = "";

            // Properties
            String properties[6] = {"", "", "", "", "", ""};
            int property = -1;

            if (data["type"] == "THB" || data["type"] == "THBX" || data["type"] == "PLANT" || data["type"] == "AIR" || data["type"] == "BATT" || data["type"] == "ACEL" || (data["type"] == "UNIQ" && data["model_id"] == "SDLS")) {
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

            if (!(line2 == "" && line3 == "" && line4 == "")) {
              // Titel
              char* topic = strdup(topicori);
              String heading = strtok(topic, "/");
              String line0 = heading + "           " + data["id"].as<String>().substring(9, 17);
              line0.toCharArray(message->title, WEBUI_TEXT_WIDTH);
              free(topic);

              // Line 1
              strlcpy(message->line1, data["model"], WEBUI_TEXT_WIDTH);

              line2.toCharArray(message->line2, WEBUI_TEXT_WIDTH);
              line3.toCharArray(message->line3, WEBUI_TEXT_WIDTH);
              line4.toCharArray(message->line4, WEBUI_TEXT_WIDTH);

              if (xQueueSend(webUIQueue, (void*)&message, 0) != pdTRUE) {
                Log.error(F("[ WebUI ] webUIQueue full, discarding signal %s" CR), message->title);
                free(message);
              } else {
                // Log.notice(F("[ WebUI ] Queued %s" CR), message->title);
              }
            } else {
              WEBUI_TRACE_LOG(F("[ WebUI ] incomplete messaage %s" CR), topicori);
              free(message);
            }

            break;
          } else {
            WEBUI_TRACE_LOG(F("[ WebUI ] incorrect model_id %s" CR), topicori);
            free(message);
            break;
          }
        }
#  endif
#  ifdef ZsensorRN8209
        case webUIHash("RN8209toMQTT"): {
          // {"volt":1073178,"current":0,"power":0}

          // Line 1

          String line1 = "";
          if (data.containsKey("volt")) {
            char volt[5];
            float voltage = data["volt"];
            dtostrf(voltage, 3, 1, volt);
            line1 = "volt: " + (String)volt;
          }
          line1.toCharArray(message->line1, WEBUI_TEXT_WIDTH);

          // Line 2

          String line2 = "";
          if (data.containsKey("current")) {
            char curr[5];
            float current = data["current"];
            dtostrf(current, 3, 1, curr);
            line2 = "current: " + (String)curr + " A";
          }
          line2.toCharArray(message->line2, WEBUI_TEXT_WIDTH);

          // Line 3

          String line3 = "";
          if (data.containsKey("power")) {
            char pow[5];
            float power = data["power"];
            dtostrf(power, 3, 1, pow);
            line3 = "power: " + (String)pow + " W";
          }
          line3.toCharArray(message->line3, WEBUI_TEXT_WIDTH);

          // Queue completed message

          if (xQueueSend(webUIQueue, (void*)&message, 0) != pdTRUE) {
            Log.error(F("[ WebUI ] webUIQueue full, discarding signal %s" CR), message->title);
            free(message);
          } else {
            // Log.notice(F("[ WebUI ] Queued %s" CR), message->title);
          }
          break;
        }
#  endif
        default:
          Log.verbose(F("[ WebUI ] unhandled topic %s" CR), message->title);
          free(message);
      }
    } else {
      Log.error(F("[ WebUI ] insufficent memory " CR));
    }
  } else {
    Log.error(F("[ WebUI ] not initalized " CR));
  }
}

/*------------------- Serial logging interceptor ----------------------*/

// This pattern was borrowed from HardwareSerial and modified to support the WebUI display

SerialWeb WebLog(0); // Not sure about this, came from Hardwareserial
SerialWeb::SerialWeb(int x) {
}

/*
Initialize WebUI oled display for use, and display OMG logo
*/
void SerialWeb::begin() {
  // WebUI.begin(); // User OMG serial support
}

/*
Dummy virtual functions carried over from Serial
*/
int SerialWeb::available(void) {
}

/*
Dummy virtual functions carried over from Serial
*/
int SerialWeb::peek(void) {
}

/*
Dummy virtual functions carried over from Serial
*/
int SerialWeb::read(void) {
}

/*
Dummy virtual functions carried over from Serial
*/
void SerialWeb::flush(void) {
}

/*
Write line of text to the display with vertical scrolling of screen
*/
size_t SerialWeb::write(const uint8_t* buffer, size_t size) {
  // Default to Serial output if the display is not available
  addLog(buffer, size);
  return Serial.write(buffer, size);
}

char line[ROW_LENGTH];
int lineIndex = 0;
void addLog(const uint8_t* buffer, size_t size) {
  for (int i = 0; i < size; i++) {
    if (char(buffer[i]) == 10 | lineIndex > ROW_LENGTH - 2) {
      if (char(buffer[i]) != 10) {
        line[lineIndex++] = char(buffer[i]);
      }
      line[lineIndex++] = char(0);
      AddLogData(1, (const char*)&line[0]);
      lineIndex = 0;
    } else {
      line[lineIndex++] = char(buffer[i]);
    }
  }
}

#endif