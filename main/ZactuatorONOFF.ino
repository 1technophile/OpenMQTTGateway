/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker 
   Send and receiving command by MQTT
 
    Output GPIO defined to High or Low
  
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
unsigned long timeinttemp = 0;

void setupONOFF() {
  pinMode(ACTUATOR_ONOFF_GPIO, OUTPUT);
#  ifdef ACTUATOR_ONOFF_DEFAULT
  digitalWrite(ACTUATOR_ONOFF_GPIO, ACTUATOR_ONOFF_DEFAULT);
#  endif
}

#  if jsonReceiving
void MQTTtoONOFF(char* topicOri, JsonObject& ONOFFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoONOFF)) {
    Log.trace(F("MQTTtoONOFF json data analysis" CR));
    int boolSWITCHTYPE = ONOFFdata["cmd"] | 99;
    int gpio = ONOFFdata["gpio"] | ACTUATOR_ONOFF_GPIO;
    if (boolSWITCHTYPE != 99) {
      Log.notice(F("MQTTtoONOFF boolSWITCHTYPE ok: %d" CR), boolSWITCHTYPE);
      Log.notice(F("GPIO number: %d" CR), gpio);
      pinMode(gpio, OUTPUT);
      digitalWrite(gpio, boolSWITCHTYPE);
      // we acknowledge the sending by publishing the value to an acknowledgement topic
      pub(subjectGTWONOFFtoMQTT, ONOFFdata);
    } else {
      if (ONOFFdata["cmd"] == "high_pulse") {
        Log.notice(F("MQTTtoONOFF high_pulse ok" CR));
        Log.notice(F("GPIO number: %d" CR), gpio);
        int pulselength = ONOFFdata["pulse_length"] | 500;
        Log.notice(F("Pulse length: %d ms" CR), pulselength);
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, HIGH);
        delay(pulselength);
        digitalWrite(gpio, LOW);
      } else if (ONOFFdata["cmd"] == "low_pulse") {
        Log.notice(F("MQTTtoONOFF low_pulse ok" CR));
        Log.notice(F("GPIO number: %d" CR), gpio);
        int pulselength = ONOFFdata["pulse_length"] | 500;
        Log.notice(F("Pulse length: %d ms" CR), pulselength);
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, LOW);
        delay(pulselength);
        digitalWrite(gpio, HIGH);
      } else {
        Log.error(F("MQTTtoONOFF failed json read" CR));
      }
    }
  }
}
#  endif

#  if simpleReceiving
void MQTTtoONOFF(char* topicOri, char* datacallback) {
  if ((cmpToMainTopic(topicOri, subjectMQTTtoONOFF))) {
    Log.trace(F("MQTTtoONOFF" CR));
    char* endptr = NULL;
    long gpio = strtol(datacallback, &endptr, 10);
    if (datacallback == endptr)
      gpio = ACTUATOR_ONOFF_GPIO;

    Log.notice(F("GPIO number: %d" CR), gpio);
    pinMode(gpio, OUTPUT);

    bool ON = false;
    if (strstr(topicOri, ONKey) != NULL)
      ON = true;
    if (strstr(topicOri, OFFKey) != NULL)
      ON = false;

    digitalWrite(gpio, ON);
    // we acknowledge the sending by publishing the value to an acknowledgement topic
    char b = ON;
    pub(subjectGTWONOFFtoMQTT, &b);
  }
}
#  endif

//Check regularly temperature of the ESP32 board and switch OFF the relay if temperature is more than MAX_TEMP_ACTUATOR
#  ifdef MAX_TEMP_ACTUATOR
void OverHeatingRelayOFF() {
#    if defined(ESP32) && !defined(NO_INT_TEMP_READING)
  if (millis() > (timeinttemp + TimeBetweenReadingIntTemp)) {
    float internalTempc = intTemperatureRead();
    Log.trace(F("Internal temperature of the ESP32 %F" CR), internalTempc);
    if (internalTempc > MAX_TEMP_ACTUATOR && digitalRead(ACTUATOR_ONOFF_GPIO) == ACTUATOR_ON) {
      Log.error(F("[ActuatorONOFF] OverTemperature detected ( %F > %F ) switching OFF Actuator" CR), internalTempc, MAX_TEMP_ACTUATOR);
      ActuatorManualTrigger(!ACTUATOR_ON);
    }
    timeinttemp = millis();
  }
#    endif
}
#  else
void OverHeatingRelayOFF() {}
#  endif

void ActuatorManualTrigger(uint8_t level) {
#  ifdef ACTUATOR_BUTTON_TRIGGER_LEVEL
  if (level == ACTUATOR_BUTTON_TRIGGER_LEVEL) {
    // Change level value to the opposite of the current level
    level = !digitalRead(ACTUATOR_ONOFF_GPIO);
  }
#  else
  level = !digitalRead(ACTUATOR_ONOFF_GPIO);
#  endif
  Log.trace(F("Actuator triggered %d" CR), level);
  digitalWrite(ACTUATOR_ONOFF_GPIO, level);
  // Send the state of the switch to the broker so as to update the status
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject ONOFFdata = jsonBuffer.to<JsonObject>();
  ONOFFdata["cmd"] = (int)level;
  pub(subjectGTWONOFFtoMQTT, ONOFFdata);
}

#endif
