/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal/BLE  and a MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
 - publish MQTT data to a different topic related to BLE devices rssi signal
 - publish MQTT data related to mi flora temperature, moisture, fertility and lux
 - publish MQTT data related to mi jia indoor temperature & humidity sensor

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

Thanks to wolass https://github.com/wolass for suggesting me HM 10 and dinosd https://github.com/dinosd/BLE_PROXIMITY for inspiring me how to implement the gateway
*/
#include "User_config.h"

#ifdef ZgatewayBT

SemaphoreHandle_t semaphoreCreateOrUpdateDevice;
SemaphoreHandle_t semaphoreBLEOperation;
QueueHandle_t BLEQueue;
// Headers used for deep sleep functions
#  include <NimBLEAdvertisedDevice.h>
#  include <NimBLEDevice.h>
#  include <NimBLEScan.h>
#  include <NimBLEUtils.h>
#  include <decoder.h>
#  include <driver/adc.h>
#  include <esp_bt.h>
#  include <esp_bt_main.h>
#  include <esp_wifi.h>

#  include <atomic>

#  include "ZgatewayBLEConnect.h"
#  include "soc/timer_group_reg.h"
#  include "soc/timer_group_struct.h"

using namespace std;

// Global struct to store live BT configuration data
BTConfig_s BTConfig;

TheengsDecoder decoder;

static TaskHandle_t xCoreTaskHandle;
static TaskHandle_t xProcBLETaskHandle;

struct decompose {
  int start;
  int len;
  bool reverse;
};

vector<BLEAction> BLEactions;

vector<BLEdevice*> devices;
int newDevices = 0;

static BLEdevice NO_BT_DEVICE_FOUND = {{0},
                                       0,
                                       false,
                                       false,
                                       false,
                                       false,
                                       TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL};
static bool oneWhite = false;

void BTConfig_init() {
  BTConfig.bleConnect = AttemptBLEConnect;
  BTConfig.BLEinterval = TimeBtwRead;
  BTConfig.adaptiveScan = AdaptiveBLEScan;
  BTConfig.intervalActiveScan = TimeBtwActive;
  BTConfig.intervalConnect = TimeBtwConnect;
  BTConfig.scanDuration = Scan_duration;
  BTConfig.pubOnlySensors = PublishOnlySensors;
  BTConfig.pubRandomMACs = PublishRandomMACs;
  BTConfig.presenceEnable = HassPresence;
  BTConfig.presenceTopic = subjectHomePresence;
  BTConfig.presenceUseBeaconUuid = useBeaconUuidForPresence;
  BTConfig.minRssi = MinimumRSSI;
  BTConfig.extDecoderEnable = UseExtDecoder;
  BTConfig.extDecoderTopic = MQTTDecodeTopic;
  BTConfig.filterConnectable = BLE_FILTER_CONNECTABLE;
  BTConfig.pubAdvData = pubBLEAdvData;
  BTConfig.pubBeaconUuidForTopic = useBeaconUuidForTopic;
  BTConfig.ignoreWBlist = false;
  BTConfig.presenceAwayTimer = PresenceAwayTimer;
}

// Watchdog, if there was no change of BLE scanCount, restart ESP
void btScanWDG() {
  static unsigned long previousBtScanCount = 0;
  static unsigned long lastBtScan = 0;
  unsigned long now = millis();
  if (!ProcessLock &&
      previousBtScanCount == scanCount &&
      scanCount != 0 &&
      (now - lastBtScan > BTConfig.BLEinterval)) {
    Log.error(F("BLE Scan watchdog triggered at : %ds" CR), lastBtScan / 1000);
    ESPRestart(4);
  } else {
    previousBtScanCount = scanCount;
    lastBtScan = now;
  }
}

unsigned long timeBetweenConnect = 0;
unsigned long timeBetweenActive = 0;

String stateBTMeasures(bool start) {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jo = jsonBuffer.to<JsonObject>();
  jo["bleconnect"] = BTConfig.bleConnect;
  jo["interval"] = BTConfig.BLEinterval;
  jo["adaptivescan"] = BTConfig.adaptiveScan;
  jo["intervalacts"] = BTConfig.intervalActiveScan;
  jo["intervalcnct"] = BTConfig.intervalConnect;
  jo["scanduration"] = BTConfig.scanDuration;
  jo["onlysensors"] = BTConfig.pubOnlySensors;
  jo["randommacs"] = BTConfig.pubRandomMACs;
  jo["hasspresence"] = BTConfig.presenceEnable;
  jo["presenceTopic"] = BTConfig.presenceTopic;
  jo["presenceUseBeaconUuid"] = BTConfig.presenceUseBeaconUuid;
  jo["minrssi"] = -abs(BTConfig.minRssi); // Always export as negative value
  jo["extDecoderEnable"] = BTConfig.extDecoderEnable;
  jo["extDecoderTopic"] = BTConfig.extDecoderTopic;
  jo["filterConnectable"] = BTConfig.filterConnectable;
  jo["pubadvdata"] = BTConfig.pubAdvData;
  jo["pubBeaconUuidForTopic"] = BTConfig.pubBeaconUuidForTopic;
  jo["ignoreWBlist"] = BTConfig.ignoreWBlist;
  jo["presenceawaytimer"] = BTConfig.presenceAwayTimer;
  jo["btqblck"] = btQueueBlocked;
  jo["btqsum"] = btQueueLengthSum;
  jo["btqsnd"] = btQueueLengthCount;
  jo["btqavg"] = (btQueueLengthCount > 0 ? btQueueLengthSum / (float)btQueueLengthCount : 0);
  jo["bletaskstack"] = uxTaskGetStackHighWaterMark(xProcBLETaskHandle);

  if (start) {
    Log.notice(F("BT sys: "));
    serializeJsonPretty(jsonBuffer, Serial);
    Serial.println();
    return ""; // Do not try to erase/write/send config at startup
  }
  String output;
  serializeJson(jo, output);
  pub(subjectBTtoMQTT, jo);
  return (output);
}

