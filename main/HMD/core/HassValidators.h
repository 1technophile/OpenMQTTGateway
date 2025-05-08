/*
  OpenMQTTGateway - Home Assistant Validators
  
  Validates Home Assistant device classes and measurement units.
  Implements Single Responsibility Principle - only handles validation.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

#include <string_view>
#include <unordered_set>

#include "HassConstants.h"

namespace omg {
namespace hass {

/**
 * @brief Validates Home Assistant device classes and measurement units
 * 
 * Single Responsibility Principle: This class only handles validation logic
 * for Home Assistant specific values.
 * 
 * Performance: Uses unordered_set for O(1) lookup time
 * Memory: Static data stored in flash memory
 */
class HassValidators {
public:
  /**
     * @brief Validates if a device class is supported by Home Assistant
     * @param deviceClass Device class string to validate
     * @return true if valid, false otherwise
     */
  static bool isValidDeviceClass(const char* deviceClass);

  /**
     * @brief Validates if a measurement unit is supported by Home Assistant
     * @param unit Unit string to validate
     * @return true if valid, false otherwise
     */
  static bool isValidUnit(const char* unit);

  /**
     * @brief Gets the number of valid device classes
     * @return Number of supported device classes
     */
  static size_t getValidClassesCount();

  /**
     * @brief Gets the number of valid units
     * @return Number of supported units
     */
  static size_t getValidUnitsCount();

private:
  /// Set of valid Home Assistant device classes
  static const std::unordered_set<std::string_view> validClasses_;

  /// Set of valid Home Assistant measurement units
  static const std::unordered_set<std::string_view> validUnits_;

  /**
     * @brief Initialize the validator sets (called automatically)
     */
  static void initialize();
};

} // namespace hass
} // namespace omg
