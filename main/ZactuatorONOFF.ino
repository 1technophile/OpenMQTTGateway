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

void setupONOFF() {
#  ifdef MAX_CURRENT_ACTUATOR
  xTaskCreate(overLimitCurrent, "overLimitCurrent", 2500, NULL, 10, NULL);
#  endif
#  ifdef MAX_TEMP_ACTUATOR
  xTaskCreate(overLimitTemp, "overLimitTemp", 2500, NULL, 10, NULL);
#  endif
  pinMode(ACTUATOR_ONOFF_GPIO, OUTPUT);
#  ifdef ACTUATOR_ONOFF_DEFAULT
  digitalWrite(ACTUATOR_ONOFF_GPIO, ACTUATOR_ONOFF_DEFAULT);
#  endif
  Log.trace(F("ZactuatorONOFF setup done" CR));
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
void overLimitTemp(void* pvParameters) {
#    if defined(ESP32) && !defined(NO_INT_TEMP_READING)
  for (;;) {
    static float previousInternalTempc = 0;
    float internalTempc = intTemperatureRead();
    Log.trace(F("Internal temperature of the ESP32 %F" CR), internalTempc);
    // We switch OFF the actuator if the temperature of the ESP32 is more than MAX_TEMP_ACTUATOR two consecutive times, so as to avoid false single readings to trigger the relay OFF.
    if (internalTempc > MAX_TEMP_ACTUATOR && previousInternalTempc > MAX_TEMP_ACTUATOR && digitalRead(ACTUATOR_ONOFF_GPIO) == ACTUATOR_ON) {
      Log.error(F("[ActuatorONOFF] OverTemperature detected ( %F > %F ) switching OFF Actuator" CR), internalTempc, MAX_TEMP_ACTUATOR);
      ActuatorTrigger();
      for (int i = 0; i < 30; i++) {
        ErrorIndicatorON();
        vTaskDelay(200);
        ErrorIndicatorOFF();
        vTaskDelay(200);
      }
    }
    previousInternalTempc = internalTempc;
    vTaskDelay(TimeBetweenReadingIntTemp);
  }
#    endif
}
#  endif

// Check regularly current the relay and switch it OFF if the current is more than MAX_CURRENT_ACTUATOR
#  ifdef MAX_CURRENT_ACTUATOR
void overLimitCurrent(void* pvParameters) {
  for (;;) {
    float current = getRN8209current();
    Log.trace(F("RN8209 Current %F" CR), current);
    // We switch OFF the actuator if the current of the RN8209 is more than MAX_CURRENT_ACTUATOR.
    if (current > MAX_CURRENT_ACTUATOR && digitalRead(ACTUATOR_ONOFF_GPIO) == ACTUATOR_ON) {
      Log.error(F("[ActuatorONOFF] OverCurrent detected ( %F > %F ) switching OFF Actuator" CR), current, MAX_CURRENT_ACTUATOR);
      ActuatorTrigger();
      for (int i = 0; i < 30; i++) {
        ErrorIndicatorON();
        vTaskDelay(200);
        ErrorIndicatorOFF();
        vTaskDelay(200);
      }
    }
    vTaskDelay(TimeBetweenReadingCurrent);
  }
}
#  endif

/*
  Handling of actuator control following the cases below:
  -Button press, if the button goes to ACTUATOR_BUTTON_TRIGGER_LEVEL we change the Actuator level
  -Status less switch state change (a switch without ON OFF labels), an action of this type of switch will trigger a change of the actuator state independently from the switch position
*/
void ActuatorTrigger() {
  uint8_t level = !digitalRead(ACTUATOR_ONOFF_GPIO);
  Log.trace(F("Actuator triggered %d" CR), level);
  digitalWrite(ACTUATOR_ONOFF_GPIO, level);
  // Send the state of the switch to the broker so as to update the status
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject ONOFFdata = jsonBuffer.to<JsonObject>();
  ONOFFdata["cmd"] = (int)level;
  pub(subjectGTWONOFFtoMQTT, ONOFFdata);
}

void stateONOFFMeasures() {
  //Publish actuator state
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject ONOFFdata = jsonBuffer.to<JsonObject>();
  ONOFFdata["cmd"] = (int)digitalRead(ACTUATOR_ONOFF_GPIO);
  pub(subjectGTWONOFFtoMQTT, ONOFFdata);
}
#endif
