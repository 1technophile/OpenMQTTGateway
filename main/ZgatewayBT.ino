/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT

  This gateway enables to:
 - publish MQTT data to a topic related to BLE devices data

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
#  include <esp_bt.h>
#  include <esp_wifi.h>

#  include <atomic>

#  include "ZgatewayBLEConnect.h"
#  include "soc/timer_group_reg.h"
#  include "soc/timer_group_struct.h"

using namespace std;

// Global struct to store live BT configuration data
BTConfig_s BTConfig;

#  if BLEDecoder
#    include <decoder.h>
TheengsDecoder decoder;
#  endif

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
                                       UNKWNON_MODEL};
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
  BTConfig.movingTimer = MovingTimer;
  BTConfig.forcePassiveScan = false;
  BTConfig.enabled = EnableBT;
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
  jo["hasspresence"] = BTConfig.presenceEnable;
  jo["prestopic"] = BTConfig.presenceTopic;
  jo["presuseuuid"] = BTConfig.presenceUseBeaconUuid;
  jo["minrssi"] = -abs(BTConfig.minRssi); // Always export as negative value
  jo["extDecoderEnable"] = BTConfig.extDecoderEnable;
  jo["extDecoderTopic"] = BTConfig.extDecoderTopic;
  jo["pubuuid4topic"] = BTConfig.pubBeaconUuidForTopic;
  jo["ignoreWBlist"] = BTConfig.ignoreWBlist;
  jo["forcepscn"] = BTConfig.forcePassiveScan;
  jo["tskstck"] = uxTaskGetStackHighWaterMark(xProcBLETaskHandle);
  jo["crstck"] = uxTaskGetStackHighWaterMark(xCoreTaskHandle);
  jo["enabled"] = BTConfig.enabled;
  jo["scnct"] = scanCount;
#  if BLEDecoder
  jo["onlysensors"] = BTConfig.pubOnlySensors;
  jo["randommacs"] = BTConfig.pubRandomMACs;
  jo["filterConnectable"] = BTConfig.filterConnectable;
  jo["pubadvdata"] = BTConfig.pubAdvData;
  jo["presenceawaytimer"] = BTConfig.presenceAwayTimer;
  jo["movingtimer"] = BTConfig.movingTimer;
#  endif

  if (start) {
    Log.notice(F("BT sys: "));
    serializeJsonPretty(jsonBuffer, Serial);
    Serial.println();
    return ""; // Do not try to erase/write/send config at startup
  }
  String output;
  serializeJson(jo, output);
  jo["origin"] = subjectBTtoMQTT;
  enqueueJsonObject(jo, QueueSemaphoreTimeOutTask);
  return (output);
}

