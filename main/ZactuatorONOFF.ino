/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker  
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

#  ifdef ESP32
// Global struct to store live ONOFF configuration data
ONOFFConfig_s ONOFFConfig;

void ONOFFConfig_init() {
  ONOFFConfig.ONOFFState = !ACTUATOR_ON;
  ONOFFConfig.useLastStateOnStart = USE_LAST_STATE_ON_RESTART;
}

void ONOFFConfig_fromJson(JsonObject& ONOFFdata) {
  Config_update(ONOFFdata, "uselaststate", ONOFFConfig.useLastStateOnStart);
  Config_update(ONOFFdata, "cmd", ONOFFConfig.ONOFFState);

  if (ONOFFdata.containsKey("erase") && ONOFFdata["erase"].as<bool>()) {
    // Erase config from NVS (non-volatile storage)
    preferences.begin(Gateway_Short_Name, false);
    if (preferences.isKey("ONOFFConfig")) {
      int result = preferences.remove("ONOFFConfig");
      Log.notice(F("ONOFF config erase result: %d" CR), result);
      preferences.end();
      return; // Erase prevails on save, so skipping save
    } else {
      Log.notice(F("ONOFF config not found" CR));
      preferences.end();
    }
  }
  if (ONOFFdata.containsKey("save") && ONOFFdata["save"].as<bool>()) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject jo = jsonBuffer.to<JsonObject>();
    jo["uselaststate"] = ONOFFConfig.useLastStateOnStart;
    jo["cmd"] = ONOFFConfig.ONOFFState;
    // Save config into NVS (non-volatile storage)
    String conf = "";
    serializeJson(jsonBuffer, conf);
    preferences.begin(Gateway_Short_Name, false);
    int result = preferences.putString("ONOFFConfig", conf);
    preferences.end();
    Log.notice(F("ONOFF Config_save: %s, result: %d" CR), conf.c_str(), result);
  }
}

void ONOFFConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (preferences.isKey("ONOFFConfig")) {
    auto error = deserializeJson(jsonBuffer, preferences.getString("ONOFFConfig", "{}"));
    preferences.end();
    if (error) {
      Log.error(F("ONOFF config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      return;
    }
    if (jsonBuffer.isNull()) {
      Log.warning(F("ONOFF config is null" CR));
      return;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    ONOFFConfig_fromJson(jo);
    Log.notice(F("ONOFF config loaded" CR));
  } else {
    preferences.end();
    Log.notice(F("ONOFF config not found" CR));
  }
}
#  else
void ONOFFConfig_init(){};
void ONOFFConfig_fromJson(JsonObject& ONOFFdata){};
void ONOFFConfig_load(){};
#  endif

void updatePowerIndicator() {
#  ifdef LED_ACTUATOR_ONOFF
  if (digitalRead(ACTUATOR_ONOFF_GPIO) == ACTUATOR_ON) {
    ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_ACTUATOR_ONOFF_COLOR);
  } else {
    ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_COLOR_BLACK);
  }
#  endif
}

void setupONOFF() {
#  ifdef MAX_TEMP_ACTUATOR
  xTaskCreate(overLimitTemp, "overLimitTemp", 4000, NULL, 10, NULL);
#  endif
#  ifdef ESP32
  ONOFFConfig_init();
  ONOFFConfig_load();
  Log.notice(F("Target state on restart: %T" CR), ONOFFConfig.ONOFFState);
  Log.notice(F("Use last state on restart: %T" CR), ONOFFConfig.useLastStateOnStart);
#  endif
  pinMode(ACTUATOR_ONOFF_GPIO, OUTPUT);
#  ifdef ACTUATOR_ONOFF_DEFAULT
  digitalWrite(ACTUATOR_ONOFF_GPIO, ACTUATOR_ONOFF_DEFAULT);
#  elif defined(ESP32)
  if (ONOFFConfig.useLastStateOnStart) {
    digitalWrite(ACTUATOR_ONOFF_GPIO, ONOFFConfig.ONOFFState);
  } else {
    digitalWrite(ACTUATOR_ONOFF_GPIO, !ACTUATOR_ON);
  }
#  endif
  updatePowerIndicator();
  Log.trace(F("ZactuatorONOFF setup done" CR));
}

#  if jsonReceiving
void XtoONOFF(const char* topicOri, JsonObject& ONOFFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoONOFF)) {
    Log.trace(F("MQTTtoONOFF json data analysis" CR));
    int boolSWITCHTYPE = ONOFFdata["cmd"] | 99;
    int gpio = ONOFFdata["gpio"] | ACTUATOR_ONOFF_GPIO;
    if (boolSWITCHTYPE != 99) {
      Log.notice(F("MQTTtoONOFF boolSWITCHTYPE ok: %d" CR), boolSWITCHTYPE);
      Log.notice(F("GPIO number: %d" CR), gpio);
      pinMode(gpio, OUTPUT);
      digitalWrite(gpio, boolSWITCHTYPE);
#    ifdef LED_ACTUATOR_ONOFF
      if (boolSWITCHTYPE == ACTUATOR_ON) {
        ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_ACTUATOR_ONOFF_COLOR);
      } else {
        ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_COLOR_BLACK);
      }