void BTConfig_fromJson(JsonObject& BTdata, bool startup = false) {
  // Attempts to connect to eligible devices or not
  Config_update(BTdata, "bleconnect", BTConfig.bleConnect);
  // Identify AdaptiveScan deactivation to pass to continuous mode or activation to come back to default settings
  if (startup == false) {
    if (BTdata.containsKey("hasspresence") && BTdata["hasspresence"] == false && BTConfig.presenceEnable == true) {
      BTdata["adaptivescan"] = true;
#  ifdef ZmqttDiscovery
      // Remove discovered entities
      eraseTopic("number", (char*)getUniqueId("presenceawaytimer", "").c_str());
#  endif
    } else if (BTdata.containsKey("hasspresence") && BTdata["hasspresence"] == true && BTConfig.presenceEnable == false) {
      BTdata["adaptivescan"] = false;
    }

    if (BTdata.containsKey("adaptivescan") && BTdata["adaptivescan"] == false && BTConfig.adaptiveScan == true) {
      BTdata["interval"] = MinTimeBtwScan;
      BTdata["intervalacts"] = MinTimeBtwScan;
      BTdata["scanduration"] = MinScanDuration;
    } else if (BTdata.containsKey("adaptivescan") && BTdata["adaptivescan"] == true && BTConfig.adaptiveScan == false) {
      BTdata["interval"] = TimeBtwRead;
      BTdata["intervalacts"] = TimeBtwActive;
      BTdata["scanduration"] = Scan_duration;
#  ifdef ZmqttDiscovery
      // Remove discovered entities
      eraseTopic("number", (char*)getUniqueId("interval", "").c_str());
      eraseTopic("number", (char*)getUniqueId("intervalacts", "").c_str());
#  endif
    }
  }
  Config_update(BTdata, "adaptivescan", BTConfig.adaptiveScan);
  // Home Assistant presence message
  Config_update(BTdata, "hasspresence", BTConfig.presenceEnable);
#  ifdef ZmqttDiscovery
  // Create discovery entities
  btScanParametersDiscovery();
  btPresenceParametersDiscovery();
#  endif
  // Time before before active scan
  // Scan interval set
  if (BTdata.containsKey("interval") && BTdata["interval"] != 0)
    Config_update(BTdata, "interval", BTConfig.BLEinterval);
  // Define if the scan is adaptive or not
  Config_update(BTdata, "intervalacts", BTConfig.intervalActiveScan);
  // Time before a connect set
  Config_update(BTdata, "intervalcnct", BTConfig.intervalConnect);
  // publish all BLE devices discovered or  only the identified sensors (like temperature sensors)
  Config_update(BTdata, "scanduration", BTConfig.scanDuration);
  // define the duration for a scan; in milliseconds
  Config_update(BTdata, "onlysensors", BTConfig.pubOnlySensors);
  // publish devices which randomly change their MAC addresses
  Config_update(BTdata, "randommacs", BTConfig.pubRandomMACs);
  // Home Assistant presence message topic
  Config_update(BTdata, "presenceTopic", BTConfig.presenceTopic);
  // Home Assistant presence message use iBeacon UUID
  Config_update(BTdata, "presenceUseBeaconUuid", BTConfig.presenceUseBeaconUuid);
  // Timer to trigger a device state as offline if not seen
  Config_update(BTdata, "presenceawaytimer", BTConfig.presenceAwayTimer);
  // MinRSSI set
  Config_update(BTdata, "minrssi", BTConfig.minRssi);
  // Send undecoded device data
  Config_update(BTdata, "extDecoderEnable", BTConfig.extDecoderEnable);
  // Topic to send undecoded device data
  Config_update(BTdata, "extDecoderTopic", BTConfig.extDecoderTopic);
  // Sets whether to filter publishing
  Config_update(BTdata, "filterConnectable", BTConfig.filterConnectable);
  // Publish advertisment data
  Config_update(BTdata, "pubadvdata", BTConfig.pubAdvData);
  // Use iBeacon UUID as topic, instead of sender (random) MAC address
  Config_update(BTdata, "pubBeaconUuidForTopic", BTConfig.pubBeaconUuidForTopic);
  // Disable Whitelist & Blacklist
  Config_update(BTdata, "ignoreWBlist", (BTConfig.ignoreWBlist));

  stateBTMeasures(startup);

  if (BTdata.containsKey("erase") && BTdata["erase"].as<bool>()) {
    // Erase config from NVS (non-volatile storage)
    preferences.begin(Gateway_Short_Name, false);
    if (preferences.isKey("BTConfig")) {
      preferences.remove("BTConfig");
      preferences.end();
      Log.notice(F("BT config erased" CR));
      return; // Erase prevails on save, so skipping save
    }
  }

  if (BTdata.containsKey("save") && BTdata["save"].as<bool>()) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject jo = jsonBuffer.to<JsonObject>();
    jo["bleconnect"] = BTConfig.bleConnect;
    jo["interval"] = BTConfig.BLEinterval;
    jo["adaptivescan"] = BTConfig.adaptiveScan;
    jo["intervalacts"] = BTConfig.intervalActiveScan;
    jo["intervalcnct"] = BTConfig.intervalConnect;
    jo["scanduration"] = BTConfig.scanDuration;
    jo["onlysensors"] = BTConfig.pubOnlySensors;
    jo["randommacs"] = BTConfig.pubRandomMACs;
    jo["hasspresence"] = BTConfig.presenceEnable;
    jo["presenceTopic"] = BTConfig.presenceTopic;
    jo["presenceUseBeaconUuid"] = BTConfig.presenceUseBeaconUuid;
    jo["minrssi"] = -abs(BTConfig.minRssi); // Always export as negative value
    jo["extDecoderEnable"] = BTConfig.extDecoderEnable;
    jo["extDecoderTopic"] = BTConfig.extDecoderTopic;
    jo["filterConnectable"] = BTConfig.filterConnectable;
    jo["pubadvdata"] = BTConfig.pubAdvData;
    jo["pubBeaconUuidForTopic"] = BTConfig.pubBeaconUuidForTopic;
    jo["ignoreWBlist"] = BTConfig.ignoreWBlist;
    jo["presenceawaytimer"] = BTConfig.presenceAwayTimer;
    // Save config into NVS (non-volatile storage)
    String conf = "";
    serializeJson(jsonBuffer, conf);
    preferences.begin(Gateway_Short_Name, false);
    preferences.putString("BTConfig", conf);
    preferences.end();
    Log.notice(F("BT config saved" CR));
  }
}

void BTConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (preferences.isKey("BTConfig")) {
    auto error = deserializeJson(jsonBuffer, preferences.getString("BTConfig", "{}"));
    preferences.end();
    Log.notice(F("BT config loaded" CR));
    if (error) {
      Log.error(F("BT config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      return;
    }
    if (jsonBuffer.isNull()) {
      Log.warning(F("BT config is null" CR));
      return;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    BTConfig_fromJson(jo, true); // Never send MQTT message with config
    Log.notice(F("BT config loaded" CR));
  }
}

void pubBTMainCore(JsonObject& data, bool haPresenceEnabled = true) {
  if (abs((int)data["rssi"] | 0) < abs(BTConfig.minRssi) && data.containsKey("id")) {
    String topic = data["id"].as<const char*>();
    topic.replace(":", ""); // Initially publish topic ends with MAC address
    if (BTConfig.pubBeaconUuidForTopic && !BTConfig.extDecoderEnable && data.containsKey("model_id") && data["model_id"].as<String>() == "IBEACON")
      topic = data["uuid"].as<const char*>(); // If model_id is IBEACON, use uuid as topic
    if (BTConfig.extDecoderEnable && !data.containsKey("model"))
      topic = BTConfig.extDecoderTopic; // If external decoder, use this topic to send data
    topic = subjectBTtoMQTT + String("/") + topic;
    pub((char*)topic.c_str(), data);
  }
  if (haPresenceEnabled && data.containsKey("distance")) {
    if (data.containsKey("servicedatauuid"))
      data.remove("servicedatauuid");
    if (data.containsKey("servicedata"))
      data.remove("servicedata");
    if (BTConfig.presenceUseBeaconUuid && data.containsKey("model_id") && data["model_id"].as<String>() == "IBEACON") {
      data["mac"] = data["id"];
      data["id"] = data["uuid"];
    }
    String topic = String(mqtt_topic) + BTConfig.presenceTopic + String(gateway_name);
    Log.trace(F("Pub HA Presence %s" CR), topic.c_str());
    pub_custom_topic((char*)topic.c_str(), data, false);
  }
}

class JsonBundle {
public:
  StaticJsonDocument<JSON_MSG_BUFFER> buffer;
  JsonObject object;
  bool haPresence;

  JsonObject& createObject(const char* json = NULL, bool haPresenceEnabled = true) {
    buffer.clear();
    haPresence = haPresenceEnabled;
    object = buffer.to<JsonObject>();

    if (json != nullptr) {
      auto error = deserializeJson(buffer, json);
      if (error) {
        Log.error(F("deserialize object failed: %s" CR), error.c_str());
      }
    }
    return object;
  }
};

void PublishDeviceData(JsonObject& BLEdata, bool processBLEData = true);

atomic_int forceBTScan;

JsonBundle jsonBTBufferQueue[BTQueueSize];
atomic_int jsonBTBufferQueueNext, jsonBTBufferQueueLast;
int btQueueBlocked = 0;
int btQueueLengthSum = 0;
int btQueueLengthCount = 0;

JsonObject& getBTJsonObject(const char* json, bool haPresenceEnabled) {
  int next, last;
  for (bool blocked = false;;) {
    next = atomic_load_explicit(&jsonBTBufferQueueNext, ::memory_order_seq_cst); // use namespace std -> ambiguous error...
    last = atomic_load_explicit(&jsonBTBufferQueueLast, ::memory_order_seq_cst); // use namespace std -> ambiguous error...
    if ((2 * BTQueueSize + last - next) % (2 * BTQueueSize) != BTQueueSize) break;
    if (!blocked) {
      blocked = true;
      btQueueBlocked++;
    }
    delay(1);
  }
  return jsonBTBufferQueue[last % BTQueueSize].createObject(json, haPresenceEnabled);
}

// should run from the BT core
void pubBT(JsonObject& data) {
  int last = atomic_load_explicit(&jsonBTBufferQueueLast, ::memory_order_seq_cst);
  atomic_store_explicit(&jsonBTBufferQueueLast, (last + 1) % (2 * BTQueueSize), ::memory_order_seq_cst); // use namespace std -> ambiguous error...
}

// should run from the main core
void emptyBTQueue() {
  for (bool first = true;;) {
    int next = atomic_load_explicit(&jsonBTBufferQueueNext, ::memory_order_seq_cst); // use namespace std -> ambiguous error...
    int last = atomic_load_explicit(&jsonBTBufferQueueLast, ::memory_order_seq_cst); // use namespace std -> ambiguous error...
    if (last == next) break;
    if (first) {
      int diff = (2 * BTQueueSize + last - next) % (2 * BTQueueSize);
      btQueueLengthSum += diff;
      btQueueLengthCount++;
      first = false;
    }
    JsonBundle& bundle = jsonBTBufferQueue[next % BTQueueSize];
    pubBTMainCore(bundle.object, bundle.haPresence);
    atomic_store_explicit(&jsonBTBufferQueueNext, (next + 1) % (2 * BTQueueSize), ::memory_order_seq_cst); // use namespace std -> ambiguous error...
    vTaskDelay(1);
  }
}

void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type = 0);

BLEdevice* getDeviceByMac(const char* mac); // Declared here to avoid pre-compilation issue (misplaced auto declaration by pio)
BLEdevice* getDeviceByMac(const char* mac) {
  Log.trace(F("getDeviceByMac %s" CR), mac);

  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    if ((strcmp((*it)->macAdr, mac) == 0)) {
      return *it;
    }
  }
  return &NO_BT_DEVICE_FOUND;
}

bool updateWorB(JsonObject& BTdata, bool isWhite) {
  Log.trace(F("update WorB" CR));
  const char* jsonKey = isWhite ? "white-list" : "black-list";

  int size = BTdata[jsonKey].size();
  if (size == 0)
    return false;

  for (int i = 0; i < size; i++) {
    const char* mac = BTdata[jsonKey][i];
    createOrUpdateDevice(mac, (isWhite ? device_flags_isWhiteL : device_flags_isBlackL),
                         TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL);
  }

  return true;
}

void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type) {
  if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(30000)) == pdFALSE) {
    Log.error(F("Semaphore NOT taken" CR));
    return;
  }

  BLEdevice* device = getDeviceByMac(mac);
  if (device == &NO_BT_DEVICE_FOUND) {
    Log.trace(F("add %s" CR), mac);
    //new device
    device = new BLEdevice();
    strcpy(device->macAdr, mac);
    device->isDisc = flags & device_flags_isDisc;
    device->isWhtL = flags & device_flags_isWhiteL;
    device->isBlkL = flags & device_flags_isBlackL;
    device->connect = flags & device_flags_connect;
    device->macType = mac_type;
    device->sensorModel_id = model;
    device->lastUpdate = millis();
    devices.push_back(device);
    newDevices++;
  } else {
    Log.trace(F("update %s" CR), mac);
    device->lastUpdate = millis();
    device->macType = mac_type;

    if (flags & device_flags_isDisc) {
      device->isDisc = true;
    }

    if (flags & device_flags_connect) {
      device->connect = true;
    }

    if (model != TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL) {
      device->sensorModel_id = model;
    }

    if (flags & device_flags_isWhiteL || flags & device_flags_isBlackL) {
      device->isWhtL = flags & device_flags_isWhiteL;
      device->isBlkL = flags & device_flags_isBlackL;
    }
  }

  // update oneWhite flag
  oneWhite = oneWhite || device->isWhtL;

  xSemaphoreGive(semaphoreCreateOrUpdateDevice);
}

void updateDevicesStatus() {
  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    BLEdevice* p = *it;
    unsigned long now = millis();
    if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::NUT ||
        p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::MIBAND ||
        p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::TAGIT ||
        p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::TILE ||
        p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::TILEN ||
        p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::ITAG ||
        p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::RUUVITAG_RAWV1 ||
        p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::RUUVITAG_RAWV2) { // We apply the offline status only for tracking device, can be extended further to all the devices
      if ((p->lastUpdate != 0) && (p->lastUpdate < (now - BTConfig.presenceAwayTimer) && (now > BTConfig.presenceAwayTimer)) &&
          (BTConfig.ignoreWBlist || ((!oneWhite || isWhite(p)) && !isBlack(p)))) { // Only if WBlist is disabled OR ((no white MAC OR this MAC is white) AND not a black listed MAC)) {
        JsonObject BLEdata = getBTJsonObject();
        BLEdata["id"] = p->macAdr;
        BLEdata["state"] = "offline";
        pubBT(BLEdata);
        // We set the lastUpdate to 0 to avoid replublishing the offline state
        p->lastUpdate = 0;
      }
    }
  }
}

void dumpDevices() {
#  if LOG_LEVEL > 4 // > NOTICE LOG LEVEL
  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    BLEdevice* p = *it;
    Log.trace(F("macAdr %s" CR), p->macAdr);
    Log.trace(F("macType %d" CR), p->macType);
    Log.trace(F("isDisc %d" CR), p->isDisc);
    Log.trace(F("isWhtL %d" CR), p->isWhtL);
    Log.trace(F("isBlkL %d" CR), p->isBlkL);
    Log.trace(F("connect %d" CR), p->connect);
    Log.trace(F("sensorModel_id %d" CR), p->sensorModel_id);
    Log.trace(F("LastUpdate %u" CR), p->lastUpdate);
  }
#  endif
}

void strupp(char* beg) {
  while ((*beg = toupper(*beg)))
    ++beg;
}