void BTConfig_fromJson(JsonObject& BTdata, bool startup = false) {
  // Attempts to connect to eligible devices or not
  Config_update(BTdata, "bleconnect", BTConfig.bleConnect);
  // Identify AdaptiveScan deactivation to pass to continuous mode or activation to come back to default settings
  if (startup == false) {
    if (BTdata.containsKey("hasspresence") && BTdata["hasspresence"] == false && BTConfig.presenceEnable == true) {
      BTdata["adaptivescan"] = true;
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
    }
    // Identify if the gateway is enabled or not and stop start accordingly
    if (BTdata.containsKey("enabled") && BTdata["enabled"] == false && BTConfig.enabled == true) {
      stopProcessing();
    } else if (BTdata.containsKey("enabled") && BTdata["enabled"] == true && BTConfig.enabled == false) {
      BTProcessLock = false;
      setupBTTasksAndBLE();
    }
  }
  // Home Assistant presence message
  Config_update(BTdata, "hasspresence", BTConfig.presenceEnable);
#  ifdef ZmqttDiscovery
  // Create discovery entities
  btScanParametersDiscovery();
  btPresenceParametersDiscovery();
#  endif
  // Time before before active scan
  // Scan interval set - and avoid intervalacts to be lower than interval
  if (BTdata.containsKey("interval") && BTdata["interval"] != 0) {
    BTConfig.adaptiveScan = false;
    Config_update(BTdata, "interval", BTConfig.BLEinterval);
    if (BTConfig.intervalActiveScan < BTConfig.BLEinterval) {
      Config_update(BTdata, "interval", BTConfig.intervalActiveScan);
    }
  }
  // Define if the scan is adaptive or not - and avoid intervalacts to be lower than interval
  if (BTdata.containsKey("intervalacts") && BTdata["intervalacts"] < BTConfig.BLEinterval) {
    BTConfig.adaptiveScan = false;
    // Config_update(BTdata, "interval", BTConfig.intervalActiveScan);
    BTConfig.intervalActiveScan = BTConfig.BLEinterval;
  } else {
    Config_update(BTdata, "intervalacts", BTConfig.intervalActiveScan);
  }
  //  Adaptive scan set
  Config_update(BTdata, "adaptivescan", BTConfig.adaptiveScan);
  // Time before a connect set
  Config_update(BTdata, "intervalcnct", BTConfig.intervalConnect);
  // publish all BLE devices discovered or  only the identified sensors (like temperature sensors)
  Config_update(BTdata, "scanduration", BTConfig.scanDuration);
  // define the duration for a scan; in milliseconds
  Config_update(BTdata, "onlysensors", BTConfig.pubOnlySensors);
  // publish devices which randomly change their MAC addresses
  Config_update(BTdata, "randommacs", BTConfig.pubRandomMACs);
  // Home Assistant presence message topic
  Config_update(BTdata, "prestopic", BTConfig.presenceTopic);
  // Home Assistant presence message use iBeacon UUID
  Config_update(BTdata, "presuseuuid", BTConfig.presenceUseBeaconUuid);
  // Timer to trigger a device state as offline if not seen
  Config_update(BTdata, "presenceawaytimer", BTConfig.presenceAwayTimer);
  // Timer to trigger a device state as offline if not seen
  Config_update(BTdata, "movingtimer", BTConfig.movingTimer);
  // Force passive scan
  Config_update(BTdata, "forcepscn", BTConfig.forcePassiveScan);
  // MinRSSI set
  Config_update(BTdata, "minrssi", BTConfig.minRssi);
  // Send undecoded device data
  Config_update(BTdata, "extDecoderEnable", BTConfig.extDecoderEnable);
  // Topic to send undecoded device data
  Config_update(BTdata, "extDecoderTopic", BTConfig.extDecoderTopic);
  // Sets whether to filter publishing
  Config_update(BTdata, "filterConnectable", BTConfig.filterConnectable);
  // Publish advertisement data
  Config_update(BTdata, "pubadvdata", BTConfig.pubAdvData);
  // Use iBeacon UUID as topic, instead of sender (random) MAC address
  Config_update(BTdata, "pubuuid4topic", BTConfig.pubBeaconUuidForTopic);
  // Disable Whitelist & Blacklist
  Config_update(BTdata, "ignoreWBlist", (BTConfig.ignoreWBlist));
  // Enable or disable the BT gateway
  Config_update(BTdata, "enabled", BTConfig.enabled);

  stateBTMeasures(startup);

  if (BTdata.containsKey("erase") && BTdata["erase"].as<bool>()) {
    // Erase config from NVS (non-volatile storage)
    preferences.begin(Gateway_Short_Name, false);
    if (preferences.isKey("BTConfig")) {
      int result = preferences.remove("BTConfig");
      Log.notice(F("BT config erase result: %d" CR), result);
      preferences.end();
      return; // Erase prevails on save, so skipping save
    } else {
      preferences.end();
      Log.notice(F("BT config not found" CR));
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
    jo["prestopic"] = BTConfig.presenceTopic;
    jo["presuseuuid"] = BTConfig.presenceUseBeaconUuid;
    jo["minrssi"] = -abs(BTConfig.minRssi); // Always export as negative value
    jo["extDecoderEnable"] = BTConfig.extDecoderEnable;
    jo["extDecoderTopic"] = BTConfig.extDecoderTopic;
    jo["filterConnectable"] = BTConfig.filterConnectable;
    jo["pubadvdata"] = BTConfig.pubAdvData;
    jo["pubuuid4topic"] = BTConfig.pubBeaconUuidForTopic;
    jo["ignoreWBlist"] = BTConfig.ignoreWBlist;
    jo["presenceawaytimer"] = BTConfig.presenceAwayTimer;
    jo["movingtimer"] = BTConfig.movingTimer;
    jo["forcepscn"] = BTConfig.forcePassiveScan;
    jo["enabled"] = BTConfig.enabled;
    // Save config into NVS (non-volatile storage)
    String conf = "";
    serializeJson(jsonBuffer, conf);
    preferences.begin(Gateway_Short_Name, false);
    int result = preferences.putString("BTConfig", conf);
    preferences.end();
    Log.notice(F("BT config save: %s, result: %d" CR), conf.c_str(), result);
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
  } else {
    preferences.end();
    Log.notice(F("BT config not found" CR));
  }
}

void PublishDeviceData(JsonObject& BLEdata);

atomic_int forceBTScan;

void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type = 0, const char* name = "");

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
                         UNKWNON_MODEL);
  }

  return true;
}

