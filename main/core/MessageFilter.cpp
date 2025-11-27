#include "MessageFilter.h"

/**
 * @brief Checks if a given MQTT value is present in the block.
 *
 * Iterates through all keys in the MQTTvalue JsonObject and checks each value
 * against the block patterns.
 *
 * @param MQTTvalue The MQTT value to check against the block.
 * @return true if the MQTT value is in the block and the block
 *         check is not ignored; false otherwise.
 */
bool MessageFilter::inBlockList(JsonObject& MQTTvalue) {
  //THEENGS_LOG_VERBOSE(F("MessageFilter: Checking block list for message." CR));
  // If block is ignored, always return false (bypass check)
  if (ignoreBlacklist) {
    THEENGS_LOG_VERBOSE(F("MessageFilter: Block list is ignored" CR));
    return false;
  }

  // Fast path: If block is empty, allow all messages (per documentation)
  if (filters.isEmptyThe(Filter::BLOCK)) {
    THEENGS_LOG_VERBOSE(F("MessageFilter: Block list is empty" CR));
    return false;
  }

  // Iterate through all keys in the MQTT message
  for (JsonPair kv : MQTTvalue) {
    // Handle string values directly without unnecessary conversions
    if (kv.value().is<const char*>()) {
      const char* key = kv.key().c_str();

      // Null check for safety
      if (key == nullptr) continue;

      const char* value = kv.value().as<const char*>();

      // Null check for safety
      if (value == nullptr) continue;

      if (filters.contains(key, value, Filter::BLOCK)) {
        // Check if this value matches any block
        THEENGS_LOG_TRACE(F("MessageFilter: Match found in BLOCK list for key '%s' with value '%s'" CR), key, value);
        return true;
      }
    }
  }
  THEENGS_LOG_TRACE(F("MessageFilter: No matches found in BLOCK list" CR));
  return false;
}

/**
 * @brief Checks if a given MQTT value is in the pass.
 *
 * Iterates through all keys in the MQTTvalue JsonObject and checks each value
 * against the pass patterns. If the pass is 
 * disabled or empty, the function returns true (allows all messages).
 *
 * @performance O(n×m) where n=message keys, m=filter count
 *              With 20 keys × 50 filters = 1000 ops/msg
 *              Early exit on first match minimizes average case
 *
 * @param MQTTvalue The MQTT value to check against the pass.
 * @return true if the pass is disabled, empty, or the value is found in the pass.
 * @return false if the value is not in the pass.
 */
bool MessageFilter::inPassList(JsonObject& MQTTvalue) {
  //THEENGS_LOG_VERBOSE(F("MessageFilter: Checking pass list for message." CR));

  // Fast path: If pass is ignored, always return true (allow all)
  if (ignoreWhitelist) {
    THEENGS_LOG_VERBOSE(F("MessageFilter: Pass is ignored, allowing all messages." CR));
    return true;
  }

  // Fast path: If pass is empty, allow all messages (per documentation)
  if (filters.isEmptyThe(Filter::PASS)) {
    THEENGS_LOG_VERBOSE(F("MessageFilter: Pass is empty, allowing all messages." CR));
    return true;
  }

  // Iterate through all keys in the MQTT message
  // Performance: O(n) where n = number of keys in message
  for (JsonPair kv : MQTTvalue) {
    // Handle string values directly without unnecessary conversions
    if (kv.value().is<const char*>()) {
      const char* key = kv.key().c_str();
      if (!key) continue; // Null check

      const char* value = kv.value().as<const char*>();
      if (!value) continue; // Null check

      // Check if this value matches any pass (early exit on match)
      if (filters.contains(key, value, Filter::PASS)) {
        THEENGS_LOG_TRACE(F("MessageFilter: Match found in PASS list for key '%s' with value '%s', allowing message." CR), key, value);
        return true; // Early exit - performance optimization
      }
    }
  }

  // No match found - return false (reject message)
  THEENGS_LOG_TRACE(F("MessageFilter: No matches found in PASS list." CR));
  return false;
}

