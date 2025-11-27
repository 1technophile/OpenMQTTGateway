#pragma once
#include <config_JSONMessages.h>

class IJsonable {
public:
  /**
   * Updates the object from a JSON object.
   *
   * @param data A reference to a JsonObject containing the data.
   */
  virtual void from(JsonObject& data) = 0;

  /**
   * Serializes the object to a JSON object.
   *
   * @param data A reference to a JsonObject where the object data will be serialized.
   */
  virtual void to(JsonObject& data) = 0;
};