void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type, const char* name) {
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
    // Check name length
    if (strlen(name) > 20) {
      Log.warning(F("Name too long, truncating" CR));
      strncpy(device->name, name, 20);
      device->name[20] = '\0';
    } else {
      strcpy(device->name, name);
    }
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

    if (model != UNKWNON_MODEL) {
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
    // Check for tracker status
    bool isTracker = false;
#  if BLEDecoder
    std::string tag = decoder.getTheengAttribute(p->sensorModel_id, "tag");
    if (tag.length() >= 4) {
      isTracker = checkIfIsTracker(tag[3]);
    }
    // Device tracker devices
    if (isTracker) { // We apply the offline status only for tracking device, can be extended further to all the devices
      if ((p->lastUpdate != 0) && (p->lastUpdate < (now - BTConfig.presenceAwayTimer) && (now > BTConfig.presenceAwayTimer)) &&
          (BTConfig.ignoreWBlist || ((!oneWhite || isWhite(p)) && !isBlack(p)))) { // Only if WBlist is disabled OR ((no white MAC OR this MAC is white) AND not a black listed MAC)) {
        StaticJsonDocument<JSON_MSG_BUFFER> BLEdataBuffer;
        JsonObject BLEdata = BLEdataBuffer.to<JsonObject>();
        BLEdata["id"] = p->macAdr;
        BLEdata["state"] = "offline";
        buildTopicFromId(BLEdata, subjectBTtoMQTT);
        enqueueJsonObject(BLEdata, QueueSemaphoreTimeOutTask);
        // We set the lastUpdate to 0 to avoid replublishing the offline state
        p->lastUpdate = 0;
      }
    }
    // Moving detection devices (devices with an accelerometer)
    if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::BC08) {
      if ((p->lastUpdate != 0) && (p->lastUpdate < (now - BTConfig.movingTimer) && (now > BTConfig.movingTimer)) &&
          (BTConfig.ignoreWBlist || ((!oneWhite || isWhite(p)) && !isBlack(p)))) { // Only if WBlist is disabled OR ((no white MAC OR this MAC is white) AND not a black listed MAC)) {
        StaticJsonDocument<JSON_MSG_BUFFER> BLEdataBuffer;
        JsonObject BLEdata = BLEdataBuffer.to<JsonObject>();
        BLEdata["id"] = p->macAdr;
        BLEdata["state"] = "offline";
        buildTopicFromId(BLEdata, subjectBTtoMQTT);
        enqueueJsonObject(BLEdata, QueueSemaphoreTimeOutTask);
        // We set the lastUpdate to 0 to avoid replublishing the offline state
        p->lastUpdate = 0;
      }
    }
#  endif
  }
}

void dumpDevices() {
#  if LOG_LEVEL > LOG_LEVEL_NOTICE
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

bool checkIfIsTracker(char ch) {
  uint8_t data = 0;
  if (ch >= '0' && ch <= '9')
    data = ch - '0';
  else if (ch >= 'a' && ch <= 'f')
    data = 10 + (ch - 'a');

  if (((data >> 3) & 0x01) == 1) {
    Log.trace(F("Is Device Tracker" CR));
    return true;
  } else {
    return false;
  }
}

void procBLETask(void* pvParameters) {
  BLEAdvertisedDevice* advertisedDevice = nullptr;

  for (;;) {
    xQueueReceive(BLEQueue, &advertisedDevice, portMAX_DELAY);
    // Feed the watchdog
    //esp_task_wdt_reset();
    if (!BTProcessLock) {
      Log.trace(F("Creating BLE buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> BLEdataBuffer;
      JsonObject BLEdata = BLEdataBuffer.to<JsonObject>();
      std::string mac_address = advertisedDevice->getAddress().toString();

      for (char& c : mac_address) {
        c = std::toupper(c);
      }

      BLEdata["id"] = mac_address;
      BLEdata["mac_type"] = advertisedDevice->getAddress().getType();
      BLEdata["adv_type"] = advertisedDevice->getAdvType();
      Log.notice(F("BT Device detected: %s" CR), BLEdata["id"].as<const char*>());
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
        if (advertisedDevice->haveRSSI() && BTConfig.presenceEnable) {
          hass_presence(BLEdata); // this device has an rssi and with either only sensors or not we can use it for home assistant room presence component
        }
        if (advertisedDevice->haveServiceData()) {
          int serviceDataCount = advertisedDevice->getServiceDataCount();
          Log.trace(F("Get services data number: %d" CR), serviceDataCount);
          for (int j = 0; j < serviceDataCount; j++) {
            StaticJsonDocument<JSON_MSG_BUFFER> BLEdataBufferTemp;
            JsonObject BLEdataTemp = BLEdataBufferTemp.to<JsonObject>();
            BLEdataBufferTemp = BLEdataBuffer;
            std::string service_data = convertServiceData(advertisedDevice->getServiceData(j));
            Log.trace(F("Service data: %s" CR), service_data.c_str());
            std::string serviceDatauuid = advertisedDevice->getServiceDataUUID(j).toString();
            Log.trace(F("Service data UUID: %s" CR), (char*)serviceDatauuid.c_str());
            BLEdataTemp["servicedata"] = (char*)service_data.c_str();
            BLEdataTemp["servicedatauuid"] = (char*)serviceDatauuid.c_str();
            PublishDeviceData(BLEdataTemp);
          }
        } else {
          PublishDeviceData(BLEdata);
        }
      } else {
        Log.trace(F("Filtered MAC device" CR));
      }
      updateDevicesStatus();
    }
    delete (advertisedDevice);
    vTaskDelay(10);
  }
}

/**
 * BLEscan used to retrieve BLE advertized data from devices without connection
 */
void BLEscan() {
  // Don't start the next scan until processing of previous results is complete.
  while (uxQueueMessagesWaiting(BLEQueue) || queueLength != 0) { // the criteria on queueLength could be adjusted to parallelize the scan and the queue processing
    delay(1); // Wait for queue to empty, a yield here instead of the delay cause the WDT to trigger
  }
  Log.notice(F("Scan begin" CR));
  BLEScan* pBLEScan = BLEDevice::getScan();
  MyAdvertisedDeviceCallbacks myCallbacks;
  pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
  if ((millis() > (timeBetweenActive + BTConfig.intervalActiveScan) || BTConfig.intervalActiveScan == BTConfig.BLEinterval) && !BTConfig.forcePassiveScan) {
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
#  if BLEDecoder
void BLEconnect() {
  if (!BTProcessLock) {
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
          } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::BM2) {
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
          } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBBT) {
            SBBT_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
          } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBCU) {
            SBCU_connect BLEclient(addr);
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
                    p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::BM2 &&
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
#  else
void BLEconnect() {}
#  endif

void stopProcessing() {
  if (BTConfig.enabled) {
    BTProcessLock = true;
    // We stop the scan
    Log.notice(F("Stopping BLE scan" CR));
    BLEScan* pBLEScan = BLEDevice::getScan();
    if (pBLEScan->isScanning()) {
      pBLEScan->stop();
    }

    if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(5000)) == pdTRUE) {
      Log.notice(F("Stopping BLE tasks" CR));
      //Suspending, deleting tasks and stopping BT to free memory
      vTaskSuspend(xCoreTaskHandle);
      vTaskDelete(xCoreTaskHandle);
      vTaskSuspend(xProcBLETaskHandle);
      vTaskDelete(xProcBLETaskHandle);
      xSemaphoreGive(semaphoreBLEOperation);
    }
  }
  Log.notice(F("BLE gateway stopped, free heap: %d" CR), ESP.getFreeHeap());
}

void coreTask(void* pvParameters) {
  while (true) {
    if (!BTProcessLock) {
      if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
        BLEscan();
        // Launching a connect every TimeBtwConnect
        if (millis() > (timeBetweenConnect + BTConfig.intervalConnect) && BTConfig.bleConnect) {
          timeBetweenConnect = millis();
          BLEconnect();
        }
        //dumpDevices();
        Log.trace(F("CoreTask stack free: %u" CR), uxTaskGetStackHighWaterMark(xCoreTaskHandle));
        xSemaphoreGive(semaphoreBLEOperation);
      } else {
        Log.error(F("Failed to start scan - BLE busy" CR));
      }
      if (SYSConfig.powerMode > 0) {
        int scan = atomic_exchange_explicit(&forceBTScan, 0, ::memory_order_seq_cst); // is this enough, it will wait the full deepsleep...
        if (scan == 1) BTforceScan();
        ready_to_sleep = true;
      } else {
        for (int interval = BTConfig.BLEinterval, waitms; interval > 0; interval -= waitms) {
          int scan = atomic_exchange_explicit(&forceBTScan, 0, ::memory_order_seq_cst);
          if (scan == 1) BTforceScan(); // should we break after this?
          delay(waitms = interval > 100 ? 100 : interval); // 100ms
        }
      }
    }
    delay(1);
  }
}

