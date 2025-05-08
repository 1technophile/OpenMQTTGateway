/*
  OpenMQTTGateway - Home Assistant Button Entity Implementation
*/

#include "HassButton.h"

#include "../core/HassConstants.h"
#include "../core/HassLogging.h"

namespace omg {
namespace hass {

// ButtonConfig methods
HassButton::ButtonConfig HassButton::ButtonConfig::createRestart(const std::string& payload) {
  ButtonConfig config;
  config.payloadPress = payload;
  config.deviceClass = HASS_CLASS_RESTART;
  return config;
}

HassButton::ButtonConfig HassButton::ButtonConfig::createUpdate(const std::string& payload) {
  ButtonConfig config;
  config.payloadPress = payload;
  config.deviceClass = "update";
  return config;
}

HassButton::ButtonConfig HassButton::ButtonConfig::createGeneric(const std::string& payload,
                                                                 const std::string& deviceClass) {
  ButtonConfig config;
  config.payloadPress = payload;
  config.deviceClass = deviceClass;
  return config;
}

// HassButton methods
HassButton::HassButton(const EntityConfig& config, const ButtonConfig& buttonConfig,
                       std::shared_ptr<HassDevice> device)
    : HassEntity(config, device), buttonConfig_(buttonConfig) {
  // Ensure component type is button
  if (config_.componentType != HASS_TYPE_BUTTON) {
    config_.componentType = HASS_TYPE_BUTTON;
  }

  // Override device class if button config has one
  if (!buttonConfig_.deviceClass.empty()) {
    config_.deviceClass = buttonConfig_.deviceClass;
  }

  validateButtonConfig();
}

HassButton::HassButton(const EntityConfig& config, std::shared_ptr<HassDevice> device)
    : HassEntity(config, device) {
  // Ensure component type is button
  if (config_.componentType != HASS_TYPE_BUTTON) {
    config_.componentType = HASS_TYPE_BUTTON;
  }

  // Set default payload if none provided
  if (buttonConfig_.payloadPress.empty()) {
    buttonConfig_.payloadPress = "{\"cmd\":\"press\"}";
  }

  validateButtonConfig();
}

void HassButton::updateButtonConfig(const ButtonConfig& config) {
  buttonConfig_ = config;

  // Update device class if provided
  if (!buttonConfig_.deviceClass.empty()) {
    config_.deviceClass = buttonConfig_.deviceClass;
  }

  validateButtonConfig();
}

void HassButton::addSpecificFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const {
  // Add payload for button press
  if (!buttonConfig_.payloadPress.empty()) {
    json["pl_prs"] = buttonConfig_.payloadPress; // payload_press
  }

  // Buttons typically don't have state topics (they're action-only)
  // If no command topic is set, warn
  if (config_.commandTopic.empty()) {
    THEENGS_LOG_WARNING(F("Button %s: no command topic set" CR),
                        config_.uniqueId.c_str());
  }

  // Note: Buttons don't need state topics as they represent actions, not states
  // The command topic is where the button press payload will be sent
}

void HassButton::validateButtonConfig() const {
  // Buttons must have a press payload
  if (buttonConfig_.payloadPress.empty()) {
    THEENGS_LOG_WARNING(F("Button %s: payloadPress is empty" CR),
                        config_.uniqueId.c_str());
  }

  // Buttons need a command topic to be functional
  if (config_.commandTopic.empty()) {
    THEENGS_LOG_WARNING(F("Button %s: no command topic set, button may not work" CR),
                        config_.uniqueId.c_str());
  }

  // Validate device class if provided
  if (!buttonConfig_.deviceClass.empty()) {
    // Common button device classes: restart, update, identify, etc.
    const char* validButtonClasses[] = {
        HASS_CLASS_RESTART,
        "update",
        "identify",
        "configure"};

    bool isValidClass = false;
    for (const char* validClass : validButtonClasses) {
      if (buttonConfig_.deviceClass == validClass) {
        isValidClass = true;
        break;
      }
    }

    if (!isValidClass) {
      THEENGS_LOG_WARNING(F("Button %s: using custom device class '%s'" CR),
                          config_.uniqueId.c_str(), buttonConfig_.deviceClass.c_str());
    }
  }
}

} // namespace hass
} // namespace omg
