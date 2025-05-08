/*
  OpenMQTTGateway - Home Assistant Discovery Manager Implementation
*/

#include "HassDiscoveryManager.h"

#include "../core/HassLogging.h"
#include "../entities/HassButton.h"
#include "../entities/HassSensor.h"
#include "../entities/HassSwitch.h"

namespace omg {
namespace hass {

HassDiscoveryManager::HassDiscoveryManager(ISettingsProvider& settingsProvider,
                                           IMqttPublisher& mqttPublisher)
    : settingsProvider_(settingsProvider),
      mqttPublisher_(mqttPublisher),
      topicBuilder_(settingsProvider) {
  initializeGatewayDevice();
}

void HassDiscoveryManager::initializeGatewayDevice() {
  gatewayDevice_ = std::make_shared<HassDevice>(HassDevice::createGatewayDevice(settingsProvider_));
  THEENGS_LOG_NOTICE(F("Gateway device initialized: %s" CR),
                     gatewayDevice_->getName().c_str());
}

std::shared_ptr<HassDevice> HassDiscoveryManager::getGatewayDevice() {
  if (!gatewayDevice_) {
    initializeGatewayDevice();
  }
  return gatewayDevice_;
}

std::shared_ptr<HassDevice> HassDiscoveryManager::createExternalDevice(
    const char* name, const char* manufacturer, const char* model, const char* identifier) {
  std::string safeName = name ? name : "Unknown Device";
  std::string safeManufacturer = manufacturer ? manufacturer : "Unknown";
  std::string safeModel = model ? model : "Unknown";
  std::string safeIdentifier = identifier ? identifier : "";

  auto device = std::make_shared<HassDevice>(
      HassDevice::createExternalDevice(safeName, safeManufacturer, safeModel, safeIdentifier, settingsProvider_));

  THEENGS_LOG_VERBOSE(F("External device created: %s (%s)" CR),
                      safeName.c_str(), safeIdentifier.c_str());

  return device;
}

bool HassDiscoveryManager::publishEntity(std::unique_ptr<HassEntity> entity) {
  if (!entity || !validateEntity(entity.get())) {
    THEENGS_LOG_ERROR(F("Invalid entity, cannot publish" CR));
    return false;
  }

  bool success = entity->publish(topicBuilder_, mqttPublisher_);
  if (success) {
    THEENGS_LOG_VERBOSE(F("Entity published: %s" CR),
                        entity->getConfig().uniqueId.c_str());
    entities_.push_back(std::move(entity));
  } else {
    THEENGS_LOG_ERROR(F("Failed to publish entity: %s" CR),
                      entity->getConfig().uniqueId.c_str());
  }

  return success;
}

void HassDiscoveryManager::publishEntityFromArray(const char* entityArray[][13], int count,
                                                  std::shared_ptr<HassDevice> device) {
  if (!entityArray || count <= 0 || !device) {
    THEENGS_LOG_ERROR(F("Invalid parameters for publishEntityFromArray" CR));
    return;
  }

  THEENGS_LOG_VERBOSE(F("Publishing %d entities from array" CR), count);

  for (int i = 0; i < count; i++) {
    auto entity = createEntityFromArray(entityArray[i], device);
    if (entity) {
      publishEntity(std::move(entity));
    }
  }
}

std::unique_ptr<HassEntity> HassDiscoveryManager::createEntityFromArray(const char* row[13],
                                                                        std::shared_ptr<HassDevice> device) {
  if (!row || !row[0] || !row[1] || !device) {
    return nullptr;
  }

  // Parse array format:
  // [0] = component type, [1] = name, [2] = unique id suffix, [3] = device class,
  // [4] = value template, [5] = payload on, [6] = payload off, [7] = unit,
  // [8] = state class, [9] = state_off, [10] = state_on, [11] = state topic, [12] = command topic

  HassEntity::EntityConfig config;
  config.componentType = row[0];
  config.name = row[1];
  config.uniqueId = row[2] ? mqttPublisher_.getUId(row[2], "").c_str() : mqttPublisher_.getUId(row[1], "").c_str();
  config.deviceClass = row[3] ? row[3] : "";
  config.valueTemplate = row[4] ? row[4] : "";
  config.unitOfMeasurement = row[7] ? row[7] : "";
  config.stateClass = row[8] ? row[8] : "";
  config.stateTopic = row[11] ? row[11] : "";
  config.commandTopic = row[12] ? row[12] : "";

  std::string componentType = config.componentType;

  if (componentType == "sensor" || componentType == "binary_sensor") {
    return std::make_unique<HassSensor>(config, device);
  } else if (componentType == "switch") {
    auto switchConfig = HassSwitch::SwitchConfig::createWithJsonPayloads(
        row[5] ? row[5] : "true",
        row[6] ? row[6] : "false");
    switchConfig.stateOn = row[10] ? row[10] : "true";
    switchConfig.stateOff = row[9] ? row[9] : "false";
    return std::make_unique<HassSwitch>(config, switchConfig, device);
  } else if (componentType == "button") {
    auto buttonConfig = HassButton::ButtonConfig::createGeneric(
        row[5] ? row[5] : "{\"cmd\":\"press\"}");
    return std::make_unique<HassButton>(config, buttonConfig, device);
  }

  THEENGS_LOG_WARNING(F("Unsupported entity type: %s" CR), componentType.c_str());
  return nullptr;
}

void HassDiscoveryManager::eraseEntity(const char* componentType, const char* uniqueId) {
  if (!componentType || !uniqueId) {
    return;
  }

  std::string topic = topicBuilder_.buildDiscoveryTopic(componentType, uniqueId);
  THEENGS_LOG_VERBOSE(F("Erasing entity: %s" CR), topic.c_str());

  // Use injected MQTT publisher interface instead of direct legacy call
  mqttPublisher_.publishMessage(topic, "", true);
}

void HassDiscoveryManager::clearEntities() {
  THEENGS_LOG_TRACE(F("Clearing %d entities" CR), entities_.size());
  entities_.clear();
}

void HassDiscoveryManager::republishAllEntities() {
  THEENGS_LOG_TRACE(F("Republishing %d entities" CR), entities_.size());

  for (const auto& entity : entities_) {
    if (entity) {
      entity->publish(topicBuilder_, mqttPublisher_);
    }
  }
}

bool HassDiscoveryManager::validateEntity(const HassEntity* entity) const {
  if (!entity) {
    return false;
  }

  const auto& config = entity->getConfig();
  if (config.componentType.empty() || config.name.empty() || config.uniqueId.empty()) {
    THEENGS_LOG_ERROR(F("Entity validation failed: missing required fields" CR));
    return false;
  }

  return true;
}

} // namespace hass
} // namespace omg