/**
 * @brief Checks if the pass is currently ignored.
 *
 * @return true if pass is ignored, false otherwise.
 */
bool MessageFilter::isPassListIgnored() const {
  THEENGS_LOG_VERBOSE(F("MessageFilter: Checking if pass list is ignored: %s." CR), ignoreWhitelist ? "true" : "false");
  return ignoreWhitelist;
}

/**
 * @brief Sets whether to ignore the pass.
 *
 * @param ignore true to ignore pass, false to enforce it.
 */
void MessageFilter::ignorePassList(bool ignore) {
  THEENGS_LOG_VERBOSE(F("MessageFilter: Setting pass list ignore to %s." CR), ignore ? "true" : "false");
  ignoreWhitelist = ignore;
}

/**
 * @brief Checks if the block is currently ignored.
 *
 * @return true if block is ignored, false otherwise.
 */
bool MessageFilter::isBlockListIgnored() const {
  //THEENGS_LOG_VERBOSE(F("MessageFilter: Checking if block list is ignored: %s." CR), ignoreBlacklist ? "true" : "false");
  return ignoreBlacklist;
}

/**
 * @brief Sets whether to ignore the block.
 *
 * @param ignore true to ignore block, false to enforce it.
 */
void MessageFilter::ignoreBlockList(bool ignore) {
  //THEENGS_LOG_VERBOSE(F("MessageFilter: Setting block list ignore to %s." CR), ignore ? "true" : "false");
  ignoreBlacklist = ignore;
}

void MessageFilter::handleMQTTCommand(JsonObject& command) {
  THEENGS_LOG_VERBOSE(F("MessageFilter: Handling MQTT command." CR));

  if (command.containsKey("filter")) {
    JsonObject filter = command["filter"].as<JsonObject>();

    if (filter.containsKey("cmd")) {
      const char* action = filter["cmd"].as<const char*>();
      THEENGS_LOG_VERBOSE(F("MessageFilter: cmd specified: '%s'." CR), action ? action : "null");

      if (action && strcmp(action, "reset") == 0) {
        THEENGS_LOG_VERBOSE(F("MessageFilter: Reset all" CR));
        filters.clear();
        ignorePassList(true);
        ignoreBlockList(true);
        return;
      }

      // Future actions can be handled here
      if (action && strcmp(action, "clear") == 0) {
        THEENGS_LOG_VERBOSE(F("MessageFilter: clear all" CR));
        filters.clear();
        return;
      }

      if (action && strcmp(action, "persist") == 0) {
        filters.saveOnStorage();
        return;
      }

      if (action && strcmp(action, "reload") == 0) {
        filters.clear();
        filters.loadFromStorage();
        return;
      }

      if (action && strcmp(action, "purge") == 0) {
        filters.clear();
        filters.eraseStorage();
        return;
      }

      if (action && strcmp(action, "new") == 0) {
        THEENGS_LOG_VERBOSE(F("MessageFilter: new filters as per action." CR));
        filters.clear();
      }
    }

    if (filter.containsKey("rules")) {
      THEENGS_LOG_VERBOSE(F("MessageFilter: Loading rules from Message." CR));
      filters.fromRulesList(filter);
    }

    // Process pass filters
    if (filter.containsKey("pass")) {
      THEENGS_LOG_VERBOSE(F("MessageFilter: Processing pass filters." CR));
      JsonObject passFilter = filter["pass"].as<JsonObject>();

      for (JsonPair kv : passFilter) {
        const char* key = kv.key().c_str();

        if (!kv.value().is<JsonArray>()) {
          THEENGS_LOG_WARNING(F("MessageFilter: Pass filter for key '%s' is not an array" CR), key);
          continue;
        }

        JsonArray values = kv.value().as<JsonArray>();
        for (const char* value : values) {
          if (!filters.add(key, value, Filter::PASS)) {
            THEENGS_LOG_WARNING(F("MessageFilter: Failed to add pass filter for key '%s' with value '%s'" CR),
                                key, value);
            // Continue trying to add other filters even if one fails
          } else {
            THEENGS_LOG_VERBOSE(F("MessageFilter: Added pass filter for key '%s' with value '%s'" CR), key, value);
          }
        }
      }
    }

    // Process block filters
    if (filter.containsKey("block")) {
      THEENGS_LOG_VERBOSE(F("MessageFilter: Processing block filters." CR));
      JsonObject blockFilter = filter["block"].as<JsonObject>();

      for (JsonPair kv : blockFilter) {
        const char* key = kv.key().c_str();

        if (!kv.value().is<JsonArray>()) {
          THEENGS_LOG_WARNING(F("MessageFilter: Block filter for key '%s' is not an array" CR), key);
          continue;
        }

        JsonArray values = kv.value().as<JsonArray>();
        for (const char* value : values) {
          if (!filters.add(key, value, Filter::BLOCK)) {
            THEENGS_LOG_WARNING(F("MessageFilter: Failed to add block filter for key '%s' with value '%s'" CR),
                                key, value);
          } else {
            THEENGS_LOG_VERBOSE(F("MessageFilter: Added block filter for key '%s' with value '%s'" CR), key, value);
          }
        }
      }
    }

    // Process ignore flags
    if (filter.containsKey("ignore_pass")) {
      THEENGS_LOG_VERBOSE(F("MessageFilter: Setting ignore_pass to %s." CR),
                          filter["ignore_pass"].as<bool>() ? "true" : "false");
      ignorePassList(filter["ignore_pass"].as<bool>());
    }

    if (filter.containsKey("ignore_block")) {
      THEENGS_LOG_VERBOSE(F("MessageFilter: Setting ignore_block to %s." CR),
                          filter["ignore_block"].as<bool>() ? "true" : "false");
      ignoreBlockList(filter["ignore_block"].as<bool>());
    }

    THEENGS_LOG_VERBOSE(F("MessageFilter: Command handling complete. Total filters: %u/%u" CR),
                        filters.getTotalFilterCount(), MAX_TOTAL_FILTERS);
  } else {
    THEENGS_LOG_VERBOSE(F("MessageFilter: No filter configuration in command." CR));
    return;
  }
}