#  ifdef ZmqttDiscovery
void DT24Discovery(const char* mac, const char* sensorModel_id) {
#    define DT24parametersCount 7
  Log.trace(F("DT24Discovery" CR));
  const char* DT24sensor[DT24parametersCount][9] = {
      {"sensor", "volt", mac, "voltage", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "amp", mac, "current", jsonCurrent, "", "", "A", stateClassMeasurement},
      {"sensor", "watt", mac, "power", jsonPower, "", "", "W", stateClassMeasurement},
      {"sensor", "watt-hour", mac, "power", jsonEnergy, "", "", "kWh", stateClassMeasurement},
      {"sensor", "price", mac, "", jsonMsg, "", "", "", stateClassNone},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"binary_sensor", "inUse", mac, "power", jsonInuse, "", "", "", stateClassNone}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, DT24sensor, DT24parametersCount, "DT24", "ATorch", sensorModel_id);
}

void BM2Discovery(const char* mac, const char* sensorModel_id) {
#    define BM2parametersCount 2
  Log.trace(F("BM2Discovery" CR));
  const char* BM2sensor[BM2parametersCount][9] = {
      {"sensor", "volt", mac, "voltage", jsonVoltBM2, "", "", "V", stateClassMeasurement}, // We use a json definition that retrieve only data from the BM2 decoder, as this sensor also advertize volt as an iBeacon
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, BM2sensor, BM2parametersCount, "BM2", "Generic", sensorModel_id);
}

void LYWSD03MMCDiscovery(const char* mac, const char* sensorModel) {
#    define LYWSD03MMCparametersCount 4
  Log.trace(F("LYWSD03MMCDiscovery" CR));
  const char* LYWSD03MMCsensor[LYWSD03MMCparametersCount][9] = {
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "volt", mac, "", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "hum", mac, "humidity", jsonHum, "", "", "%", stateClassMeasurement}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, LYWSD03MMCsensor, LYWSD03MMCparametersCount, "LYWSD03MMC", "Xiaomi", sensorModel);
}

void MHO_C401Discovery(const char* mac, const char* sensorModel) {
#    define MHO_C401parametersCount 4
  Log.trace(F("MHO_C401Discovery" CR));
  const char* MHO_C401sensor[MHO_C401parametersCount][9] = {
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "volt", mac, "", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "hum", mac, "humidity", jsonHum, "", "", "%", stateClassMeasurement}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MHO_C401sensor, MHO_C401parametersCount, "MHO_C401", "Xiaomi", sensorModel);
}

void HHCCJCY01HHCCDiscovery(const char* mac, const char* sensorModel) {
#    define HHCCJCY01HHCCparametersCount 5
  Log.trace(F("HHCCJCY01HHCCDiscovery" CR));
  const char* HHCCJCY01HHCCsensor[HHCCJCY01HHCCparametersCount][9] = {
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "lux", mac, "illuminance", jsonLux, "", "", "lx", stateClassMeasurement},
      {"sensor", "fer", mac, "", jsonFer, "", "", "µS/cm", stateClassMeasurement},
      {"sensor", "moi", mac, "", jsonMoi, "", "", "%", stateClassMeasurement}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, HHCCJCY01HHCCsensor, HHCCJCY01HHCCparametersCount, "HHCCJCY01HHCC", "Xiaomi", sensorModel);
}

void XMWSDJ04MMCDiscovery(const char* mac, const char* sensorModel) {
#    define XMWSDJ04MMCparametersCount 4
  Log.trace(F("XMWSDJ04MMCDiscovery" CR));
  const char* XMWSDJ04MMCsensor[XMWSDJ04MMCparametersCount][9] = {
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "volt", mac, "", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "hum", mac, "humidity", jsonHum, "", "", "%", stateClassMeasurement}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, XMWSDJ04MMCsensor, XMWSDJ04MMCparametersCount, "XMWSDJ04MMC", "Xiaomi", sensorModel);
}

#  else
void LYWSD03MMCDiscovery(const char* mac, const char* sensorModel) {}
void MHO_C401Discovery(const char* mac, const char* sensorModel) {}
void HHCCJCY01HHCCDiscovery(const char* mac, const char* sensorModel) {}
void DT24Discovery(const char* mac, const char* sensorModel_id) {}
void BM2Discovery(const char* mac, const char* sensorModel_id) {}
void XMWSDJ04MMCDiscovery(const char* mac, const char* sensorModel_id) {}
#  endif

/*
       Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
       Ported to Arduino ESP32 by Evandro Copercini
    */
// core task implementation thanks to https://techtutorialsx.com/2017/05/09/esp32-running-code-on-a-specific-core/

//core on which the BLE detection task will run
static int taskCore = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice* advertisedDevice) {
    BLEAdvertisedDevice* ad = new BLEAdvertisedDevice(*advertisedDevice);
    if (xQueueSend(BLEQueue, &ad, 0) != pdTRUE) {
      Log.error(F("BLEQueue full" CR));
      delete (ad);
    }
  }
};

std::string convertServiceData(std::string deviceServiceData) {
  int serviceDataLength = (int)deviceServiceData.length();
  char spr[2 * serviceDataLength + 1];
  for (int i = 0; i < serviceDataLength; i++) sprintf(spr + 2 * i, "%.2x", (unsigned char)deviceServiceData[i]);
  spr[2 * serviceDataLength] = 0;
  Log.trace(F("Converted service data (%d) to %s" CR), serviceDataLength, spr);
  return spr;
}

void procBLETask(void* pvParameters) {
  BLEAdvertisedDevice* advertisedDevice = nullptr;

  for (;;) {
    xQueueReceive(BLEQueue, &advertisedDevice, portMAX_DELAY);

    if (!ProcessLock) {
      Log.trace(F("Creating BLE buffer" CR));
      JsonObject& BLEdata = getBTJsonObject();
      String mac_adress = advertisedDevice->getAddress().toString().c_str();
      mac_adress.toUpperCase();
      BLEdata["id"] = (char*)mac_adress.c_str();
#  if defined(ESP8266) || defined(ESP32)
#    if message_UTCtimestamp == true
      BLEdata["UTCtime"] = UTCtimestamp();
#    endif
#    if message_unixtimestamp == true
      BLEdata["unixtime"] = unixtimestamp();
#    endif
#  endif
      BLEdata["mac_type"] = advertisedDevice->getAddress().getType();
      BLEdata["adv_type"] = advertisedDevice->getAdvType();
      Log.notice(F("Device detected: %s" CR), (char*)mac_adress.c_str());
      BLEdevice* device = getDeviceByMac(BLEdata["id"].as<const char*>());

      if (BTConfig.filterConnectable && device->connect) {
        Log.notice(F("Filtered connectable device" CR));
        delete (advertisedDevice);
        continue;
      }

      if (BTConfig.ignoreWBlist || ((!oneWhite || isWhite(device)) && !isBlack(device))) { // Only if WBlist is disabled OR ((no white MAC OR this MAC is white) AND not a black listed MAC)
        if (advertisedDevice->haveName())
          BLEdata["name"] = (char*)advertisedDevice->getName().c_str();
        if (advertisedDevice->haveManufacturerData()) {
          char* manufacturerdata = BLEUtils::buildHexData(NULL, (uint8_t*)advertisedDevice->getManufacturerData().data(), advertisedDevice->getManufacturerData().length());
          BLEdata["manufacturerdata"] = manufacturerdata;
          free(manufacturerdata);
        }
        if (advertisedDevice->haveRSSI())
          BLEdata["rssi"] = (int)advertisedDevice->getRSSI();
        if (advertisedDevice->haveTXPower())
          BLEdata["txpower"] = (int8_t)advertisedDevice->getTXPower();
        if (advertisedDevice->haveRSSI() && !BTConfig.pubOnlySensors && BTConfig.presenceEnable) {
          hass_presence(BLEdata); // this device has an rssi and we don't want only sensors so in consequence we can use it for home assistant room presence component
        }
        if (advertisedDevice->haveServiceData()) {
          int serviceDataCount = advertisedDevice->getServiceDataCount();
          Log.trace(F("Get services data number: %d" CR), serviceDataCount);
          for (int j = 0; j < serviceDataCount; j++) {
            std::string service_data = convertServiceData(advertisedDevice->getServiceData(j));
            Log.trace(F("Service data: %s" CR), service_data.c_str());
            BLEdata["servicedata"] = (char*)service_data.c_str();
            std::string serviceDatauuid = advertisedDevice->getServiceDataUUID(j).toString();
            Log.trace(F("Service data UUID: %s" CR), (char*)serviceDatauuid.c_str());
            BLEdata["servicedatauuid"] = (char*)serviceDatauuid.c_str();
            process_bledata(BLEdata); // this will force to resolve all the service data
          }

          PublishDeviceData(BLEdata, false);
        } else {
          PublishDeviceData(BLEdata); // PublishDeviceData has its own logic whether it needs to publish the json or not
        }
      } else {
        Log.trace(F("Filtered MAC device" CR));
      }
      if (BTConfig.presenceEnable)
        updateDevicesStatus();
    }
    delete (advertisedDevice);
  }
}

