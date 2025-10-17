#include "RFConfiguration.h"

#include <config_RF.h>

#ifdef ZgatewayRTL_433
#  include <rtl_433_ESP.h>
extern rtl_433_ESP rtl_433;
#endif

// Constructor
RFConfiguration::RFConfiguration(RFReceiver& receiver) : iRFReceiver(receiver) {
  reInit();
}

// Destructor
RFConfiguration::~RFConfiguration() {
}

// Getters and Setters
float RFConfiguration::getFrequency() const {
  return frequency;
}

void RFConfiguration::setFrequency(float freq) {
  frequency = freq;
}

int RFConfiguration::getRssiThreshold() const {
  return rssiThreshold;
}

void RFConfiguration::setRssiThreshold(int threshold) {
  rssiThreshold = threshold;
}

int RFConfiguration::getNewOokThreshold() const {
  return newOokThreshold;
}

void RFConfiguration::setNewOokThreshold(int threshold) {
  newOokThreshold = threshold;
}

int RFConfiguration::getActiveReceiver() const {
  return activeReceiver;
}

void RFConfiguration::setActiveReceiver(int receiver) {
  activeReceiver = receiver;
}

/**
 * @brief Initializes the RFConfiguration  with default values.
 * 
 * This function sets up the RFConfiguration by assigning default values 
 * to its members, including frequency, active receiver, RSSI threshold, 
 * and new OOK threshold. It also clears and shrinks the whiteList and 
 * blackList containers to ensure they are empty and optimized for memory usage.
 * 
 * @note This function should be called during the initialization phase 
 *       to ensure the RFConfiguration is properly configured.
 */
void RFConfiguration::reInit() {
  frequency = RF_FREQUENCY;
  activeReceiver = ACTIVE_RECEIVER;
  rssiThreshold = 0;
  newOokThreshold = 0;
}

/**
 * @brief Erases the RF configuration from non-volatile storage (NVS).
 *
 * This function removes the RF configuration stored in NVS. It checks if
 * the configuration exists and, if so, removes it. If the configuration
 * is not found, a notice is logged.
 *
 * @note This function is only available on ESP32 platforms.
 */
void RFConfiguration::eraseStorage() {
#ifdef ESP32
  // Erase config from NVS (non-volatile storage)
  preferences.begin(Gateway_Short_Name, false);
  if (preferences.isKey("RFConfig")) {
    int result = preferences.remove("RFConfig");
    Log.notice(F("RF config erase result: %d" CR), result);
  } else {
    Log.notice(F("RF config not found" CR));
  }
  preferences.end();
#else
  Log.warning(F("RF Config Erase not support with this board" CR));
#endif
}

/**
   * @brief Saves the RF configuration to non-volatile storage (NVS).
   *
   * This function serializes the RF configuration data into a JSON object
   * and saves it to NVS. The saved configuration includes frequency, active
   * receiver, and other relevant parameters.
   *
   * @note This function is only available on ESP32 platforms.
   * @note Ensure that the `JSON_MSG_BUFFER` is large enough to hold the
   *       serialized configuration data to avoid deserialization errors.
   */
void RFConfiguration::saveOnStorage() {
#ifdef ESP32
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jo = jsonBuffer.to<JsonObject>();
  toJson(jo);
#  ifdef ZgatewayRTL_433
  // FROM ORIGINAL CONFIGURATION:
  // > Don't save those for now, need to be tested
  jo.remove("rssithreshold");
  jo.remove("ookthreshold");
#  endif
  // Save config into NVS (non-volatile storage)
  String conf = "";
  serializeJson(jsonBuffer, conf);
  preferences.begin(Gateway_Short_Name, false);
  int result = preferences.putString("RFConfig", conf);
  preferences.end();
  Log.notice(F("RF Config_save: %s, result: %d" CR), conf.c_str(), result);
#else
  Log.warning(F("RF Config_save not support with this board" CR));
#endif
}

/**
 * @brief Loads the RF configuration from persistent storage and applies it.
 *
 * This function retrieves the RF configuration stored in non-volatile
 * storage (NVS) and applies it to the RF receiver. If the configuration
 * is not found, a notice is logged, and the RF receiver is enabled with
 * default settings.
 *
 * @note This function has specific behavior for ESP32 platforms. On ESP32,
 *       it uses the Preferences library to access stored configuration data.
 *       For other platforms, it directly enables the active receiver.
 */
void RFConfiguration::loadFromStorage() {
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
    fromJson(jo);
    Log.notice(F("RF Config loaded" CR));
  } else {
    preferences.end();
    Log.notice(F("RF Config not found using default" CR));
    iRFReceiver.enable();
  }
#else
  iRFReceiver.enable();
#endif
}

/**
 * @brief Loads the RF configuration from a JSON object and applies it.
 *
 * This function takes a JSON object containing RF configuration data and
 * applies it to the RF receiver. It also handles the erasure and saving of
 * the configuration based on the provided JSON data.
 * 
 * Configuration modifications priorities:
 * - First `init=true` and `load=true` commands are executed (if both are present, INIT prevails on LOAD)
 * - Then parameters included in json are taken in account
 * - Finally `erase=true` and `save=true` commands are executed (if both are present, ERASE prevails on SAVE)* 
 *
 * @param RFdata A reference to a JsonObject containing the RF configuration data.
 *
 * The following keys are supported in the JSON object:
 * - "init": If true, restores the default RF configuration.
 * - "load": If true, loads the saved RF configuration from storage.
 * - "erase": If true, erases the RF configuration from storage.
 * - "save": If true, saves the current RF configuration to storage.
 *
 * Logs messages to indicate the success or failure of each operation.
 */
