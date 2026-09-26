/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR/BLE signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send LORA signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received LORA signal

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

#ifdef ZgatewayLORA
#  include <TheengsUtils.h>
#  include <Wire.h>

#  include "TheengsCommon.h"
#  include "config_LORA.h"
#  include "config_mqttDiscovery.h"
#  include "lora/LoRaPayloadCodec.h"
#  include "lora/LoRaRadio.h"

#  define WIPHONE_MESSAGE_MAGIC   0x6c6d
#  define WIPHONE_MESSAGE_MIN_LEN sizeof(wiphone_message) - WIPHONE_MAX_MESSAGE_LEN
#  define WIPHONE_MAX_MESSAGE_LEN 230

LORAConfig_s LORAConfig;

bool LORAConfig_fromJson(JsonObject& LORAdata, LORAConfig_s& candidate);
void LORAConfig_fromJson(JsonObject& LORAdata);
bool LORAConfig_apply(const LORAConfig_s& candidate);
String stateLORAMeasures();

#  ifdef ZmqttDiscovery
#    include "config_mqttDiscovery.h"
extern void createDiscovery(const char* sensor_type,
                            const char* st_topic, const char* s_name, const char* unique_id,
                            const char* availability_topic, const char* device_class, const char* value_template,
                            const char* payload_on, const char* payload_off, const char* unit_of_meas,
                            int off_delay,
                            const char* payload_available, const char* payload_not_available, bool gateway_entity, const char* cmd_topic,
                            const char* device_name, const char* device_manufacturer, const char* device_model, const char* device_id, bool retainCmd,
                            const char* state_class, const char* state_off, const char* state_on, const char* enum_options, const char* command_template);

SemaphoreHandle_t semaphorecreateOrUpdateDeviceLORA;
std::vector<LORAdevice*> LORAdevices;
int newLORADevices = 0;

static LORAdevice NO_LORA_DEVICE_FOUND = {{0},
                                          0,
                                          false};

LORAdevice* getDeviceById(const char* id); // Declared here to avoid pre-compilation issue (misplaced auto declaration by pio)
LORAdevice* getDeviceById(const char* id) {
  THEENGS_LOG_TRACE(F("getDeviceById %s" CR), id);

  for (std::vector<LORAdevice*>::iterator it = LORAdevices.begin(); it != LORAdevices.end(); ++it) {
    if ((strcmp((*it)->uniqueId, id) == 0)) {
      return *it;
    }
  }
  return &NO_LORA_DEVICE_FOUND;
}

void dumpLORADevices() {
  for (std::vector<LORAdevice*>::iterator it = LORAdevices.begin(); it != LORAdevices.end(); ++it) {
    LORAdevice* p = *it;
    THEENGS_LOG_TRACE(F("uniqueId %s" CR), p->uniqueId);
    THEENGS_LOG_TRACE(F("modelName %s" CR), p->modelName);
    THEENGS_LOG_TRACE(F("isDisc %d" CR), p->isDisc);
  }
}

void createOrUpdateDeviceLORA(const char* id, const char* model, uint8_t flags) {
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceLORA, pdMS_TO_TICKS(30000)) == pdFALSE) {
    THEENGS_LOG_ERROR(F("[LORA] semaphorecreateOrUpdateDeviceLORA Semaphore NOT taken" CR));
    return;
  }

  LORAdevice* device = getDeviceById(id);
  if (device == &NO_LORA_DEVICE_FOUND) {
    THEENGS_LOG_TRACE(F("add %s" CR), id);
    //new device
    device = new LORAdevice();
    if (strlcpy(device->uniqueId, id, uniqueIdSize) > uniqueIdSize) {
      THEENGS_LOG_WARNING(F("[LORA] Device id %s exceeds available space" CR), id); // Remove from production release ?
    };
    if (strlcpy(device->modelName, model, modelNameSize) > modelNameSize) {
      THEENGS_LOG_WARNING(F("[LORA] Device model %s exceeds available space" CR), id); // Remove from production release ?
    };
    device->isDisc = flags & device_flags_isDisc;
    LORAdevices.push_back(device);
    newLORADevices++;
  } else {
    THEENGS_LOG_TRACE(F("update %s" CR), id);

    if (flags & device_flags_isDisc) {
      device->isDisc = true;
    }
  }

  xSemaphoreGive(semaphorecreateOrUpdateDeviceLORA);
}

