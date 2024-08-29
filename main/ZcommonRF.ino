/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a wifi or ethernet gateway between your BLE/433mhz/infrared IR signal and an MQTT broker 
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

#if defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRTL_433) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

void initCC1101() {
#  ifdef ZradioCC1101 //receiving with CC1101
  // Loop on getCC1101() until it returns true and break after 10 attempts
  int delayMS = 16;
  int delayMaxMS = 500;
  for (int i = 0; i < 10; i++) {
    if (ELECHOUSE_cc1101.getCC1101()) {
      Log.notice(F("C1101 spi Connection OK" CR));
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.SetRx(RFConfig.frequency);
      break;
    } else {
      Log.error(F("C1101 spi Connection Error" CR));
      delay(delayMS);
    }
    // truncated exponential backoff
    delayMS = delayMS * 2;
    if (delayMS > delayMaxMS) delayMS = delayMaxMS;
  }
#  endif
}

void setupCommonRF() {
  RFConfig_init();
  RFConfig_load();
}

bool validFrequency(float mhz) {
  //  CC1101 valid frequencies 300-348 MHZ, 387-464MHZ and 779-928MHZ.
  if (mhz >= 300 && mhz <= 348)
    return true;
  if (mhz >= 387 && mhz <= 464)
    return true;
  if (mhz >= 779 && mhz <= 928)
    return true;
  return false;
}

int currentReceiver = ACTIVE_NONE;

#  if !defined(ZgatewayRFM69) && !defined(ZactuatorSomfy)
// Check if a receiver is available
bool validReceiver(int receiver) {
  switch (receiver) {
#    ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      return true;
#    endif
#    ifdef ZgatewayRF
    case ACTIVE_RF:
      return true;
#    endif
#    ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      return true;
#    endif
#    ifdef ZgatewayRF2
    case ACTIVE_RF2:
      return true;
#    endif
    default:
      Log.error(F("ERROR: stored receiver %d not available" CR), receiver);
  }
  return false;
}
#  endif

void disableCurrentReceiver() {
  Log.trace(F("disableCurrentReceiver: %d" CR), currentReceiver);
  switch (currentReceiver) {
    case ACTIVE_NONE:
      break;
#  ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      disablePilightReceive();
      break;
#  endif
#  ifdef ZgatewayRF
    case ACTIVE_RF:
      disableRFReceive();
      break;
#  endif
#  ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      disableRTLreceive();
      break;
#  endif
#  ifdef ZgatewayRF2
    case ACTIVE_RF2:
      disableRF2Receive();
      break;
#  endif
    default:
      Log.error(F("ERROR: unsupported receiver %d" CR), RFConfig.activeReceiver);
  }
}

void enableActiveReceiver() {
  Log.trace(F("enableActiveReceiver: %d" CR), RFConfig.activeReceiver);
  switch (RFConfig.activeReceiver) {
#  ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      initCC1101();
      enablePilightReceive();
      currentReceiver = ACTIVE_PILIGHT;
      break;
#  endif
#  ifdef ZgatewayRF
    case ACTIVE_RF:
      initCC1101();
      enableRFReceive();
      currentReceiver = ACTIVE_RF;
      break;
#  endif
#  ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      initCC1101();
      enableRTLreceive();
      currentReceiver = ACTIVE_RTL;
      break;
#  endif
#  ifdef ZgatewayRF2
    case ACTIVE_RF2:
      initCC1101();
      enableRF2Receive();
      currentReceiver = ACTIVE_RF2;
      break;
#  endif
    case ACTIVE_RECERROR:
      Log.error(F("ERROR: no receiver selected" CR));
      break;
    default:
      Log.error(F("ERROR: unsupported receiver %d" CR), RFConfig.activeReceiver);
  }
}

String stateRFMeasures() {
  //Publish RTL_433 state
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject RFdata = jsonBuffer.to<JsonObject>();
  RFdata["active"] = RFConfig.activeReceiver;
#  if defined(ZradioCC1101) || defined(ZradioSX127x)
  RFdata["frequency"] = RFConfig.frequency;
  if (RFConfig.activeReceiver == ACTIVE_RTL) {
#    ifdef ZgatewayRTL_433
    RFdata["rssithreshold"] = (int)getRTLrssiThreshold();
    RFdata["rssi"] = (int)getRTLCurrentRSSI();
    RFdata["avgrssi"] = (int)getRTLAverageRSSI();
    RFdata["count"] = (int)getRTLMessageCount();
#    endif
#    ifdef ZradioSX127x
    RFdata["ookthreshold"] = (int)getOOKThresh();
#    endif
  }
#  endif
  RFdata["origin"] = subjectcommonRFtoMQTT;
  enqueueJsonObject(RFdata);

  String output;
  serializeJson(RFdata, output);
  return output;
}

