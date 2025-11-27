#include <TheengsLogs.h>
#include <core/Filter.h>

#include <algorithm>
#include <cstring>

// Constructor
Filter::Filter(IStorage& storageRef) : AbstractStorageObject(storageRef, "Filters") {
  THEENGS_LOG_VERBOSE(F("Filter: Initialized with storage reference." CR));
  pass_.reserve(10); // Pre-allocate for common case
  block_.reserve(10);
}

// Updates the object from a JSON object
void Filter::from(JsonObject& data) {
  THEENGS_LOG_VERBOSE(F("Filter: Loading filters from JSON data." CR));

  auto extractAllValue = [](JsonObject& data, RuleType rule, Filter* filter) {
    const char* ruleTypeName = (rule == PASS) ? "pass" : "block";

    for (JsonPair kv : data) {
      if (!kv.value().is<JsonArray>()) {
        THEENGS_LOG_WARNING(F("Filter: Invalid structure for key '%s', expected JsonArray" CR), kv.key().c_str());
        continue;
      }

      JsonArray arr = kv.value().as<JsonArray>();
      for (JsonVariant v : arr) {
        if (!v.is<const char*>()) {
          THEENGS_LOG_WARNING(F("Filter: Invalid value type for key '%s'" CR), kv.key().c_str());
          continue;
        }

        const char* value = v.as<const char*>();
        if (!filter->add(kv.key().c_str(), value, rule)) {
          THEENGS_LOG_WARNING(F("Filter: Failed to add %s filter for key '%s' with value '%s' (limit reached)" CR),
                              ruleTypeName, kv.key().c_str(), value);
          return; // Stop processing if limit reached
        }

        THEENGS_LOG_VERBOSE(F("Filter: Added %s filter for key '%s' with value '%s'" CR),
                            ruleTypeName, kv.key().c_str(), value);
      }
    }
  };

  extractAllValue(data, PASS, this);
  extractAllValue(data, BLOCK, this);

  THEENGS_LOG_VERBOSE(F("Filter: Finished loading filters from JSON. Total: %u/%u" CR),
                      getTotalFilterCount(), MAX_TOTAL_FILTERS);
}

// Serializes the object to a JSON object
void Filter::to(JsonObject& data) {
  THEENGS_LOG_VERBOSE(F("Filter: Serializing filters to JSON object." CR));

  auto serializeAllValues = [](JsonObject& data, RuleType rule, Filter* filter) {
    const auto& target = (rule == PASS) ? filter->pass_ : filter->block_;

    JsonObject nestedObj = data.createNestedObject((rule == PASS) ? "pass" : "block");

    for (const auto& [key, patterns] : target) {
      JsonArray arr = nestedObj.createNestedArray(key.c_str());
      for (const auto& ruleValue : patterns) {
        arr.add(ruleValue.value.c_str());
      }
    }
  };

  serializeAllValues(data, PASS, this);
  serializeAllValues(data, BLOCK, this);

  THEENGS_LOG_VERBOSE(F("Filter: Finished serializing filters to JSON." CR));
}

// Updates the object from a JSON object
void Filter::fromRulesList(JsonObject& data) {
  if (data.containsKey("rules")) {
    THEENGS_LOG_VERBOSE(F("Filter: Loading rules from Message." CR));
    if (data["rules"].is<JsonArray>()) {
      JsonArray listOfRules = data["rules"].as<JsonArray>();
      for (JsonVariant ruleVariant : listOfRules) {
        // Verify that the element is a JsonObject before processing
        if (!ruleVariant.is<JsonObject>()) {
          THEENGS_LOG_ERROR(F("Filter: Rule is malformed, is not a object skipping." CR));
          continue;
        }

        JsonObject rule = ruleVariant.as<JsonObject>();
        const char* target = rule["target"];
        const char* action = rule["action"];
        const char* value = rule["value"];
        const char* key = rule["key"];
        if (action && value) {
          Filter::RuleType ruleType;
          if (strcmp(action, "pass") == 0) {
            ruleType = Filter::PASS;
          } else if (strcmp(action, "block") == 0) {
            ruleType = Filter::BLOCK;
          } else {
            THEENGS_LOG_WARNING(F("Filter: Unknown action '%s', skipping rule." CR), action);
            continue;
          }

          if (target && (strcmp(target, "topic") == 0)) {
            if (!this->addTopicFilters(value, ruleType)) {
              THEENGS_LOG_WARNING(F("Filter: Failed to add topic rule - target: '%s', action: '%s', value: '%s'" CR),
                                  target, action, value);
            } else {
              THEENGS_LOG_TRACE(F("Filter: Added topic rule - target: '%s', action: '%s', value: '%s'" CR), target, action, value);
            }
          } else {
            if (key) {
              if (!this->add(key, value, ruleType)) {
                THEENGS_LOG_WARNING(F("Filter: Failed to add rule - target: '%s', action: '%s', key: '%s', value: '%s'" CR),
                                    target, action, key, value);
              } else {
                THEENGS_LOG_VERBOSE(F("Filter: Added rule - target: '%s', action: '%s', key: '%s', value: '%s'" CR),
                                    target, action, key, value);
              }
            } else {
              THEENGS_LOG_ERROR(F("Filter: Rule missing 'key' field - target: '%s', action: '%s', value: '%s'" CR),
                                target, action, value);
            }
          }
        } else {
          THEENGS_LOG_ERROR(F("Filter: Rule malformed  missing required fields." CR));
        }
      }
    } else {
      THEENGS_LOG_WARNING(F("Filter: 'rules' field is not a Array" CR));
    }
  }
}

