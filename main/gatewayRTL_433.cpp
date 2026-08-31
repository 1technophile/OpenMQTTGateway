/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal
 - leverage the rtl_433 device decoders on a ESP32 device

    Copyright: (c)Florian ROBERT

    This file is part of OpenMQTTGateway.

    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "User_config.h"

#ifdef ZgatewayRTL_433
#  include <rtl_433_ESP.h>

#  include "TheengsCommon.h"
#  include "config_RF.h"
#  ifdef ZmqttDiscovery
#    include "config_mqttDiscovery.h"
#  endif

#  include <vector>

static char messageBuffer[JSON_MSG_BUFFER];
rtl_433_ESP rtl_433;

static const uint8_t RTL_433_WHITELIST_MAX = 5;
static const uint8_t RTL_433_SEEN_MAX = 30;
static const char* RTL_433_CONFIG_KEY = "RTL433Config";

bool RTL_433WhitelistEnabled = false;
char RTL_433Whitelist[RTL_433_WHITELIST_MAX][uniqueIdSize] = {{0}};

struct RTL_433seenDevice {
  char deviceId[uniqueIdSize];
  char modelName[modelNameSize];
  char type[typeSize];
};

std::vector<RTL_433seenDevice*> RTL_433seenDevices;

static String htmlEscape(const char* value) {
  String escaped = value;
  escaped.replace("&", "&amp;");
  escaped.replace("\"", "&quot;");
  escaped.replace("'", "&#39;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  return escaped;
}

static bool RTL_433Config_isWhitelisted(const char* id) {
  for (uint8_t i = 0; i < RTL_433_WHITELIST_MAX; i++) {
    if (RTL_433Whitelist[i][0] && strcmp(RTL_433Whitelist[i], id) == 0) {
      return true;
    }
  }
  return false;
}

static void RTL_433Config_storeSeenDevice(const char* id, const char* model, const char* type) {
  if (!id || !id[0]) {
    return;
  }
  for (std::vector<RTL_433seenDevice*>::iterator it = RTL_433seenDevices.begin(); it != RTL_433seenDevices.end(); ++it) {
    if (strcmp((*it)->deviceId, id) == 0) {
      return;
    }
  }
  if (RTL_433seenDevices.size() >= RTL_433_SEEN_MAX) {
    delete RTL_433seenDevices.front();
    RTL_433seenDevices.erase(RTL_433seenDevices.begin());
  }
  RTL_433seenDevice* device = new RTL_433seenDevice();
  strlcpy(device->deviceId, id, uniqueIdSize);
  strlcpy(device->modelName, model ? model : "", modelNameSize);
  strlcpy(device->type, type ? type : "", typeSize);
  RTL_433seenDevices.push_back(device);
}

void RTL_433Config_setWhitelistEnabled(bool enabled) {
  RTL_433WhitelistEnabled = enabled;
}

bool RTL_433Config_isWhitelistEnabled() {
  return RTL_433WhitelistEnabled;
}

void RTL_433Config_clearWhitelist() {
  for (uint8_t i = 0; i < RTL_433_WHITELIST_MAX; i++) {
    RTL_433Whitelist[i][0] = '\0';
  }
}

bool RTL_433Config_addWhitelistId(const char* id) {
  if (!id || !id[0]) {
    return false;
  }
  if (RTL_433Config_isWhitelisted(id)) {
    return true;
  }
  for (uint8_t i = 0; i < RTL_433_WHITELIST_MAX; i++) {
    if (!RTL_433Whitelist[i][0]) {
      strlcpy(RTL_433Whitelist[i], id, uniqueIdSize);
      return true;
    }
  }
  THEENGS_LOG_WARNING(F("[rtl_433] Whitelist is full, ignoring %s" CR), id);
  return false;
}

void RTL_433Config_addState(JsonObject& data) {
  data["rtl433wle"] = RTL_433WhitelistEnabled;
  for (uint8_t i = 0; i < RTL_433_WHITELIST_MAX; i++) {
    if (!RTL_433Whitelist[i][0]) {
      continue;
    }
    String key = "rtl433wl" + String(i + 1);
    data[key] = RTL_433Whitelist[i];
  }
}

bool RTL_433Config_fromJson(JsonObject& data) {
  bool updated = false;
  if (data.containsKey("rtl433wle") && data["rtl433wle"].is<bool>()) {
    bool enabled = data["rtl433wle"].as<bool>();
    if (RTL_433WhitelistEnabled != enabled) {
      RTL_433WhitelistEnabled = enabled;
      updated = true;
    }
  }
  for (uint8_t i = 0; i < RTL_433_WHITELIST_MAX; i++) {
    String key = "rtl433wl" + String(i + 1);
    if (data.containsKey(key) && data[key].is<const char*>()) {
      const char* newId = data[key].as<const char*>();
      if (strncmp(RTL_433Whitelist[i], newId, uniqueIdSize) != 0) {
        strlcpy(RTL_433Whitelist[i], newId, uniqueIdSize);
        updated = true;
      }
    }
  }
  return updated;
}

bool RTL_433Config_save() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject data = jsonBuffer.to<JsonObject>();
  RTL_433Config_addState(data);
  String conf = "";
  serializeJson(jsonBuffer, conf);
  preferences.begin(Gateway_Short_Name, false);
  int result = preferences.putString(RTL_433_CONFIG_KEY, conf);
  preferences.end();
  THEENGS_LOG_NOTICE(F("[rtl_433] Config_save: %s, result: %d" CR), conf.c_str(), result);
  return result > 0;
}

