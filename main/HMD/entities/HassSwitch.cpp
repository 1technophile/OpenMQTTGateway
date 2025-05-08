/*
  OpenMQTTGateway - Home Assistant Switch Entity Implementation
*/

#include "HassSwitch.h"

#include "../core/HassConstants.h"
#include "../core/HassLogging.h"

namespace omg {
namespace hass {

// SwitchConfig methods
HassSwitch::SwitchConfig HassSwitch::SwitchConfig::createWithJsonPayloads(
    const std::string& onPayload, const std::string& offPayload) {
  SwitchConfig config;
  config.payloadOn = onPayload;
  config.payloadOff = offPayload;
  config.stateOn = "true";
  config.stateOff = "false";
  return config;
}

// HassSwitch methods
HassSwitch::HassSwitch(const EntityConfig& config, const SwitchConfig& switchConfig,
                       std::shared_ptr<HassDevice> device)
    : HassEntity(config, device), switchConfig_(switchConfig) {
  // Ensure component type is switch
  if (config_.componentType != HASS_TYPE_SWITCH) {
    config_.componentType = HASS_TYPE_SWITCH;
  }

  validateSwitchConfig();
}

HassSwitch::HassSwitch(const EntityConfig& config, std::shared_ptr<HassDevice> device)
    : HassEntity(config, device) {
  // Ensure component type is switch
  if (config_.componentType != HASS_TYPE_SWITCH) {
    config_.componentType = HASS_TYPE_SWITCH;
  }

  validateSwitchConfig();
}

void HassSwitch::updateSwitchConfig(const SwitchConfig& config) {
  switchConfig_ = config;
  validateSwitchConfig();
}

void HassSwitch::addSpecificFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const {
  // Add payload for ON command
  if (!switchConfig_.payloadOn.empty()) {
    // Check if payload is boolean-like
    if (switchConfig_.payloadOn == "true" || switchConfig_.payloadOn == "True") {
      json["pl_on"] = true;
    } else if (switchConfig_.payloadOn == "false" || switchConfig_.payloadOn == "False") {
      json["pl_on"] = false;
    } else {
      json["pl_on"] = switchConfig_.payloadOn;
    }
  }

  // Add payload for OFF command
  if (!switchConfig_.payloadOff.empty()) {
    // Check if payload is boolean-like
    if (switchConfig_.payloadOff == "true" || switchConfig_.payloadOff == "True") {
      json["pl_off"] = true;
    } else if (switchConfig_.payloadOff == "false" || switchConfig_.payloadOff == "False") {
      json["pl_off"] = false;
    } else {
      json["pl_off"] = switchConfig_.payloadOff;
    }
  }

  // Add state values
  if (!switchConfig_.stateOn.empty()) {
    if (switchConfig_.stateOn == "true") {
      json["stat_on"] = true;
    } else {
      json["stat_on"] = switchConfig_.stateOn;
    }
  }

  if (!switchConfig_.stateOff.empty()) {
    if (switchConfig_.stateOff == "false") {
      json["stat_off"] = false;
    } else {
      json["stat_off"] = switchConfig_.stateOff;
    }
  }

  // Add command template if specified
  if (!switchConfig_.commandTemplate.empty()) {
    json["cmd_tpl"] = switchConfig_.commandTemplate;
  }

  // If no state topic is defined, Home Assistant will automatically use optimistic mode
  if (config_.stateTopic.empty() || switchConfig_.optimistic) {
    json["optimistic"] = true;
  }

  // Note: optimistic mode is implicit when no state_topic is provided
  // Home Assistant will automatically use optimistic mode
}

void HassSwitch::validateSwitchConfig() const {
  // Basic validation - switches need at least ON payload
  if (switchConfig_.payloadOn.empty()) {
    THEENGS_LOG_WARNING(F("Switch %s: payloadOn is empty, using default" CR),
                        config_.uniqueId.c_str());
  }

  if (switchConfig_.payloadOff.empty()) {
    THEENGS_LOG_WARNING(F("Switch %s: payloadOff is empty, using default" CR),
                        config_.uniqueId.c_str());
  }

  // If no command topic is set and it's not optimistic, warn
  if (config_.commandTopic.empty() && !switchConfig_.optimistic) {
    THEENGS_LOG_WARNING(F("Switch %s: no command topic set, switch may not be controllable" CR),
                        config_.uniqueId.c_str());
  }
}

} // namespace hass
} // namespace omg