// This function always should be called from the main core as it generates direct mqtt messages
// When overrideDiscovery=true, we publish discovery messages of known LORAdevices (even if no new)
void launchLORADiscovery(bool overrideDiscovery) {
  if (!overrideDiscovery && newLORADevices == 0)
    return;
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceLORA, pdMS_TO_TICKS(QueueSemaphoreTimeOutLoop)) == pdFALSE) {
    THEENGS_LOG_ERROR(F("[LORA] semaphorecreateOrUpdateDeviceLORA Semaphore NOT taken" CR));
    return;
  }
  newLORADevices = 0;
  std::vector<LORAdevice*> localDevices = LORAdevices;
  xSemaphoreGive(semaphorecreateOrUpdateDeviceLORA);
  for (std::vector<LORAdevice*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
    LORAdevice* pdevice = *it;
    THEENGS_LOG_TRACE(F("Device id %s" CR), pdevice->uniqueId);
    // Do not launch discovery for the LORAdevices already discovered (unless we have overrideDiscovery) or that are not unique by their MAC Address (Ibeacon, GAEN and Microsoft Cdp)
    if (overrideDiscovery || !isDiscovered(pdevice)) {
      size_t numRows = sizeof(LORAparameters) / sizeof(LORAparameters[0]);
      for (int i = 0; i < numRows; i++) {
        if (strstr(pdevice->uniqueId, LORAparameters[i][0]) != 0) {
          // Remove the key from the unique id to extract the device id
          String idWoKey = pdevice->uniqueId;
          idWoKey.remove(idWoKey.length() - (strlen(LORAparameters[i][0]) + 1));
          THEENGS_LOG_TRACE(F("idWoKey %s" CR), idWoKey.c_str());
          String value_template = "{{ value_json." + String(LORAparameters[i][0]) + " | is_defined }}";

          String topic = idWoKey;
          topic = String(subjectLORAtoMQTT) + "/" + topic;

          createDiscovery("sensor", //set Type
                          (char*)topic.c_str(), LORAparameters[i][1], pdevice->uniqueId, //set state_topic,name,uniqueId
                          "", LORAparameters[i][3], (char*)value_template.c_str(), //set availability_topic,device_class,value_template,
                          "", "", LORAparameters[i][2], //set,payload_on,payload_off,unit_of_meas,
                          0, //set  off_delay
                          "", "", false, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                          (char*)idWoKey.c_str(), "", pdevice->modelName, (char*)idWoKey.c_str(), false, // device name, device manufacturer, device model, device ID, retain
                          stateClassMeasurement //State Class
          );
          pdevice->isDisc = true; // we don't need the semaphore and all the search magic via createOrUpdateDevice
          dumpLORADevices();
          break;
        }
      }
      if (!pdevice->isDisc) {
        THEENGS_LOG_TRACE(F("Device id %s was not discovered" CR), pdevice->uniqueId); // Remove from production release ?
      }
    } else {
      THEENGS_LOG_TRACE(F("Device already discovered or that doesn't require discovery %s" CR), pdevice->uniqueId);
    }
  }
}

void storeLORADiscovery(JsonObject& RFLORA_ESPdata, const char* model, const char* uniqueid) {
  //Sanitize model name
  String modelSanitized = model;
  modelSanitized.replace(" ", "_");
  modelSanitized.replace("/", "_");
  modelSanitized.replace(".", "_");
  modelSanitized.replace("&", "");

  //Sensors translation matrix for sensors that requires statistics by using stateClassMeasurement
  size_t numRows = sizeof(LORAparameters) / sizeof(LORAparameters[0]);

  for (int i = 0; i < numRows; i++) {
    if (RFLORA_ESPdata.containsKey(LORAparameters[i][0])) {
      String key_id = String(uniqueid) + "-" + String(LORAparameters[i][0]);
      createOrUpdateDeviceLORA((char*)key_id.c_str(), (char*)modelSanitized.c_str(), device_flags_init);
    }
  }
}
#  endif