bool RTL_433Config_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (!preferences.isKey(RTL_433_CONFIG_KEY)) {
    preferences.end();
    THEENGS_LOG_NOTICE(F("[rtl_433] Config not found" CR));
    return false;
  }
  auto error = deserializeJson(jsonBuffer, preferences.getString(RTL_433_CONFIG_KEY, "{}"));
  preferences.end();
  if (error) {
    THEENGS_LOG_ERROR(F("[rtl_433] Config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
    return false;
  }
  JsonObject data = jsonBuffer.as<JsonObject>();
  RTL_433Config_fromJson(data);
  THEENGS_LOG_NOTICE(F("[rtl_433] Config loaded" CR));
  return true;
}

String RTL_433Config_webWhitelist() {
  String html = "<p><label><input id='rwe' name='rwe' type='checkbox' ";
  html += RTL_433WhitelistEnabled ? "checked" : "";
  html += "> RTL_433 Whitelist enabled</label></p>";
  html += "<input type='hidden' name='rwlsave' value='1'>";
  html += "<p><b>RTL_433 Whitelist devices</b></p>";
  if (RTL_433seenDevices.empty()) {
    html += "<p>No RTL_433 devices seen yet</p>";
  }
  uint8_t index = 0;
  for (std::vector<RTL_433seenDevice*>::iterator it = RTL_433seenDevices.begin(); it != RTL_433seenDevices.end() && index < RTL_433_SEEN_MAX; ++it) {
    RTL_433seenDevice* device = *it;
    String escapedId = htmlEscape(device->deviceId);
    html += "<p><label><input type='checkbox' name='rwl";
    html += String(index);
    html += "' value='";
    html += escapedId;
    html += "' ";
    html += RTL_433Config_isWhitelisted(device->deviceId) ? "checked" : "";
    html += "> ";
    html += escapedId;
    if (device->modelName[0]) {
      html += " (";
      html += htmlEscape(device->modelName);
      html += ")";
    }
    html += "</label></p>";
    index++;
  }
  for (uint8_t i = 0; i < RTL_433_WHITELIST_MAX; i++) {
    if (!RTL_433Whitelist[i][0]) {
      continue;
    }
    bool alreadyListed = false;
    for (std::vector<RTL_433seenDevice*>::iterator it = RTL_433seenDevices.begin(); it != RTL_433seenDevices.end(); ++it) {
      if (strcmp((*it)->deviceId, RTL_433Whitelist[i]) == 0) {
        alreadyListed = true;
        break;
      }
    }
    if (alreadyListed) {
      continue;
    }
    String escapedId = htmlEscape(RTL_433Whitelist[i]);
    html += "<p><label><input type='checkbox' name='rwlm";
    html += String(i);
    html += "' value='";
    html += escapedId;
    html += "' checked> ";
    html += escapedId;
    html += " (not seen in this session)</label></p>";
  }
  return html;
}

#  ifdef ZmqttDiscovery
SemaphoreHandle_t semaphorecreateOrUpdateDeviceRTL_433;
std::vector<RTL_433device*> RTL_433devices;
int newRTL_433Devices = 0;

static RTL_433device NO_RTL_433_DEVICE_FOUND = {{0},
                                                {0},
                                                {0},
                                                {0},
                                                false};

RTL_433device* getDeviceById(const char* id); // Declared here to avoid pre-compilation issue (misplaced auto declaration by pio)
RTL_433device* getDeviceById(const char* id) {
  DISCOVERY_TRACE_LOG(F("getDeviceById %s" CR), id);

  for (std::vector<RTL_433device*>::iterator it = RTL_433devices.begin(); it != RTL_433devices.end(); ++it) {
    if ((strcmp((*it)->uniqueId, id) == 0)) {
      return *it;
    }
  }
  return &NO_RTL_433_DEVICE_FOUND;
}

void dumpRTL_433Devices() {
  for (std::vector<RTL_433device*>::iterator it = RTL_433devices.begin(); it != RTL_433devices.end(); ++it) {
    RTL_433device* p = *it;
    DISCOVERY_TRACE_LOG(F("uniqueId %s" CR), p->uniqueId);
    DISCOVERY_TRACE_LOG(F("deviceId %s" CR), p->deviceId);
    DISCOVERY_TRACE_LOG(F("modelName %s" CR), p->modelName);
    DISCOVERY_TRACE_LOG(F("type %s" CR), p->type);
    DISCOVERY_TRACE_LOG(F("isDisc %d" CR), p->isDisc);
  }
}

void createOrUpdateDeviceRTL_433(const char* id, const char* deviceId, const char* model, const char* type, uint8_t flags) {
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceRTL_433, pdMS_TO_TICKS(30000)) == pdFALSE) {
    THEENGS_LOG_ERROR(F("[rtl_433] semaphorecreateOrUpdateDeviceRTL_433 Semaphore NOT taken" CR));
    return;
  }

  RTL_433device* device = getDeviceById(id);
  if (device == &NO_RTL_433_DEVICE_FOUND) {
    DISCOVERY_TRACE_LOG(F("add %s" CR), id);
    //new device
    device = new RTL_433device();
    if (strlcpy(device->uniqueId, id, uniqueIdSize) > uniqueIdSize) {
      THEENGS_LOG_WARNING(F("[rtl_433] Device id %s exceeds available space" CR), id); // Remove from production release ?
    };
    if (strlcpy(device->deviceId, deviceId, uniqueIdSize) > uniqueIdSize) {
      THEENGS_LOG_WARNING(F("[rtl_433] Device base id %s exceeds available space" CR), deviceId); // Remove from production release ?
    };
    if (strlcpy(device->modelName, model, modelNameSize) > modelNameSize) {
      THEENGS_LOG_WARNING(F("[rtl_433] Device model %s exceeds available space" CR), model); // Remove from production release ?
    };
    if (strlcpy(device->type, type, typeSize) > typeSize) {
      THEENGS_LOG_WARNING(F("[rtl_433] Device type %s exceeds available space" CR), type); // Remove from production release ?
    }
    DISCOVERY_TRACE_LOG(F("[rtl_433] Device type is %s." CR), device->type); // Remove from production release ?
    device->isDisc = flags & device_flags_isDisc;
    RTL_433devices.push_back(device);
    newRTL_433Devices++;
  } else {
    DISCOVERY_TRACE_LOG(F("update %s" CR), id);

    if (flags & device_flags_isDisc) {
      device->isDisc = true;
    }
  }

  xSemaphoreGive(semaphorecreateOrUpdateDeviceRTL_433);
}