#    endif
#    ifdef ESP32
      if (ONOFFConfig.useLastStateOnStart) {
        ONOFFdata["save"] = true;
        ONOFFConfig_fromJson(ONOFFdata);
      }
#    endif
      // we acknowledge the sending by publishing the value to an acknowledgement topic
      stateONOFFMeasures();
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
  if (cmpToMainTopic(topicOri, subjectMQTTtoONOFFset)) {
    Log.trace(F("MQTTtoONOFF json set" CR));
    /*
     * Configuration modifications priorities:
     *  First `init=true` and `load=true` commands are executed (if both are present, INIT prevails on LOAD)
     *  Then parameters included in json are taken in account
     *  Finally `erase=true` and `save=true` commands are executed (if both are present, ERASE prevails on SAVE)
     */
    if (ONOFFdata.containsKey("init") && ONOFFdata["init"].as<bool>()) {
      // Restore the default (initial) configuration
      ONOFFConfig_init();
    } else if (ONOFFdata.containsKey("load") && ONOFFdata["load"].as<bool>()) {
      // Load the saved configuration, if not initialised
      ONOFFConfig_load();
    }

    // Load config from json if available
    ONOFFConfig_fromJson(ONOFFdata);
    stateONOFFMeasures();
  }
}
#  endif

#  if simpleReceiving
void XtoONOFF(const char* topicOri, const char* datacallback) {
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
#    ifdef LED_ACTUATOR_ONOFF
    if (ON == ACTUATOR_ON) {
      ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_ACTUATOR_ONOFF_COLOR);
    } else {
      ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_COLOR_BLACK);
    }
#    endif
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
    if (internalTempc > MAX_TEMP_ACTUATOR && previousInternalTempc > MAX_TEMP_ACTUATOR) {
      if (digitalRead(ACTUATOR_ONOFF_GPIO) == ACTUATOR_ON) { // This could be with the previous condition, but it is better to trigger the digitalRead only if the previous condition is met to avoid the digitalRead
        Log.error(F("[ActuatorONOFF] OverTemperature detected ( %F > %F ) switching OFF Actuator" CR), internalTempc, MAX_TEMP_ACTUATOR);
        ActuatorTrigger();
#      ifdef LED_ACTUATOR_ONOFF
        ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_ERROR_COLOR, -1);
#      endif
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
void overLimitCurrent(float RN8209current) {
  static float RN8209previousCurrent = 0;
  Log.trace(F("RN8209 Current %F" CR), RN8209current);
  // We switch OFF the actuator if the current of the RN8209 is more than MAX_CURRENT_ACTUATOR.
  if (RN8209current > MAX_CURRENT_ACTUATOR && RN8209previousCurrent > MAX_CURRENT_ACTUATOR) {
    if (digitalRead(ACTUATOR_ONOFF_GPIO) == ACTUATOR_ON) { // This could be with the previous condition, but it is better to trigger the digitalRead only if the previous condition is met to avoid the digitalRead
      Log.error(F("[ActuatorONOFF] OverCurrent detected ( %F > %F ) switching OFF Actuator" CR), RN8209current, MAX_CURRENT_ACTUATOR);
      ActuatorTrigger();
#    ifdef LED_ACTUATOR_ONOFF
      ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_ERROR_COLOR, -1);
#    endif
    }
  }
  RN8209previousCurrent = RN8209current;
}
#  else
void overLimitCurrent(float RN8209current) {}
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
#  ifdef LED_ACTUATOR_ONOFF
  if (level == ACTUATOR_ON) {
    ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_ACTUATOR_ONOFF_COLOR);
  } else {
    ledManager.setMode(LED_ACTUATOR_ONOFF, 0, LEDManager::Mode::STATIC, LED_COLOR_BLACK);
  }
#  endif

#  ifdef ESP32
  if (ONOFFConfig.useLastStateOnStart) {
    StaticJsonDocument<64> jsonBuffer;
    JsonObject ONOFFdata = jsonBuffer.to<JsonObject>();
    ONOFFdata["cmd"] = (int)level;
    ONOFFdata["save"] = true;
    ONOFFConfig_fromJson(ONOFFdata);
  }
#  endif
  stateONOFFMeasures();
}

void stateONOFFMeasures() {
  //Publish actuator state
  StaticJsonDocument<128> jsonBuffer;
  JsonObject ONOFFdata = jsonBuffer.to<JsonObject>();
  ONOFFdata["cmd"] = (int)digitalRead(ACTUATOR_ONOFF_GPIO);
#  ifdef ESP32
  ONOFFdata["uselaststate"] = ONOFFConfig.useLastStateOnStart;
#  endif
  ONOFFdata["origin"] = subjectGTWONOFFtoMQTT;
  enqueueJsonObject(ONOFFdata, QueueSemaphoreTimeOutTask);
}
#endif