typedef struct __attribute__((packed)) {
  // WiPhone uses RadioHead library which has additional (unused) headers
  uint8_t rh_to;
  uint8_t rh_from;
  uint8_t rh_id;
  uint8_t rh_flags;
  uint16_t magic;
  uint32_t to;
  uint32_t from;
  char message[WIPHONE_MAX_MESSAGE_LEN];
} wiphone_message;

enum LORA_ID_NUM {
  UNKNOWN_DEVICE = -1,
  WIPHONE,
};
typedef enum LORA_ID_NUM LORA_ID_NUM;

/*
Try and determine device given the payload
 */
uint8_t _determineDevice(byte* packet, int packetSize) {
  // Check WiPhone header
  if (packetSize >= WIPHONE_MESSAGE_MIN_LEN && ((wiphone_message*)packet)->magic == WIPHONE_MESSAGE_MAGIC)
    return WIPHONE;

  // No matches
  return UNKNOWN_DEVICE;
}

/*
Try and determine device given the JSON type
 */
uint8_t _determineDevice(JsonObject& LORAdata) {
  const char* protocol_name = LORAdata["type"];

  // No type provided
  if (!protocol_name)
    return UNKNOWN_DEVICE;

  if (strcmp(protocol_name, "WiPhone") == 0)
    return WIPHONE;

  // No matches
  return UNKNOWN_DEVICE;
}

/*
Create JSON information from WiPhone packet
 */
boolean _WiPhonetoX(byte* packet, JsonObject& LORAdata) {
  // Decode the LoRa packet and send over MQTT
  wiphone_message* msg = (wiphone_message*)packet;

  // Set the header information
  char from[9] = {0};
  char to[9] = {0};
  snprintf(from, 9, "%06X", msg->from);
  snprintf(to, 9, "%06X", msg->to);

  // From and To are the last 3 octets from the WiPhone's ESP32 chip ID
  // Special case is 0x000000: "broadcast"
  LORAdata["from"] = from;
  LORAdata["to"] = to;

  LORAdata["message"] = msg->message;
  LORAdata["type"] = "WiPhone";
  return true;
}

/*
Create WiPhone packet from JSON
 */
boolean _MQTTtoWiPhone(JsonObject& LORAdata, uint8_t* packet, size_t capacity, size_t& packetLength) {
  // Prepare a LoRa packet to send to the WiPhone
  wiphone_message wiphonemsg = {};
  wiphonemsg.rh_to = 0xff;
  wiphonemsg.rh_from = 0xff;
  wiphonemsg.rh_id = 0x00;
  wiphonemsg.rh_flags = 0x00;

  wiphonemsg.magic = WIPHONE_MESSAGE_MAGIC;
  const char* from = LORAdata["from"];
  const char* to = LORAdata["to"];
  const char* message = LORAdata["message"];
  if (packet == nullptr || from == nullptr || to == nullptr || message == nullptr) {
    packetLength = 0;
    return false;
  }
  wiphonemsg.from = strtol(from, NULL, 16);
  wiphonemsg.to = strtol(to, NULL, 16);
  strlcpy(wiphonemsg.message, message, WIPHONE_MAX_MESSAGE_LEN);
  packetLength = strlen(wiphonemsg.message) + WIPHONE_MESSAGE_MIN_LEN + 1;
  if (packetLength > capacity || packetLength > 255) {
    packetLength = 0;
    return false;
  }
  memcpy(packet, &wiphonemsg, packetLength);
  return true;
}