// Serializes the object to a JSON object
void Filter::toRulesList(JsonObject& data) {
  THEENGS_LOG_VERBOSE(F("Filter: Serializing to JSON." CR));
  JsonArray rulesArray = data.createNestedArray("rules");

  // Serialize pass filters
  for (const auto& [key, values] : pass_) {
    for (const auto& ruleValue : values) {
      JsonObject ruleObj = rulesArray.createNestedObject();
      ruleObj["target"] = (key == TOPIC_FILTER_KEY) ? "topic" : "value";
      ruleObj["action"] = "pass";
      ruleObj["key"] = (key == TOPIC_FILTER_KEY) ? "" : key.c_str();
      ruleObj["value"] = ruleValue.value.c_str();
    }
  }

  // Serialize block filters
  for (const auto& [key, values] : block_) {
    for (const auto& ruleValue : values) {
      JsonObject ruleObj = rulesArray.createNestedObject();
      ruleObj["target"] = (key == TOPIC_FILTER_KEY) ? "topic" : "value";
      ruleObj["action"] = "block";
      ruleObj["key"] = (key == TOPIC_FILTER_KEY) ? "" : key.c_str();
      ruleObj["value"] = ruleValue.value.c_str();
    }
  }

  THEENGS_LOG_VERBOSE(F("Filter: Serialization complete." CR));
}

// Saves the filter configuration to a string
void Filter::to(const char* data) {
  if (!data) {
    THEENGS_LOG_ERROR(F("Filter: Null data pointer provided to to()" CR));
    return;
  }

  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject obj = doc.to<JsonObject>();
  this->to(obj);
  serializeJson(doc, const_cast<char*>(data), JSON_MSG_BUFFER);

  THEENGS_LOG_VERBOSE(F("Filter: Serialized filters to string." CR));
}

// Adds a value to the pass or block with capacity checking
bool Filter::add(const char* key, const char* value, RuleType rule) {
  // Input validation
  if (!key || !value) {
    THEENGS_LOG_ERROR(F("Filter: Null key or value provided to add()" CR));
    return false;
  }

  // Check capacity BEFORE adding (fail-silent for ESP32 safety)
  if (!hasCapacity()) {
    THEENGS_LOG_WARNING(F("Filter: Cannot add filter - limit of %u reached" CR), MAX_TOTAL_FILTERS);
    return false;
  }

  const char* ruleTypeName = (rule == PASS) ? "pass" : "block";
  THEENGS_LOG_VERBOSE(F("Filter: Adding %s filter for key '%s' with value '%s'" CR),
                      ruleTypeName, key, value);

  // Select target map
  auto& target = (rule == PASS) ? pass_ : block_;

  // Intern the key for memory efficiency
  std::string internedKey(key);

  // Add the rule value (exceptions disabled on ESP8266)
  target[internedKey].emplace_back(value);

  THEENGS_LOG_VERBOSE(F("Filter: Successfully added %s filter. Total: %u/%u" CR),
                      ruleTypeName, getTotalFilterCount(), MAX_TOTAL_FILTERS);
  return true;
}

