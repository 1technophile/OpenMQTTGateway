/*
  OpenMQTTGateway - Home Assistant Sensor Entity
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include "HassEntity.h"

namespace omg {
namespace hass {

/**
 * @brief Sensor entity implementation for Home Assistant
 * 
 * Handles sensor entities like temperature, humidity, battery level, etc.
 */
class HassSensor : public HassEntity {
public:
  /**
     * @brief Constructor for sensor entity
     * @param config Entity configuration
     * @param device Associated device
     * @param mqttPublisher MQTT publisher for publishing messages
     */
  explicit HassSensor(const EntityConfig& config, std::shared_ptr<HassDevice> device);

protected:
  /**
     * @brief Adds sensor-specific fields to JSON
     * @param json JSON object to populate
     * @param topicBuilder Topic builder for generating topics
     */
  void addSpecificFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const override;
};

} // namespace hass
} // namespace omg