bool buildLoRaPayload(JsonObject& LORAdata, uint8_t* packet, size_t capacity, size_t& packetLength) {
  const uint8_t deviceId = _determineDevice(LORAdata);
  if (deviceId == WIPHONE) {
    return _MQTTtoWiPhone(LORAdata, packet, capacity, packetLength);
  }

  const char* hex = LORAdata["hex"];
  if (hex != nullptr) return decodeLoRaHex(hex, packet, capacity, packetLength);

  const char* message = LORAdata["message"];
  return copyLoRaText(message, packet, capacity, packetLength);
}

void LORAConfig_defaults(LORAConfig_s& config) {
  config.frequency = LORA_BAND;
  config.txPower = LORA_TX_POWER;
  config.spreadingFactor = LORA_SPREADING_FACTOR;
  config.signalBandwidth = LORA_SIGNAL_BANDWIDTH;
  config.codingRateDenominator = LORA_CODING_RATE;
  config.preambleLength = LORA_PREAMBLE_LENGTH;
  config.syncWord = LORA_SYNC_WORD;
  config.crc = DEFAULT_CRC;
  config.invertIQ = INVERT_IQ;
  config.onlyKnown = LORA_ONLY_KNOWN;
}

void LORAConfig_init() {
  LORAConfig_defaults(LORAConfig);
}