void RFConfig_fromJson(JsonObject& RFdata) {
  bool success = false;
  if (RFdata.containsKey("frequency") && validFrequency(RFdata["frequency"])) {
    Config_update(RFdata, "frequency", RFConfig.frequency);
    Log.notice(F("RF Receive mhz: %F" CR), RFConfig.frequency);
    success = true;
  }
  if (RFdata.containsKey("active")) {
    Log.notice(F("RF receiver active: %d" CR), RFConfig.activeReceiver);
    Config_update(RFdata, "active", RFConfig.activeReceiver);
    success = true;
  }
#  ifdef ZgatewayRTL_433
  if (RFdata.containsKey("rssithreshold")) {
    Log.notice(F("RTL_433 RSSI Threshold : %d " CR), RFConfig.rssiThreshold);
    Config_update(RFdata, "rssithreshold", RFConfig.rssiThreshold);
    rtl_433.setRSSIThreshold(RFConfig.rssiThreshold);
    success = true;
  }
#    if defined(RF_SX1276) || defined(RF_SX1278)
  if (RFdata.containsKey("ookthreshold")) {
    Config_update(RFdata, "ookthreshold", RFConfig.newOokThreshold);
    Log.notice(F("RTL_433 ookThreshold %d" CR), RFConfig.newOokThreshold);
    rtl_433.setOOKThreshold(RFConfig.newOokThreshold);
    success = true;
  }
#    endif
  if (RFdata.containsKey("status")) {
    Log.notice(F("RF get status:" CR));
    rtl_433.getStatus();
    success = true;
  }
  if (!success) {
    Log.error(F("MQTTtoRF Fail json" CR));
  }
#  endif
  disableCurrentReceiver();
  enableActiveReceiver();
#  ifdef ESP32
  if (RFdata.containsKey("erase") && RFdata["erase"].as<bool>()) {
    // Erase config from NVS (non-volatile storage)
    preferences.begin(Gateway_Short_Name, false);
    if (preferences.isKey("RFConfig")) {
      int result = preferences.remove("RFConfig");
      Log.notice(F("RF config erase result: %d" CR), result);
      preferences.end();
      return; // Erase prevails on save, so skipping save
    } else {
      Log.notice(F("RF config not found" CR));
      preferences.end();
    }
  }
  if (RFdata.containsKey("save") && RFdata["save"].as<bool>()) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject jo = jsonBuffer.to<JsonObject>();
    jo["frequency"] = RFConfig.frequency;
    jo["active"] = RFConfig.activeReceiver;
// Don't save those for now, need to be tested
#    ifdef ZgatewayRTL_433
//jo["rssithreshold"] = RFConfig.rssiThreshold;
//jo["ookthreshold"] = RFConfig.newOokThreshold;
#    endif
    // Save config into NVS (non-volatile storage)
    String conf = "";
    serializeJson(jsonBuffer, conf);
    preferences.begin(Gateway_Short_Name, false);
    int result = preferences.putString("RFConfig", conf);
    preferences.end();
    Log.notice(F("RF Config_save: %s, result: %d" CR), conf.c_str(), result);
  }
#  endif
}

void RFConfig_init() {
  RFConfig.frequency = RF_FREQUENCY;
  RFConfig.activeReceiver = ACTIVE_RECEIVER;
  RFConfig.rssiThreshold = 0;
  RFConfig.newOokThreshold = 0;
}

void RFConfig_load() {
#  ifdef ESP32
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (preferences.isKey("RFConfig")) {
    auto error = deserializeJson(jsonBuffer, preferences.getString("RFConfig", "{}"));
    preferences.end();
    if (error) {
      Log.error(F("RF Config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      return;
    }
    if (jsonBuffer.isNull()) {
      Log.warning(F("RF Config is null" CR));
      return;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    RFConfig_fromJson(jo);
    Log.notice(F("RF Config loaded" CR));
  } else {
    preferences.end();
    Log.notice(F("RF Config not found using default" CR));
    enableActiveReceiver();
  }
#  else
  enableActiveReceiver();
#  endif
}

void XtoRFset(const char* topicOri, JsonObject& RFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRFset)) {
    Log.trace(F("MQTTtoRF json set" CR));

    /*
     * Configuration modifications priorities:
     *  First `init=true` and `load=true` commands are executed (if both are present, INIT prevails on LOAD)
     *  Then parameters included in json are taken in account
     *  Finally `erase=true` and `save=true` commands are executed (if both are present, ERASE prevails on SAVE)
     */
    if (RFdata.containsKey("init") && RFdata["init"].as<bool>()) {
      // Restore the default (initial) configuration
      RFConfig_init();
    } else if (RFdata.containsKey("load") && RFdata["load"].as<bool>()) {
      // Load the saved configuration, if not initialised
      RFConfig_load();
    }

    // Load config from json if available
    RFConfig_fromJson(RFdata);
    stateRFMeasures();
  }
}
#endif
