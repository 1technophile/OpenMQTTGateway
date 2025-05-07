/**
 * @file RFConfig.h
 * @brief Configuration class for RF communication settings.
 */

#ifndef RF_CONFIG_H
#define RF_CONFIG_H

#include <ArduinoJson.h>
#include <main_utils.h>

#if defined(ESP32)
class Preferences;
#endif

class ZCommonRF;

/**
 * @class RFConfig
 * @brief Manages RF communication settings and preferences.
 */
class RFConfig {
public:
#if defined(ESP32)
  /**
     * @brief Constructor for ESP32 platform.
     * @param iPreferences Reference to Preferences object for storing settings.
     */
  RFConfig(Preferences& iPreferences);
#else
  /**
     * @brief Default constructor.
     */
  RFConfig();
#endif

  /**
     * @brief Loads the RF configuration settings.
     */
  void load();

  /** Reset to original state */
  void reset();

  /**
     * @brief Loads RF configuration from a JSON object.
     * @param RFdata JSON object containing RF configuration data.
     */
  void from(JsonObject& RFdata);

  /**
     * @brief Sets the RF handler.
     * @param iRFHandler Reference to ZCommonRF object.
     */
  void setRFHandler(ZCommonRF* iRFHandler);

  /**
     * @brief Gets the RF communication frequency.
     * @return Frequency in MHz.
     */
  float getFrequency() const;

  /**
     * @brief Sets the RF communication frequency.
     * @param freq Frequency in MHz.
     */
  bool setFrequency(float freq);

  /**
     * @brief Gets the RSSI threshold value.
     * @return RSSI threshold value.
     */
  int getRssiThreshold() const;

  /**
     * @brief Sets the RSSI threshold value.
     * @param threshold RSSI threshold value.
     */
  void setRssiThreshold(int threshold);

  /**
     * @brief Gets the new OOK threshold value.
     * @return New OOK threshold value.
     */
  int getNewOokThreshold() const;

  /**
     * @brief Sets the new OOK threshold value.
     * @param threshold New OOK threshold value.
     */
  void setNewOokThreshold(int threshold);

  /**
     * @brief Gets the identifier for the active receiver.
     * @return Active receiver identifier.
     */
  int getActiveReceiver() const;

  /**
     * @brief Sets the identifier for the active receiver.
     * @param receiver Active receiver identifier.
     */
  void setActiveReceiver(int receiver);

private:
#if defined(ESP32)
  Preferences& preferences; /**< Reference to Preferences object for ESP32. */
#endif

  ZCommonRF* rfHandler = nullptr; /**< Reference to RF handler object. */

  float frequency; /**< The frequency for RF communication. */

  int rssiThreshold; /**< The RSSI threshold value. */

  int newOokThreshold; /**< The new OOK threshold value. */

  int activeReceiver; /**< The identifier for the active receiver. */

  /**
 * @brief Extracts a value from a JSON object and updates the variable if the value has changed.
 *
 * This template function checks if the specified key exists in the provided JSON object.
 * If the key exists and the value associated with the key is different from the current value of the variable,
 * the variable is updated with the new value. It also logs a message indicating whether the value was changed or remained the same.
 *
 * @tparam T The type of the variable to be updated.
 * @param data The JSON object containing the data.
 * @param key The key to look for in the JSON object.
 * @param var The variable to be updated with the value from the JSON object.
 */
  template <typename T>
  void extractAndUpdate(JsonObject& data, const char* key, T& var);
};

#endif // RF_CONFIG_H