bool LORAConfig_load(LORAConfig_s& candidate) {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (!preferences.isKey("LORAConfig")) {
    preferences.end();
    THEENGS_LOG_NOTICE(F("LORA Config not found" CR));
    return true;
  }
  const String storedConfig = preferences.getString("LORAConfig", "{}");
  preferences.end();

  auto error = deserializeJson(jsonBuffer, storedConfig);
  if (error) {
    THEENGS_LOG_ERROR(F("LORA Config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
    return false;
  }
  if (jsonBuffer.isNull()) {
    THEENGS_LOG_WARNING(F("LORA Config is null" CR));
    return false;
  }
  JsonObject jo = jsonBuffer.as<JsonObject>();
  if (!LORAConfig_fromJson(jo, candidate)) return false;
  THEENGS_LOG_NOTICE(F("LORA Config loaded" CR));
  return true;
}

uint8_t hexStringToByte(const String& hexString) {
  return static_cast<uint8_t>(strtol(hexString.c_str(), NULL, 16));
}

bool LORAConfig_apply(const LORAConfig_s& candidate) {
  if (OMGLoRaRadio.apply(candidate)) return true;
  THEENGS_LOG_ERROR(F("[LORA] Configuration failed: %d" CR), OMGLoRaRadio.lastError());
  return false;
}

bool LORAConfig_fromJson(JsonObject& LORAdata, LORAConfig_s& candidate) {
  Config_update(LORAdata, "frequency", candidate.frequency);
  Config_update(LORAdata, "txpower", candidate.txPower);
  Config_update(LORAdata, "spreadingfactor", candidate.spreadingFactor);
  Config_update(LORAdata, "signalbandwidth", candidate.signalBandwidth);
  Config_update(LORAdata, "codingrate", candidate.codingRateDenominator);
  Config_update(LORAdata, "preamblelength", candidate.preambleLength);
  Config_update(LORAdata, "onlyknown", candidate.onlyKnown);
  // Handle syncword separately as it requires hex string conversion
  if (LORAdata.containsKey("syncword")) {
    String syncWordStr = LORAdata["syncword"].as<String>();
    uint8_t newSyncWord = hexStringToByte(syncWordStr);
    if (newSyncWord != candidate.syncWord) {
      candidate.syncWord = newSyncWord;
      THEENGS_LOG_NOTICE(F("Config syncword changed: %d" CR), candidate.syncWord);
    } else {
      THEENGS_LOG_NOTICE(F("Config syncword unchanged, currently: %d" CR), candidate.syncWord);
    }
  }
  Config_update(LORAdata, "enablecrc", candidate.crc);
  Config_update(LORAdata, "invertiq", candidate.invertIQ);

#  if defined(LORA_RADIO_SX1262)
  if (!validateSX1262Config(candidate)) {
    THEENGS_LOG_ERROR(F("[LORA] Invalid SX1262 configuration rejected" CR));
    return false;
  }
#  endif
  return true;
}

void LORAConfig_persist(const LORAConfig_s& config, bool erase, bool save) {
  if (erase) {
    // Erase config from NVS (non-volatile storage)
    preferences.begin(Gateway_Short_Name, false);
    if (preferences.isKey("LORAConfig")) {
      int result = preferences.remove("LORAConfig");
      THEENGS_LOG_NOTICE(F("LORA config erase result: %d" CR), result);
    } else {
      THEENGS_LOG_NOTICE(F("LORA config not found" CR));
    }
    preferences.end();
    return; // Erase prevails on save, so skipping save
  }
  if (save) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject jo = jsonBuffer.to<JsonObject>();
    jo["frequency"] = config.frequency;
    jo["txpower"] = config.txPower;
    jo["spreadingfactor"] = config.spreadingFactor;
    jo["signalbandwidth"] = config.signalBandwidth;
    jo["codingrate"] = config.codingRateDenominator;
    jo["preamblelength"] = config.preambleLength;
    char syncWordHex[5];
    snprintf(syncWordHex, sizeof(syncWordHex), "0x%02X", config.syncWord);
    jo["syncword"] = syncWordHex;
    jo["enablecrc"] = config.crc;
    jo["invertiq"] = config.invertIQ;
    jo["onlyknown"] = config.onlyKnown;
    // Save config into NVS (non-volatile storage)
    String conf = "";
    serializeJson(jsonBuffer, conf);
    preferences.begin(Gateway_Short_Name, false);
    int result = preferences.putString("LORAConfig", conf);
    preferences.end();
    THEENGS_LOG_NOTICE(F("LORA Config_save: %s, result: %d" CR), conf.c_str(), result);
  }
}

bool hasLORAConfig(JsonObject& LORAdata) {
  return LORAdata.containsKey("frequency") || LORAdata.containsKey("txpower") ||
         LORAdata.containsKey("spreadingfactor") || LORAdata.containsKey("signalbandwidth") ||
         LORAdata.containsKey("codingrate") || LORAdata.containsKey("preamblelength") ||
         LORAdata.containsKey("syncword") || LORAdata.containsKey("enablecrc") ||
         LORAdata.containsKey("invertiq") || LORAdata.containsKey("onlyknown") ||
         LORAdata.containsKey("init") || LORAdata.containsKey("load") ||
         LORAdata.containsKey("erase") || LORAdata.containsKey("save");
}

bool LORAConfig_update(JsonObject& LORAdata) {
  const LORAConfig_s previous = LORAConfig;
  LORAConfig_s candidate = previous;

  if (LORAdata.containsKey("init") && LORAdata["init"].as<bool>()) {
    LORAConfig_defaults(candidate);
  } else if (LORAdata.containsKey("load") && LORAdata["load"].as<bool>()) {
    if (!LORAConfig_load(candidate)) return false;
  }

  if (!LORAConfig_fromJson(LORAdata, candidate)) return false;
  if (!LORAConfig_apply(candidate)) {
    if (OMGLoRaRadio.ready()) LORAConfig_apply(previous);
    return false;
  }

  LORAConfig = candidate;
  LORAConfig_persist(candidate,
                     LORAdata["erase"] | false,
                     LORAdata["save"] | false);
  return true;
}

// Preserve the public hook used by webUI.cpp while keeping validation and
// persistence transactional inside the gateway.
void LORAConfig_fromJson(JsonObject& LORAdata) {
  LORAConfig_update(LORAdata);
}

void setupLORA() {
  LORAConfig_init();
  LORAConfig_s loadedConfig = LORAConfig;
  if (LORAConfig_load(loadedConfig)) LORAConfig = loadedConfig;
#  ifdef ZmqttDiscovery
  semaphorecreateOrUpdateDeviceLORA = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphorecreateOrUpdateDeviceLORA);
#  endif
  THEENGS_LOG_NOTICE(F("LORA Frequency: %d" CR), LORAConfig.frequency);
  if (!OMGLoRaRadio.begin(LORAConfig)) {
    THEENGS_LOG_ERROR(F("gatewayLORA %s setup failed: %d" CR), OMGLoRaRadio.family(), OMGLoRaRadio.lastError());
    return;
  }
  THEENGS_LOG_NOTICE(F("LORA_SCK: %d" CR), LORA_SCK);
  THEENGS_LOG_NOTICE(F("LORA_MISO: %d" CR), LORA_MISO);
  THEENGS_LOG_NOTICE(F("LORA_MOSI: %d" CR), LORA_MOSI);
  THEENGS_LOG_NOTICE(F("LORA_SS: %d" CR), LORA_SS);
  THEENGS_LOG_NOTICE(F("LORA_RST: %d" CR), LORA_RST);
  THEENGS_LOG_NOTICE(F("LORA_DIO0: %d" CR), LORA_DIO0);
  THEENGS_LOG_NOTICE(F("LORA_DIO1: %d" CR), LORA_DIO1);
  THEENGS_LOG_NOTICE(F("LORA radio: %s" CR), OMGLoRaRadio.family());
  THEENGS_LOG_TRACE(F("gatewayLORA setup done" CR));
}