void setupBTTasksAndBLE() {
  BLEDevice::setScanDuplicateCacheSize(BLEScanDuplicateCacheSize);
  BLEDevice::init("");
  xTaskCreatePinnedToCore(
      procBLETask, /* Function to implement the task */
      "procBLETask", /* Name of the task */
#  if defined(USE_ESP_IDF) || defined(USE_BLUFI)
      13500,
#  else
      8500, /* Stack size in bytes */
#  endif
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
  Log.notice(F("Presence Away Timer: %d" CR), BTConfig.presenceAwayTimer);
  Log.notice(F("Moving Timer: %d" CR), BTConfig.movingTimer);
  Log.notice(F("Force passive scan: %T" CR), BTConfig.forcePassiveScan);
  Log.notice(F("Enabled BLE: %T" CR), BTConfig.enabled);

  atomic_init(&forceBTScan, 0); // in theory, we don't need this

  semaphoreCreateOrUpdateDevice = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreCreateOrUpdateDevice);

  semaphoreBLEOperation = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreBLEOperation);

  BLEQueue = xQueueCreate(QueueSize, sizeof(NimBLEAdvertisedDevice*));
  if (BTConfig.enabled) {
    setupBTTasksAndBLE();
    Log.notice(F("ZgatewayBT multicore ESP32 setup done" CR));
  } else {
    Log.notice(F("ZgatewayBT multicore ESP32 setup disabled" CR));
  }
}

boolean valid_service_data(const char* data, int size) {
  for (int i = 0; i < size; ++i) {
    if (data[i] != 48) // 48 correspond to 0 in ASCII table
      return true;
  }
  return false;
}

