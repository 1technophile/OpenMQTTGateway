#pragma once
#include <config_JSONMessages.h>
#include <storage/AbstractStorageObject.h>
#include <storage/IJsonable.h>
#include <storage/IStorage.h>

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

// ESP32 memory constraints - limit total filters
static constexpr size_t MAX_TOTAL_FILTERS = 50;

// Topic filter key constant - stored efficiently for memory-constrained systems
static constexpr const char TOPIC_FILTER_KEY[] = "_topic_";

/**
 * @brief Wildcard pattern matching for ESP32.
 * 
 * Supports simple wildcards: * (any sequence) and ? (single char).
 * Much lighter than regex - suitable for resource-constrained ESP32.
 * 
 * @param pattern Pattern with wildcards (* and ?)
 * @param text Text to match against pattern
 * @return true if text matches pattern
 */
inline bool wildcardMatch(const char* pattern, const char* text) {
  if (!pattern || !text) return false;

  const char* p = pattern;
  const char* t = text;
  const char* starPos = nullptr;
  const char* textPos = nullptr;

  while (*t) {
    if (*p == '*') {
      // Remember position after * for backtracking
      starPos = p++;
      textPos = t;
    } else if (*p == '?' || *p == *t) {
      // Match single char or exact match
      p++;
      t++;
    } else if (starPos) {
      // Backtrack to last * and try next position
      p = starPos + 1;
      t = ++textPos;
    } else {
      return false;
    }
  }

  // Skip trailing * in pattern
  while (*p == '*') p++;

  return *p == '\0';
}

/**
 * @brief Represents a filter rule value with optional wildcard pattern.
 * 
 * Optimized for ESP32 with lightweight wildcard matching (* and ?).
 * Avoids heavy regex compilation, saving memory and CPU cycles.
 */
struct RuleValue {
  std::string value;
  bool is_pattern;

  explicit RuleValue(const char* str) : value(str ? str : ""), is_pattern(false) {
    if (value.empty()) return;

    // Check if value contains wildcard characters
    is_pattern = (value.find('*') != std::string::npos ||
                  value.find('?') != std::string::npos);

    if (is_pattern) {
      THEENGS_LOG_VERBOSE(F("Filter: Wildcard pattern detected: '%s'" CR), value.c_str());
    }
  }

  // Default copy/move semantics are fine for simple types
  RuleValue(const RuleValue&) = default;
  RuleValue& operator=(const RuleValue&) = default;
  RuleValue(RuleValue&&) noexcept = default;
  RuleValue& operator=(RuleValue&&) noexcept = default;

  /**
   * @brief Checks if input matches this rule value.
   * @param input String to match against
   * @return true if matches, false otherwise
   */
  bool matches(const char* input) const {
    if (!input) return false;

    if (is_pattern) {
      // Wildcard matching (* and ?)
      return wildcardMatch(value.c_str(), input);
    } else {
      // Direct string comparison
      return value == input;
    }
  }

  /**
   * @brief Checks if this is a wildcard pattern.
   */
  bool is_wildcard() const { return is_pattern; }
};

/**
 * @brief Filter class for MQTT message filtering with ESP32 optimizations.
 * 
 * Implements pass/block list filtering with:
 * - Memory-safe string handling (no dangling pointers)
 * - 50-filter total limit to prevent heap exhaustion
 * - Lightweight wildcard matching (* and ?) instead of regex
 * - RAII resource management
 * - Fail-silent overflow protection
 */
class Filter : public AbstractStorageObject, public IJsonable {
public:
  enum RuleType { BLOCK = 0,
                  PASS = 1 };

  explicit Filter(IStorage& storageRef);
  virtual ~Filter() = default;

  // Disable copy, enable move
  Filter(const Filter&) = delete;
  Filter& operator=(const Filter&) = delete;
  Filter(Filter&&) noexcept = default;
  Filter& operator=(Filter&&) noexcept = default;

  /**
   * @brief Updates the object from a JSON object.
   * @param data A reference to a JsonObject containing the data.
   */
  void from(JsonObject& data) override;

  /**
   * @brief Serializes the object to a JSON object.
   * @param data A reference to a JsonObject where the object data will be serialized.
   */
  void to(JsonObject& data) override;

  /**
   * @brief Updates the object from a JSON object that have a "rules" array.
   * @param data A reference to a JsonObject containing the data.
   */
  void fromRulesList(JsonObject& data);

  /**
   * @brief Serializes the object to a JSON object that have a "rules" array.
   * @param data A reference to a JsonObject where the object data will be serialized.
   */
  void toRulesList(JsonObject& data);

  /**
   * @brief Saves the filter configuration to a string.
   * @param data A pointer to a character array where the configuration will be saved.
   */
  void to(const char* data);

  /**
   * @brief Adds a value to the pass or block list.
   * @param key The key associated with the value to be added.
   * @param value The value to be added.
   * @param rule PASS to add to pass list, BLOCK to add to block list
   * @return true if added successfully, false if limit reached
   */
  bool add(const char* key, const char* value, RuleType rule);

  /**
   * @brief Removes a value from pass and block lists.
   * @param key The key associated with the value to be removed.
   * @param value The value to be removed.
   * @return true if removed successfully, false if not found
   */
  bool remove(const char* key, const char* value);

  /**
   * @brief Checks if a value is contained in the specified list.
   * @param key The key associated with the value to be checked.
   * @param value The value to be checked.
   * @param rule PASS to check in pass list, BLOCK to check in block list
   * @return true if the value is found, false otherwise.
   */
  bool contains(const char* key, const char* value, RuleType rule) const;

  /**
   * @brief Checks if the specified list is empty.
   * @param rule PASS to check pass list, BLOCK to check block list
   * @return true if the list is empty, false otherwise.
   */
  bool isEmptyThe(RuleType rule) const;

  /**
   * @brief Gets total number of filters across both lists.
   * @return Total filter count
   */
  size_t getTotalFilterCount() const;

  /**
   * @brief Gets number of filters in specified list.
   * @param rule PASS or BLOCK
   * @return Filter count for specified list
   */
  size_t getFilterCount(RuleType rule) const;

  /**
   * @brief Checks if more filters can be added.
   * @return true if under limit, false if at capacity
   */
  bool hasCapacity() const { return getTotalFilterCount() < MAX_TOTAL_FILTERS; }

  /**
   * @brief Clears all filters.
   */
  void clear();

  /**
   * 
   * @brief Adds multiple topic filters.
   * 
   * @param value The topic Filter
   * @param rule PASS to add to pass list, BLOCK to add to block list
   * @return true if added successfully, false otherwise
   */
  bool addTopicFilters(const char* value, RuleType rule);

  /**
   * 
   * @brief Removes multiple topic filters.
   * 
   * @param value The topic Filter
   * @return true if removed successfully, false otherwise
   */
  bool removeTopicFilters(const char* value);

  /**
   * @brief Checks if a topic filter is present in the specified list.
   */
  bool isTopicFilterPresent(const char* value, RuleType rule);

private:
  std::unordered_map<std::string, std::vector<RuleValue>> pass_;
  std::unordered_map<std::string, std::vector<RuleValue>> block_;

  /**
   * @brief Counts total filters in a map.
   * @param map Map to count filters in
   * @return Total number of filter values
   */
  size_t countFiltersIn(const std::unordered_map<std::string, std::vector<RuleValue>>& map) const;
};
