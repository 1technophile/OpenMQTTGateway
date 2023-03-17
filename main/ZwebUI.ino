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
#  include <AsyncTCP.h>
#  include <ESPAsyncWebServer.h>
#  include <SPIFFS.h>
// #  include <WebSerial.h>

#  include "ArduinoLog.h"
#  include "config_WebContent.h"
#  include "config_WebUI.h"

#  if defined(ZgatewayCloud)
#    include "config_Cloud.h"
#  endif

#  if defined(ZdisplaySSD1306)
#    include "config_SSD1306.h"
#  endif

AsyncWebServer server(80);

/*------------------- Local functions ----------------------*/

void notFound(AsyncWebServerRequest* request);
void handleSlash(AsyncWebServerRequest* request);
void handleCS(AsyncWebServerRequest* request);
void handleCN(AsyncWebServerRequest* request);
void handleIN(AsyncWebServerRequest* request);
void handleRT(AsyncWebServerRequest* request);
void handleCL(AsyncWebServerRequest* request);
void handleTK(AsyncWebServerRequest* request);

/*------------------- External functions ----------------------*/

esp_err_t nvs_flash_erase(void);
extern String stateMeasures(); // Send a status message
extern void eraseAndRestart();

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

void handleSlash(AsyncWebServerRequest* request) {
  Log.trace(F("handleSlash: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }
    if (request->hasArg("m")) {
#  if defined(ZdisplaySSD1306)
      request->send(200, "application/json", "{t}{s}<b>" + String(currentOledMessage->title) + "</b>{e}{s}" + String(currentOledMessage->line1) + "{e}{s}" + String(currentOledMessage->line2) + "{e}{s}" + String(currentOledMessage->line3) + "{e}{s}" + String(currentOledMessage->line4) + "{e}</table>");
#  else
      request->send(200, "application/json", "{t}{s}Uptime:{m}" + String(uptime()) + "{e}</table>");
#  endif
    } else if (request->hasArg("rst")) { // TODO: This should redirect to the RST page
      Log.warning(F("[WebUI] Restart" CR));
#  if defined(ESP8266)
      ESP.reset();
#  else
      ESP.restart();
#  endif
    } else {
      // Log.trace(F("Arguments %s" CR), message);
      request->send(200, "text/plain", "00:14:36.767 RSL: RESULT = {\"Topic\":\"topic\"}");
    }
  } else {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);

    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->printf(header_html, (String(gateway_name) + " - Main Menu").c_str());
    response->print(slash_script);
    response->print(script);
    response->print(style);
    response->printf(slash_body, jsonChar, gateway_name);
    response->printf(footer, OMG_VERSION);
    request->send(response);
  }
}

void handleCN(AsyncWebServerRequest* request) {
  Log.trace(F("handleCN: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("handleCN Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }
  } else {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);

    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->printf(header_html, (String(gateway_name) + " - Configuration").c_str());
    response->print(script);
    response->print(style);
    response->printf(config_body, jsonChar, gateway_name);
    response->printf(footer, OMG_VERSION);
    request->send(response);
  }
}

void handleRT(AsyncWebServerRequest* request) {
  Log.trace(F("handleRT: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("handleRT Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }
  }
  if (request->hasArg("non")) {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);
    Log.warning(F("[WebUI] Erase and Restart" CR));
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->printf(header_html, (String(gateway_name) + " - Reset").c_str());
    response->print(script);
    response->print(style);
    response->printf(reset_body, jsonChar, gateway_name);
    response->printf(footer, OMG_VERSION);
    request->send(response);

    nvs_flash_erase();
    ESP.restart();
  } else {
    handleCN(request);
  }
}

#  if defined(ZgatewayCloud)
void handleCL(AsyncWebServerRequest* request) {
  Log.trace(F("handleCL: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("handleCL Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }
  }
  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);
  AsyncResponseStream* response = request->beginResponseStream("text/html");
  response->printf(header_html, (String(gateway_name) + " - Cloud").c_str());
  response->print(script);
  response->print(style);
#    ifdef ESP32_ETHERNET
  response->printf(cloud_body, jsonChar, gateway_name, " cloud checked", (String(CLOUDGATEWAY) + "token/start").c_str(), (char*)ETH.macAddress().c_str(), ("http://" + String(ip2CharArray(ETH.localIP())) + "/").c_str(), gateway_name);
#    else
  response->printf(cloud_body, jsonChar, gateway_name, " cloud checked", " Not", (String(CLOUDGATEWAY) + "token/start").c_str(), (char*)WiFi.macAddress().c_str(), ("http://" + String(ip2CharArray(WiFi.localIP())) + "/").c_str(), gateway_name);
#    endif
  response->printf(footer, OMG_VERSION);
  request->send(response);
}