#  if defined(ZmqttDiscovery) && BLEDecoder == true
// This function always should be called from the main core as it generates direct mqtt messages
// When overrideDiscovery=true, we publish discovery messages of known devices (even if no new)
void launchBTDiscovery(bool overrideDiscovery) {
  if (!overrideDiscovery && newDevices == 0)
    return;
  if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(QueueSemaphoreTimeOutTask)) == pdFALSE) {
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
      if (p->sensorModel_id >= 0) {
        Log.trace(F("Looking for Model_id: %d" CR), p->sensorModel_id);
        std::string properties = decoder.getTheengProperties(p->sensorModel_id);
        Log.trace(F("properties: %s" CR), properties.c_str());
        std::string brand = decoder.getTheengAttribute(p->sensorModel_id, "brand");
        std::string model = decoder.getTheengAttribute(p->sensorModel_id, "model");
#    if ForceDeviceName
        if (p->name[0] != '\0') {
          model = p->name;
        }
#    endif
        std::string model_id = decoder.getTheengAttribute(p->sensorModel_id, "model_id");

        // Check for tracker status
        bool isTracker = false;
        std::string tag = decoder.getTheengAttribute(p->sensorModel_id, "tag");
        if (tag.length() >= 4) {
          isTracker = checkIfIsTracker(tag[3]);
        }

        String discovery_topic = String(subjectBTtoMQTT) + "/" + macWOdots;
        if (!BTConfig.extDecoderEnable && // Do not decode if an external decoder is configured
            p->sensorModel_id > UNKWNON_MODEL &&
            p->sensorModel_id < TheengsDecoder::BLE_ID_NUM::BLE_ID_MAX &&
            p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC && p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::BM2) { // Exception on HHCCJCY01HHCC and BM2 as these ones are discoverable and connectable
          if (isTracker) {
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
          if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::BC08) {
            String sensor_name = String(model_id.c_str()) + "-moving";
            String sensor_id = macWOdots + "-moving";
            createDiscovery("binary_sensor",
                            discovery_topic.c_str(), sensor_name.c_str(), sensor_id.c_str(),
                            will_Topic, "moving", "{% if value_json.get('accx') -%}on{%- else -%}off{%- endif %}",
                            "on", "off", "",
                            0, "", "", false, "",
                            model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                            stateClassNone);
          }
          if (!properties.empty()) {
            StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
            auto error = deserializeJson(jsonBuffer, properties);
            if (error) {
              if (jsonBuffer.overflowed()) {
                // This should not happen if JSON_MSG_BUFFER is large enough for
                // the Theengs json properties
                Log.error(F("JSON deserialization of Theengs properties overflowed (error %s), buffer capacity: %u. Program might crash. Properties json: %s" CR),
                          error.c_str(), jsonBuffer.capacity(), properties.c_str());
              } else {
                Log.error(F("JSON deserialization of Theengs properties errored: %" CR),
                          error.c_str());
              }
            }
            for (JsonPair prop : jsonBuffer["properties"].as<JsonObject>()) {
              Log.trace(F("Key: %s"), prop.key().c_str());
              Log.trace(F("Unit: %s"), prop.value()["unit"].as<const char*>());
              Log.trace(F("Name: %s"), prop.value()["name"].as<const char*>());
              String entity_name = String(model_id.c_str()) + "-" + String(prop.key().c_str());
              String unique_id = macWOdots + "-" + String(prop.key().c_str());
              String value_template = "{{ value_json." + String(prop.key().c_str()) + " | is_defined }}";
              if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBS1 && strcmp(prop.key().c_str(), "state") == 0) {
                String payload_on = "{\"model_id\":\"X1\",\"cmd\":\"on\",\"id\":\"" + String(p->macAdr) + "\"}";
                String payload_off = "{\"model_id\":\"X1\",\"cmd\":\"off\",\"id\":\"" + String(p->macAdr) + "\"}";
                createDiscovery("switch", //set Type
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, "switch", value_template.c_str(),
                                payload_on.c_str(), payload_off.c_str(), "", 0,
                                Gateway_AnnouncementMsg, will_Message, false, subjectMQTTtoBT,
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                stateClassNone, "off", "on");
                unique_id = macWOdots + "-press";
                entity_name = String(model_id.c_str()) + "-press";
                String payload_press = "{\"model_id\":\"X1\",\"cmd\":\"press\",\"id\":\"" + String(p->macAdr) + "\"}";
                createDiscovery("button", //set Type
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, "button", "",
                                payload_press.c_str(), "", "", //set,payload_on,payload_off,unit_of_meas,
                                0, //set  off_delay
                                Gateway_AnnouncementMsg, will_Message, false, subjectMQTTtoBT,
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                stateClassNone);
              } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBBT && strcmp(prop.key().c_str(), "open") == 0) {
                value_template = "{% if value_json.direction == \"up\" -%} {{ 100 - value_json.open/2 }}{% elif value_json.direction == \"down\" %}{{ value_json.open/2 }}{% else %} {{ value_json.open/2 }}{%- endif %}";
                String command_template = "{\"model_id\":\"W270160X\",\"tilt\":{{ value | int }},\"id\":\"" + String(p->macAdr) + "\"}";
                createDiscovery("cover", //set Type
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, "cover", value_template.c_str(),
                                "50", "", "", 0,
                                Gateway_AnnouncementMsg, will_Message, false, subjectMQTTtoBT,
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                "blind", nullptr, nullptr, nullptr, command_template.c_str());
              } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBCU && strcmp(prop.key().c_str(), "position") == 0) {
                String command_template = "{\"model_id\":\"W070160X\",\"position\":{{ value | int }},\"id\":\"" + String(p->macAdr) + "\"}";
                createDiscovery("cover", //set Type
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, "cover", "{{ value_json.position }}",
                                "0", "100", "", 0,
                                Gateway_AnnouncementMsg, will_Message, false, subjectMQTTtoBT,
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                "curtain", nullptr, nullptr, nullptr, command_template.c_str());
              } else if ((p->sensorModel_id == TheengsDecoder::XMTZC04HMKG || p->sensorModel_id == TheengsDecoder::XMTZC04HMLB || p->sensorModel_id == TheengsDecoder::XMTZC05HMKG || p->sensorModel_id == TheengsDecoder::XMTZC05HMLB) &&
                         strcmp(prop.key().c_str(), "weighing_mode") == 0) {
                createDiscovery("sensor",
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, "enum", value_template.c_str(),
                                "", "", prop.value()["unit"],
                                0, "", "", false, "",
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                stateClassMeasurement, nullptr, nullptr, "[\"person\",\"object\"]");
              } else if ((p->sensorModel_id == TheengsDecoder::XMTZC04HMKG || p->sensorModel_id == TheengsDecoder::XMTZC04HMLB || p->sensorModel_id == TheengsDecoder::XMTZC05HMKG || p->sensorModel_id == TheengsDecoder::XMTZC05HMLB) &&
                         strcmp(prop.key().c_str(), "unit") == 0) {
                createDiscovery("sensor",
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, "enum", value_template.c_str(),
                                "", "", prop.value()["unit"],
                                0, "", "", false, "",
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                stateClassMeasurement, nullptr, nullptr, "[\"lb\",\"kg\",\"jin\"]");
              } else if (strcmp(prop.value()["unit"], "string") == 0 && strcmp(prop.key().c_str(), "mac") != 0) {
                createDiscovery("sensor",
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, prop.value()["name"], value_template.c_str(),
                                "", "", "",
                                0, "", "", false, "",
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                stateClassNone);
              } else if (p->sensorModel_id == TheengsDecoder::MUE4094RT && strcmp(prop.value()["unit"], "status") == 0) { // This device does not a broadcast when there is nothing detected so adding a timeout
                createDiscovery("binary_sensor",
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, prop.value()["name"], value_template.c_str(),
                                "True", "False", "",
                                BTConfig.presenceAwayTimer / 1000, "", "", false, "",
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                stateClassNone);
              } else if (strcmp(prop.value()["unit"], "status") == 0) {
                createDiscovery("binary_sensor",
                                discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                                will_Topic, prop.value()["name"], value_template.c_str(),
                                "True", "False", "",
                                0, "", "", false, "",
                                model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                                stateClassNone);
              } else if (strcmp(prop.key().c_str(), "device") != 0 && strcmp(prop.key().c_str(), "mac") != 0) { // Exception on device and mac as these ones are not sensors
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
            if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::BM2) {
              // Sensor discovery
              BM2Discovery(macWOdots.c_str(), "BM2");
              // Device tracker discovery
              String tracker_id = macWOdots + "-tracker";
              createDiscovery("device_tracker",
                              discovery_topic.c_str(), "BM2-tracker", tracker_id.c_str(),
                              will_Topic, "occupancy", "{% if value_json.get('rssi') -%}home{%- else -%}not_home{%- endif %}",
                              "", "", "",
                              0, "", "", false, "",
                              model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                              stateClassNone);
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
      }
      p->isDisc = true; // we don't need the semaphore and all the search magic via createOrUpdateDevice
    } else {
      Log.trace(F("Device already discovered or that doesn't require discovery %s" CR), p->macAdr);
    }
  }
}
#  else
void launchBTDiscovery(bool overrideDiscovery) {}
#  endif

