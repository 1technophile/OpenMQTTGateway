/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    HELTEC ESP32 LORA - SSD1306 / Onboard 0.96-inch 128*64 dot matrix OLED display
  
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
#include "User_config.h"
#if defined(ZboardHELTEC)
#  include "ArduinoLog.h"
#  include "OledSerial.h"
#  include "config_HELTEC.h"

void logToLCD(bool display) {
  display ? Log.begin(LOG_LEVEL_LCD, &Oled) : Log.begin(LOG_LEVEL, &Serial); // Log on LCD following LOG_LEVEL_LCD
}

// LCD HeltecLCD;

void setupHELTEC() {
  Log.notice(F("Setup HELTEC Display" CR));
  Oled.begin();
  Log.notice(F("Setup HELTEC Display end" CR));

#  if LOG_TO_LCD
  Log.begin(LOG_LEVEL_LCD, &Oled); // Log on LCD following LOG_LEVEL_LCD
#  endif
}

void loopHELTEC() {
}

void MQTTtoHELTEC(char* topicOri, JsonObject& HELTECdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoHELTECset)) {
    Log.trace(F("MQTTtoHELTEC json set" CR));
    // Log display set between HELTEC lcd (true) and serial monitor (false)
    if (HELTECdata.containsKey("log-lcd")) {
      bool displayOnLCD = HELTECdata["log-lcd"];
      Log.notice(F("Set lcd log: %T" CR), displayOnLCD);
      logToLCD(displayOnLCD);
    }
  }
}

size_t HELTECPrint(String msg) {
  return Oled.print(msg);
}

#endif