/**
 * BLEscan used to retrieve BLE advertized data from devices without connection
 */
void BLEscan() {
  // Don't start the next scan until processing of previous results is complete.
  while (uxQueueMessagesWaiting(BLEQueue)) {
    yield();
  }
  Log.notice(F("Scan begin" CR));
  BLEScan* pBLEScan = BLEDevice::getScan();
  MyAdvertisedDeviceCallbacks myCallbacks;
  pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
  if (millis() > (timeBetweenActive + BTConfig.intervalActiveScan)) {
    pBLEScan->setActiveScan(true);
    timeBetweenActive = millis();
  } else {
    pBLEScan->setActiveScan(false);
  }
  pBLEScan->setInterval(BLEScanInterval);
  pBLEScan->setWindow(BLEScanWindow);
  BLEScanResults foundDevices = pBLEScan->start(BTConfig.scanDuration / 1000, false);
  if (foundDevices.getCount())
    scanCount++;
  Log.notice(F("Found %d devices, scan number %d end" CR), foundDevices.getCount(), scanCount);
  Log.trace(F("Process BLE stack free: %u" CR), uxTaskGetStackHighWaterMark(xProcBLETaskHandle));
}

/**
 * Connect to BLE devices and initiate the callbacks with a service/characteristic request
 */
void BLEconnect() {
  if (!ProcessLock) {
    Log.notice(F("BLE Connect begin" CR));
    do {
      for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
        BLEdevice* p = *it;
        if (p->connect) {
          Log.trace(F("Model to connect found: %s" CR), p->macAdr);
          NimBLEAddress addr((const char*)p->macAdr, p->macType);
          if (p->sensorModel_id == BLEconectable::id::LYWSD03MMC ||
              p->sensorModel_id == BLEconectable::id::MHO_C401) {
            LYWSD03MMC_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == BLEconectable::id::DT24_BLE) {
            DT24_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == BLEconectable::id::BM2) {
            BM2_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {
            HHCCJCY01HHCC_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == BLEconectable::id::XMWSDJ04MMC) {
            XMWSDJ04MMC_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBS1) {
            SBS1_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
          } else {
            GENERIC_connect BLEclient(addr);
            if (BLEclient.processActions(BLEactions)) {
              // If we don't regularly connect to this, disable connections so advertisements
              // won't be filtered if BLE_FILTER_CONNECTABLE is set.
              p->connect = false;
            }
          }
          if (BLEactions.size() > 0) {
            std::vector<BLEAction> swap;
            for (auto& it : BLEactions) {
              if (!it.complete && --it.ttl) {
                swap.push_back(it);
              } else if (memcmp(it.addr, p->macAdr, sizeof(it.addr)) == 0) {
                if (p->sensorModel_id != BLEconectable::id::DT24_BLE &&
                    p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC &&
                    p->sensorModel_id != BLEconectable::id::LYWSD03MMC &&
                    p->sensorModel_id != BLEconectable::id::BM2 &&
                    p->sensorModel_id != BLEconectable::id::MHO_C401 &&
                    p->sensorModel_id != BLEconectable::id::XMWSDJ04MMC) {
                  // if irregulary connected to and connection failed clear the connect flag.
                  p->connect = false;
                }
              }
            }
            std::swap(BLEactions, swap);
          }
        }
      }
    } while (BLEactions.size() > 0);
    Log.notice(F("BLE Connect end" CR));
  }
}

void stopProcessing() {
  ProcessLock = true;
  delay(BTConfig.scanDuration < 2000 ? BTConfig.scanDuration : 2000);
  //Suspending, deleting tasks and stopping BT to free memory
  vTaskSuspend(xCoreTaskHandle);
  vTaskDelete(xCoreTaskHandle);
  vTaskSuspend(xProcBLETaskHandle);
  vTaskDelete(xProcBLETaskHandle);
  if (BLEDevice::getInitialized()) BLEDevice::deinit(true);
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  Log.notice(F("BLE gateway stopped, free heap: %d" CR), ESP.getFreeHeap());
}

void coreTask(void* pvParameters) {
  while (true) {
    if (!ProcessLock) {
      int n = 0;
      while (client.state() != 0 && n <= InitialMQTTConnectionTimeout && !ProcessLock) {
        n++;
        delay(1000);
      }
      if (client.state() != 0) {
        Log.warning(F("MQTT client disconnected no BLE scan" CR));
      } else if (!ProcessLock) {
        if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
          BLEscan();
          // Launching a connect every TimeBtwConnect
          if (millis() > (timeBetweenConnect + BTConfig.intervalConnect) && BTConfig.bleConnect) {
            timeBetweenConnect = millis();
            BLEconnect();
          }
          dumpDevices();
          Log.trace(F("CoreTask stack free: %u" CR), uxTaskGetStackHighWaterMark(xCoreTaskHandle));
          xSemaphoreGive(semaphoreBLEOperation);
        } else {
          Log.error(F("Failed to start scan - BLE busy" CR));
        }
      }
      if (lowpowermode > 0) {
        lowPowerESP32();
        int scan = atomic_exchange_explicit(&forceBTScan, 0, ::memory_order_seq_cst); // is this enough, it will wait the full deepsleep...
        if (scan == 1) BTforceScan();
      } else {
        for (int interval = BTConfig.BLEinterval, waitms; interval > 0; interval -= waitms) {
          int scan = atomic_exchange_explicit(&forceBTScan, 0, ::memory_order_seq_cst);
          if (scan == 1) BTforceScan(); // should we break after this?
          delay(waitms = interval > 100 ? 100 : interval); // 100ms
        }
      }
    } else {
      Log.trace(F("BLE core task canceled by processLock" CR));
    }
  }
}

#  if DEFAULT_LOW_POWER_MODE != -1
void lowPowerESP32() { // low power mode
  Log.trace(F("Going to deep sleep for: %l s" CR), (TimeBtwRead / 1000));
  deepSleep(TimeBtwRead * 1000);
}

