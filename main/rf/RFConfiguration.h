#ifndef RFCONFIG_H
#define RFCONFIG_H
#pragma once

#include <TheengsCommon.h>
#include <rf/RFReceiver.h>

class RFConfiguration {
public:
  // Constructor
  RFConfiguration(RFReceiver& receiver);
  ~RFConfiguration();

  // Getters and Setters
  float getFrequency() const;
  void setFrequency(float freq);

  int getRssiThreshold() const;
  void setRssiThreshold(int threshold);

  int getNewOokThreshold() const;
  void setNewOokThreshold(int threshold);

  int getActiveReceiver() const;
  void setActiveReceiver(int receiver);

  /**
   * Initializes the  structure with default values.
   * 
   * @note This function should be called during the initialization phase 
   *       to ensure the  structure is properly configured.
   */
  void reInit();

  /**
   * Erases the RF configuration from non-volatile storage (NVS).
   * 
   * @note This function is only available on ESP32 platforms.
   */
  void eraseStorage();

  /**
   * Saves the RF configuration to non-volatile storage (NVS).
   *
   * @note This function is only available on ESP32 platforms.
   */
  void saveOnStorage();

  /**
   * Loads the RF configuration from persistent storage and applies it.
   *
   * @param reinitReceiver If true (default), disables and re-enables the receiver.
   *                       If false, only loads the configuration without reinitialization.
   *
   * @note This function has specific behavior for ESP32 platforms. On ESP32,
   *       it uses the Preferences library to access stored configuration data.
   *       For other platforms, it directly enables the active receiver.
   */
  void loadFromStorage(bool reinitReceiver = true);

  /**
   * Loads the RF configuration from a JSON object and applies it.
   *
   * @param RFdata A reference to a JsonObject containing the RF configuration data.
   *
   * The following keys are supported in the JSON object:
   * - "erase": If true, erases the RF configuration from storage.
   * - "save": If true, saves the current RF configuration to storage.
   *
   * Logs messages to indicate the success or failure of each operation.
   */
  void loadFromMessage(JsonObject& RFdata);

  /**
   * Updates the RF configuration from a JSON object.
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
   */
  void fromJson(JsonObject& RFdata);

  /**
   * Serializes the RF configuration to a JSON object.
   *
   * @param RFdata A reference to a JsonObject where the RF configuration will be serialized.
   */
  void toJson(JsonObject& RFdata);

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
  bool validFrequency(float mhz);

private:
  // Reference to the RFReceiver object
  RFReceiver& iRFReceiver;
  float frequency;
  int rssiThreshold;
  int newOokThreshold;
  int activeReceiver;
};

#endif // RFCONFIG_H