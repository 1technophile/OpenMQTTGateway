#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <main_utils.h>

class MQTTPublisher {
public:
  MQTTPublisher() {};
  bool enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc) {};
};

#endif // MQTT_PUBLISHER_H