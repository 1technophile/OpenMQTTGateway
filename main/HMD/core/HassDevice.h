/*
  OpenMQTTGateway - Home Assistant Device
  
  Represents a Home Assistant device with its metadata.
  Implements Single Responsibility Principle - only manages device information.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include <ArduinoJson.h>

#include <string>

#include "../ISettingsProvider.h"

namespace omg {
namespace hass {

/**
 * @brief Represents a Home Assistant device with its metadata
 * 
 * Single Responsibility Principle: Only manages device information
 * 
 * Performance: Efficient JSON serialization without copying
 * Reliability: Validates device information on construction
 */
class HassDevice {
public:
  /**
     * @brief Device information structure
     */
  struct DeviceInfo {
    std::string name; ///< Human-readable device name
    std::string manufacturer; ///< Device manufacturer
    std::string model; ///< Device model
    std::string identifier; ///< Unique device identifier (MAC, etc.)
    std::string configUrl; ///< Configuration URL for device
    std::string swVersion; ///< Software/firmware version
    bool isGateway = false; ///< Whether this is the gateway device

    /**
         * @brief Default constructor with sensible defaults
         */
    DeviceInfo();

    /**
         * @brief Validates the device information
         * @return true if valid, false otherwise
         */
    bool isValid() const;
  };

  /**
     * @brief Constructor with device information
     * @param info Device information structure
     * @param settingsProvider Settings provider for configuration access
     */
  HassDevice(const DeviceInfo& info, const ISettingsProvider& settingsProvider);

  /**
     * @brief Serializes device information to JSON
     * @param deviceJson JSON object to populate
     */
  void toJson(JsonObject& deviceJson) const;

  /**
     * @brief Gets the device identifier
     * @return Device identifier string
     */
  const std::string& getIdentifier() const { return info_.identifier; }

  /**
     * @brief Gets the device name
     * @return Device name string
     */
  const std::string& getName() const { return info_.name; }

  /**
     * @brief Gets the device manufacturer
     * @return Device manufacturer string
     */
  const std::string& getManufacturer() const { return info_.manufacturer; }

  /**
     * @brief Gets the device model
     * @return Device model string
     */
  const std::string& getModel() const { return info_.model; }

  /**
     * @brief Checks if this is a gateway device
     * @return true if gateway device, false otherwise
     */
  bool isGateway() const { return info_.isGateway; }

  /**
     * @brief Updates device information
     * @param info New device information
     * @return true if update successful, false otherwise
     */
  bool updateInfo(const DeviceInfo& info);

  /**
     * @brief Gets the complete device information
     * @return Reference to device information structure
     */
  const DeviceInfo& getInfo() const { return info_; }

  /**
     * @brief Creates a gateway device with current system information
     * @param settingsProvider Settings provider for configuration access
     * @return HassDevice instance configured as gateway
     */
  static HassDevice createGatewayDevice(const ISettingsProvider& settingsProvider);

  /**
     * @brief Creates an external device
     * @param name Device name
     * @param manufacturer Device manufacturer
     * @param model Device model
     * @param identifier Device identifier
     * @param settingsProvider Settings provider for configuration access
     * @return HassDevice instance configured as external device
     */
  static HassDevice createExternalDevice(const std::string& name,
                                         const std::string& manufacturer,
                                         const std::string& model,
                                         const std::string& identifier,
                                         const ISettingsProvider& settingsProvider);

private:
  DeviceInfo info_; ///< Device information
  const ISettingsProvider& settingsProvider_; ///< Settings provider for configuration access

  /**
     * @brief Adds gateway-specific information to JSON
     * @param deviceJson JSON object to populate
     */
  void addGatewayInfo(JsonObject& deviceJson) const;

  /**
     * @brief Adds external device information to JSON
     * @param deviceJson JSON object to populate
     */
  void addExternalDeviceInfo(JsonObject& deviceJson) const;

  /**
     * @brief Validates and sanitizes device information
     */
  void validateAndSanitize();
};

} // namespace hass
} // namespace omg