#  if BLEDecoder
void process_bledata(JsonObject& BLEdata) {
  yield(); // Necessary to let the loop run in case of connectivity issues
  if (!BLEdata.containsKey("id")) {
    Log.error(F("No mac address in the payload" CR));
    return;
  }
  const char* mac = BLEdata["id"].as<const char*>();
  Log.trace(F("Processing BLE data %s" CR), BLEdata["id"].as<const char*>());
  int model_id = BTConfig.extDecoderEnable ? -1 : decoder.decodeBLEJson(BLEdata);
  int mac_type = BLEdata["mac_type"].as<int>();

  // Convert prmacs to RMACS until or if OMG gets Identity MAC/IRK decoding
  if (BLEdata["prmac"]) {
    BLEdata.remove("prmac");
    if (BLEdata["track"]) {
      BLEdata.remove("track");
    }
    BLEdata["type"] = "RMAC";
    Log.trace(F("Potential RMAC (prmac) converted to RMAC" CR));
  }
  const char* deviceName = BLEdata["name"] | "";

  if ((BLEdata["type"].as<string>()).compare("RMAC") != 0 && model_id != TheengsDecoder::BLE_ID_NUM::IBEACON) { // Do not store in memory the random mac devices and iBeacons
    if (model_id >= 0) { // Broadcaster devices
      Log.trace(F("Decoder found device: %s" CR), BLEdata["model_id"].as<const char*>());
      if (model_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC || model_id == TheengsDecoder::BLE_ID_NUM::BM2) { // Device that broadcast and can be connected
        createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type, deviceName);
      } else {
        createOrUpdateDevice(mac, device_flags_init, model_id, mac_type, deviceName);
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
      if (BLEdata.containsKey("name")) { // Connectable only devices
        std::string name = BLEdata["name"];
        if (name.compare("LYWSD03MMC") == 0)
          model_id = BLEconectable::id::LYWSD03MMC;
        else if (name.compare("DT24-BLE") == 0)
          model_id = BLEconectable::id::DT24_BLE;
        else if (name.compare("MHO-C401") == 0)
          model_id = BLEconectable::id::MHO_C401;
        else if (name.compare("XMWSDJ04MMC") == 0)
          model_id = BLEconectable::id::XMWSDJ04MMC;

        if (model_id > 0) {
          Log.trace(F("Connectable device found: %s" CR), name.c_str());
          createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type, deviceName);
        }
      } else if (BTConfig.extDecoderEnable && model_id < 0 && BLEdata.containsKey("servicedata")) {
        const char* service_data = (const char*)(BLEdata["servicedata"] | "");
        if (strstr(service_data, "209800") != NULL) {
          model_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC;
          Log.trace(F("Connectable device found: HHCCJCY01HHCC" CR));
          createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type, deviceName);
        }
      }
    }
  } else {
    Log.trace(F("Random MAC or iBeacon device filtered" CR));
  }
  if (!BTConfig.extDecoderEnable && model_id < 0) {
    Log.trace(F("No eligible device found " CR));
  }
}
void PublishDeviceData(JsonObject& BLEdata) {
  if (abs((int)BLEdata["rssi"] | 0) < abs(BTConfig.minRssi)) { // process only the devices close enough
    // Decode the payload
    process_bledata(BLEdata);
    // If the device is a random MAC and pubRandomMACs is false we don't publish this payload
    if (!BTConfig.pubRandomMACs && (BLEdata["type"].as<string>()).compare("RMAC") == 0) {
      Log.trace(F("Random MAC, device filtered" CR));
      return;
    }
    // If pubAdvData is false we don't publish the adv data
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
      BLEdata.remove("ctrl");
    }
    // if distance available, check if presenceUseBeaconUuid is true, model_id is IBEACON then set id as uuid
    if (BLEdata.containsKey("distance")) {
      if (BTConfig.presenceUseBeaconUuid && BLEdata.containsKey("model_id") && BLEdata["model_id"].as<String>() == "IBEACON") {
        BLEdata["mac"] = BLEdata["id"].as<std::string>();
        BLEdata["id"] = BLEdata["uuid"].as<std::string>();
      }
      String topic = String(mqtt_topic) + BTConfig.presenceTopic + String(gateway_name);
      Log.trace(F("Pub HA Presence %s" CR), topic.c_str());
      BLEdata["topic"] = topic;
      enqueueJsonObject(BLEdata, QueueSemaphoreTimeOutTask);
    }

    // If the device is not a sensor and pubOnlySensors is true we don't publish this payload
    if (!BTConfig.pubOnlySensors || BLEdata.containsKey("model") || !BLEDecoder) { // Identified device
      buildTopicFromId(BLEdata, subjectBTtoMQTT);
      enqueueJsonObject(BLEdata, QueueSemaphoreTimeOutTask);
    } else {
      Log.notice(F("Not a sensor device filtered" CR));
      return;
    }

#    if BLEDecoder
    if (enableMultiGTWSync && BLEdata.containsKey("model_id") && BLEdata.containsKey("id")) {
      // Publish tracker sync message
      bool isTracker = false;
      std::string tag = decoder.getTheengAttribute(BLEdata["model_id"].as<const char*>(), "tag");
      if (tag.length() >= 4) {
        isTracker = checkIfIsTracker(tag[3]);
      }

      if (isTracker) {
        StaticJsonDocument<JSON_MSG_BUFFER> BLEdataBuffer;
        JsonObject TrackerSyncdata = BLEdataBuffer.to<JsonObject>();
        TrackerSyncdata["gatewayid"] = gateway_name;
        TrackerSyncdata["trackerid"] = BLEdata["id"].as<const char*>();
        String topic = String(mqtt_topic) + String(subjectTrackerSync);
        TrackerSyncdata["topic"] = topic.c_str();
        enqueueJsonObject(TrackerSyncdata);
      }
    }
#    endif
  } else {
    Log.notice(F("Low rssi, device filtered" CR));
    return;
  }
}
#  else
void process_bledata(JsonObject& BLEdata) {}
void PublishDeviceData(JsonObject& BLEdata) {
  if (abs((int)BLEdata["rssi"] | 0) < abs(BTConfig.minRssi)) { // process only the devices close enough
    // if distance available, check if presenceUseBeaconUuid is true, model_id is IBEACON then set id as uuid
    if (BLEdata.containsKey("distance")) {
      if (BTConfig.presenceUseBeaconUuid && BLEdata.containsKey("model_id") && BLEdata["model_id"].as<String>() == "IBEACON") {
        BLEdata["mac"] = BLEdata["id"].as<std::string>();
        BLEdata["id"] = BLEdata["uuid"].as<std::string>();
      }
      enqueueJsonObject(BLEdata, QueueSemaphoreTimeOutTask);
    }
    buildTopicFromId(BLEdata, subjectBTtoMQTT);
    enqueueJsonObject(BLEdata, QueueSemaphoreTimeOutTask);
  } else {
    Log.notice(F("Low rssi, device filtered" CR));
    return;
  }
}
#  endif

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
  if (!BTProcessLock) {
    BLEscan();
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
    BTProcessLock = true;
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
      BTProcessLock = false;
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
      StaticJsonDocument<JSON_MSG_BUFFER> BLEdataBuffer;
      JsonObject BLEdata = BLEdataBuffer.to<JsonObject>();
      BLEdata["id"] = BLEactions.back().addr;
      BLEdata["success"] = false;
      buildTopicFromId(BLEdata, subjectBTtoMQTT);
      enqueueJsonObject(BLEdata, QueueSemaphoreTimeOutTask);
      BLEactions.pop_back();
      BTProcessLock = false;
    }
  }
  vTaskDelete(NULL);
}

