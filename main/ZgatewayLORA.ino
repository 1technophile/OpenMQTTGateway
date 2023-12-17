/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

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

#  include <LoRa.h>
#  include <SPI.h>
#  include <Wire.h>

#  define WIPHONE_MESSAGE_MAGIC   0x6c6d
#  define WIPHONE_MESSAGE_MIN_LEN sizeof(wiphone_message) - WIPHONE_MAX_MESSAGE_LEN
#  define WIPHONE_MAX_MESSAGE_LEN 230

LORAConfig_s LORAConfig;

#  ifdef ZmqttDiscovery
SemaphoreHandle_t semaphorecreateOrUpdateDeviceLORA;
std::vector<LORAdevice*> LORAdevices;
int newLORADevices = 0;

static LORAdevice NO_LORA_DEVICE_FOUND = {{0},
                                          0,
                                          false};

LORAdevice* getDeviceById(const char* id); // Declared here to avoid pre-compilation issue (misplaced auto declaration by pio)
LORAdevice* getDeviceById(const char* id) {
  Log.trace(F("getDeviceById %s" CR), id);

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
    Log.trace(F("uniqueId %s" CR), p->uniqueId);
    Log.trace(F("modelName %s" CR), p->modelName);
    Log.trace(F("isDisc %d" CR), p->isDisc);
  }
}

