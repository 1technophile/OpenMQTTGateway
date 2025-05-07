#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <main_utils.h>

#ifndef JSON_MSG_BUFFER
#  if defined(ESP32)
#    define JSON_MSG_BUFFER 1024 // adjusted to minimum size covering largest home assistant discovery messages
#    if MQTT_SECURE_DEFAULT
#      define JSON_MSG_BUFFER_MAX 2048 // Json message buffer size increased to handle certificate changes through MQTT, used for the queue and the coming MQTT messages
#    else
#      define JSON_MSG_BUFFER_MAX 1024 // Minimum size for the cover MQTT discovery message
#    endif
#  elif defined(ESP8266)
#    define JSON_MSG_BUFFER     512 // Json message max buffer size, don't put 768 or higher it is causing unexpected behaviour on ESP8266, certificates handling with ESP8266 is not tested
#    define JSON_MSG_BUFFER_MAX 832 // Minimum size for MQTT discovery message
#  endif
#endif

class MQTTPublisher {
public:
  MQTTPublisher() {};

  bool enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc) {};

  /**
 * @brief Compares a given topic string with a constructed topic string.
 *
 * This function checks if the provided `topicOri` string is equal to the 
 * concatenation of `mqtt_topic`, `gateway_name`, and `toAdd`. It first 
 * checks if `topicOri` is exactly equal to `toAdd`. If not, it then 
 * constructs a new string by concatenating `mqtt_topic`, `gateway_name`, 
 * and `toAdd`, and compares it with `topicOri`.
 *
 * @param topicOri The original topic string to compare.
 * @param toAdd The string to add to the constructed topic string.
 * @return true if `topicOri` matches the constructed topic string, false otherwise.
 */
  bool cmpToMainTopic(const char* topicOri, const char* toAdd) {}
};

#endif // MQTT_PUBLISHER_H