void startBTActionTask() {
  TaskHandle_t th;
  xTaskCreatePinnedToCore(
      immediateBTAction, /* Function to implement the task */
      "imActTask", /* Name of the task */
      8000, /* Stack size in bytes */
      NULL, /* Task input parameter */
      3, /* Priority of the task (set higher than core task) */
      &th, /* Task handle. */
      1); /* Core where the task should run */
}

#  if BLEDecoder
void KnownBTActions(JsonObject& BTdata) {
  if (!BTdata.containsKey("id")) {
    Log.error(F("BLE mac address missing" CR));
    gatewayState = GatewayState::ERROR;
    return;
  }
  BLEAction action;
  memset(&action, 0, sizeof(BLEAction));
  strcpy(action.addr, (const char*)BTdata["id"]);
  action.write = true;
  action.ttl = 3;
  bool res = false;
  if (BTdata.containsKey("model_id") && BTdata["model_id"].is<const char*>()) {
    if (BTdata["model_id"] == "X1") {
      if (BTdata.containsKey("cmd") && BTdata["cmd"].is<const char*>()) {
        action.value_type = BLE_VAL_STRING;
        std::string val = BTdata["cmd"].as<std::string>(); // Fix #1694
        action.value = val;
        createOrUpdateDevice(action.addr, device_flags_connect,
                             TheengsDecoder::BLE_ID_NUM::SBS1, 1);
        res = true;
      }
    } else if (BTdata["model_id"] == "W270160X") {
      if (BTdata.containsKey("tilt") && BTdata["tilt"].is<int>()) {
        action.value_type = BLE_VAL_INT;
        res = true;
      } else if (BTdata.containsKey("tilt") && BTdata["tilt"].is<const char*>()) {
        action.value_type = BLE_VAL_STRING;
        res = true;
      }
      if (res) {
        std::string val = BTdata["tilt"].as<std::string>(); // Fix #1694
        action.value = val;
        createOrUpdateDevice(action.addr, device_flags_connect,
                             TheengsDecoder::BLE_ID_NUM::SBBT, 1);
      }
    } else if (BTdata["model_id"] == "W070160X") {
      if (BTdata.containsKey("position") && BTdata["position"].is<int>()) {
        action.value_type = BLE_VAL_INT;
        res = true;
      } else if (BTdata.containsKey("position") && BTdata["position"].is<const char*>()) {
        action.value_type = BLE_VAL_STRING;
        res = true;
      }
      if (res) {
        std::string val = BTdata["position"].as<std::string>(); // Fix #1694
        action.value = val;
        createOrUpdateDevice(action.addr, device_flags_connect,
                             TheengsDecoder::BLE_ID_NUM::SBCU, 1);
      }
    }
    if (res) {
      BLEactions.push_back(action);
      startBTActionTask();
    } else {
      Log.error(F("BLE action not recognized" CR));
      gatewayState = GatewayState::ERROR;
    }
  }
}
#  else
void KnownBTActions(JsonObject& BTdata) {}
#  endif