void LORAtoX() {
  uint8_t packet[256] = {};
  size_t packetSize = 0;
  LoRaPacketMetrics metrics;
  if (OMGLoRaRadio.receive(packet, sizeof(packet) - 1, packetSize, metrics)) {
    StaticJsonDocument<JSON_MSG_BUFFER> LORAdataBuffer;
    JsonObject LORAdata = LORAdataBuffer.to<JsonObject>();
    THEENGS_LOG_TRACE(F("Rcv. LORA" CR));
#  ifdef ESP32
    String taskMessage = "LORA Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
    //trc(taskMessage);
#  endif
    boolean binary = false;
    for (size_t i = 0; i < packetSize; i++) {
      if (packet[i] < 32 || packet[i] > 127)
        binary = true;
    }
    // Terminate with a null character in case we have a string
    packet[packetSize] = 0;
    uint8_t deviceId = _determineDevice(packet, static_cast<int>(packetSize));
    if (deviceId == WIPHONE) {
      _WiPhonetoX(packet, LORAdata);
    } else if (binary) {
      if (LORAConfig.onlyKnown) {
        THEENGS_LOG_TRACE(F("Ignoring non identifiable packet" CR));
        return;
      }
      // We have non-ascii data: create hex string of the data
      char hex[511];
      TheengsUtils::_rawToHex(packet, hex, static_cast<int>(packetSize));
      // Terminate with a null character
      hex[packetSize * 2] = 0;

      LORAdata["hex"] = hex;
    } else {
      // ascii payload
      std::string packetStrStd = (char*)packet;
      auto result = deserializeJson(LORAdataBuffer, packetStrStd);
      if (result) {
        THEENGS_LOG_NOTICE(F("LORA packet deserialization failed, not a json, sending raw message" CR));
        LORAdata = LORAdataBuffer.to<JsonObject>();
        LORAdata["message"] = (char*)packet;
      } else {
        THEENGS_LOG_TRACE(F("LORA packet deserialization OK" CR));
      }
    }

    LORAdata["rssi"] = static_cast<int>(metrics.rssi);
    LORAdata["snr"] = metrics.snr;
    LORAdata["pferror"] = metrics.frequencyError;
    LORAdata["packetSize"] = (int)packetSize;

    if (LORAdata.containsKey("id")) {
      std::string id = LORAdata["id"];
      id.erase(std::remove(id.begin(), id.end(), ':'), id.end());
#  ifdef ZmqttDiscovery
      if (SYSConfig.discovery) {
        if (!LORAdata.containsKey("model"))
          LORAdataBuffer["model"] = "LORA_NODE";
        storeLORADiscovery(LORAdata, LORAdata["model"].as<char*>(), id.c_str());
      }
#  endif
      buildTopicFromId(LORAdata, subjectLORAtoMQTT);
    } else {
      LORAdataBuffer["origin"] = subjectLORAtoMQTT;
    }

    enqueueJsonObject(LORAdata);
    if (repeatLORAwMQTT) {
      THEENGS_LOG_TRACE(F("Pub LORA for rpt" CR));
      LORAdata["origin"] = subjectMQTTtoLORA;
      enqueueJsonObject(LORAdata);
    }
  }
}

