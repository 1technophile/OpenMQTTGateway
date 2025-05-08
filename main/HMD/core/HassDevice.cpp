/*
  OpenMQTTGateway - Home Assistant Device Implementation
*/

#include "HassDevice.h"

#include "HassLogging.h"

namespace omg {
namespace hass {

// DeviceInfo default constructor
HassDevice::DeviceInfo::DeviceInfo() {
  // Note: manufacturer and swVersion will be set by the creating context
  // using the ISettingsProvider methods
}

bool HassDevice::DeviceInfo::isValid() const {
  return !name.empty() && !identifier.empty();
}

HassDevice::HassDevice(const DeviceInfo& info, const ISettingsProvider& settingsProvider)
    : info_(info), settingsProvider_(settingsProvider) {
  validateAndSanitize();
}

void HassDevice::toJson(JsonObject& deviceJson) const {
  if (info_.isGateway) {
    addGatewayInfo(deviceJson);
  } else {
    addExternalDeviceInfo(deviceJson);
  }
}

bool HassDevice::updateInfo(const DeviceInfo& info) {
  if (!info.isValid()) {
    return false;
  }

  info_ = info;
  validateAndSanitize();
  return true;
}

HassDevice HassDevice::createGatewayDevice(const ISettingsProvider& settingsProvider) {
  DeviceInfo info;
  info.name = settingsProvider.getGatewayName();
  info.manufacturer = settingsProvider.getGatewayManufacturer();
  info.swVersion = settingsProvider.getGatewayVersion();
  info.identifier = settingsProvider.getNetworkMacAddress();
  info.isGateway = true;

#ifndef GATEWAY_MODEL
  std::string model;
  JsonArray modules = settingsProvider.getModules();
  // Serialize to a temporary buffer then convert to std::string
  char buffer[256];
  serializeJson(modules, buffer, sizeof(buffer));
  model = buffer;
  info.model = model;
#else
  info.model = GATEWAY_MODEL;
#endif

  info.configUrl = std::string("http://") + settingsProvider.getNetworkIPAddress() + "/";

  return HassDevice(info, settingsProvider);
}

HassDevice HassDevice::createExternalDevice(const std::string& name,
                                            const std::string& manufacturer,
                                            const std::string& model,
                                            const std::string& identifier,
                                            const ISettingsProvider& settingsProvider) {
  DeviceInfo info;
  info.name = name;
  info.manufacturer = manufacturer.empty() ? "Unknown" : manufacturer;
  info.model = model.empty() ? "Unknown" : model;
  info.identifier = identifier;
  info.isGateway = false;

  return HassDevice(info, settingsProvider);
}

void HassDevice::addGatewayInfo(JsonObject& deviceJson) const {
  deviceJson["name"] = info_.name;
  deviceJson["mf"] = info_.manufacturer;
  deviceJson["mdl"] = info_.model;
  deviceJson["sw"] = info_.swVersion;

  if (!info_.configUrl.empty()) {
    deviceJson["cu"] = info_.configUrl;
  }

  // Add identifiers
  JsonArray identifiers = deviceJson.createNestedArray("ids");
  identifiers.add(info_.identifier);

  // Add connections (MAC address)
  JsonArray connections = deviceJson.createNestedArray("cns");
  JsonArray connection_mac = connections.createNestedArray();
  connection_mac.add("mac");
  connection_mac.add(info_.identifier);
}

void HassDevice::addExternalDeviceInfo(JsonObject& deviceJson) const {
  if (!info_.name.empty()) {
    // Generate unique device name if needed
    std::string deviceName = info_.name;
    if (info_.name != info_.identifier && !info_.identifier.empty()) {
      // Add part of identifier for uniqueness
      std::string shortId = info_.identifier.length() > 6 ? info_.identifier.substr(info_.identifier.length() - 6) : info_.identifier;
      deviceName += "-" + shortId;
    }
    deviceJson["name"] = deviceName;
  }

  if (!info_.manufacturer.empty()) {
    deviceJson["mf"] = info_.manufacturer;
  }

  if (!info_.model.empty()) {
    deviceJson["mdl"] = info_.model;
  }

  if (!info_.swVersion.empty()) {
    deviceJson["sw"] = info_.swVersion;
  }

  if (!info_.identifier.empty()) {
    // Add identifiers
    JsonArray identifiers = deviceJson.createNestedArray("ids");
    identifiers.add(info_.identifier);

    // Add connections
    JsonArray connections = deviceJson.createNestedArray("cns");
    JsonArray connection_mac = connections.createNestedArray();
    connection_mac.add("mac");
    connection_mac.add(info_.identifier);
  }

  // Link to gateway device
  deviceJson["via_device"] = settingsProvider_.getNetworkMacAddress();
}

void HassDevice::validateAndSanitize() {
  // Ensure required fields are not empty
  if (info_.name.empty()) {
    info_.name = info_.isGateway ? "OpenMQTTGateway" : "Unknown Device";
  }

  if (info_.identifier.empty() && info_.isGateway) {
    info_.identifier = settingsProvider_.getNetworkMacAddress();
  }

  if (info_.manufacturer.empty()) {
    info_.manufacturer = info_.isGateway ? settingsProvider_.getGatewayManufacturer() : "Unknown";
  }

  if (info_.model.empty()) {
    info_.model = info_.isGateway ? "ESP32/ESP8266" : "Unknown";
  }

  // Validate identifier format (basic MAC address validation)
  if (!info_.identifier.empty() && info_.identifier.find(':') == std::string::npos) {
    // Not a MAC address format - could be other identifier format
    // Keep as is but log warning
  }
}

} // namespace hass
} // namespace omg
