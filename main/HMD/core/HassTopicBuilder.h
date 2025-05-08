/*
  OpenMQTTGateway - Home Assistant Topic Builder
  
  Constructs MQTT topics following Home Assistant discovery format.
  Implements Single Responsibility Principle - only handles topic construction.
  
  Copyright: (c) OpenMQTTGateway Contributors*/
#pragma once

#include <string>

#include "../ISettingsProvider.h"

namespace omg {
namespace hass {

/**
 * @brief Constructs MQTT topics following Home Assistant discovery format
 * 
 * Single Responsibility Principle: Only handles topic construction logic
 * 
 * Performance: Efficient string building without unnecessary allocations
 * Reliability: Handles null/empty inputs gracefully
 */
class HassTopicBuilder {
public:
  /**
     * @brief Constructor with settings provider
     * @param settingsProvider Settings provider for configuration access
     */
  explicit HassTopicBuilder(const ISettingsProvider& settingsProvider);

  /**
     * @brief Builds Home Assistant discovery topic
     * Format: <prefix>/<component>/<unique_id>/config
     * @param component Component type (sensor, switch, etc.)
     * @param uniqueId Unique identifier for the entity
     * @return Complete discovery topic string
     */
  std::string buildDiscoveryTopic(const char* component, const char* uniqueId) const;

  /**
     * @brief Builds state topic for entity updates
     * @param topic Base topic path
     * @param gatewayEntity Whether this is a gateway entity or external device
     * @return Complete state topic string
     */
  std::string buildStateTopic(const char* topic, bool gatewayEntity) const;

  /**
     * @brief Builds availability topic for entity status
     * @param topic Base topic path
     * @param gatewayEntity Whether this is a gateway entity or external device
     * @return Complete availability topic string
     */
  std::string buildAvailabilityTopic(const char* topic, bool gatewayEntity) const;

  /**
     * @brief Builds command topic for entity control
     * @param topic Base topic path
     * @return Complete command topic string
     */
  std::string buildCommandTopic(const char* topic) const;

  /**
     * @brief Gets the discovery prefix
     * @return Discovery prefix string
     */
  std::string getDiscoveryPrefix() const { return settingsProvider_.getDiscoveryPrefix(); }

  /**
     * @brief Validates topic components for safety
     * @param component Component to validate
     * @return true if valid, false otherwise
     */
  static bool isValidTopicComponent(const char* component);

  /**
     * @brief Sanitizes topic component for MQTT compatibility
     * @param component Component to sanitize
     * @return Sanitized component string
     */
  static std::string sanitizeTopicComponent(const char* component);

private:
  const ISettingsProvider& settingsProvider_; ///< Settings provider for configuration access

  /**
     * @brief Builds base topic part based on entity type
     * @param gatewayEntity Whether this is a gateway entity
     * @return Base topic string
     */
  std::string buildBaseTopic(bool gatewayEntity) const;
};

} // namespace hass
} // namespace omg