#  if jsonReceiving
void XtoLORA(const char* topicOri, JsonObject& LORAdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORA)) {
    THEENGS_LOG_TRACE(F("MQTTtoLORA json" CR));
    const char* message = LORAdata["message"];
    const char* hex = LORAdata["hex"];
    if (hasLORAConfig(LORAdata) && !LORAConfig_update(LORAdata)) return;
    if (message || hex) {
      uint8_t packet[255] = {};
      size_t packetLength = 0;
      if (!buildLoRaPayload(LORAdata, packet, sizeof(packet), packetLength)) {
        THEENGS_LOG_ERROR(F("MQTTtoLORA payload invalid or longer than 255 bytes" CR));
        return;
      }
      if (!OMGLoRaRadio.transmit(packet, packetLength)) {
        THEENGS_LOG_ERROR(F("MQTTtoLORA transmit failed: %d" CR), OMGLoRaRadio.lastError());
        return;
      }
      THEENGS_LOG_TRACE(F("MQTTtoLORA OK" CR));
      // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      LORAdata["origin"] = subjectGTWLORAtoMQTT;
      enqueueJsonObject(LORAdata);
    } else {
      THEENGS_LOG_ERROR(F("MQTTtoLORA Fail json" CR));
    }
  }
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORAset)) {
    THEENGS_LOG_TRACE(F("MQTTtoLORA json set" CR));
    LORAConfig_update(LORAdata);
    stateLORAMeasures();
  }
}
#  endif
#  if simpleReceiving
void XtoLORA(const char* topicOri, const char* LORAarray) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORA)) {
    uint8_t packet[255] = {};
    size_t packetLength = 0;
    if (!copyLoRaText(LORAarray, packet, sizeof(packet), packetLength) ||
        !OMGLoRaRadio.transmit(packet, packetLength)) {
      THEENGS_LOG_ERROR(F("MQTTtoLORA transmit failed: %d" CR), OMGLoRaRadio.lastError());
      return;
    }
    THEENGS_LOG_NOTICE(F("MQTTtoLORA OK" CR));
    // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    pub(subjectGTWLORAtoMQTT, LORAarray);
  }
}
#  endif
String stateLORAMeasures() {
  //Publish LORA state
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject LORAdata = jsonBuffer.to<JsonObject>();
  LORAdata["frequency"] = LORAConfig.frequency;
  LORAdata["txpower"] = LORAConfig.txPower;
  LORAdata["spreadingfactor"] = LORAConfig.spreadingFactor;
  LORAdata["signalbandwidth"] = LORAConfig.signalBandwidth;
  LORAdata["codingrate"] = LORAConfig.codingRateDenominator;
  LORAdata["preamblelength"] = LORAConfig.preambleLength;
  char syncWordHex[5];
  snprintf(syncWordHex, sizeof(syncWordHex), "0x%02X", LORAConfig.syncWord);
  LORAdata["syncword"] = syncWordHex;
  LORAdata["enablecrc"] = LORAConfig.crc;
  LORAdata["invertiq"] = LORAConfig.invertIQ;
  LORAdata["onlyknown"] = LORAConfig.onlyKnown;
  LORAdata["origin"] = subjectGTWLORAtoMQTT;
  enqueueJsonObject(LORAdata);

  String output;
  serializeJson(LORAdata, output);
  return output;
}
#endif
