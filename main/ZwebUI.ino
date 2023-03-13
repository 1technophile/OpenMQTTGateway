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
#  include <WebServer.h>

#  include "ArduinoLog.h"
#  include "config_WebUI.h"

WebServer server(80);

bool exists(String path) {
  bool yes = false;
  File file = FILESYSTEM.open(path, "r");
  if (!file.isDirectory()) {
    yes = true;
  }
  file.close();
  return yes;
}

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

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool handleFileRead(String path) {
  Log.trace(F("handleFileRead: path: %s, uri: %s, args: %d, method: %d" CR), path, server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("Arg: %d, %s=%s" CR), i, server.argName(i), server.arg(i));
    }
    // Log.trace(F("Arguments %s" CR), message);
    server.send(200, "text/plain", "00:14:36.767 RSL: RESULT = {\"Topic\":\"topic\"}");
    return true;
  } else {
    if (path.endsWith("/")) {
      path += "index.html";
    }
    String pathWithGz = path + ".gz";
    String pathWithHtml = path + ".html";
    if (exists(pathWithGz) || exists(path) || exists(pathWithHtml)) {
      if (exists(pathWithGz)) {
        path += ".gz";
      } else if (exists(pathWithHtml)) {
        path += ".html";
      }
      return returnFile(path);
    }
  }
  return false;
}

bool handleSlash() {
  Log.trace(F("handleSlash: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("Arg: %d, %s=%s" CR), i, server.argName(i), server.arg(i));
    }
    if (server.hasArg("m")) {
      server.send(200, "application/json", stateMeasures());
      return true;
    } else {
      // Log.trace(F("Arguments %s" CR), message);
      server.send(200, "text/plain", "00:14:36.767 RSL: RESULT = {\"Topic\":\"topic\"}");
      return true;
    }
  } else {
    return returnFile("/index.html");
  }
  return false;
}

bool handleCS() {
  Log.trace(F("handleCS: uri: %s, args: %d, method: %d" CR), server.uri(), server.args(), server.method());
  if (server.args()) {
    for (uint8_t i = 0; i < server.args(); i++) {
      Log.trace(F("Arg: %d, %s=%s" CR), i, server.argName(i), server.arg(i));
    }

    Log.trace(F("c1: %s, c2: %s" CR), server.arg("c1"), server.arg("c2"));
    if (server.hasArg("c1")) {
      String c1 = server.arg("c1");

      String cmdTopic = String(mqtt_topic) + String(gateway_name) + "/" + c1.substring(0, c1.indexOf(' '));
      String command = c1.substring(c1.indexOf(' ') + 1);
      Log.trace(F("inject MQTT Command topic: '%s', command: '%s'" CR), cmdTopic, command);
      receivingMQTT((char*)cmdTopic.c_str(), (char*)command.c_str());

      server.send(200, "text/plain", "106}11}1");
      return true;
    }

    // Log.trace(F("Arguments %s" CR), message);
    server.send(200, "text/plain", "200");
    return true;
  } else {
    return returnFile("/cs.html");
  }
  return false;
}

bool returnFile(String path) {
  String contentType = getContentType(path);
  File file = FILESYSTEM.open(path, "r");
  server.streamFile(file, contentType);
  file.close();
  return true;
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

  server.on("/cs", handleCS);

  server.on("/", handleSlash);

  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  server.begin();

  Log.notice(F("OpenMQTTGateway URL: http://%s/" CR), WiFi.localIP().toString().c_str());
  Log.notice(F("ZwebUI setup done" CR));
}

void WebUILoop() {
  server.handleClient();
}

#endif