void XtoBTAction(JsonObject& BTdata) {
  BLEAction action;
  memset(&action, 0, sizeof(BLEAction));
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
    std::string val = BTdata["ble_write_value"].as<std::string>(); // Fix #1694
    action.value = val;
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
                       UNKWNON_MODEL,
                       action.addr_type);

  BLEactions.push_back(action);
  if (BTdata.containsKey("immediate") && BTdata["immediate"].as<bool>()) {
    startBTActionTask();
  }
}

void XtoBT(const char* topicOri, JsonObject& BTdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoBTset)) {
    Log.trace(F("MQTTtoBT json set" CR));

    // Black list & white list set
    bool WorBupdated;
    WorBupdated = updateWorB(BTdata, true);
    WorBupdated |= updateWorB(BTdata, false);

    if (WorBupdated) {
      if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(QueueSemaphoreTimeOutTask)) == pdTRUE) {
        //dumpDevices();
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

  } else if (cmpToMainTopic(topicOri, subjectMQTTtoBT)) {
    if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(5000)) == pdTRUE) {
      KnownBTActions(BTdata);
      XtoBTAction(BTdata);
      xSemaphoreGive(semaphoreBLEOperation);
    } else {
      Log.error(F("BLE busy - command not sent" CR));
      gatewayState = GatewayState::ERROR;
    }
  } else if (strstr(topicOri, subjectTrackerSync) != NULL) {
    if (BTdata.containsKey("gatewayid") && BTdata.containsKey("trackerid") && BTdata["gatewayid"] != gateway_name) {
      BLEdevice* device = getDeviceByMac(BTdata["trackerid"].as<const char*>());
      if (device != &NO_BT_DEVICE_FOUND && device->lastUpdate != 0) {
        device->lastUpdate = 0;
        Log.notice(F("Tracker %s disassociated by gateway %s" CR), BTdata["trackerid"].as<const char*>(), BTdata["gatewayid"].as<const char*>());
      }
    }
  }
}
#endif