void deepSleep(uint64_t time_in_us) {
#    if defined(ZboardM5STACK) || defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  sleepScreen();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)SLEEP_BUTTON, LOW);
#    endif

  Log.trace(F("Deactivating ESP32 components" CR));
  stopProcessing();
  // Ignore the deprecated warning, this call is necessary here.
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  adc_power_off();
#    pragma GCC diagnostic pop
  esp_wifi_stop();
  esp_deep_sleep(time_in_us);
}
#  else
void lowPowerESP32() {}
#  endif

void changelowpowermode(int newLowPowerMode) {
#  if DEFAULT_LOW_POWER_MODE != -1
  Log.notice(F("Changing LOW POWER mode to: %d" CR), newLowPowerMode);
#    if defined(ZboardM5STACK) || defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  if (lowpowermode == 2) {
#      ifdef ZboardM5STACK
    M5.Lcd.wakeup();
#      endif
#      if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
    M5.Axp.SetLDO2(true);
    M5.Lcd.begin();
#      endif
  }
  char lpm[2];
  sprintf(lpm, "%d", newLowPowerMode);
  M5Print("Changing LOW POWER mode to:", lpm, "");
#    endif
  lowpowermode = newLowPowerMode;
  preferences.begin(Gateway_Short_Name, false);
  preferences.putUInt("lowpowermode", lowpowermode);
  preferences.end();
  // Publish the states to update the controller switch status
  stateMeasures();
#  endif
}

void setupBT() {
  BTConfig_init();
  BTConfig_load();
  Log.notice(F("BLE scans interval: %d" CR), BTConfig.BLEinterval);
  Log.notice(F("BLE connects interval: %d" CR), BTConfig.intervalConnect);
  Log.notice(F("BLE scan duration: %d" CR), BTConfig.scanDuration);
  Log.notice(F("Publishing only BLE sensors: %T" CR), BTConfig.pubOnlySensors);
  Log.notice(F("Publishing random MAC devices: %T" CR), BTConfig.pubRandomMACs);
  Log.notice(F("Adaptive BLE scan: %T" CR), BTConfig.adaptiveScan);
  Log.notice(F("Active BLE scan interval: %d" CR), BTConfig.intervalActiveScan);
  Log.notice(F("minrssi: %d" CR), -abs(BTConfig.minRssi));
  Log.notice(F("Low Power Mode: %d" CR), lowpowermode);
  Log.notice(F("Presence Away Timer: %d" CR), BTConfig.presenceAwayTimer);

  atomic_init(&forceBTScan, 0); // in theory, we don't need this
  atomic_init(&jsonBTBufferQueueNext, 0); // in theory, we don't need this
  atomic_init(&jsonBTBufferQueueLast, 0); // in theory, we don't need this

  semaphoreCreateOrUpdateDevice = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreCreateOrUpdateDevice);

  semaphoreBLEOperation = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreBLEOperation);

  BLEQueue = xQueueCreate(32, sizeof(NimBLEAdvertisedDevice*));

  BLEDevice::setScanDuplicateCacheSize(BLEScanDuplicateCacheSize);
  BLEDevice::init("");

  xTaskCreatePinnedToCore(
      procBLETask, /* Function to implement the task */
      "procBLETask", /* Name of the task */
      5120, /* Stack size in bytes */
      NULL, /* Task input parameter */
      2, /* Priority of the task (set higher than core task) */
      &xProcBLETaskHandle, /* Task handle. */
      1); /* Core where the task should run */

  // we setup a task with priority one to avoid conflict with other gateways
  xTaskCreatePinnedToCore(
      coreTask, /* Function to implement the task */
      "coreTask", /* Name of the task */
      5120, /* Stack size in bytes */
      NULL, /* Task input parameter */
      1, /* Priority of the task */
      &xCoreTaskHandle, /* Task handle. */
      taskCore); /* Core where the task should run */
  Log.trace(F("ZgatewayBT multicore ESP32 setup done " CR));
}

bool BTtoMQTT() { // for on demand BLE scans
  BLEscan();
  return true;
}

boolean valid_service_data(const char* data, int size) {
  for (int i = 0; i < size; ++i) {
    if (data[i] != 48) // 48 correspond to 0 in ASCII table
      return true;
  }
  return false;
}

#  ifdef ZmqttDiscovery
// This function always should be called from the main core as it generates direct mqtt messages
// When overrideDiscovery=true, we publish discovery messages of known devices (even if no new)
void launchBTDiscovery(bool overrideDiscovery) {
  if (!overrideDiscovery && newDevices == 0)
    return;
  if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(1000)) == pdFALSE) {
    Log.error(F("Semaphore NOT taken" CR));
    return;
  }
  newDevices = 0;
  vector<BLEdevice*> localDevices = devices;
  xSemaphoreGive(semaphoreCreateOrUpdateDevice);
  for (vector<BLEdevice*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
    BLEdevice* p = *it;
    Log.trace(F("Device mac %s" CR), p->macAdr);
    // Do not launch discovery for the devices already discovered (unless we have overrideDiscovery) or that are not unique by their MAC Address (iBeacon, GAEN and Microsoft CDP)
    if (overrideDiscovery || !isDiscovered(p)) {
      String macWOdots = String(p->macAdr);
      macWOdots.replace(":", "");
      if (!BTConfig.extDecoderEnable && // Do not decode if an external decoder is configured
          p->sensorModel_id > TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL &&
          p->sensorModel_id < TheengsDecoder::BLE_ID_NUM::BLE_ID_MAX &&
          p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC && p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::BM2) { // Exception on HHCCJCY01HHCC and BM2 as these ones are discoverable and connectable
        Log.trace(F("Looking for Model_id: %d" CR), p->sensorModel_id);
        std::string properties = decoder.getTheengProperties(p->sensorModel_id);
        Log.trace(F("properties: %s" CR), properties.c_str());
        std::string brand = decoder.getTheengAttribute(p->sensorModel_id, "brand");
        std::string model = decoder.getTheengAttribute(p->sensorModel_id, "model");
        std::string model_id = decoder.getTheengAttribute(p->sensorModel_id, "model_id");
        String discovery_topic = String(subjectBTtoMQTT) + "/" + macWOdots;
        if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::NUT ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::MIBAND ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::TAGIT ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::TILE ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::TILEN ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::ITAG ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::RUUVITAG_RAWV1 ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::RUUVITAG_RAWV2) {
          String tracker_name = String(model_id.c_str()) + "-tracker";
          String tracker_id = macWOdots + "-tracker";
          createDiscovery("device_tracker",
                          discovery_topic.c_str(), tracker_name.c_str(), tracker_id.c_str(),
                          will_Topic, "occupancy", "{% if value_json.get('rssi') -%}home{%- else -%}not_home{%- endif %}",
                          "", "", "",
                          0, "", "", false, "",
                          model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                          stateClassNone);
        }
        if (!properties.empty()) {
          StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
          deserializeJson(jsonBuffer, properties);
          for (JsonPair prop : jsonBuffer["properties"].as<JsonObject>()) {
            Log.trace(F("Key: %s"), prop.key().c_str());
            Log.trace(F("Unit: %s"), prop.value()["unit"].as<const char*>());
            Log.trace(F("Name: %s"), prop.value()["name"].as<const char*>());
            String entity_name = String(model_id.c_str()) + "-" + String(prop.key().c_str());
            String unique_id = macWOdots + "-" + String(prop.key().c_str());
#    if OpenHABDiscovery
            String value_template = "{{ value_json." + String(prop.key().c_str()) + "}}";
#    else
            String value_template = "{{ value_json." + String(prop.key().c_str()) + " | is_defined }}";
#    endif
            if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBS1 && strcmp(prop.key().c_str(), "state") != 0) {
              String payload_on = "{\"SBS1\":\"on\",\"mac\":\"" + String(p->macAdr) + "\"}";
              String payload_off = "{\"SBS1\":\"off\",\"mac\":\"" + String(p->macAdr) + "\"}";
              createDiscovery("switch", //set Type
                              discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                              will_Topic, "switch", value_template.c_str(),
                              payload_on.c_str(), payload_off.c_str(), "", 0,
                              Gateway_AnnouncementMsg, will_Message, false, subjectMQTTtoBTset,
                              model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                              stateClassNone, "off", "on");
            } else if (strcmp(prop.key().c_str(), "device") != 0) {
              createDiscovery("sensor",
                              discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                              will_Topic, prop.value()["name"], value_template.c_str(),
                              "", "", prop.value()["unit"],
                              0, "", "", false, "",
                              model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                              stateClassMeasurement);
            }
          }
        }
      } else {
        if (p->sensorModel_id > BLEconectable::id::MIN &&
                p->sensorModel_id < BLEconectable::id::MAX ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC || p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::BM2) {
          // Discovery of sensors from which we retrieve data only by connect
          if (p->sensorModel_id == BLEconectable::id::DT24_BLE) {
            DT24Discovery(macWOdots.c_str(), "DT24-BLE");
          }
          if (p->sensorModel_id == BLEconectable::id::BM2) {
            BM2Discovery(macWOdots.c_str(), "BM2");
          }
          if (p->sensorModel_id == BLEconectable::id::LYWSD03MMC) {
            LYWSD03MMCDiscovery(macWOdots.c_str(), "LYWSD03MMC");
          }
          if (p->sensorModel_id == BLEconectable::id::MHO_C401) {
            MHO_C401Discovery(macWOdots.c_str(), "MHO-C401");
          }
          if (p->sensorModel_id == BLEconectable::id::XMWSDJ04MMC) {
            XMWSDJ04MMCDiscovery(macWOdots.c_str(), "XMWSDJ04MMC");
          }
          if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {
            HHCCJCY01HHCCDiscovery(macWOdots.c_str(), "HHCCJCY01HHCC");
          }
        } else {
          Log.trace(F("Device UNKNOWN_MODEL %s" CR), p->macAdr);
        }
      }
      p->isDisc = true; // we don't need the semaphore and all the search magic via createOrUpdateDevice
    } else {
      Log.trace(F("Device already discovered or that doesn't require discovery %s" CR), p->macAdr);
    }
  }
}
#  endif

