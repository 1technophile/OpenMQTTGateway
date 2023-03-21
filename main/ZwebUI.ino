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

WebServer server(80);

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
#  if defined(ZdisplaySSD1306)
      if (currentOledMessage) {
        server.send(200, "application/json", "{t}{s}<b>" + String(currentOledMessage->title) + "</b>{e}{s}" + String(currentOledMessage->line1) + "{e}{s}" + String(currentOledMessage->line2) + "{e}{s}" + String(currentOledMessage->line3) + "{e}{s}" + String(currentOledMessage->line4) + "{e}</table>");
      } else {
        server.send(200, "application/json", "{t}{s}Uptime:{m}" + String(uptime()) + "{e}</table>");
      }
#  else
      server.send(200, "application/json", "{t}{s}Uptime:{m}" + String(uptime()) + "{e}</table>");
#  endif
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

void WebUILoop() {
  server.handleClient();
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