void createOrUpdateDeviceLORA(const char* id, const char* model, uint8_t flags) {
  if (xSemaphoreTake(semaphorecreateOrUpdateDeviceLORA, pdMS_TO_TICKS(30000)) == pdFALSE) {
    Log.error(F("[LORA] semaphorecreateOrUpdateDeviceLORA Semaphore NOT taken" CR));
    return;
  }

  LORAdevice* device = getDeviceById(id);
  if (device == &NO_LORA_DEVICE_FOUND) {
    Log.trace(F("add %s" CR), id);
    //new device
    device = new LORAdevice();
    if (strlcpy(device->uniqueId, id, uniqueIdSize) > uniqueIdSize) {
      Log.warning(F("[LORA] Device id %s exceeds available space" CR), id); // Remove from production release ?
    };
    if (strlcpy(device->modelName, model, modelNameSize) > modelNameSize) {
      Log.warning(F("[LORA] Device model %s exceeds available space" CR), id); // Remove from production release ?
    };
    device->isDisc = flags & device_flags_isDisc;
    LORAdevices.push_back(device);
    newLORADevices++;
  } else {
    Log.trace(F("update %s" CR), id);

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
    Log.error(F("[LORA] semaphorecreateOrUpdateDeviceLORA Semaphore NOT taken" CR));
    return;
  }
  newLORADevices = 0;
  std::vector<LORAdevice*> localDevices = LORAdevices;
  xSemaphoreGive(semaphorecreateOrUpdateDeviceLORA);
  for (std::vector<LORAdevice*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
    LORAdevice* pdevice = *it;
    Log.trace(F("Device id %s" CR), pdevice->uniqueId);
    // Do not launch discovery for the LORAdevices already discovered (unless we have overrideDiscovery) or that are not unique by their MAC Address (Ibeacon, GAEN and Microsoft Cdp)
    if (overrideDiscovery || !isDiscovered(pdevice)) {
      size_t numRows = sizeof(LORAparameters) / sizeof(LORAparameters[0]);
      for (int i = 0; i < numRows; i++) {
        if (strstr(pdevice->uniqueId, LORAparameters[i][0]) != 0) {
          // Remove the key from the unique id to extract the device id
          String idWoKey = pdevice->uniqueId;
          idWoKey.remove(idWoKey.length() - (strlen(LORAparameters[i][0]) + 1));
          Log.trace(F("idWoKey %s" CR), idWoKey.c_str());
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
        Log.trace(F("Device id %s was not discovered" CR), pdevice->uniqueId); // Remove from production release ?
      }
    } else {
      Log.trace(F("Device already discovered or that doesn't require discovery %s" CR), pdevice->uniqueId);
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
boolean _WiPhoneToMQTT(byte* packet, JsonObject& LORAdata) {
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
boolean _MQTTtoWiPhone(JsonObject& LORAdata) {
  // Prepare a LoRa packet to send to the WiPhone
  wiphone_message wiphonemsg;
  wiphonemsg.rh_to = 0xff;
  wiphonemsg.rh_from = 0xff;
  wiphonemsg.rh_id = 0x00;
  wiphonemsg.rh_flags = 0x00;

  wiphonemsg.magic = WIPHONE_MESSAGE_MAGIC;
  wiphonemsg.from = strtol(LORAdata["from"], NULL, 16);
  wiphonemsg.to = strtol(LORAdata["to"], NULL, 16);
  const char* message = LORAdata["message"];
  strlcpy(wiphonemsg.message, message, WIPHONE_MAX_MESSAGE_LEN);
  LoRa.write((uint8_t*)&wiphonemsg, strlen(message) + WIPHONE_MESSAGE_MIN_LEN + 1);
  return true;
}

void LORAConfig_init() {
  LORAConfig.frequency = LORA_BAND;
  LORAConfig.txPower = LORA_TX_POWER;
  LORAConfig.spreadingFactor = LORA_SPREADING_FACTOR;
  LORAConfig.signalBandwidth = LORA_SIGNAL_BANDWIDTH;
  LORAConfig.codingRateDenominator = LORA_CODING_RATE;
  LORAConfig.preambleLength = LORA_PREAMBLE_LENGTH;
  LORAConfig.syncWord = LORA_SYNC_WORD;
  LORAConfig.crc = DEFAULT_CRC;
  LORAConfig.invertIQ = INVERT_IQ;
  LORAConfig.onlyKnown = LORA_ONLY_KNOWN;
}

void LORAConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (preferences.isKey("LORAConfig")) {
    auto error = deserializeJson(jsonBuffer, preferences.getString("LORAConfig", "{}"));
    preferences.end();
    Log.notice(F("LORA Config loaded" CR));
    if (error) {
      Log.error(F("LORA Config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      return;
    }
    if (jsonBuffer.isNull()) {
      Log.warning(F("LORA Config is null" CR));
      return;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    LORAConfig_fromJson(jo);
    Log.notice(F("LORA Config loaded" CR));
  } else {
    preferences.end();
    Log.notice(F("LORA Config not found" CR));
  }
}

byte hexStringToByte(const String& hexString) {
  return (byte)strtol(hexString.c_str(), NULL, 16);
}

void LORAConfig_fromJson(JsonObject& LORAdata) {
  Config_update(LORAdata, "frequency", LORAConfig.frequency);
  Config_update(LORAdata, "txpower", LORAConfig.txPower);
  Config_update(LORAdata, "spreadingfactor", LORAConfig.spreadingFactor);
  Config_update(LORAdata, "signalbandwidth", LORAConfig.signalBandwidth);
  Config_update(LORAdata, "codingrate", LORAConfig.codingRateDenominator);
  Config_update(LORAdata, "preamblelength", LORAConfig.preambleLength);
  Config_update(LORAdata, "onlyknown", LORAConfig.onlyKnown);
  // Handle syncword separately
  if (LORAdata.containsKey("syncword")) {
    String syncWordStr = LORAdata["syncword"].as<String>();
    LORAConfig.syncWord = hexStringToByte(syncWordStr);
    Log.notice(F("Config syncword changed: %d" CR), LORAConfig.syncWord);
  }
  Config_update(LORAdata, "enablecrc", LORAConfig.crc);
  Config_update(LORAdata, "invertiq", LORAConfig.invertIQ);

  LoRa.setFrequency(LORAConfig.frequency);
  LoRa.setTxPower(LORAConfig.txPower);
  LoRa.setSpreadingFactor(LORAConfig.spreadingFactor);
  LoRa.setSignalBandwidth(LORAConfig.signalBandwidth);
  LoRa.setCodingRate4(LORAConfig.codingRateDenominator);
  LoRa.setPreambleLength(LORAConfig.preambleLength);
  LoRa.setSyncWord(LORAConfig.syncWord);
  LORAConfig.crc ? LoRa.enableCrc() : LoRa.disableCrc();
  LORAConfig.invertIQ ? LoRa.enableInvertIQ() : LoRa.disableInvertIQ();

  if (LORAdata.containsKey("erase") && LORAdata["erase"].as<bool>()) {
    // Erase config from NVS (non-volatile storage)
    preferences.begin(Gateway_Short_Name, false);
    if (preferences.isKey("LORAConfig")) {
      int result = preferences.remove("LORAConfig");
      Log.notice(F("LORA config erase result: %d" CR), result);
      preferences.end();
      return; // Erase prevails on save, so skipping save
    } else {
      Log.notice(F("LORA config not found" CR));
      preferences.end();
    }
  }
  if (LORAdata.containsKey("save") && LORAdata["save"].as<bool>()) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject jo = jsonBuffer.to<JsonObject>();
    jo["frequency"] = LORAConfig.frequency;
    // Save config into NVS (non-volatile storage)
    String conf = "";
    serializeJson(jsonBuffer, conf);
    preferences.begin(Gateway_Short_Name, false);
    int result = preferences.putString("LORAConfig", conf);
    preferences.end();
    Log.notice(F("LORA Config_save: %s, result: %d" CR), conf.c_str(), result);
  }
}

void setupLORA() {
  LORAConfig_init();
  LORAConfig_load();
#  ifdef ZmqttDiscovery
  semaphorecreateOrUpdateDeviceLORA = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphorecreateOrUpdateDeviceLORA);
#  endif
  Log.notice(F("LORA Frequency: %d" CR), LORAConfig.frequency);
#  ifdef ESP8266
  SPI.begin();
#  else
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
#  endif

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);

  if (!LoRa.begin(LORAConfig.frequency)) {
    Log.error(F("ZgatewayLORA setup failed!" CR));
    while (1)
      ;
  }
  LoRa.receive();
  Log.notice(F("LORA_SCK: %d" CR), LORA_SCK);
  Log.notice(F("LORA_MISO: %d" CR), LORA_MISO);
  Log.notice(F("LORA_MOSI: %d" CR), LORA_MOSI);
  Log.notice(F("LORA_SS: %d" CR), LORA_SS);
  Log.notice(F("LORA_RST: %d" CR), LORA_RST);
  Log.notice(F("LORA_DI0: %d" CR), LORA_DI0);
  Log.trace(F("ZgatewayLORA setup done" CR));
}

void LORAtoMQTT() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    StaticJsonDocument<JSON_MSG_BUFFER> LORAdataBuffer;
    JsonObject LORAdata = LORAdataBuffer.to<JsonObject>();
    Log.trace(F("Rcv. LORA" CR));
#  ifdef ESP32
    String taskMessage = "LORA Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
    //trc(taskMessage);
#  endif
    // Create packet and reserve null terminator space
    byte packet[packetSize + 1];
    boolean binary = false;
    for (int i = 0; i < packetSize; i++) {
      packet[i] = (char)LoRa.read();

      if (packet[i] < 32 || packet[i] > 127)
        binary = true;
    }
    // Terminate with a null character in case we have a string
    packet[packetSize] = 0;
    uint8_t deviceId = _determineDevice(packet, packetSize);
    if (deviceId == WIPHONE) {
      _WiPhoneToMQTT(packet, LORAdata);
    } else if (binary) {
      if (LORAConfig.onlyKnown) {
        Log.trace(F("Ignoring non identifiable packet" CR));
        return;
      }
      // We have non-ascii data: create hex string of the data
      char hex[packetSize * 2 + 1];
      _rawToHex(packet, hex, packetSize);
      // Terminate with a null character
      hex[packetSize * 2] = 0;

      LORAdata["hex"] = hex;
    } else {
      // ascii payload
      std::string packetStrStd = (char*)packet;
      auto error = deserializeJson(LORAdataBuffer, packetStrStd);
      if (error) {
        Log.error(F("LORA packet deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), LORAdataBuffer.capacity());
      }
    }

    LORAdata["rssi"] = (int)LoRa.packetRssi();
    LORAdata["snr"] = (float)LoRa.packetSnr();
    LORAdata["pferror"] = (float)LoRa.packetFrequencyError();
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
    handleJsonEnqueue(LORAdata);
    if (repeatLORAwMQTT) {
      Log.trace(F("Pub LORA for rpt" CR));
      LORAdata["origin"] = subjectMQTTtoLORA;
      handleJsonEnqueue(LORAdata);
    }
  }
}

#  if jsonReceiving
void MQTTtoLORA(char* topicOri, JsonObject& LORAdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORA)) {
    Log.trace(F("MQTTtoLORA json" CR));
    const char* message = LORAdata["message"];
    const char* hex = LORAdata["hex"];
    LORAConfig_fromJson(LORAdata);
    if (message || hex) {
      LoRa.beginPacket();
      uint8_t deviceId = _determineDevice(LORAdata);
      if (deviceId == WIPHONE) {
        _MQTTtoWiPhone(LORAdata);
      } else if (hex) {
        // We have hex data: create convert to binary
        byte raw[strlen(hex) / 2];
        _hexToRaw(hex, raw, sizeof(raw));
        LoRa.write((uint8_t*)raw, sizeof(raw));
      } else {
        // ascii payload
        LoRa.print(message);
      }

      LoRa.endPacket();
      Log.trace(F("MQTTtoLORA OK" CR));
      // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      pub(subjectGTWLORAtoMQTT, LORAdata);
    } else {
      Log.error(F("MQTTtoLORA Fail json" CR));
    }
  }
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORAset)) {
    Log.trace(F("MQTTtoLORA json set" CR));
    /*
     * Configuration modifications priorities:
     *  First `init=true` and `load=true` commands are executed (if both are present, INIT prevails on LOAD)
     *  Then parameters included in json are taken in account
     *  Finally `erase=true` and `save=true` commands are executed (if both are present, ERASE prevails on SAVE)
     */
    if (LORAdata.containsKey("init") && LORAdata["init"].as<bool>()) {
      // Restore the default (initial) configuration
      LORAConfig_init();
    } else if (LORAdata.containsKey("load") && LORAdata["load"].as<bool>()) {
      // Load the saved configuration, if not initialised
      LORAConfig_load();
    }

    // Load config from json if available
    LORAConfig_fromJson(LORAdata);
    stateLORAMeasures();
  }
}
#  endif
#  if simpleReceiving
void MQTTtoLORA(char* topicOri, char* LORAarray) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORA)) {
    LoRa.beginPacket();
    LoRa.print(LORAarray);
    LoRa.endPacket();
    Log.notice(F("MQTTtoLORA OK" CR));
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
  // Convert syncWord to a hexadecimal string and store it in the JSON
  char syncWordHex[5]; // Enough space for 0xXX and null terminator
  snprintf(syncWordHex, sizeof(syncWordHex), "0x%02X", LORAConfig.syncWord);
  LORAdata["syncword"] = syncWordHex;
  LORAdata["enablecrc"] = LORAConfig.crc;
  LORAdata["invertiq"] = LORAConfig.invertIQ;
  LORAdata["onlyknown"] = LORAConfig.onlyKnown;

  pub(subjectGTWLORAtoMQTT, LORAdata);

  String output;
  serializeJson(LORAdata, output);
  return output;
}
#endif
