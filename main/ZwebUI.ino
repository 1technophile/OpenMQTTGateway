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

boolean displayMetric = DISPLAY_METRIC;

/*------------------- Local functions ----------------------*/

void notFound();
void handleRoot(); // "/"

void handleCS(); // Console
void handleCN(); // Configuration

void handleIN(); // Information

void handleRT(); // Restart
void handleCL(); // Configure Cloud
void handleTK(); // Return Cloud token

/*------------------- External functions ----------------------*/

esp_err_t nvs_flash_erase(void);
extern String stateMeasures(); // Send a status message
extern void eraseAndRestart();
extern unsigned long uptime();

/*------------------- Local functions ----------------------*/

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

void handleRoot() {
  Log.trace(F("handleRoot: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
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

#  if defined(ESP8266)
      ESP.reset();
#  else
      ESP.restart();
#  endif
    } else {
      // Log.trace(F("Arguments %s" CR), message);
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

void handleCN() {
  Log.trace(F("handleCN: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("handleCN Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
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

void handleRT() {
  Log.trace(F("handleRT: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("handleRT Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
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

    nvs_flash_erase();
    ESP.restart();
  } else {
    handleCN();
  }
}

#  if defined(ZgatewayCloud)
void handleCL() {
  Log.trace(F("handleCL: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("handleCL Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
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
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, cloud_body, jsonChar, gateway_name, " cloud checked", " Not", (String(CLOUDGATEWAY) + "token/start").c_str(), (char*)ETH.macAddress().c_str(), ("http://" + String(ip2CharArray(ETH.localIP())) + "/").c_str(), gateway_name, uptime(), requestToken);
#    else
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, cloud_body, jsonChar, gateway_name, cloudEnabled, deviceToken, (String(CLOUDGATEWAY) + "token/start").c_str(), (char*)WiFi.macAddress().c_str(), ("http://" + String(ip2CharArray(WiFi.localIP())) + "/").c_str(), gateway_name, uptime(), requestToken);
#    endif
  response += String(buffer);
  snprintf(buffer, WEB_TEMPLATE_BUFFER_MAX_SIZE, footer, OMG_VERSION);
  response += String(buffer);
  server.send(200, "text/html", response);
}

void handleTK() {
  Log.trace(F("handleTK: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("handleTK Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
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
      Log.trace(F("handleTK: uptime: %u, uptime: %u, ok: %T" CR), server.arg("uptime").toInt(), uptime(), server.arg("uptime").toInt() + 600 > uptime());
      Log.trace(F("handleTK: RT: %d, RT: %d, ok: %T " CR), server.arg("RT").toInt(), requestToken, server.arg("RT").toInt() == requestToken);
      Log.error(F("[WebUI] Invalid Token Response: RT: %T, uptime: %T" CR), server.arg("RT").toInt() == requestToken, server.arg("uptime").toInt() + 600 > uptime());
      server.send(500, "text/html", "Internal ERROR - Invalid Token");
    }
  }
}

#  endif

void handleIN() {
  Log.trace(F("handleCN: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("handleIN Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }
  } else {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);

    String informationDisplay = stateMeasures(); // .replace(",\"", "}1");  // .replace("\":", "=2")

    // }1 json-oled }2 true } }1 Cloud }2 cloudEnabled}2true}1c
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

    Log.trace(F("[WebUI] informationDisplay before %s" CR), informationDisplay.c_str());

    // before {"uptime":33,"version":"lilygo-rtl_433-test-B-v1.4.0-98-gcff461db[WebUI]","env":"lilygo-rtl_433-test-B","freemem":106356,"mqttport":"1883","mqttsecure":false,"maxBlock":65136,"tempc":46.11111,"freestack":3208,"rssi":-53,"SSID":"The_Beach","BSSID":"90:72:40:18:A7:BE","ip":"192.168.1.160","mac":"4C:75:25:A8:85:04","actRec":3,"mhz":433.92,"RTLRssiThresh":-82,"RTLRssi":-95,"RTLAVGRssi":0,"RTLCnt":5,"RTLOOKThresh":90,"modules":["LILYGO_OLED","ZwebUI","CLOUD","rtl_433"]}1<BR>SSD1306}2}1{"onstate":true,"brightness":50,"displaymetric":true,"display-flip":true,"idlelogo":false,"log-oled":false,"json-oled":true}1<BR>Cloud}2}1{"cloudEnabled":true,"cloudActive":true,"cloudState":0}

    // TODO: need to fix display of modules array within SYStoMQTT

    informationDisplay += "1}2";
    informationDisplay.replace(",\"", "}1");
    informationDisplay.replace("\":", "}2");
    informationDisplay.replace("{\"", "");
    informationDisplay.replace("\"", "\\\"");
    Log.trace(F("[WebUI] informationDisplay after %s" CR), informationDisplay.c_str());

    // after uptime}233}1version}2\"lilygo-rtl_433-test-B-v1.4.0-98-gcff461db[WebUI]\"}1env}2\"lilygo-rtl_433-test-B\"}1freemem}2106356}1mqttport}2\"1883\"}1mqttsecure}2false}1maxBlock}265136}1tempc}246.11111}1freestack}23208}1rssi}2-53}1SSID}2\"The_Beach\"}1BSSID}2\"90:72:40:18:A7:BE\"}1ip}2\"192.168.1.160\"}1mac}2\"4C:75:25:A8:85:04\"}1actRec}23}1mhz}2433.92}1RTLRssiThresh}2-82}1RTLRssi}2-95}1RTLAVGRssi}20}1RTLCnt}25}1RTLOOKThresh}290}1modules}2[\"LILYGO_OLED\"}1ZwebUI\"}1CLOUD\"}1rtl_433\"]}1<BR>SSD1306}2}1onstate}2true}1brightness}250}1displaymetric}2true}1display-flip}2true}1idlelogo}2false}1log-oled}2false}1json-oled}2true}1<BR>Cloud}2}1cloudEnabled}2true}1cloudActive}2true}1cloudState}20}1}2

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

uint32_t logIndex = 0;

void handleCS() {
  Log.trace(F("handleCS: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("handleCS Arg: %d, %s=%s" CR), i, server.argName(i).c_str(), server.arg(i).c_str());
    }

    // Log.trace(F("handleCS c1: %s, c2: %s" CR), server.arg("c1").c_str(), server.arg("c2").c_str());
    String message = "";
    if (server.hasArg("c1")) {
      String c1 = server.arg("c1");

      String cmdTopic = String(mqtt_topic) + String(gateway_name) + "/" + c1.substring(0, c1.indexOf(' '));
      String command = c1.substring(c1.indexOf(' ') + 1);
      Log.trace(F("handleCS inject MQTT Command topic: '%s', command: '%s'" CR), cmdTopic.c_str(), command.c_str());
      receivingMQTT((char*)cmdTopic.c_str(), (char*)command.c_str());
      message += logIndex + '}1' + '0' + '}1' + '}1';
      server.send(200, "text/plain", message);
    }
    if (server.hasArg("c2")) {
    }

    // Log.trace(F("Arguments %s" CR), message);
    message += logIndex + '}1' + '0' + '}1' + '}1';
    server.send(200, "text/plain", message);
  } else {
    File file = FILESYSTEM.open("/cs.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  }
}

void notFound() {
  String path = server.uri();
  if (!exists(path)) {
    if (exists(path + ".html")) {
      path += ".html";
    } else {
      Log.warning(F("[WebUI] notFound: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
      server.send(404, "text/plain", "Not found");
      return;
    }
  }
  Log.trace(F("notFound returning: actual uri: %s, args: %d, method: %d" CR), path, server.args(), server.method());
  File file = FILESYSTEM.open(path, "r");
  server.streamFile(file, "text/html");
  file.close();
}

void WebUISetup() {
  Log.trace(F("ZwebUI setup start" CR));

  webUIQueue = xQueueCreate(5, sizeof(webUIQueueMessage*));

  FILESYSTEM.begin();
  {
    File root = FILESYSTEM.open("/");
    File file = root.openNextFile();
    while (file) {
      String fileName = file.name();
      size_t fileSize = file.size();
      Log.trace(F("FS File: %s, size: %s" CR), fileName.c_str(), formatBytes(fileSize).c_str());
      file = root.openNextFile();
    }
  }
  server.onNotFound(notFound);

  server.on("/", handleRoot); // Main Menu

  server.on("/in", handleIN); // Information
  server.on("/cs", handleCS); // Console

  server.on("/cn", handleCN); // Configuration
#  if defined(ZgatewayCloud)
  server.on("/cl", handleCL); // Cloud configuration
  server.on("/tk", handleTK); // Store Device Token
#  endif

  server.on("/rt", handleRT); // Reset configuration ( Erase and Restart )
  server.begin();

  Log.begin(LOG_LEVEL, &WebLog);

  Log.notice(F("OpenMQTTGateway URL: http://%s/" CR), WiFi.localIP().toString().c_str());
  Log.notice(F("ZwebUI setup done" CR));
}

unsigned long nextWebUIPage = uptime() + DISPLAY_WEBUI_INTERVAL;

void WebUILoop() {
  server.handleClient();

  if (uptime() >= nextWebUIPage && uxQueueMessagesWaiting(webUIQueue)) {
    webUIQueueMessage* message = nullptr;
    xQueueReceive(webUIQueue, &message, portMAX_DELAY);
    if (currentWebUIMessage) {
      free(currentWebUIMessage);
    }
    currentWebUIMessage = message;
    nextWebUIPage = uptime() + DISPLAY_WEBUI_INTERVAL;
  }
}

void MQTTtoWebUI(char* topicOri, JsonObject& WebUIdata) { // json object decoding
  bool success = false;
  if (cmpToMainTopic(topicOri, subjectMQTTtoWebUIset)) {
    Log.trace(F("MQTTtoWebUI json set" CR));
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
  // Save config into NVS (non-volatile storage)
  String conf = "";
  serializeJson(jsonBuffer, conf);
  preferences.begin(Gateway_Short_Name, false);
  preferences.putString("WebUIConfig", conf);
  preferences.end();
  return true;
}

void WebUIConfig_init() {
  boolean displayMetric = DISPLAY_METRIC;
  Log.notice(F("WebUI config initialised" CR));
}

bool WebUIConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  String exists = preferences.getString("WebUIConfig", "{}");
  if (exists != "{}") {
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
  Log.trace(F("[ webUIPubPrint ] pub %s " CR), topicori);
  if (webUIQueue) {
    webUIQueueMessage* message = (webUIQueueMessage*)malloc(sizeof(webUIQueueMessage));
    if (message != NULL) {
      // Initalize message
      strlcpy(message->line1, "", WEBUI_TEXT_WIDTH);
      strlcpy(message->line2, "", WEBUI_TEXT_WIDTH);
      strlcpy(message->line3, "", WEBUI_TEXT_WIDTH);
      strlcpy(message->line4, "", WEBUI_TEXT_WIDTH);
      char* topic = strdup(topicori);
      strlcpy(message->title, strtok(topic, "/"), WEBUI_TEXT_WIDTH);
      free(topic);

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
          String line2 = "uptime: " + uptime;
          line2.toCharArray(message->line2, WEBUI_TEXT_WIDTH);

          // Line 3

          String freemem = data["freemem"];
          String line3 = "freemem: " + freemem;
          line3.toCharArray(message->line3, WEBUI_TEXT_WIDTH);

          // Line 4

          String ip = data["ip"];
          String line4 = "ip: " + ip;
          line4.toCharArray(message->line4, WEBUI_TEXT_WIDTH);

          // Queue completed message

          if (xQueueSend(webUIQueue, (void*)&message, 0) != pdTRUE) {
            Log.error(F("[ WebUI ] ERROR: webUIQueue full, discarding signal %s" CR), message->title);
          } else {
            // Log.notice(F("Queued %s" CR), message->title);
          }
          break;
        }

#  ifdef ZgatewayRTL_433
        case webUIHash("RTL_433toMQTT"): {
          // {"model":"Acurite-Tower","id":2043,"channel":"B","battery_ok":1,"temperature_C":5.3,"humidity":81,"mic":"CHECKSUM","protocol":"Acurite 592TXR Temp/Humidity, 5n1 Weather Station, 6045 Lightning, 3N1, Atlas","rssi":-81,"duration":121060}

          // Line 1

          strlcpy(message->line1, data["model"], WEBUI_TEXT_WIDTH);

          // Line 2

          String id = data["id"];
          String channel = data["channel"];
          String line2 = "id: " + id + " channel: " + channel;
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

          String rssi = data["rssi"];
          String battery_ok = data["battery_ok"];

          String line4 = "batt: " + battery_ok + " rssi: " + rssi;
          line4.toCharArray(message->line4, WEBUI_TEXT_WIDTH);

          // Queue completed message

          if (xQueueSend(webUIQueue, (void*)&message, 0) != pdTRUE) {
            Log.error(F("[ WebUI ] webUIQueue full, discarding signal %s" CR), message->title);
            free(message);
          } else {
            // Log.notice(F("Queued %s" CR), message->title);
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
            // Log.notice(F("Queued %s" CR), message->title);
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
                // Log.notice(F("Queued %s" CR), message->title);
              }
            } else {
              free(message);
            }

            break;
          } else {
            free(message);
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
            // Log.notice(F("Queued %s" CR), message->title);
          }
          break;
        }
#  endif
        default:
          Log.error(F("[ WebUI ] unhandled topic %s" CR), message->title);
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

#  define lineBuffer 200

char line[lineBuffer];
char lineIndex = 0;
void addLog(const uint8_t* buffer, size_t size) {
  String d = "";
  for (int i = 0; i < size; i++) {
    d += char(buffer[i]);
    if (buffer[i] == 10 || lineIndex > lineBuffer - 1) {
      line[lineIndex++] = char(0);
      // Serial.write(line);
      // WebSerial.println(line);
      lineIndex = 0;
    } else {
      line[lineIndex++] = char(buffer[i]);
    }
  }
}

#endif