/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker 
   Send and receiving command by MQTT
 
    Output pin defined to High or Low
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - 1technophile
  
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

#ifdef ZactuatorONOFF

#  ifdef jsonReceiving
void MQTTtoONOFF(char* topicOri, JsonObject& ONOFFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoONOFF)) {
    Log.trace(F("MQTTtoONOFF json data analysis" CR));
    int boolSWITCHTYPE = ONOFFdata["cmd"] | 99;
    int pin = ONOFFdata["pin"] | ACTUATOR_ONOFF_GPIO;
    if (boolSWITCHTYPE != 99) {
      Log.notice(F("MQTTtoONOFF boolSWITCHTYPE ok: %d" CR), boolSWITCHTYPE);
      Log.notice(F("GPIO number: %d" CR), pin);
      pinMode(pin, OUTPUT);
      digitalWrite(pin, boolSWITCHTYPE);
      // we acknowledge the sending by publishing the value to an acknowledgement topic
      pub(subjectGTWONOFFtoMQTT, ONOFFdata);
    } else {
      Log.error(F("MQTTtoONOFF failed json read" CR));
    }
  }
}
#  endif

#  ifdef simpleReceiving
void MQTTtoONOFF(char* topicOri, char* datacallback) {
  if ((cmpToMainTopic(topicOri, subjectMQTTtoONOFF))) {
    Log.trace(F("MQTTtoONOFF" CR));
    int pin = strtol(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
    Log.notice(F("GPIO number: %d" CR), pin);
    pinMode(pin, OUTPUT);

    bool ON = false;
    if (strstr(topicOri, ONKey) != NULL)
      ON = true;
    if (strstr(topicOri, OFFKey) != NULL)
      ON = false;

    digitalWrite(pin, ON);
    // we acknowledge the sending by publishing the value to an acknowledgement topic
    char b = ON;
    pub(subjectGTWONOFFtoMQTT, &b);
  }
}
#  endif

#endif