void PublishDeviceData(JsonObject& BLEdata, bool processBLEData) {
  if (abs((int)BLEdata["rssi"] | 0) < abs(BTConfig.minRssi)) { // process only the devices close enough
    if (processBLEData) process_bledata(BLEdata);
    if (!BTConfig.pubRandomMACs && (BLEdata["type"].as<string>()).compare("RMAC") == 0) {
      return;
    }
    if (!BTConfig.pubAdvData) {
      BLEdata.remove("servicedatauuid");
      BLEdata.remove("servicedata");
      BLEdata.remove("manufacturerdata");
      BLEdata.remove("mac_type");
      BLEdata.remove("adv_type");
      // tag device properties
      // BLEdata.remove("type");   type is used by the WebUI module to determine the template used to display the signal
      BLEdata.remove("cidc");
      BLEdata.remove("acts");
      BLEdata.remove("cont");
      BLEdata.remove("track");
    }
    if (!BTConfig.pubOnlySensors || BLEdata.containsKey("model") || BLEdata.containsKey("distance")) { // Identified device
      pubBT(BLEdata);
    }
  } else if (BLEdata.containsKey("distance")) {
    pubBT(BLEdata);
  } else {
    Log.trace(F("Low rssi, device filtered" CR));
  }
}

void process_bledata(JsonObject& BLEdata) {
  const char* mac = BLEdata["id"].as<const char*>();
  int model_id = BTConfig.extDecoderEnable ? -1 : decoder.decodeBLEJson(BLEdata);
  int mac_type = BLEdata["mac_type"].as<int>();
  if ((BLEdata["type"].as<string>()).compare("RMAC") != 0 && model_id != TheengsDecoder::BLE_ID_NUM::IBEACON) { // Do not store in memory the random mac devices and iBeacons
    if (model_id >= 0) { // Broadcaster devices
      Log.trace(F("Decoder found device: %s" CR), BLEdata["model_id"].as<const char*>());
      if (model_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC || model_id == TheengsDecoder::BLE_ID_NUM::BM2) {
        createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type); // Device that broadcast and can be connected
      } else {
        createOrUpdateDevice(mac, device_flags_init, model_id, mac_type);
        if (BTConfig.adaptiveScan == true && (BTConfig.BLEinterval != MinTimeBtwScan || BTConfig.intervalActiveScan != MinTimeBtwScan)) {
          if (BLEdata.containsKey("acts") && BLEdata.containsKey("cont")) {
            if (BLEdata["acts"] && BLEdata["cont"]) {
              BTConfig.BLEinterval = MinTimeBtwScan;
              BTConfig.intervalActiveScan = MinTimeBtwScan;
              BTConfig.scanDuration = MinScanDuration;
              Log.notice(F("Active and continuous scanning required, paramaters adapted" CR));
              stateBTMeasures(false);
            }
          } else if (BLEdata.containsKey("cont") && BTConfig.BLEinterval != MinTimeBtwScan) {
            if (BLEdata["cont"]) {
              BTConfig.BLEinterval = MinTimeBtwScan;
              if ((BLEdata["type"].as<string>()).compare("CTMO") == 0) {
                BTConfig.scanDuration = MinScanDuration;
              }
              Log.notice(F("Passive continuous scanning required, paramaters adapted" CR));
              stateBTMeasures(false);
            }
          }
        }
      }
    } else {
      if (BLEdata.containsKey("name")) { // Connectable devices
        std::string name = BLEdata["name"];
        if (name.compare("LYWSD03MMC") == 0)
          model_id = BLEconectable::id::LYWSD03MMC;
        else if (name.compare("DT24-BLE") == 0)
          model_id = BLEconectable::id::DT24_BLE;
        else if (name.compare("Battery Monitor") == 0)
          model_id = BLEconectable::id::BM2;
        else if (name.compare("MHO-C401") == 0)
          model_id = BLEconectable::id::MHO_C401;
        else if (name.compare("XMWSDJ04MMC") == 0)
          model_id = BLEconectable::id::XMWSDJ04MMC;

        if (model_id > 0) {
          Log.trace(F("Connectable device found: %s" CR), name.c_str());
          createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type);
        }
      } else if (BTConfig.extDecoderEnable && model_id < 0 && BLEdata.containsKey("servicedata")) {
        const char* service_data = (const char*)(BLEdata["servicedata"] | "");
        if (strstr(service_data, "209800") != NULL) {
          model_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC;
          Log.trace(F("Connectable device found: HHCCJCY01HHCC" CR));
          createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type);
        }
      }
    }
    Log.trace(F("Random MAC or iBeacon device filtered" CR));
  }
  if (!BTConfig.extDecoderEnable && model_id < 0) {
    Log.trace(F("No eligible device found " CR));
  }
}

