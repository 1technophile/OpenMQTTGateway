/*
  OpenMQTTGateway - Home Assistant Topic Builder Implementation
*/

#include "HassTopicBuilder.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace omg {
namespace hass {

HassTopicBuilder::HassTopicBuilder(const ISettingsProvider& settingsProvider)
    : settingsProvider_(settingsProvider) {
}

std::string HassTopicBuilder::buildDiscoveryTopic(const char* component, const char* uniqueId) const {
  if (!component || !uniqueId || !component[0] || !uniqueId[0]) {
    return "";
  }

  std::string sanitizedComponent = sanitizeTopicComponent(component);
  std::string sanitizedUniqueId = sanitizeTopicComponent(uniqueId);

  return settingsProvider_.getDiscoveryPrefix() + "/" + sanitizedComponent + "/" + sanitizedUniqueId + "/config";
}

std::string HassTopicBuilder::buildStateTopic(const char* topic, bool gatewayEntity) const {
  if (!topic || !topic[0]) {
    return "";
  }

  std::string baseTopic = buildBaseTopic(gatewayEntity);
  return baseTopic + topic;
}

std::string HassTopicBuilder::buildAvailabilityTopic(const char* topic, bool gatewayEntity) const {
  if (!gatewayEntity) {
    return ""; // External devices don't have availability topics managed by gateway
  }

  std::string baseTopic = buildBaseTopic(true);
  return baseTopic + (topic ? topic : "/LWT");
}

std::string HassTopicBuilder::buildCommandTopic(const char* topic) const {
  if (!topic || !topic[0]) {
    return "";
  }

  return settingsProvider_.getMqttTopic() + settingsProvider_.getGatewayName() + topic;
}

std::string HassTopicBuilder::buildBaseTopic(bool gatewayEntity) const {
  if (gatewayEntity) {
    return settingsProvider_.getMqttTopic() + settingsProvider_.getGatewayName();
  } else {
    return "+/+"; // Wildcard for external devices
  }
}

bool HassTopicBuilder::isValidTopicComponent(const char* component) {
  if (!component || !component[0]) {
    return false;
  }

  // Check for MQTT topic restrictions
  const char* invalidChars = "+#";
  for (const char* c = component; *c; ++c) {
    if (strchr(invalidChars, *c)) {
      return false;
    }
    // Check for control characters
    if (*c < 32 || *c == 127) {
      return false;
    }
  }

  return true;
}

std::string HassTopicBuilder::sanitizeTopicComponent(const char* component) {
  if (!component) {
    return "";
  }

  std::string result(component);

  // Replace invalid characters with underscores
  std::replace_if(result.begin(), result.end(), [](char c) { return c == '+' || c == '#' || c < 32 || c == 127 || c == '/'; }, '_');

  // Remove consecutive underscores
  auto newEnd = std::unique(result.begin(), result.end(), [](char a, char b) {
    return a == '_' && b == '_';
  });
  result.erase(newEnd, result.end());

  // Remove leading/trailing underscores
  if (!result.empty() && result.front() == '_') {
    result.erase(0, 1);
  }
  if (!result.empty() && result.back() == '_') {
    result.pop_back();
  }

  return result.empty() ? "unknown" : result;
}

} // namespace hass
} // namespace omg
