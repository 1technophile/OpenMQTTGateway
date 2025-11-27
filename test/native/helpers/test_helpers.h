#pragma once

#include <gtest/gtest.h>

#include <string>
#include <vector>

/**
 * @file test_helpers.h
 * @brief Common test utilities for OpenMQTTGateway testing
 * 
 * This file contains helper functions and utilities that can be used
 * across multiple test files to reduce code duplication and improve
 * test maintainability.
 */

namespace TestHelpers {

/**
 * @brief Compare two floating point numbers with epsilon tolerance
 * @param a First number to compare
 * @param b Second number to compare
 * @param epsilon Tolerance for comparison (default: 1e-6)
 * @return true if numbers are equal within tolerance
 */
bool isNearlyEqual(double a, double b, double epsilon = 1e-6);

/**
 * @brief Generate test JSON string with specified key-value pairs
 * @param keys Vector of JSON keys
 * @param values Vector of JSON values (as strings)
 * @return Formatted JSON string
 */
std::string generateTestJson(const std::vector<std::string>& keys,
                             const std::vector<std::string>& values);

/**
 * @brief Create a test MQTT topic following OpenMQTTGateway conventions
 * @param gateway_name Gateway identifier
 * @param module Module name (e.g., "BT", "RF", "IR")
 * @param direction Direction ("toMQTT" or "fromMQTT")
 * @param device_id Optional device identifier
 * @return Formatted MQTT topic string
 */
std::string createMQTTTopic(const std::string& gateway_name,
                            const std::string& module,
                            const std::string& direction,
                            const std::string& device_id = "");

/**
 * @brief Validate JSON string format
 * @param json_str JSON string to validate
 * @return true if valid JSON format
 */
bool isValidJson(const std::string& json_str);

/**
 * @brief Convert byte array to hex string representation
 * @param data Byte array to convert
 * @param length Length of byte array
 * @return Hex string representation
 */
std::string bytesToHex(const uint8_t* data, size_t length);

} // namespace TestHelpers