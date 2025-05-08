/*
  OpenMQTTGateway - Home Assistant Entity Base Implementation
*/

#include "HassEntity.h"

#include "../core/HassConstants.h"
#include "../core/HassLogging.h"
#include "../core/HassTemplates.h"
#include "../core/HassValidators.h"

namespace omg {
namespace hass {

// EntityConfig methods
bool HassEntity::EntityConfig::isValid() const {
  return !componentType.empty() && !name.empty() && !uniqueId.empty();
}

HassEntity::EntityConfig HassEntity::EntityConfig::createSensor(const std::string& name,
                                                                const std::string& uniqueId,
                                                                const std::string& deviceClass,
                                                                const std::string& unit) {
  EntityConfig config;
  config.componentType = HASS_TYPE_SENSOR;
  config.name = name;
  config.uniqueId = uniqueId;
  config.deviceClass = deviceClass;
  config.unitOfMeasurement = unit;
  config.stateClass = unit.empty() ? "" : stateClassMeasurement;
  return config;
}

HassEntity::EntityConfig HassEntity::EntityConfig::createSwitch(const std::string& name,
                                                                const std::string& uniqueId) {
  EntityConfig config;
  config.componentType = HASS_TYPE_SWITCH;
  config.name = name;
  config.uniqueId = uniqueId;
  return config;
}

HassEntity::EntityConfig HassEntity::EntityConfig::createButton(const std::string& name,
                                                                const std::string& uniqueId) {
  EntityConfig config;
  config.componentType = HASS_TYPE_BUTTON;
  config.name = name;
  config.uniqueId = uniqueId;
  return config;
}

// HassEntity methods
HassEntity::HassEntity(const EntityConfig& config, std::shared_ptr<HassDevice> device)
    : config_(config), device_(device) {
  validateConfig();
}

bool HassEntity::publish(const HassTopicBuilder& topicBuilder, IMqttPublisher& publisher) const {
  try {
    auto doc = createDiscoveryMessage(topicBuilder, publisher);
    JsonObject json = doc.as<JsonObject>();

    // Set the topic for publishing
    std::string topic = getDiscoveryTopic(topicBuilder);
    json["topic"] = topic;
    json["retain"] = true;

    THEENGS_LOG_TRACE(F("Publishing HA Discovery: %s" CR), topic.c_str());

    return publisher.publishJson(json);
  } catch (const std::exception& e) {
    THEENGS_LOG_ERROR(F("Failed to publish entity %s: %s" CR),
                      config_.uniqueId.c_str(), e.what());
    return false;
  }
}

bool HassEntity::erase(const HassTopicBuilder& topicBuilder, IMqttPublisher& publisher) const {
  try {
    std::string topic = getDiscoveryTopic(topicBuilder);
    THEENGS_LOG_TRACE(F("Erasing HA entity: %s" CR), topic.c_str());

    // Publish empty payload to remove entity
    return publisher.publishMessage(topic, "", true);
  } catch (const std::exception& e) {
    THEENGS_LOG_ERROR(F("Failed to erase entity %s: %s" CR),
                      config_.uniqueId.c_str(), e.what());
    return false;
  }
}

bool HassEntity::updateConfig(const EntityConfig& config) {
  if (!config.isValid()) {
    return false;
  }

  config_ = config;
  validateConfig();
  return true;
}

std::string HassEntity::getDiscoveryTopic(const HassTopicBuilder& topicBuilder) const {
  return topicBuilder.buildDiscoveryTopic(config_.componentType.c_str(),
                                          config_.uniqueId.c_str());
}

void HassEntity::validateConfig() const {
  if (!config_.isValid()) {
    THEENGS_LOG_ERROR(F("Invalid entity config: missing required fields" CR));
    throw std::invalid_argument("Invalid entity configuration");
  }

  // Validate device class if provided
  if (!config_.deviceClass.empty() &&
      !HassValidators::isValidDeviceClass(config_.deviceClass.c_str())) {
    THEENGS_LOG_WARNING(F("Unknown device class: %s" CR), config_.deviceClass.c_str());
  }

  // Validate unit if provided
  if (!config_.unitOfMeasurement.empty() &&
      !HassValidators::isValidUnit(config_.unitOfMeasurement.c_str())) {
    THEENGS_LOG_WARNING(F("Unknown unit: %s" CR), config_.unitOfMeasurement.c_str());
  }
}

void HassEntity::addCommonFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const {
  // Basic entity information
  json["name"] = config_.name;
  json["uniq_id"] = config_.uniqueId;

  // Device class (validated)
  if (!config_.deviceClass.empty() &&
      HassValidators::isValidDeviceClass(config_.deviceClass.c_str())) {
    json["dev_cla"] = config_.deviceClass;
  }

  // Unit of measurement (validated)
  if (!config_.unitOfMeasurement.empty() &&
      HassValidators::isValidUnit(config_.unitOfMeasurement.c_str())) {
    json["unit_of_meas"] = config_.unitOfMeasurement;
  }

  // Value template: prefer explicit config value, otherwise infer from unit using common templates
  if (!config_.valueTemplate.empty()) {
    json["val_tpl"] = config_.valueTemplate;
  } else if (!config_.unitOfMeasurement.empty()) {
    // Provide sensible defaults for common units
    if (config_.unitOfMeasurement == HASS_UNIT_CELSIUS) {
      json["val_tpl"] = jsonTempc;
    } else if (config_.unitOfMeasurement == HASS_UNIT_LX) {
      json["val_tpl"] = jsonLux;
    } else if (config_.unitOfMeasurement == HASS_UNIT_HPA) {
      json["val_tpl"] = jsonPa;
    } else if (config_.unitOfMeasurement == HASS_UNIT_PERCENT) {
      json["val_tpl"] = jsonHum;
    }
  }

  // State class
  if (!config_.stateClass.empty()) {
    json["stat_cla"] = config_.stateClass;
  }

  // Entity category (diagnostic)
  if (config_.isDiagnostic) {
    json["ent_cat"] = "diagnostic";
  }

  // Off delay
  if (config_.offDelay > 0) {
    json["off_dly"] = config_.offDelay;
  }

  // Retain command
  if (config_.retain) {
    json["retain"] = config_.retain;
  }

  // State topic
  if (!config_.stateTopic.empty()) {
    std::string stateTopic = topicBuilder.buildStateTopic(config_.stateTopic.c_str(),
                                                          device_->isGateway());
    json["stat_t"] = stateTopic;
  }

  // Command topic
  if (!config_.commandTopic.empty()) {
    std::string commandTopic = topicBuilder.buildCommandTopic(config_.commandTopic.c_str());
    json["cmd_t"] = commandTopic;
  }

  // Availability topic (for gateway entities)
  if (device_->isGateway()) {
    std::string availTopic = config_.availabilityTopic.empty() ? "/LWT" : config_.availabilityTopic;
    std::string fullAvailTopic = topicBuilder.buildAvailabilityTopic(availTopic.c_str(), true);
    if (!fullAvailTopic.empty()) {
      json["avty_t"] = fullAvailTopic;
      json["pl_avail"] = "online";
      json["pl_not_avail"] = "offline";
    }
  }
}

void HassEntity::addDeviceInfo(JsonObject& json) const {
  if (!device_) {
    return;
  }

  StaticJsonDocument<JSON_MSG_BUFFER> deviceBuffer;
  JsonObject deviceJson = deviceBuffer.to<JsonObject>();
  device_->toJson(deviceJson);
  json["dev"] = deviceJson;
}

StaticJsonDocument<JSON_MSG_BUFFER> HassEntity::createDiscoveryMessage(const HassTopicBuilder& topicBuilder, IMqttPublisher& publisher) const {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject json = doc.to<JsonObject>();

  // Add common fields
  addCommonFields(json, topicBuilder);

  // Add entity-specific fields (implemented by derived classes)
  addSpecificFields(json, topicBuilder);

  // Add device information
  addDeviceInfo(json);

  return doc;
}

} // namespace hass
} // namespace omg
