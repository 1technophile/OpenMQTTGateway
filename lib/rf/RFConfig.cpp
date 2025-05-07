#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <User_config.h>
#include <config_RF.h>
#include <main_utils.h>
#include <rf/RFConfig.h>
#include <rf/ZCommonRF.h>

#if defined(ESP32)
#  include <Preferences.h>
#endif

#if defined(ESP32)
RFConfig::RFConfig(Preferences& iPreferences) : preferences(iPreferences) { reset(); };
#else
RFConfig::RFConfig() { reset(); };
#endif

void RFConfig::reset() {
  frequency = RF_FREQUENCY;
  activeReceiver = ACTIVE_RF;
  rssiThreshold = 0;
  newOokThreshold = 0;
}

void RFConfig::setRFHandler(ZCommonRF* iRFHandler) {
  rfHandler = iRFHandler;
}

float RFConfig::getFrequency() const {
  return frequency;
}

bool RFConfig::setFrequency(float newFrequency) {
  if (rfHandler->validFrequency(newFrequency)) {
    frequency = newFrequency;
    return true;
  } else {
    Log.error(F("RF Invalid frequency: %F" CR), newFrequency);
    return false;
  }
}

int RFConfig::getActiveReceiver() const {
  return activeReceiver;
}

void RFConfig::setActiveReceiver(int newActiveReceiver) {
  activeReceiver = newActiveReceiver;
}

int RFConfig::getRssiThreshold() const {
  return rssiThreshold;
}

void RFConfig::setRssiThreshold(int newRssiThreshold) {
  rssiThreshold = newRssiThreshold;
}

int RFConfig::getNewOokThreshold() const {
  return newOokThreshold;
}

void RFConfig::setNewOokThreshold(int newOokThreshold) {
  newOokThreshold = newOokThreshold;
}

void RFConfig::load() {
#ifdef ESP32
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
    this->from(jo);
    Log.notice(F("RF Config loaded" CR));
  } else {
    preferences.end();
    Log.notice(F("RF Config not found using default" CR));
    rfHandler->enableActiveReceiver();
  }
#else
  rfHandler->enableActiveReceiver();
#endif
}

void RFConfig::from(JsonObject& RFdata) {
  bool success = false;
  if (RFdata.containsKey("frequency") && rfHandler->validFrequency(RFdata["frequency"])) {
    extractAndUpdate(RFdata, "frequency", frequency);
    Log.notice(F("RF Receive mhz: %F" CR), frequency);
    success = true;
  }
  if (RFdata.containsKey("active")) {
    Log.notice(F("RF receiver active: %d" CR), activeReceiver);
    extractAndUpdate(RFdata, "active", activeReceiver);
    success = true;
  }
#ifdef ZgatewayRTL_433
  if (RFdata.containsKey("rssithreshold")) {
    Log.notice(F("RTL_433 RSSI Threshold : %d " CR), RFConfig.rssiThreshold);
    Config_update(RFdata, "rssithreshold", RFConfig.rssiThreshold);
    rtl_433.setRSSIThreshold(RFConfig.rssiThreshold);
    success = true;
  }
#  if defined(RF_SX1276) || defined(RF_SX1278)
  if (RFdata.containsKey("ookthreshold")) {
    Config_update(RFdata, "ookthreshold", RFConfig.newOokThreshold);
    Log.notice(F("RTL_433 ookThreshold %d" CR), RFConfig.newOokThreshold);
    rtl_433.setOOKThreshold(RFConfig.newOokThreshold);
    success = true;
  }
#  endif
  if (RFdata.containsKey("status")) {
    Log.notice(F("RF get status:" CR));
    rtl_433.getStatus();
    success = true;
  }
  if (!success) {
    Log.error(F("MQTTtoRF Fail json" CR));
  }
#endif
  rfHandler->disableCurrentReceiver();
  rfHandler->enableActiveReceiver();
#ifdef ESP32
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
    jo["frequency"] = frequency;
    jo["active"] = activeReceiver;
// Don't save those for now, need to be tested
#  ifdef ZgatewayRTL_433
//jo["rssithreshold"] = RFConfig.rssiThreshold;
//jo["ookthreshold"] = RFConfig.newOokThreshold;
#  endif
    // Save config into NVS (non-volatile storage)
    String conf = "";
    serializeJson(jsonBuffer, conf);
    preferences.begin(Gateway_Short_Name, false);
    int result = preferences.putString("RFConfig", conf);
    preferences.end();
    Log.notice(F("RF Config_save: %s, result: %d" CR), conf.c_str(), result);
  }
#endif
}

template <typename T>
void RFConfig::extractAndUpdate(JsonObject& data, const char* key, T& var) {
  if (data.containsKey(key)) {
    if (var != data[key].as<T>()) {
      var = data[key].as<T>();
      Log.notice(F("Config %s changed to: %T" CR), key, data[key].as<T>());
    } else {
      Log.notice(F("Config %s unchanged, currently: %T" CR), key, data[key].as<T>());
    }
  }
}