void handleTK(AsyncWebServerRequest* request) {
  Log.trace(F("handleTK: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("handleTK Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }
  }

  if (request->hasArg("deviceToken")) {
    String deviceToken = request->arg("deviceToken");

    String cmdTopic = String(mqtt_topic) + String(gateway_name) + String(subjectMQTTtoCLOUDset);
    String command = "{ \"cloudEnabled\": true, \"deviceToken\": \"" + deviceToken + "\" }";
    Log.trace(F("handleTK inject MQTT Command topic: '%s', command: '%s'" CR), cmdTopic.c_str(), command.c_str());
    receivingMQTT((char*)cmdTopic.c_str(), (char*)command.c_str());

    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->printf(header_html, (String(gateway_name) + " - Cloud").c_str());
    response->print(script);
    response->print(style);
    response->printf(token_body, jsonChar, gateway_name);

    response->printf(footer, OMG_VERSION);
    request->send(response);
  }
}

#  endif

void handleIN(AsyncWebServerRequest* request) {
  Log.trace(F("handleCN: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("handleIN Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }
  } else {
    char jsonChar[100];
    serializeJson(modules, jsonChar, measureJson(modules) + 1);

    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->printf(header_html, (String(gateway_name) + " - Information").c_str());

    String informationDisplay = stateMeasures(); // .replace(",\"", "}1");  // .replace("\":", "=2")

    // }1 json-oled }2 true } }1 Cloud }2 cloudEnabled}2true}1c
#  if defined(ZdisplaySSD1306)
    informationDisplay += "1SSD1306}2}1"; // }1 the bracket is not needed as the previous message ends with }
    informationDisplay += stateSSD1306Display();
#  endif
#  if defined(ZgatewayCloud)
    informationDisplay += "1Cloud}2}1";
    informationDisplay += stateCLOUDStatus();
#  endif

    informationDisplay += "1}2";
    informationDisplay.replace(",\"", "}1");
    informationDisplay.replace("\":", "}2");
    informationDisplay.replace("{\"", "");
    informationDisplay.replace("\"", "\\\"");
    response->printf(information_script, informationDisplay.c_str());

    response->print(script);
    response->print(style);
    response->printf(information_body, jsonChar, gateway_name);
    response->printf(footer, OMG_VERSION);
    request->send(response);
  }
}

uint32_t logIndex = 0;

void handleCS(AsyncWebServerRequest* request) {
  Log.trace(F("handleCS: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("handleCS Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }

    Log.trace(F("handleCS c1: %s, c2: %s" CR), request->arg("c1"), request->arg("c2"));
    String message = "";
    if (request->hasArg("c1")) {
      String c1 = request->arg("c1");

      String cmdTopic = String(mqtt_topic) + String(gateway_name) + "/" + c1.substring(0, c1.indexOf(' '));
      String command = c1.substring(c1.indexOf(' ') + 1);
      Log.trace(F("handleCS inject MQTT Command topic: '%s', command: '%s'" CR), cmdTopic, command);
      receivingMQTT((char*)cmdTopic.c_str(), (char*)command.c_str());
      message += logIndex + '}1' + '0' + '}1' + '}1';
      request->send(200, "text/plain", message);
    }
    if (request->hasArg("c2")) {
    }

    // Log.trace(F("Arguments %s" CR), message);
    message += logIndex + '}1' + '0' + '}1' + '}1';
    request->send(200, "text/plain", message);
  } else {
    request->send(SPIFFS, "/cs.html");
  }
}

void notFound(AsyncWebServerRequest* request) {
  //  if (!handleFileRead(request)) {
  //  }
  Log.trace(F("notFound: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  String path = request->url();
  if (!exists(path)) {
    if (exists(path + ".html")) {
      path += ".html";
    } else {
      request->send(404, "text/plain", "Not found");
      return;
    }
  }
  Log.trace(F("notFound: actual uri: %s, args: %d, method: %d" CR), path, request->args(), request->method());
  request->send(SPIFFS, path);
  // request->send(404, "text/plain", "Not found");
}

void recvMsg(uint8_t* data, size_t len) {
  // WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++) {
    d += char(data[i]);
  }
  // WebSerial.println(d);
}

void WebUISetup() {
  Log.trace(F("ZwebUI setup start" CR));

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

  server.on("/", HTTP_GET, handleSlash); // Main Menu
  server.on("/cs", HTTP_GET, handleCS); // Console

  server.on("/in", HTTP_GET, handleIN); // Information

  server.on("/cn", HTTP_GET, handleCN); // Configuration
#  if defined(ZgatewayCloud)
  server.on("/cl", HTTP_GET, handleCL); // Cloud configuration
  server.on("/tk", HTTP_POST, handleTK); // Store Device Token
#  endif

  server.on("/rt", HTTP_GET, handleRT); // Reset configuration

  server.onNotFound(notFound);

  // WebSerial.begin(&server);
  // WebSerial.msgCallback(recvMsg);

  server.begin();

  Log.begin(LOG_LEVEL, &WebLog);

  Log.notice(F("OpenMQTTGateway URL: http://%s/" CR), WiFi.localIP().toString().c_str());
  Log.notice(F("ZwebUI setup done" CR));
}

void WebUILoop() {
}

/*------------------- Serial logging interceptor ----------------------*/

// This pattern was borrowed from HardwareSerial and modified to support the ssd1306 display

SerialWeb WebLog(0); // Not sure about this, came from Hardwareserial
SerialWeb::SerialWeb(int x) {
}

/*
Initialize ssd1306 oled display for use, and display OMG logo
*/
void SerialWeb::begin() {
  // SSD1306.begin(); // User OMG serial support
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