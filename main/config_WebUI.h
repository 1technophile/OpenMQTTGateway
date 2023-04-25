/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the DHT11/22 sensor
  
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
#ifndef config_WebUI_h
#define config_WebUI_h

#include <ArduinoJson.h>
#include <Wire.h>

#define WEBUI_TEXT_WIDTH 128

/*------------------- Optional Compiler Directives ----------------------*/

#ifndef WEB_TEMPLATE_BUFFER_MAX_SIZE
#  define WEB_TEMPLATE_BUFFER_MAX_SIZE 2000
#endif

#ifndef DISPLAY_METRIC
#  define DISPLAY_METRIC true // Units used for display of sensor data
#endif

#ifndef DISPLAY_WEBUI_INTERVAL
#  define DISPLAY_WEBUI_INTERVAL 3 // Number of seconds between json message displays
#endif

#ifdef WEBUI_DEVELOPMENT
#  pragma message("[WebUI] Usage of SPIFFS for missing WebUI content enabled")
#  define FILESYSTEM SPIFFS
#  define WEBUI_TRACE_LOGGING
#endif

#ifdef WEBUI_TRACE_LOGGING
#  define WEBUI_TRACE_LOG(...) Log.trace(__VA_ARGS__)
#else
#  define WEBUI_TRACE_LOG(...)
#endif

#ifndef WEBUI_AUTH
#  define WEBUI_AUTH true // Default to WebUI authentication
#endif

#ifndef WEBUI_LOGIN
#  define WEBUI_LOGIN "admin"
#endif

/*------------------- End of Compiler Directives ----------------------*/

#define WEBUI_SECURE                                                                    \
  if (webUISecure) {                                                                    \
    if (!server.authenticate(www_username, ota_pass)) {                                 \
      return server.requestAuthentication(DIGEST_AUTH, gateway_name, authFailResponse); \
    }                                                                                   \
  }

#define MAX_WIFI_NETWORKS_TO_SHOW 10

/*
Structure for queueing OMG messages to the display.
Length of each line is WEBUI_TEXT_WIDTH
- title
- line1
- line2
- line3
- line4
*/

struct webUIQueueMessage {
  char title[WEBUI_TEXT_WIDTH];
  char line1[WEBUI_TEXT_WIDTH];
  char line2[WEBUI_TEXT_WIDTH];
  char line3[WEBUI_TEXT_WIDTH];
  char line4[WEBUI_TEXT_WIDTH];
} webUIQueueMessage_t;

/*------------------- Global Functions and Variables ----------------------*/

#ifdef ZwebUI
#  define pubWebUI(...) webUIPubPrint(__VA_ARGS__)
void webUIPubPrint(const char*, JsonObject&);
#endif
void WebUISetup();
void WebUILoop();
void MQTTtoWebUI(char*, JsonObject&);

String stateWebUIStatus();

webUIQueueMessage* currentWebUIMessage;
bool newSSD1306Message = false; // Flag to indicate new message to display

/*------------------- End of Global Functions ----------------------*/

#define subjectMQTTtoWebUIset "/commands/MQTTtoWebUI/config"
#define subjectWebUItoMQTT    "/WebUItoMQTT"

/*------------------- Unit Conversion Functions ----------------------*/

#define convert_kmph2mph(kmph) (kmph * (1.0f / 1.609344f))

#define convert_mph2kmph(mph) (mph * 1.609344f)

#define convert_mm2inch(mm) (mm * 0.039370f)

#define convert_inch2mm(inch) (inch * 25.4f)

#define convert_kpa2psi(kpa) (kpa * (1.0f / 6.89475729f))

#define convert_psi2kpa(psi) (psi * 6.89475729f)

#define convert_hpa2inhg(hpa) (hpa * (1.0f / 33.8639f))

#define convert_inhg2hpa(inhg) (inhg * 33.8639f)

/*------------------- Take over serial output and split to  ----------------------*/

class SerialWeb : public Stream {
public:
  SerialWeb(int);
  void begin();

  int available(void); // Dummy functions
  int peek(void); // Dummy functions
  int read(void); // Dummy functions
  void flush(void); // Dummy functions

  inline size_t write(uint8_t x) {
    return write(&x, 1);
  }

  size_t write(const uint8_t* buffer, size_t size);
  inline size_t write(const char* buffer, size_t size) {
    return write((uint8_t*)buffer, size);
  }
  inline size_t write(const char* s) {
    return write((uint8_t*)s, strlen(s));
  }
  inline size_t write(unsigned long n) {
    return write((uint8_t)n);
  }
  inline size_t write(long n) {
    return write((uint8_t)n);
  }
  inline size_t write(unsigned int n) {
    return write((uint8_t)n);
  }
  inline size_t write(int n) {
    return write((uint8_t)n);
  }

protected:
};

extern SerialWeb WebLog;

/*------------------- Take over serial output and split to  ----------------------*/

#endif