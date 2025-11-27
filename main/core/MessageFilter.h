#pragma once
#include <config_JSONMessages.h>
#include <core/Filter.h>

class MessageFilter : public IJsonable {
public:
  MessageFilter(Filter& filterConfig) : filters(filterConfig) {}

  virtual ~MessageFilter() {}

  /**
 * @brief Checks if a given MQTT value is present in the block.
 *
 * This function determines whether the specified MQTT message is included
 * in the block defined in the  If the `ignoreBlacklist` 
 * flag in RFConfiguration is set to true, the function will always return false,
 * effectively bypassing the block check.
 *
 * @param MQTTvalue The MQTT value to check against the block.
 * @return true if the MQTT value is in the block and the block
 *         check is not ignored; false otherwise.
 */
  bool inBlockList(JsonObject& MQTTvalue);

  /**
 * @brief Checks if a given MQTT value is in the pass.
 *
 * This function determines whether the specified MQTT value is present
 * in the pass. If the pass is disabled (via the `ignoreWhitelist`
 * flag) or is empty, the function will always return true.
 *
 * @param MQTTvalue The MQTT value to check against the pass.
 * @return true if the pass is disabled, empty, or the value is found in the pass.
 * @return false if the value is not in the pass.
 */
  bool inPassList(JsonObject& MQTTvalue);

  /**
   * @brief Checks whether the topic is allowed to pass based on the filters.
   * 
   * @param topic The topic to be checked.
   * @return true if the topic is allowed to pass; false otherwise.
   */
  bool allowedTopic(const char* topic);

  /**
   * @brief Checks whether the pass should be ignored during message filtering.
   * 
   * @return true if the pass is set to be ignored; false otherwise.
   */
  bool isPassListIgnored() const;

  /**
   * @brief Sets whether the pass should be ignored during message filtering.
   * 
   * @param ignore If true, the pass will be ignored; otherwise, it will be applied.
   */
  void ignorePassList(bool ignore);

  /**
   * @brief Checks whether the block should be ignored during message filtering.
   * 
   * @return true if the block is set to be ignored; false otherwise.
   */
  bool isBlockListIgnored() const;

  /**
   * @brief Sets whether the block should be ignored during message filtering.
   * 
   * @param ignore If true, the block will be ignored; otherwise, it will be applied.
   */
  void ignoreBlockList(bool ignore);

  /**
   * @brief Handles an MQTT command to modify the filter configuration.
   * 
   * This function processes a JSON object representing an MQTT command
   * to update the filter settings, such as adding or removing entries
   * from the pass or block lists.
   * 
   * @param command A reference to a JsonObject containing the command details.
   */
  void handleMQTTCommand(JsonObject& command);

  /**
   * @brief Serializes the object to a JSON object.
   *
   * @param data A reference to a JsonObject where the object data will be serialized.
   */
  void to(JsonObject& data) override;

  /**
   * @brief Updates the object from a JSON object.
   *
   * @param data A reference to a JsonObject containing the data.
   */
  void from(JsonObject& data) { THEENGS_LOG_ERROR(F("MessageFilter::from() not available" CR)); };

private:
  Filter& filters;
  bool ignoreWhitelist = false;
  bool ignoreBlacklist = false;
};