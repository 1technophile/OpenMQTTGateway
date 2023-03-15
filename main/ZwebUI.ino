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
#  include <WebSerial.h>

#  include "ArduinoLog.h"
#  include "config_WebContent.h"
#  include "config_WebUI.h"

AsyncWebServer server(80);

/*------------------- Local functions ----------------------*/

void notFound(AsyncWebServerRequest* request);
void handleSlash(AsyncWebServerRequest* request);
void handleCS(AsyncWebServerRequest* request);
void handleCN(AsyncWebServerRequest* request);

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

String processer(const String& var) {
  if (var == "_OMG_GATEWAYNAME_")
    return F("Hello world!");
  return String();
}

void handleSlash(AsyncWebServerRequest* request) {
  Log.trace(F("handleSlash: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }
    if (request->hasArg("m")) {
      request->send(200, "application/json", stateMeasures());
    } else {
      // Log.trace(F("Arguments %s" CR), message);
      request->send(200, "text/plain", "00:14:36.767 RSL: RESULT = {\"Topic\":\"topic\"}");
    }
  } else {
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->print(header_html);
    response->print(slash_script);
    response->print(script);
    response->print(style);
    response->print(slash_body);
    response->print(footer);
    request->send(response);
  }
}

void handleCN(AsyncWebServerRequest* request) {
  Log.trace(F("handleCN: uri: %s, args: %d, method: %d" CR), request->url(), request->args(), request->method());
  if (request->args()) {
    for (uint8_t i = 0; i < request->args(); i++) {
      Log.trace(F("handleCN Arg: %d, %s=%s" CR), i, request->argName(i).c_str(), request->arg(i).c_str());
    }
    if (request->hasArg("m")) {
      request->send(200, "application/json", stateMeasures());
    } else {
      // Log.trace(F("Arguments %s" CR), message);
      request->send(200, "text/plain", "00:14:36.767 RSL: RESULT = {\"Topic\":\"topic\"}");
    }
  } else {
    AsyncResponseStream* response = request->beginResponseStream("text/html");
    response->print(header_html);
    response->print(script);
    response->print(style);
    response->print(config_body);
    response->print(footer);
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
  WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++) {
    d += char(data[i]);
  }
  WebSerial.println(d);
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

  server.on("/cs", HTTP_GET, handleCS);
  server.on("/cn", HTTP_GET, handleCN);

  server.on("/", HTTP_GET, handleSlash);

  server.onNotFound(notFound);

  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);

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
      WebSerial.println(line);
      lineIndex = 0;
    } else {
      line[lineIndex++] = char(buffer[i]);
    }
  }
}

#endif