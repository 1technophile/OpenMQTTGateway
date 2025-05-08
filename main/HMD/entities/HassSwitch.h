/*
  OpenMQTTGateway - Home Assistant Switch Entity
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include "HassEntity.h"

namespace omg {
namespace hass {

/**
 * @brief Switch entity implementation for Home Assistant
 * 
 * Handles switch entities with on/off states and commands
 */
class HassSwitch : public HassEntity {
public:
  /**
     * @brief Switch-specific configuration
     */
  struct SwitchConfig {
    std::string payloadOn = "true"; ///< Payload for ON command
    std::string payloadOff = "false"; ///< Payload for OFF command
    std::string stateOn = "true"; ///< State value for ON
    std::string stateOff = "false"; ///< State value for OFF
    std::string commandTemplate; ///< Command template (optional)
    bool optimistic = false; ///< Whether switch is optimistic

    /**
         * @brief Default constructor with sensible defaults
         */
    SwitchConfig() = default;

    /**
         * @brief Creates switch config with JSON payloads
         * @param onPayload JSON payload for ON command
         * @param offPayload JSON payload for OFF command
         * @return Configured SwitchConfig
         */
    static SwitchConfig createWithJsonPayloads(const std::string& onPayload,
                                               const std::string& offPayload);
  };

  /**
     * @brief Constructor for switch entity
     * @param config Entity configuration
     * @param switchConfig Switch-specific configuration
     * @param device Associated device
     * @param mqttPublisher MQTT publisher for publishing messages
     */
  HassSwitch(const EntityConfig& config, const SwitchConfig& switchConfig,
             std::shared_ptr<HassDevice> device);

  /**
     * @brief Constructor with default switch config
     * @param config Entity configuration
     * @param device Associated device
     */
  HassSwitch(const EntityConfig& config, std::shared_ptr<HassDevice> device);

  /**
     * @brief Gets the switch configuration
     * @return Reference to switch configuration
     */
  const SwitchConfig& getSwitchConfig() const { return switchConfig_; }

  /**
     * @brief Updates switch configuration
     * @param config New switch configuration
     */
  void updateSwitchConfig(const SwitchConfig& config);

protected:
  /**
     * @brief Adds switch-specific fields to JSON
     * @param json JSON object to populate
     * @param topicBuilder Topic builder for generating topics
     */
  void addSpecificFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const override;

private:
  SwitchConfig switchConfig_; ///< Switch-specific configuration

  /**
     * @brief Validates switch configuration
     */
  void validateSwitchConfig() const;
};

} // namespace hass
} // namespace omg