// This function always should be called from the main core as it generates direct mqtt messages
// When overrideDiscovery=true, we publish discovery messages of known RTL_433devices (even if no new)
void launchRTL_433Discovery(bool overrideDiscovery) {
  if (!overrideDiscovery && newRTL_433Devices == 0)
    return;
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceRTL_433, pdMS_TO_TICKS(QueueSemaphoreTimeOutLoop)) == pdFALSE) {
    THEENGS_LOG_ERROR(F("[rtl_433] semaphorecreateOrUpdateDeviceRTL_433 Semaphore NOT taken" CR));
    return;
  }
  newRTL_433Devices = 0;
  std::vector<RTL_433device*> localDevices = RTL_433devices;
  xSemaphoreGive(semaphorecreateOrUpdateDeviceRTL_433);
  for (std::vector<RTL_433device*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
    RTL_433device* pdevice = *it;
    DISCOVERY_TRACE_LOG(F("Device id %s" CR), pdevice->uniqueId);
    if (RTL_433WhitelistEnabled && !RTL_433Config_isWhitelisted(pdevice->deviceId)) {
      DISCOVERY_TRACE_LOG(F("Device skipped by whitelist %s" CR), pdevice->deviceId);
      continue;
    }
    // Do not launch discovery for the RTL_433devices already discovered (unless we have overrideDiscovery) or that are not unique by their MAC Address (Ibeacon, GAEN and Microsoft Cdp)
    if (overrideDiscovery || !isDiscovered(pdevice)) {
      size_t numRows = sizeof(parameters) / sizeof(parameters[0]);
      for (int i = 0; i < numRows; i++) {
        char deviceKeyParameter[25];
        memcpy(deviceKeyParameter, &pdevice->uniqueId[strlen(pdevice->uniqueId) - strlen(parameters[i][0])], strlen(parameters[i][0]));
        deviceKeyParameter[strlen(parameters[i][0])] = '\0';
        THEENGS_LOG_TRACE(F("deviceKeyParameter: %s" CR), deviceKeyParameter);

        if (strcmp(deviceKeyParameter, parameters[i][0]) == 0) {
          // Remove the key from the unique id to extract the device id
          String idWoKey = pdevice->uniqueId;
          idWoKey.remove(idWoKey.length() - (strlen(parameters[i][0]) + 1));
          DISCOVERY_TRACE_LOG(F("idWoKey %s" CR), idWoKey.c_str());
          String value_template = "";
          value_template = "{{ value_json." + String(parameters[i][0]) + " | is_defined }}";
          String topic = subjectRTL_433toMQTT;
#    if valueAsATopic
          // Remove the key from the unique id to extract the device id
          String idWoKeyAndModel = idWoKey;
          if (strcmp(pdevice->type, "null")) {
            idWoKeyAndModel.remove(0, (strlen(pdevice->modelName) + strlen(pdevice->type) + 1)); // type is present
            topic = topic + "/" + String(pdevice->type) + "/" + String(pdevice->modelName);
          } else {
            idWoKeyAndModel.remove(0, (strlen(pdevice->modelName)));
            topic = topic + "/" + String(pdevice->modelName);
          }
          DISCOVERY_TRACE_LOG(F("idWoKeyAndModel %s" CR), idWoKeyAndModel.c_str());
          idWoKeyAndModel.replace("-", "/");
          idWoKeyAndModel.replace("//", "/-");
          topic = topic + idWoKeyAndModel;
#    endif
          if (strcmp(parameters[i][0], "battery_ok") == 0) {
            if (strcmp(pdevice->modelName, "Govee-Water") == 0 || strcmp(pdevice->modelName, "Govee-Contact") == 0 || strcmp(pdevice->modelName, "Archos-TBH") == 0 || strcmp(pdevice->modelName, "FineOffset-WH31L") == 0 || strcmp(pdevice->modelName, "Fineoffset-WH45") == 0 || strcmp(pdevice->modelName, "Fineoffset-WN34") == 0 || strcmp(pdevice->modelName, "Fineoffset-WS80") == 0 || strcmp(pdevice->modelName, "Fineoffset-WH0290") == 0 || strcmp(pdevice->modelName, "Fineoffset-WH51") == 0 || strcmp(pdevice->modelName, "Kedsum-TH") == 0 || strcmp(pdevice->modelName, "AVE") == 0 || strcmp(pdevice->modelName, "TPMS") == 0) {
              value_template = "{{ (float(value_json." + String(parameters[i][0]) + ") * 100) | round(0) | is_defined }}";
              createDiscovery("sensor", //set Type
                              (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                              "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                              "", "", HASS_UNIT_PERCENT, //set,payload_on,payload_off,unit_of_meas,
                              0, //set  off_delay
                              "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                              (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                              stateClassMeasurement //State Class
              );
            } else {
              createDiscovery("binary_sensor", //set Type
                              (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                              "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                              "0", "1", "", //set,payload_on,payload_off,unit_of_meas,
                              0, //set  off_delay
                              "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                              (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                              "" //State Class
              );
            }
          } else if (strcmp(parameters[i][0], "tamper") == 0 || strcmp(parameters[i][0], "alarm") == 0 || strcmp(parameters[i][0], "motion") == 0) {
            createDiscovery("binary_sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "1", "0", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            0, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            "" //State Class
            );
          } else if (strcmp(parameters[i][0], "state") == 0 && (strcmp(pdevice->modelName, "Nexa-Security") == 0 || strcmp(pdevice->modelName, "Brennenstuhl-RCS2044") == 0 || strcmp(pdevice->modelName, "Proove-Security") == 0 || strcmp(pdevice->modelName, "Waveman-Switch") == 0)) {
            createDiscovery("binary_sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "ON", "OFF", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            0, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            "" //State Class
            );
          } else if (strcmp(parameters[i][0], "strike_count") == 0) {
            createDiscovery("sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "1", "0", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            0, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            stateClassTotalIncreasing //State Class
            );
          } else if (strcmp(parameters[i][0], "event") == 0 && strcmp(pdevice->modelName, "Govee-Water") == 0) { //the entity will detect Water Leak Event and go back to Off state after 60seconds
            createDiscovery("binary_sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "Water Leak", "", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            60, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "Govee", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            stateClassMeasurement //State Class
            );
          } else if (strcmp(pdevice->modelName, "Interlogix-Security") != 0) {
            createDiscovery("sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "", "", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                            0, //set  off_delay
                            "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                            (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                            stateClassMeasurement //State Class
            );
          }
          pdevice->isDisc = true; // we don't need the semaphore and all the search magic via createOrUpdateDevice
          dumpRTL_433Devices();
          break;
        }
      }
      if (!pdevice->isDisc) {
        DISCOVERY_TRACE_LOG(F("Device id %s was not discovered" CR), pdevice->uniqueId); // Remove from production release ?
      }
    } else {
      DISCOVERY_TRACE_LOG(F("Device already discovered or that doesn't require discovery %s" CR), pdevice->uniqueId);
    }
  }
}

void storeRTL_433Discovery(JsonObject& RFrtl_433_ESPdata, const char* model, const char* type, const char* uniqueid, const char* rawDeviceId) {
  //Sanitize model name
  String modelSanitized = model;
  modelSanitized.replace(" ", "_");
  modelSanitized.replace("/", "_");
  modelSanitized.replace(".", "_");
  modelSanitized.replace("&", "");

  //Sensors translation matrix for sensors that requires statistics by using stateClassMeasurement
  size_t numRows = sizeof(parameters) / sizeof(parameters[0]);

  for (int i = 0; i < numRows; i++) {
    if (RFrtl_433_ESPdata.containsKey(parameters[i][0])) {
      String key_id = String(uniqueid) + "-" + String(parameters[i][0]);
      createOrUpdateDeviceRTL_433((char*)key_id.c_str(), rawDeviceId, (char*)modelSanitized.c_str(), (char*)type, device_flags_init);
    }
  }
}
#  endif

void rtl_433_Callback(char* message) {
  DynamicJsonDocument jsonBuffer2(JSON_MSG_BUFFER);
  JsonObject RFrtl_433_ESPdata = jsonBuffer2.to<JsonObject>();
  auto error = deserializeJson(jsonBuffer2, message);
  if (error) {
    THEENGS_LOG_ERROR(F("[rtl_433] deserializeJson() failed: %s" CR), error.c_str());
    return;
  }

  // Hash the message payload excluding volatile fields (time, rssi, snr, noise_floor)
  // that change between retransmissions of the same physical signal. This replaces the
  // previous id+temperature_C approach which failed for devices lacking temperature_C.
  // Use an order-independent (additive) combination of per-key hashes since JsonObject
  // iteration order is not guaranteed to be consistent.
  unsigned long MQTTvalue = 0;
  for (JsonPair kv : RFrtl_433_ESPdata) {
    const char* key = kv.key().c_str();
    if (strcmp(key, "time") == 0 || strcmp(key, "time_ms") == 0 || strcmp(key, "rssi") == 0 ||
        strcmp(key, "snr") == 0 || strcmp(key, "noise_floor") == 0 || strcmp(key, "pulses") == 0 ||
        strcmp(key, "duration") == 0) {
      continue;
    }
    unsigned long kvHash = 0;
    for (int i = 0; key[i] != '\0'; i++)
      kvHash = kvHash * 31 + (unsigned char)key[i];
    kvHash = kvHash * 31 + 0xFF; // separator between key and value
    String val;
    serializeJson(kv.value(), val);
    for (unsigned int i = 0; i < val.length(); i++)
      kvHash = kvHash * 31 + (unsigned char)val[i];
    kvHash ^= kvHash >> 16;
    kvHash *= 0x85ebca6bUL;
    kvHash ^= kvHash >> 13;
    kvHash *= 0xc2b2ae35UL;
    kvHash ^= kvHash >> 16;
    MQTTvalue += kvHash; // additive combination is order-independent
  }
  String topic = subjectRTL_433toMQTT;
  String model = RFrtl_433_ESPdata["model"];
  String type = RFrtl_433_ESPdata["type"];
  String uniqueid;

  const char naming_keys[5][8] = {"type", "model", "subtype", "channel", "id"}; // from rtl_433_mqtt_hass.py
  size_t numRows = sizeof(naming_keys) / sizeof(naming_keys[0]);
  for (int i = 0; i < numRows; i++) {
    if (RFrtl_433_ESPdata.containsKey(naming_keys[i])) {
      if (uniqueid == 0) {
        uniqueid = RFrtl_433_ESPdata[naming_keys[i]].as<String>(); // Start of the unique id with the first key
      } else {
        uniqueid = uniqueid + "/" + RFrtl_433_ESPdata[naming_keys[i]].as<String>(); // Following keys
      }
    }
  }

#  if valueAsATopic
  topic = topic + "/" + uniqueid;
#  endif

  String rawDeviceId = uniqueid;
  uniqueid.replace("/", "-");

  DISCOVERY_TRACE_LOG(F("uniqueid: %s" CR), uniqueid.c_str());
  RTL_433Config_storeSeenDevice(rawDeviceId.c_str(), model.c_str(), type.c_str());
  if (RTL_433WhitelistEnabled && !RTL_433Config_isWhitelisted(rawDeviceId.c_str())) {
    THEENGS_LOG_TRACE(F("[rtl_433] Message skipped by whitelist: %s" CR), rawDeviceId.c_str());
    return;
  }
  if (!isAduplicateSignal(MQTTvalue)) {
#  ifdef ZmqttDiscovery
    if (SYSConfig.discovery)
      storeRTL_433Discovery(RFrtl_433_ESPdata, (char*)model.c_str(), (char*)type.c_str(), (char*)uniqueid.c_str(), (char*)rawDeviceId.c_str());
#  endif
    RFrtl_433_ESPdata["origin"] = (char*)topic.c_str();
    enqueueJsonObject(RFrtl_433_ESPdata);
    storeSignalValue(MQTTvalue);
  }
#  ifdef MEMORY_DEBUG
  THEENGS_LOG_TRACE(F("Post rtl_433_Callback: %d" CR), ESP.getFreeHeap());
#  endif
}

void setupRTL_433() {
  RTL_433Config_load();
  rtl_433.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);
#  ifdef ZmqttDiscovery
  semaphorecreateOrUpdateDeviceRTL_433 = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphorecreateOrUpdateDeviceRTL_433);
#  endif
  THEENGS_LOG_TRACE(F("gatewayRTL_433 command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoRFset);
  THEENGS_LOG_NOTICE(F("gatewayRTL_433 setup done " CR));
}

void RTL_433Loop() {
  rtl_433.loop();
}

extern void enableRTLreceive() {
  THEENGS_LOG_NOTICE(F("Enable RTL_433 Receiver: %FMhz" CR), iRFConfig.getFrequency());
  rtl_433.initReceiver(RF_MODULE_RECEIVER_GPIO, iRFConfig.getFrequency());
  rtl_433.enableReceiver();
}

extern void disableRTLreceive() {
  THEENGS_LOG_TRACE(F("disableRTLreceive" CR));
  rtl_433.disableReceiver();
}

extern int getRTLrssiThreshold() {
  return rtl_433.rssiThreshold;
}

extern int getRTLAverageRSSI() {
  return rtl_433.averageRssi;
}

extern int getRTLCurrentRSSI() {
  return rtl_433.currentRssi;
}

extern int getRTLMessageCount() {
  return rtl_433.messageCount;
}

#  if defined(RF_SX1276) || defined(RF_SX1278)
extern int getOOKThresh() {
  return rtl_433.OokFixedThreshold;
}
#  endif

#endif