void hass_presence(JsonObject& HomePresence) {
  int BLErssi = HomePresence["rssi"];
  Log.trace(F("BLErssi %d" CR), BLErssi);
  int txPower = HomePresence["txpower"] | 0;
  if (txPower >= 0)
    txPower = -59; //if tx power is not found we set a default calibration value
  Log.trace(F("TxPower: %d" CR), txPower);
  double ratio = BLErssi * 1.0 / txPower;
  double distance;
  if (ratio < 1.0) {
    distance = pow(ratio, 10);
  } else {
    distance = (0.89976) * pow(ratio, 7.7095) + 0.111;
  }
  HomePresence["distance"] = distance;
  Log.trace(F("Ble distance %D" CR), distance);
}

void BTforceScan() {
  if (!ProcessLock) {
    BTtoMQTT();
    Log.trace(F("Scan done" CR));
    if (BTConfig.bleConnect)
      BLEconnect();
  } else {
    Log.trace(F("Cannot launch scan due to other process running" CR));
  }
}

void immediateBTAction(void* pvParameters) {
  if (BLEactions.size()) {
    // Immediate action; we need to prevent the normal connection action and stop scanning
    ProcessLock = true;
    NimBLEScan* pScan = NimBLEDevice::getScan();
    if (pScan->isScanning()) {
      pScan->stop();
    }

    if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(5000)) == pdTRUE) {
      // swap the vectors so only this device is processed
      std::vector<BLEdevice*> dev_swap;
      dev_swap.push_back(getDeviceByMac(BLEactions.back().addr));
      std::swap(devices, dev_swap);

      std::vector<BLEAction> act_swap;
      act_swap.push_back(BLEactions.back());
      BLEactions.pop_back();
      std::swap(BLEactions, act_swap);

      // Unlock here to allow the action to be performed
      ProcessLock = false;
      BLEconnect();
      // back to normal
      std::swap(devices, dev_swap);
      std::swap(BLEactions, act_swap);

      // If we stopped the scheduled connect for this action, do the scheduled now
      if (millis() > (timeBetweenConnect + BTConfig.intervalConnect) && BTConfig.bleConnect) {
        timeBetweenConnect = millis();
        BLEconnect();
      }
      xSemaphoreGive(semaphoreBLEOperation);
    } else {
      Log.error(F("BLE busy - command not sent" CR));
      JsonObject result = getBTJsonObject();
      result["id"] = BLEactions.back().addr;
      result["success"] = false;
      pubBT(result);
      BLEactions.pop_back();
      ProcessLock = false;
    }
  }
  vTaskDelete(NULL);
}

void startBTActionTask() {
  TaskHandle_t th;
  xTaskCreatePinnedToCore(
      immediateBTAction, /* Function to implement the task */
      "imActTask", /* Name of the task */
      5120, /* Stack size in bytes */
      NULL, /* Task input parameter */
      3, /* Priority of the task (set higher than core task) */
      &th, /* Task handle. */
      1); /* Core where the task should run */
}

void MQTTtoBTAction(JsonObject& BTdata) {
  BLEAction action;
  memset(&action, 0, sizeof(BLEAction));
  if (BTdata.containsKey("SBS1")) {
    strcpy(action.addr, (const char*)BTdata["mac"]);
    action.write = true;
    action.value = BTdata["SBS1"].as<std::string>();
    action.ttl = 1;
    createOrUpdateDevice(action.addr, device_flags_connect,
                         TheengsDecoder::BLE_ID_NUM::SBS1, 1);
    BLEactions.push_back(action);
    startBTActionTask();
    return;
  }

  action.ttl = BTdata.containsKey("ttl") ? (uint8_t)BTdata["ttl"] : 1;
  action.addr_type = BTdata.containsKey("mac_type") ? BTdata["mac_type"].as<int>() : 0;
  action.value_type = BLE_VAL_STRING;
  if (BTdata.containsKey("value_type")) {
    String vt = BTdata["value_type"];
    vt.toUpperCase();
    if (vt == "HEX")
      action.value_type = BLE_VAL_HEX;
    else if (vt == "INT")
      action.value_type = BLE_VAL_INT;
    else if (vt == "FLOAT")
      action.value_type = BLE_VAL_FLOAT;
    else if (vt != "STRING") {
      Log.error(F("BLE value type invalid %s" CR), vt.c_str());
      return;
    }
  }

  Log.trace(F("BLE ACTION TTL = %u" CR), action.ttl);
  action.complete = false;
  if (BTdata.containsKey("ble_write_address") &&
      BTdata.containsKey("ble_write_service") &&
      BTdata.containsKey("ble_write_char") &&
      BTdata.containsKey("ble_write_value")) {
    strcpy(action.addr, (const char*)BTdata["ble_write_address"]);
    action.service = NimBLEUUID((const char*)BTdata["ble_write_service"]);
    action.characteristic = NimBLEUUID((const char*)BTdata["ble_write_char"]);
    action.value = std::string((const char*)BTdata["ble_write_value"]);
    action.write = true;
    Log.trace(F("BLE ACTION Write" CR));
  } else if (BTdata.containsKey("ble_read_address") &&
             BTdata.containsKey("ble_read_service") &&
             BTdata.containsKey("ble_read_char")) {
    strcpy(action.addr, (const char*)BTdata["ble_read_address"]);
    action.service = NimBLEUUID((const char*)BTdata["ble_read_service"]);
    action.characteristic = NimBLEUUID((const char*)BTdata["ble_read_char"]);
    action.write = false;
    Log.trace(F("BLE ACTION Read" CR));
  } else {
    return;
  }

  createOrUpdateDevice(action.addr, device_flags_connect,
                       TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL,
                       action.addr_type);

  BLEactions.push_back(action);
  if (BTdata.containsKey("immediate") && BTdata["immediate"].as<bool>()) {
    startBTActionTask();
  }
}

void MQTTtoBT(char* topicOri, JsonObject& BTdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoBTset)) {
    Log.trace(F("MQTTtoBT json set" CR));

    // Black list & white list set
    bool WorBupdated;
    WorBupdated = updateWorB(BTdata, true);
    WorBupdated |= updateWorB(BTdata, false);

    if (WorBupdated) {
      if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(1000)) == pdTRUE) {
        dumpDevices();
        xSemaphoreGive(semaphoreCreateOrUpdateDevice);
      }
    }

    // Force scan now
    if (BTdata.containsKey("interval") && BTdata["interval"] == 0) {
      Log.notice(F("BLE forced scan" CR));
      atomic_store_explicit(&forceBTScan, 1, ::memory_order_seq_cst); // ask the other core to do the scan for us
    }

    /*
     * Configuration modifications priorities:
     *  First `init=true` and `load=true` commands are executed (if both are present, INIT prevails on LOAD)
     *  Then parameters included in json are taken in account
     *  Finally `erase=true` and `save=true` commands are executed (if both are present, ERASE prevails on SAVE)
     */
    if (BTdata.containsKey("init") && BTdata["init"].as<bool>()) {
      // Restore the default (initial) configuration
      BTConfig_init();
    } else if (BTdata.containsKey("load") && BTdata["load"].as<bool>()) {
      // Load the saved configuration, if not initialised
      BTConfig_load();
    }

    // Load config from json if available
    BTConfig_fromJson(BTdata);

    if (BTdata.containsKey("lowpowermode")) {
      changelowpowermode((int)BTdata["lowpowermode"]);
    }

    MQTTtoBTAction(BTdata);
  }
}
#endif