void MessageFilter::to(JsonObject& data) {
  THEENGS_LOG_VERBOSE(F("MessageFilter: Serializing to JSON." CR));
  JsonObject filterObj = data.createNestedObject("filter");

  // Serialize ignore flags
  filterObj["ignore_pass"] = ignoreWhitelist;
  filterObj["ignore_block"] = ignoreBlacklist;

  filters.toRulesList(filterObj); //retun as "rules" array more usable and human readable

  THEENGS_LOG_VERBOSE(F("MessageFilter: Serialization complete." CR));
}

bool MessageFilter::allowedTopic(const char* topic) {
  if (!topic) {
    THEENGS_LOG_WARNING(F("MessageFilter: allowedTopic called with null topic." CR));
    return false;
  }
  // Check block list first
  if (!ignoreBlacklist && !filters.isEmptyThe(Filter::BLOCK)) {
    if (filters.isTopicFilterPresent(topic, Filter::BLOCK)) {
      THEENGS_LOG_WARNING(F("MessageFilter: Topic '%s' is blocked." CR), topic);
      return false;
    }
  }

  // Check pass list - allow if ignored, empty, or topic matches
  if (ignoreWhitelist) {
    //THEENGS_LOG_TRACE(F("MessageFilter: Pass list ignored, allowing topic '%s'." CR), topic);
    return true;
  }

  if (filters.isEmptyThe(Filter::PASS)) {
    //THEENGS_LOG_TRACE(F("MessageFilter: Pass list empty, allowing topic '%s'." CR), topic);
    return true;
  }

  if (filters.isTopicFilterPresent(topic, Filter::PASS)) {
    //THEENGS_LOG_TRACE(F("MessageFilter: Topic '%s' found in pass list." CR), topic);
    return true;
  }

  // Topic not in pass list
  THEENGS_LOG_WARNING(F("MessageFilter: Topic '%s' is blocked, not in pass list." CR), topic);
  return false;
}