/*
  OpenMQTTGateway - MQTT Publisher Interface
  
  Interface for MQTT publishing operations.
  Implements Dependency Inversion Principle.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include <ArduinoJson.h>

#include <string>

namespace omg {
namespace hass {

/**
 * @brief Interface for MQTT publishing operations
 * 
 * Dependency Inversion Principle: High-level modules should not depend 
 * on low-level modules. Both should depend on abstractions.
 */
class IMqttPublisher {
public:
  virtual ~IMqttPublisher() = default;
  virtual bool publishJson(JsonObject& json) = 0;
  virtual bool publishMessage(const std::string& topic, const std::string& payload, bool retain = false) = 0;
  virtual std::string getUId(const std::string& name, const std::string& suffix = "") = 0;
};

} // namespace hass
} // namespace omg
