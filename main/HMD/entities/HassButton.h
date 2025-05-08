/*
  OpenMQTTGateway - Home Assistant Button Entity
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include "HassEntity.h"

namespace omg {
namespace hass {

/**
 * @brief Button entity implementation for Home Assistant
 * 
 * Handles button entities for triggering actions (restart, erase, etc.)
 */
class HassButton : public HassEntity {
public:
  /**
     * @brief Button-specific configuration
     */
  struct ButtonConfig {
    std::string payloadPress; ///< Payload for button press
    std::string deviceClass; ///< Device class (restart, update, etc.)

    /**
         * @brief Default constructor
         */
    ButtonConfig() = default;

    /**
         * @brief Creates button config for restart action
         * @param payload JSON payload for restart command
         * @return Configured ButtonConfig
         */
    static ButtonConfig createRestart(const std::string& payload = "{\"cmd\":\"restart\"}");

    /**
         * @brief Creates button config for update action
         * @param payload JSON payload for update command
         * @return Configured ButtonConfig
         */
    static ButtonConfig createUpdate(const std::string& payload);

    /**
         * @brief Creates button config for generic action
         * @param payload JSON payload for action
         * @param deviceClass Device class for the button
         * @return Configured ButtonConfig
         */
    static ButtonConfig createGeneric(const std::string& payload,
                                      const std::string& deviceClass = "");
  };

  /**
     * @brief Constructor for button entity
     * @param config Entity configuration
     * @param buttonConfig Button-specific configuration
     * @param device Associated device
     * @param mqttPublisher MQTT publisher for publishing messages
     */
  HassButton(const EntityConfig& config, const ButtonConfig& buttonConfig,
             std::shared_ptr<HassDevice> device);

  /**
     * @brief Constructor with default button config
     * @param config Entity configuration
     * @param device Associated device
     */
  HassButton(const EntityConfig& config, std::shared_ptr<HassDevice> device);

  /**
     * @brief Gets the button configuration
     * @return Reference to button configuration
     */
  const ButtonConfig& getButtonConfig() const { return buttonConfig_; }

  /**
     * @brief Updates button configuration
     * @param config New button configuration
     */
  void updateButtonConfig(const ButtonConfig& config);

protected:
  /**
     * @brief Adds button-specific fields to JSON
     * @param json JSON object to populate
     * @param topicBuilder Topic builder for generating topics
     */
  void addSpecificFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const override;

private:
  ButtonConfig buttonConfig_; ///< Button-specific configuration

  /**
     * @brief Validates button configuration
     */
  void validateButtonConfig() const;
};

} // namespace hass
} // namespace omg
