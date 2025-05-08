/*
  OpenMQTTGateway - Settings Provider Interface
  
  Interface for configuration settings access.
  Implements Single Responsibility and Interface Segregation Principles.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include <ArduinoJson.h>

#include <string>

namespace omg {
namespace hass {

/**
 * @brief Interface for configuration settings access
 * 
 * Single Responsibility Principle: Only handles configuration access
 * Interface Segregation Principle: Minimal interface with only needed methods
 */
class ISettingsProvider {
public:
  virtual ~ISettingsProvider() = default;
  virtual std::string getDiscoveryPrefix() const = 0;
  virtual std::string getMqttTopic() const = 0;
  virtual std::string getGatewayName() const = 0;
  virtual bool isEthConnected() const = 0;
  virtual std::string getNetworkMacAddress() const = 0;
  virtual std::string getNetworkIPAddress() const = 0;
  virtual JsonArray getModules() const = 0;
  virtual std::string getGatewayManufacturer() const = 0;
  virtual std::string getGatewayVersion() const = 0;
};

} // namespace hass
} // namespace omg