// Removes a value from the pass or block
bool Filter::remove(const char* key, const char* value) {
  if (!key || !value) {
    THEENGS_LOG_ERROR(F("Filter: Null key or value provided to remove()" CR));
    return false;
  }

  THEENGS_LOG_VERBOSE(F("Filter: Removing filter for key '%s' with value '%s'" CR), key, value);

  bool removed = false;
  std::string keyStr(key);

  auto removeFromList = [value, &removed](std::vector<RuleValue>& list) {
    auto it = std::remove_if(list.begin(), list.end(),
                             [value](const RuleValue& rv) {
                               return rv.value == value;
                             });

    if (it != list.end()) {
      list.erase(it, list.end());
      removed = true;
    }
  };

  // Remove from both lists
  if (auto it = pass_.find(keyStr); it != pass_.end()) {
    removeFromList(it->second);
    if (it->second.empty()) {
      pass_.erase(it);
    }
  }

  if (auto it = block_.find(keyStr); it != block_.end()) {
    removeFromList(it->second);
    if (it->second.empty()) {
      block_.erase(it);
    }
  }

  if (removed) {
    THEENGS_LOG_VERBOSE(F("Filter: Removed filter for key '%s' with value '%s'. Total: %u/%u" CR),
                        key, value, getTotalFilterCount(), MAX_TOTAL_FILTERS);
  } else {
    THEENGS_LOG_VERBOSE(F("Filter: Filter not found for key '%s' with value '%s'" CR), key, value);
  }

  return removed;
}

// Checks if a value is contained in the pass or block
bool Filter::contains(const char* key, const char* value, RuleType rule) const {
  if (!key || !value) {
    return false;
  }

  const char* ruleTypeName = (rule == PASS) ? "pass" : "block";
  //  THEENGS_LOG_VERBOSE(F("Filter: Checking %s filters for key '%s' with value '%s'" CR),  ruleTypeName, key, value);

  const auto& target = (rule == PASS) ? pass_ : block_;
  std::string keyStr(key);

  auto it = target.find(keyStr);
  if (it == target.end()) {
    //  THEENGS_LOG_VERBOSE(F("Filter: Key '%s' not found in %s filters" CR), key, ruleTypeName);
    return false;
  }

  // Check all rule values for this key
  for (const auto& ruleValue : it->second) {
    if (ruleValue.matches(value)) {
      //  THEENGS_LOG_VERBOSE(F("Filter: Match found for key '%s' with value '%s' in %s filters" CR), key, value, ruleTypeName);
      return true;
    }
  }

  //  THEENGS_LOG_VERBOSE(F("Filter: No match found for key '%s' with value '%s' in %s filters" CR), key, value, ruleTypeName);
  return false;
}

// Checks if the pass or block is empty
bool Filter::isEmptyThe(RuleType rule) const {
  const auto& target = (rule == PASS) ? pass_ : block_;
  bool empty = target.empty();
  return empty;
}

// Counts total filters in a map
size_t Filter::countFiltersIn(const std::unordered_map<std::string, std::vector<RuleValue>>& map) const {
  size_t count = 0;
  for (const auto& [key, values] : map) {
    count += values.size();
  }
  return count;
}

// Gets total number of filters
size_t Filter::getTotalFilterCount() const {
  return countFiltersIn(pass_) + countFiltersIn(block_);
}

// Gets number of filters in specified list
size_t Filter::getFilterCount(RuleType rule) const {
  const auto& target = (rule == PASS) ? pass_ : block_;
  return countFiltersIn(target);
}

// Clears all filters
void Filter::clear() {
  THEENGS_LOG_VERBOSE(F("Filter: Clearing all filters" CR));
  pass_.clear();
  block_.clear();
  THEENGS_LOG_VERBOSE(F("Filter: All filters cleared" CR));
}

bool Filter::addTopicFilters(const char* value, RuleType rule) {
  if (!value) {
    THEENGS_LOG_ERROR(F("Filter: Null value provided to addTopicFilters()" CR));
    return false;
  }

  THEENGS_LOG_VERBOSE(F("Filter: Adding topic filters from value '%s'" CR), value);
  return add(TOPIC_FILTER_KEY, value, rule);
}

bool Filter::removeTopicFilters(const char* value) {
  if (!value) {
    THEENGS_LOG_ERROR(F("Filter: Null value provided to addTopicFilters()" CR));
    return false;
  }

  //THEENGS_LOG_VERBOSE(F("Filter: Adding topic filters from value '%s'" CR), value);
  return remove(TOPIC_FILTER_KEY, value);
}

bool Filter::isTopicFilterPresent(const char* value, RuleType rule) {
  if (!value) {
    THEENGS_LOG_ERROR(F("Filter: Null value provided to isTopicFilterPresent()" CR));
    return false;
  }

  //THEENGS_LOG_VERBOSE(F("Filter: Checking topic filters for value '%s'" CR), value);
  return contains(TOPIC_FILTER_KEY, value, rule);
}