void RFConfiguration::loadFromMessage(JsonObject& RFdata) {
  if (RFdata.containsKey("init") && RFdata["init"].as<bool>()) {
    // Restore the default (initial) configuration
    reInit();
  } else if (RFdata.containsKey("load") && RFdata["load"].as<bool>()) {
    // Load the saved configuration, if not initialised
    loadFromStorage();
  }

  fromJson(RFdata);

  iRFReceiver.disable();
  iRFReceiver.enable();

  if (RFdata.containsKey("erase") && RFdata["erase"].as<bool>()) {
    eraseStorage();
    Log.notice(F("RF Config erased" CR));
  } else if (RFdata.containsKey("save") && RFdata["save"].as<bool>()) {
    saveOnStorage();
    Log.notice(F("RF Config saved" CR));
  }
}

/**
 * @brief Updates the RF configuration from a JSON object.
 *
 * This function parses the provided JSON object and updates the RF configuration
 * based on the keys and values present in the object. It supports updating
 * white-list, black-list, frequency, active receiver status, and other RF-related
 * parameters depending on the defined preprocessor directives.
 *
 * @param RFdata A reference to a JsonObject containing the RF configuration data.
 *
 * The following keys are supported in the JSON object:
 * - "white-list": Updates the RF white-list.
 * - "black-list": Updates the RF black-list.
 * - "frequency": Updates the RF frequency if the value is valid.
 * - "active": Updates the active receiver status.
 *
 * Additional keys supported when ZgatewayRTL_433 is defined:
 * - "rssithreshold": Updates the RSSI threshold for RTL_433.
 * - "ookthreshold": Updates the OOK threshold for RTL_433 (requires RF_SX1276 or RF_SX1278).
 * - "status": Retrieves the current status of the RF configuration.
 *
 * Logs messages to indicate the success or failure of each update operation.
 * If no valid keys are found in the JSON object, an error message is logged.
 */
void RFConfiguration::fromJson(JsonObject& RFdata) {
  bool success = false;

  if (RFdata.containsKey("frequency") && validFrequency(RFdata["frequency"])) {
    Config_update(RFdata, "frequency", frequency);
    Log.notice(F("RF Receive mhz: %F" CR), frequency);
    success = true;
  }
  if (RFdata.containsKey("active")) {
    Config_update(RFdata, "active", activeReceiver);
    Log.notice(F("RF receiver active: %d" CR), activeReceiver);
    success = true;
  }
#ifdef ZgatewayRTL_433
  if (RFdata.containsKey("rssithreshold")) {
    Log.notice(F("RTL_433 RSSI Threshold : %d " CR), rssiThreshold);
    Config_update(RFdata, "rssithreshold", rssiThreshold);
    rtl_433.setRSSIThreshold(rssiThreshold);
    success = true;
  }
#  if defined(RF_SX1276) || defined(RF_SX1278)
  if (RFdata.containsKey("ookthreshold")) {
    Config_update(RFdata, "ookthreshold", newOokThreshold);
    Log.notice(F("RTL_433 ookThreshold %d" CR), newOokThreshold);
    rtl_433.setOOKThreshold(newOokThreshold);
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
}

/**
 * @brief Serializes the RFConfiguration object into a JSON object.
 * 
 * This method populates the provided JSON object with the configuration
 * details of the RF module, including frequency, RSSI threshold, OOK threshold,
 * active receiver status, and ignore list settings. Additionally, it includes
 * the white-list and black-list vectors as nested JSON arrays.
 * 
 * @param RFdata A reference to a JsonObject where the RF configuration data 
 *               will be serialized.
 * 
 * JSON Structure:
 * {
 *   "frequency": <float>,          // Frequency value
 *   "rssithreshold": <float>,      // RSSI threshold value
 *   "ookthreshold": <float>,       // OOK threshold value
 *   "active": <bool>,              // Active receiver status
 *   "ignoreWhitelist": <bool>,     // Ignore white-list flag
 *   "ignoreBlacklist": <bool>,     // Ignore black-list flag
 *   "white-list": [<int>, ...],    // Array of white-list values
 *   "black-list": [<int>, ...]     // Array of black-list values
 * }
 */
void RFConfiguration::toJson(JsonObject& RFdata) {
  RFdata["frequency"] = frequency;
  RFdata["rssithreshold"] = rssiThreshold;
  RFdata["ookthreshold"] = newOokThreshold;
  RFdata["active"] = activeReceiver;

  // Add white-list vector to the JSON object
  JsonArray whiteListArray = RFdata.createNestedArray("white-list");
  // Add black-list vector to the JSON object
  JsonArray blackListArray = RFdata.createNestedArray("black-list");
}

/**
 * @brief Validates if the given frequency is within the acceptable ranges for the CC1101 module.
 *
 * The CC1101 module supports the following frequency ranges:
 * - 300 MHz to 348 MHz
 * - 387 MHz to 464 MHz
 * - 779 MHz to 928 MHz
 *
 * @param mhz The frequency in MHz to validate.
 * @return true if the frequency is within one of the valid ranges, false otherwise.
 */
bool RFConfiguration::validFrequency(float mhz) {
  //  CC1101 valid frequencies 300-348 MHZ, 387-464MHZ and 779-928MHZ.
  if (mhz >= 300 && mhz <= 348)
    return true;
  if (mhz >= 387 && mhz <= 464)
    return true;
  if (mhz >= 779 && mhz <= 928)
    return true;
  return false;
}
