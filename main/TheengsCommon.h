#pragma once

#include "User_config.h"

#define ARDUINOJSON_USE_LONG_LONG     1
#define ARDUINOJSON_ENABLE_STD_STRING 1
#include <ArduinoJson.h>
#include <ArduinoLog.h>

#if defined(ESP32)
#  include <Preferences.h>
extern Preferences preferences;
#endif

// Flags definition for white list, black list, discovery management
#define device_flags_init     0 << 0
#define device_flags_isDisc   1 << 0
#define device_flags_isWhiteL 1 << 1
#define device_flags_isBlackL 1 << 2
#define device_flags_connect  1 << 3
#define isWhite(device)       device->isWhtL
#define isBlack(device)       device->isBlkL
#define isDiscovered(device)  device->isDisc

enum GatewayState {
  WAITING_ONBOARDING,
  ONBOARDING,
  OFFLINE,
  NTWK_CONNECTED,
  BROKER_CONNECTED,
  PROCESSING,
  NTWK_DISCONNECTED,
  BROKER_DISCONNECTED,
  LOCAL_OTA_IN_PROGRESS,
  REMOTE_OTA_IN_PROGRESS,
  SLEEPING,
  ERROR
};

enum PowerMode {
  DEACTIVATED = -1,
  ALWAYS_ON,
  INTERVAL,
  ACTION
};

struct SYSConfig_s {
  bool mqtt; // if true the gateway will publish the received data on the MQTT broker
  bool serial; // if true the gateway will publish the received data on the SERIAL
  bool blufi; // if true the gateway will be accesible with blufi
  bool offline;
  bool discovery; // HA discovery convention
#ifdef LED_ADDRESSABLE
  int rgbbrightness; // brightness of the RGB LED
#endif
  enum PowerMode powerMode;
};

extern GatewayState gatewayState;
extern SYSConfig_s SYSConfig;
extern bool ready_to_sleep;
extern char mqtt_topic[];
extern char gateway_name[];
extern unsigned long lastDiscovery; // Time of the last discovery to trigger automaticaly to off after DiscoveryAutoOffTimer

extern bool enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc, int timeout);
extern bool enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc);
extern void buildTopicFromId(JsonObject& Jsondata, const char* origin);
extern bool pubMQTT(const char* topic, const char* payload);
extern bool pubMQTT(const char* topic, const char* payload, bool retainFlag);
extern bool pubMQTT(String topic, const char* payload);
extern bool pubMQTT(const char* topic, unsigned long payload);
extern bool pubMQTT(const char* topic, unsigned long long payload);
extern bool pubMQTT(const char* topic, String payload);
extern bool pubMQTT(String topic, String payload);
extern bool pubMQTT(String topic, int payload);
extern bool pubMQTT(String topic, unsigned long long payload);
extern bool pubMQTT(String topic, float payload);
extern bool pubMQTT(const char* topic, float payload);
extern bool pubMQTT(const char* topic, int payload);
extern bool pubMQTT(const char* topic, unsigned int payload);
extern bool pubMQTT(const char* topic, long payload);
extern bool pubMQTT(const char* topic, double payload);
extern bool pubMQTT(String topic, unsigned long payload);
extern unsigned long uptime();
extern bool cmpToMainTopic(const char*, const char*);
extern bool pub(const char*, const char*, bool);
extern bool pub(const char*, const char*);

template <typename T>
void Config_update(JsonObject& data, const char* key, T& var) {
  if (data.containsKey(key)) {
    if (var != data[key].as<T>()) {
      var = data[key].as<T>();
      Log.notice(F("Config %s changed to: %T" CR), key, data[key].as<T>());
    } else {
      Log.notice(F("Config %s unchanged, currently: %T" CR), key, data[key].as<T>());
    }
  }
}
