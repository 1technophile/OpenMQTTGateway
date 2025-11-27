#include "test_helpers.h"

#include <ArduinoJson.h>

#include <cmath>
#include <iomanip>
#include <sstream>

namespace TestHelpers {

bool isNearlyEqual(double a, double b, double epsilon) {
  return std::abs(a - b) < epsilon;
}

std::string generateTestJson(const std::vector<std::string>& keys,
                             const std::vector<std::string>& values) {
  if (keys.size() != values.size()) {
    return "{}"; // Return empty JSON if sizes don't match
  }

  DynamicJsonDocument doc(1024);

  for (size_t i = 0; i < keys.size(); ++i) {
    doc[keys[i]] = values[i];
  }

  std::string result;
  serializeJson(doc, result);
  return result;
}

std::string createMQTTTopic(const std::string& gateway_name,
                            const std::string& module,
                            const std::string& direction,
                            const std::string& device_id) {
  std::string topic = "home/" + gateway_name + "/" + module + direction;

  if (!device_id.empty()) {
    topic += "/" + device_id;
  }

  return topic;
}

bool isValidJson(const std::string& json_str) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, json_str);
  return error == DeserializationError::Ok;
}

std::string bytesToHex(const uint8_t* data, size_t length) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (size_t i = 0; i < length; ++i) {
    oss << std::setw(2) << static_cast<unsigned>(data[i]);
  }

  return oss.str();
}

} // namespace TestHelpers