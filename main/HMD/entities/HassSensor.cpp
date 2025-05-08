/*
  OpenMQTTGateway - Home Assistant Sensor Entity Implementation
*/

#include "HassSensor.h"

#include "../core/HassConstants.h"

namespace omg {
namespace hass {

HassSensor::HassSensor(const EntityConfig& config, std::shared_ptr<HassDevice> device)
    : HassEntity(config, device) {
  // Sensors typically use "measurement" state class if they have units
  if (config_.stateClass.empty() && !config_.unitOfMeasurement.empty()) {
    config_.stateClass = stateClassMeasurement;
  }
}

void HassSensor::addSpecificFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const {
  // Sensors don't have many specific fields beyond the common ones
  // Most sensor-specific behavior is handled by the base class

  // For binary sensors, we might want to add payload_on/payload_off
  if (config_.componentType == HASS_TYPE_BINARY_SENSOR) {
    // These could be configurable in the future
    json["pl_on"] = "true";
    json["pl_off"] = "false";
  }

  // Device tracker sensors need source_type
  if (config_.componentType == HASS_TYPE_DEVICE_TRACKER) {
    json["src_type"] = "bluetooth_le";
  }
}

} // namespace hass
} // namespace omg
