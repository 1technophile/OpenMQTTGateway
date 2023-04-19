/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
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
#ifdef ZgatewayRTL_433

#  include <ArduinoJson.h>
#  include <config_RF.h>
#  include <rtl_433_ESP.h>

#  include "ArduinoLog.h"
#  include "User_config.h"
#  ifdef ZmqttDiscovery
#    include "config_mqttDiscovery.h"
#  endif

char messageBuffer[JSON_MSG_BUFFER];

rtl_433_ESP rtl_433;

#  ifdef ZmqttDiscovery
SemaphoreHandle_t semaphorecreateOrUpdateDeviceRTL_433;
std::vector<RTL_433device*> RTL_433devices;
int newRTL_433Devices = 0;

static RTL_433device NO_RTL_433_DEVICE_FOUND = {{0},
                                                0,
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
    DISCOVERY_TRACE_LOG(F("modelName %s" CR), p->modelName);
    DISCOVERY_TRACE_LOG(F("isDisc %d" CR), p->isDisc);
  }
}

void createOrUpdateDeviceRTL_433(const char* id, const char* model, uint8_t flags) {
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceRTL_433, pdMS_TO_TICKS(30000)) == pdFALSE) {
    Log.error(F("[rtl_433] semaphorecreateOrUpdateDeviceRTL_433 Semaphore NOT taken" CR));
    return;
  }

  RTL_433device* device = getDeviceById(id);
  if (device == &NO_RTL_433_DEVICE_FOUND) {
    DISCOVERY_TRACE_LOG(F("add %s" CR), id);
    //new device
    device = new RTL_433device();
    if (strlcpy(device->uniqueId, id, uniqueIdSize) > uniqueIdSize) {
      Log.warning(F("[rtl_433] Device id %s exceeds available space" CR), id); // Remove from production release ?
    };
    if (strlcpy(device->modelName, model, modelNameSize) > modelNameSize) {
      Log.warning(F("[rtl_433] Device model %s exceeds available space" CR), id); // Remove from production release ?
    };
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
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceRTL_433, pdMS_TO_TICKS(1000)) == pdFALSE) {
    Log.error(F("[rtl_433] semaphorecreateOrUpdateDeviceRTL_433 Semaphore NOT taken" CR));
    return;
  }
  newRTL_433Devices = 0;
  std::vector<RTL_433device*> localDevices = RTL_433devices;
  xSemaphoreGive(semaphorecreateOrUpdateDeviceRTL_433);
  for (std::vector<RTL_433device*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
    RTL_433device* pdevice = *it;
    DISCOVERY_TRACE_LOG(F("Device id %s" CR), pdevice->uniqueId);
    // Do not launch discovery for the RTL_433devices already discovered (unless we have overrideDiscovery) or that are not unique by their MAC Address (Ibeacon, GAEN and Microsoft Cdp)
    if (overrideDiscovery || !isDiscovered(pdevice)) {
      size_t numRows = sizeof(parameters) / sizeof(parameters[0]);
      for (int i = 0; i < numRows; i++) {
        if (strstr(pdevice->uniqueId, parameters[i][0]) != 0) {
          // Remove the key from the unique id to extract the device id
          String idWoKey = pdevice->uniqueId;
          idWoKey.remove(idWoKey.length() - (strlen(parameters[i][0]) + 1));
          DISCOVERY_TRACE_LOG(F("idWoKey %s" CR), idWoKey.c_str());
#    if OpenHABDiscovery
          String value_template = "{{ value_json." + String(parameters[i][0]) + "}}";
          if (strcmp(parameters[i][0], "battery_ok") == 0)) {
              value_template = "{{ float(value_json." + String(parameters[i][0]) + ") * 99 + 1 }}"; // https://github.com/merbanan/rtl_433/blob/93f0f30c28cfb6b82b8cc3753415b01a85bee91d/examples/rtl_433_mqtt_hass.py#L187
            }
#    else
          String value_template = "{{ value_json." + String(parameters[i][0]) + " | is_defined }}";
          if (strcmp(parameters[i][0], "battery_ok") == 0) {
            value_template = "{{ float(value_json." + String(parameters[i][0]) + ") * 99 + 1 | is_defined }}"; // https://github.com/merbanan/rtl_433/blob/93f0f30c28cfb6b82b8cc3753415b01a85bee91d/examples/rtl_433_mqtt_hass.py#L187
          }
#    endif
          String topic = subjectRTL_433toMQTT;
#    if valueAsATopic
          // Remove the key from the unique id to extract the device id
          String idWoKeyAndModel = idWoKey;
          idWoKeyAndModel.remove(0, strlen(pdevice->modelName));
          idWoKeyAndModel.replace("-", "/");
          DISCOVERY_TRACE_LOG(F("idWoKeyAndModel %s" CR), idWoKeyAndModel.c_str());
          topic = topic + "/" + String(pdevice->modelName) + idWoKeyAndModel;
#    endif
          if (strcmp(parameters[i][0], "tamper") == 0 || strcmp(parameters[i][0], "alarm") == 0 || strcmp(parameters[i][0], "motion") == 0) {
            createDiscovery("binary_sensor", //set Type
                            (char*)topic.c_str(), parameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                            "", parameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                            "1", "0", parameters[i][2], //set,payload_on,payload_off,unit_of_meas,
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

void storeRTL_433Discovery(JsonObject& RFrtl_433_ESPdata, const char* model, const char* uniqueid) {
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
      createOrUpdateDeviceRTL_433((char*)key_id.c_str(), (char*)modelSanitized.c_str(), device_flags_init);
    }
  }
}
#  endif

void rtl_433_Callback(char* message) {
  DynamicJsonDocument jsonBuffer2(JSON_MSG_BUFFER);
  JsonObject RFrtl_433_ESPdata = jsonBuffer2.to<JsonObject>();
  auto error = deserializeJson(jsonBuffer2, message);
  if (error) {
    Log.error(F("[rtl_433] deserializeJson() failed: %s" CR), error.c_str());
    return;
  }

  unsigned long MQTTvalue = (int)RFrtl_433_ESPdata["id"] + round((float)RFrtl_433_ESPdata["temperature_C"]);
  String topic = subjectRTL_433toMQTT;
  String model = RFrtl_433_ESPdata["model"];
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

  uniqueid.replace("/", "-");

  // Log.notice(F("uniqueid: %s" CR), uniqueid.c_str());
  if (!isAduplicateSignal(MQTTvalue)) {
#  ifdef ZmqttDiscovery
    if (disc)
      storeRTL_433Discovery(RFrtl_433_ESPdata, (char*)model.c_str(), (char*)uniqueid.c_str());
#  endif
    pub((char*)topic.c_str(), RFrtl_433_ESPdata);
    storeSignalValue(MQTTvalue);
    pubOled((char*)topic.c_str(), RFrtl_433_ESPdata);
  }
#  ifdef MEMORY_DEBUG
  Log.trace(F("Post rtl_433_Callback: %d" CR), ESP.getFreeHeap());
#  endif
}

void setupRTL_433() {
  rtl_433.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);
#  ifdef ZmqttDiscovery
  semaphorecreateOrUpdateDeviceRTL_433 = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphorecreateOrUpdateDeviceRTL_433);
#  endif
  Log.trace(F("ZgatewayRTL_433 command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoRTL_433);
  Log.notice(F("ZgatewayRTL_433 setup done " CR));
}

void RTL_433Loop() {
  rtl_433.loop();
}

extern void MQTTtoRTL_433(char* topicOri, JsonObject& RTLdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRTL_433)) {
    float tempMhz = RTLdata["mhz"];
    bool success = false;
    if (RTLdata.containsKey("mhz") && validFrequency(tempMhz)) {
      receiveMhz = tempMhz;
      Log.notice(F("RTL_433 Receive mhz: %F" CR), receiveMhz);
      success = true;
    }
    if (RTLdata.containsKey("active")) {
      Log.trace(F("RTL_433 active:" CR));
      activeReceiver = ACTIVE_RTL; // Enable RTL_433 Gateway
      success = true;
    }
    if (RTLdata.containsKey("rssi")) {
      int rssiThreshold = RTLdata["rssi"] | 0;
      Log.notice(F("RTL_433 RSSI Threshold Delta: %d " CR), rssiThreshold);
      rtl_433.setRSSIThreshold(rssiThreshold);
      success = true;
    }
#  if defined(RF_SX1276) || defined(RF_SX1278)
    if (RTLdata.containsKey("ookThreshold")) {
      int newOokThreshold = RTLdata["ookThreshold"] | 0;
      Log.notice(F("RTL_433 ookThreshold %d" CR), newOokThreshold);
      rtl_433.setOOKThreshold(newOokThreshold);
      success = true;
    }
#  endif
    if (RTLdata.containsKey("debug")) {
      int debug = RTLdata["debug"] | -1;
      Log.notice(F("RTL_433 set debug: %d" CR), debug);
      // rtl_433.setDebug(debug);
      rtl_433.initReceiver(RF_MODULE_RECEIVER_GPIO, receiveMhz);
      success = true;
    }
    if (RTLdata.containsKey("status")) {
      Log.notice(F("RTL_433 get status:" CR));
      rtl_433.getStatus();
      success = true;
    }
    if (success) {
      pub(subjectRTL_433toMQTT, RTLdata);
    } else {
      pub(subjectRTL_433toMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("[rtl_433] MQTTtoRTL_433 Fail json" CR));
    }
    enableActiveReceiver(false);
  }
}

extern void enableRTLreceive() {
  Log.notice(F("Switching to RTL_433 Receiver: %FMhz" CR), receiveMhz);
#  ifdef ZgatewayRF
  disableRFReceive();
#  endif
#  ifdef ZgatewayRF2
  disableRF2Receive();
#  endif
#  ifdef ZgatewayPilight
  disablePilightReceive();
#  endif

  rtl_433.initReceiver(RF_MODULE_RECEIVER_GPIO, receiveMhz);
  rtl_433.enableReceiver();
}

extern void disableRTLreceive() {
  Log.trace(F("disableRTLreceive" CR));
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
