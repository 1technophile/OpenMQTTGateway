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

#  ifdef ESP32
#    include "FreeRTOS.h"
FreeRTOS::Semaphore semaphoreCreateOrUpdateDevice = FreeRTOS::Semaphore("createOrUpdateDevice");
// Headers used for deep sleep functions
#    include <NimBLEAdvertisedDevice.h>
#    include <NimBLEDevice.h>
#    include <NimBLEScan.h>
#    include <NimBLEUtils.h>
#    include <driver/adc.h>
#    include <esp_bt.h>
#    include <esp_bt_main.h>
#    include <esp_wifi.h>
#    include <stdatomic.h>

#    include "soc/timer_group_reg.h"
#    include "soc/timer_group_struct.h"

void notifyCB(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify);
#  endif

#  if !defined(ESP32) && !defined(ESP8266)
#    include <ArduinoSTL.h>
#  endif

#  include <vector>
using namespace std;

struct BLEdevice {
  char macAdr[18];
  bool isDisc;
  bool isWhtL;
  bool isBlkL;
  ble_sensor_model sensorModel;
};

#  define device_flags_init     0 << 0
#  define device_flags_isDisc   1 << 0
#  define device_flags_isWhiteL 1 << 1
#  define device_flags_isBlackL 1 << 2

struct decompose {
  int start;
  int len;
  bool reverse;
};

vector<BLEdevice*> devices;
int newDevices = 0;

static BLEdevice NO_DEVICE_FOUND = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, false, false, false, UNKNOWN_MODEL};
static bool oneWhite = false;

int minRssi = abs(MinimumRSSI); //minimum rssi value

void pubBTMainCore(JsonObject& data, bool haPresenceEnabled = true) {
  if (abs((int)data["rssi"] | 0) < minRssi) {
    String mac_address = data["id"].as<const char*>();
    mac_address.replace(":", "");
    String mactopic = subjectBTtoMQTT + String("/") + mac_address;
    pub((char*)mactopic.c_str(), data);
  }
  if (haPresenceEnabled && data.containsKey("distance")) {
    if (data.containsKey("servicedatauuid"))
      data.remove("servicedatauuid");
    if (data.containsKey("servicedata"))
      data.remove("servicedata");
    String topic = String(Base_Topic) + "home_presence/" + String(gateway_name);
    pub_custom_topic((char*)topic.c_str(), data, false);
  }
}

class JsonBundle {
public:
  StaticJsonBuffer<JSON_MSG_BUFFER> buffer;
  JsonObject* object;
  bool haPresence;

  JsonObject& createObject(const char* json = NULL, bool haPresenceEnabled = true) {
    buffer.clear();
    haPresence = haPresenceEnabled;
    object = &(json == NULL ? buffer.createObject() : buffer.parseObject(json));
    return *object;
  }
};

void PublishDeviceData(JsonObject& BLEdata, bool processBLEData = true);

#  ifdef ESP32
static TaskHandle_t xCoreTaskHandle;

atomic_int forceBTScan;

JsonBundle jsonBTBufferQueue[BTQueueSize];
atomic_int jsonBTBufferQueueNext, jsonBTBufferQueueLast;
int btQueueBlocked = 0;
int btQueueLengthSum = 0;
int btQueueLengthCount = 0;

JsonObject& getBTJsonObject(const char* json = NULL, bool haPresenceEnabled = true) {
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
    pubBTMainCore(*bundle.object, bundle.haPresence);
    atomic_store_explicit(&jsonBTBufferQueueNext, (next + 1) % (2 * BTQueueSize), ::memory_order_seq_cst); // use namespace std -> ambiguous error...
    vTaskDelay(1);
  }
}

#  else

JsonBundle jsonBTBuffer;

JsonObject& getBTJsonObject(const char* json = NULL, bool haPresenceEnabled = true) {
  return jsonBTBuffer.createObject();
}

void pubBT(JsonObject& data) {
  pubBTMainCore(data);
}

#  endif

bool ProcessLock = false; // Process lock when we want to use a critical function like OTA for example

BLEdevice* getDeviceByMac(const char* mac);
void createOrUpdateDevice(const char* mac, uint8_t flags, ble_sensor_model model);

BLEdevice* getDeviceByMac(const char* mac) {
  Log.trace(F("getDeviceByMac %s" CR), mac);

  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    if ((strcmp((*it)->macAdr, mac) == 0)) {
      return *it;
    }
  }
  return &NO_DEVICE_FOUND;
}

bool updateWorB(JsonObject& BTdata, bool isWhite) {
  Log.trace(F("update WorB" CR));
  const char* jsonKey = isWhite ? "white-list" : "black-list";

  int size = BTdata[jsonKey].size();
  if (size == 0)
    return false;

  for (int i = 0; i < size; i++) {
    const char* mac = BTdata[jsonKey][i];
    createOrUpdateDevice(mac, (isWhite ? device_flags_isWhiteL : device_flags_isBlackL), UNKNOWN_MODEL);
  }

  return true;
}

void createOrUpdateDevice(const char* mac, uint8_t flags, ble_sensor_model model) {
#  ifdef ESP32
  if (!semaphoreCreateOrUpdateDevice.take(30000, "createOrUpdateDevice"))
    return;
#  endif

  BLEdevice* device = getDeviceByMac(mac);
  if (device == &NO_DEVICE_FOUND) {
    Log.trace(F("add %s" CR), mac);
    //new device
    device = new BLEdevice();
    strcpy(device->macAdr, mac);
    device->isDisc = flags & device_flags_isDisc;
    device->isWhtL = flags & device_flags_isWhiteL;
    device->isBlkL = flags & device_flags_isBlackL;
    if (model != UNKNOWN_MODEL) device->sensorModel = model;
    devices.push_back(device);
    newDevices++;
  } else {
    Log.trace(F("update %s" CR), mac);

    if (flags & device_flags_isDisc) {
      device->isDisc = true;
    }

    if (model != UNKNOWN_MODEL) device->sensorModel = model;

    if (flags & device_flags_isWhiteL || flags & device_flags_isBlackL) {
      device->isWhtL = flags & device_flags_isWhiteL;
      device->isBlkL = flags & device_flags_isBlackL;
    }
  }

  // update oneWhite flag
  oneWhite = oneWhite || device->isWhtL;

#  ifdef ESP32
  semaphoreCreateOrUpdateDevice.give();
#  endif
}

#  define isWhite(device)      device->isWhtL
#  define isBlack(device)      device->isBlkL
#  define isDiscovered(device) device->isDisc

void dumpDevices() {
  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    BLEdevice* p = *it;
    Log.trace(F("macAdr %s" CR), p->macAdr);
    Log.trace(F("isDisc %d" CR), p->isDisc);
    Log.trace(F("isWhtL %d" CR), p->isWhtL);
    Log.trace(F("isBlkL %d" CR), p->isBlkL);
    Log.trace(F("sensorModel %d" CR), p->sensorModel);
  }
}

void strupp(char* beg) {
  while (*beg = toupper(*beg))
    ++beg;
}

#  ifdef ZmqttDiscovery
void MiFloraDiscovery(char* mac, char* sensorModel) {
#    define MiFloraparametersCount 4
  Log.trace(F("MiFloraDiscovery" CR));
  char* MiFlorasensor[MiFloraparametersCount][8] = {
      {"sensor", "MiFlora-lux", mac, "illuminance", jsonLux, "", "", "lx"},
      {"sensor", "MiFlora-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "MiFlora-fer", mac, "", jsonFer, "", "", "µS/cm"},
      {"sensor", "MiFlora-moi", mac, "", jsonMoi, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiFlorasensor, MiFloraparametersCount, "Mi-Flora", "Xiaomi", sensorModel);
}

void VegTrugDiscovery(char* mac, char* sensorModel) {
#    define VegTrugparametersCount 4
  Log.trace(F("VegTrugDiscovery" CR));
  char* VegTrugsensor[VegTrugparametersCount][8] = {
      {"sensor", "VegTrug-lux", mac, "illuminance", jsonLux, "", "", "lx"},
      {"sensor", "VegTrug-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "VegTrug-fer", mac, "", jsonFer, "", "", "µS/cm"},
      {"sensor", "VegTrug-moi", mac, "", jsonMoi, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, VegTrugsensor, VegTrugparametersCount, "VegTrug", "VEGTRUG", sensorModel);
}

void MiJiaDiscovery(char* mac, char* sensorModel) {
#    define MiJiaparametersCount 3
  Log.trace(F("MiJiaDiscovery" CR));
  char* MiJiasensor[MiJiaparametersCount][8] = {
      {"sensor", "MiJia-batt", mac, "battery", jsonBatt, "", "", "%"},
      {"sensor", "MiJia-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "MiJia-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiJiasensor, MiJiaparametersCount, "MiJia", "", sensorModel);
}

void FormalDiscovery(char* mac, char* sensorModel) {
#    define FormalparametersCount 4
  Log.trace(F("FormalDiscovery" CR));
  char* Formalsensor[FormalparametersCount][8] = {
      {"sensor", "Formal-batt", mac, "battery", jsonBatt, "", "", "%"},
      {"sensor", "Formal-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "Formal-hum", mac, "humidity", jsonHum, "", "", "%"},
      {"sensor", "Formal-for", mac, "", jsonFor, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, Formalsensor, FormalparametersCount, "Formal", "", sensorModel);
}

void LYWSD02Discovery(char* mac, char* sensorModel) {
#    define LYWSD02parametersCount 3
  Log.trace(F("LYWSD02Discovery" CR));
  char* LYWSD02sensor[LYWSD02parametersCount][8] = {
      {"sensor", "LYWSD02-batt", mac, "battery", jsonBatt, "", "", "V"},
      {"sensor", "LYWSD02-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "LYWSD02-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, LYWSD02sensor, LYWSD02parametersCount, "LYWSD02", "Xiaomi", sensorModel);
}

void CLEARGRASSTRHDiscovery(char* mac, char* sensorModel) {
#    define CLEARGRASSTRHparametersCount 3
  Log.trace(F("CLEARGRASSTRHDiscovery" CR));
  char* CLEARGRASSTRHsensor[CLEARGRASSTRHparametersCount][8] = {
      {"sensor", "CLEARGRASSTRH-batt", mac, "battery", jsonBatt, "", "", "V"},
      {"sensor", "CLEARGRASSTRH-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "CLEARGRASSTRH-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, CLEARGRASSTRHsensor, CLEARGRASSTRHparametersCount, "CLEARGRASSTRH", "ClearGrass", sensorModel);
}

void CLEARGRASSCGD1Discovery(char* mac, char* sensorModel) {
#    define CLEARGRASSCGD1parametersCount 2
  Log.trace(F("CLEARGRASSCGD1Discovery" CR));
  char* CLEARGRASSCGD1sensor[CLEARGRASSCGD1parametersCount][8] = {
      {"sensor", "CLEARGRASSCGD1-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "CLEARGRASSCGD1-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, CLEARGRASSCGD1sensor, CLEARGRASSCGD1parametersCount, "CLEARGRASSCGD1", "ClearGrass", sensorModel);
}

void CLEARGRASSCGDK2Discovery(char* mac, char* sensorModel) {
#    define CLEARGRASSCGDK2parametersCount 2
  Log.trace(F("CLEARGRASSCGDK2Discovery" CR));
  char* CLEARGRASSCGDK2sensor[CLEARGRASSCGDK2parametersCount][8] = {
      {"sensor", "CLEARGRASSCGDK2-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "CLEARGRASSCGDK2-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, CLEARGRASSCGDK2sensor, CLEARGRASSCGDK2parametersCount, "CLEARGRASSCGDK2", "ClearGrass", sensorModel);
}

void CLEARGRASSCGPR1Discovery(char* mac, char* sensorModel) {
#    define CLEARGRASSCGPR1parametersCount 2
  Log.trace(F("CLEARGRASSCGPR1Discovery" CR));
  char* CLEARGRASSCGPR1sensor[CLEARGRASSCGPR1parametersCount][8] = {
      {"sensor", "CLEARGRASSCGPR1-pres", mac, "", jsonPres, "", "", ""},
      {"sensor", "CLEARGRASSCGPR1-lux", mac, "illuminance", jsonLux, "", "", "lx"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, CLEARGRASSCGPR1sensor, CLEARGRASSCGPR1parametersCount, "CLEARGRASSCGPR1", "ClearGrass", sensorModel);
}

void CLEARGRASSCGH1Discovery(char* mac, char* sensorModel) {
#    define CLEARGRASSCGH1parametersCount 1
  Log.trace(F("CLEARGRASSCGH1Discovery" CR));
  char* CLEARGRASSCGH1sensor[CLEARGRASSCGH1parametersCount][8] = {
      {"binary_sensor", "CLEARGRASSCGH1-open", mac, "door", jsonOpen, "True", "False", ""},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, CLEARGRASSCGH1sensor, CLEARGRASSCGH1parametersCount, "CLEARGRASSCGH1", "ClearGrass", sensorModel);
}

void CLEARGRASSTRHKPADiscovery(char* mac, char* sensorModel) {
#    define CLEARGRASSTRHKPAparametersCount 3
  Log.trace(F("CLEARGRASSTRHKPADiscovery" CR));
  char* CLEARGRASSTRHKPAsensor[CLEARGRASSTRHKPAparametersCount][8] = {
      {"sensor", "CLEARGRASSTRHKPA-pres", mac, "pressure", jsonPres, "", "", "kPa"},
      {"sensor", "CLEARGRASSTRHKPA-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "CLEARGRASSTRHKPA-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  Log.trace(F("CLEARGRASSTRHKPADiscovery %s" CR), sensorModel);
  createDiscoveryFromList(mac, CLEARGRASSTRHKPAsensor, CLEARGRASSTRHKPAparametersCount, "CLEARGRASSTRHKPA", "ClearGrass", sensorModel);
}

void MiScaleDiscovery(char* mac, char* sensorModel) {
#    define MiScaleparametersCount 1
  Log.trace(F("MiScaleDiscovery" CR));
  char* MiScalesensor[MiScaleparametersCount][8] = {
      {"sensor", "MiScale-weight", mac, "", jsonWeight, "", "", "kg"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiScalesensor, MiScaleparametersCount, "MiScale", "Xiaomi", sensorModel);
}

void MiLampDiscovery(char* mac, char* sensorModel) {
#    define MiLampparametersCount 1
  Log.trace(F("MiLampDiscovery" CR));
  char* MiLampsensor[MiLampparametersCount][8] = {
      {"sensor", "MiLamp-presence", mac, "", jsonPresence, "", "", "d"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiLampsensor, MiLampparametersCount, "MiLamp", "Xiaomi", sensorModel);
}

void MiBandDiscovery(char* mac, char* sensorModel) {
#    define MiBandparametersCount 1
  Log.trace(F("MiBandDiscovery" CR));
  char* MiBandsensor[MiBandparametersCount][8] = {
      {"sensor", "MiBand-steps", mac, "", jsonStep, "", "", "nb"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiBandsensor, MiBandparametersCount, "MiBand", "Xiaomi", sensorModel);
}

void InkBirdDiscovery(char* mac, char* sensorModel) {
#    define InkBirdparametersCount 3
  Log.trace(F("InkBirdDiscovery" CR));
  char* InkBirdsensor[InkBirdparametersCount][8] = {
      {"sensor", "InkBird-batt", mac, "battery", jsonBatt, "", "", "%"},
      {"sensor", "InkBird-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "InkBird-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, InkBirdsensor, InkBirdparametersCount, "", "InkBird", sensorModel);
}

void LYWSD03MMCDiscovery(char* mac, char* sensorModel) {
#    define LYWSD03MMCparametersCount 4
  Log.trace(F("LYWSD03MMCDiscovery" CR));
  char* LYWSD03MMCsensor[LYWSD03MMCparametersCount][8] = {
      {"sensor", "LYWSD03MMC-batt", mac, "battery", jsonBatt, "", "", "%"},
      {"sensor", "LYWSD03MMC-volt", mac, "", jsonVolt, "", "", "V"},
      {"sensor", "LYWSD03MMC-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "LYWSD03MMC-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, LYWSD03MMCsensor, LYWSD03MMCparametersCount, "LYWSD03MMC", "Xiaomi", sensorModel);
}

void MHO_C401Discovery(char* mac, char* sensorModel) {
#    define MHO_C401parametersCount 4
  Log.trace(F("MHO_C401Discovery" CR));
  char* MHO_C401sensor[MHO_C401parametersCount][8] = {
      {"sensor", "MHO_C401-batt", mac, "battery", jsonBatt, "", "", "%"},
      {"sensor", "MHO_C401-volt", mac, "", jsonVolt, "", "", "V"},
      {"sensor", "MHO_C401-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "MHO_C401-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MHO_C401sensor, MHO_C401parametersCount, "MHO_C401", "Xiaomi", sensorModel);
}

void INodeEMDiscovery(char* mac, char* sensorModel) {
#    define INodeEMparametersCount 3
  Log.trace(F("INodeEMDiscovery" CR));
  char* INodeEMsensor[INodeEMparametersCount][8] = {
      {"sensor", "iNodeEM-power", mac, "power", jsonPower, "", "", "W"},
      {"sensor", "iNodeEM-energy", mac, "", jsonEnergy, "", "", "kWh"},
      {"sensor", "iNodeEM-batt", mac, "battery", jsonBatt, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, INodeEMsensor, INodeEMparametersCount, "INode-Energy-Meter", "INode", sensorModel);
}

void WS02Discovery(char* mac, char* sensorModel) {
#    define WS02parametersCount 3
  Log.trace(F("WS02Discovery" CR));
  char* WS02sensor[WS02parametersCount][8] = {
      {"sensor", "WS02-volt", mac, "", jsonVolt, "", "", "V"},
      {"sensor", "WS02-temp", mac, "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "WS02-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, WS02sensor, WS02parametersCount, "WS02", "SensorBlue", sensorModel);
}

#  else
void MiFloraDiscovery(char* mac, char* sensorModel) {}
void VegTrugDiscovery(char* mac, char* sensorModel) {}
void MiJiaDiscovery(char* mac, char* sensorModel) {}
void FormalDiscovery(char* mac, char* sensorModel) {}
void LYWSD02Discovery(char* mac, char* sensorModel) {}
void CLEARGRASSTRHDiscovery(char* mac, char* sensorModel) {}
void CLEARGRASSCGD1Discovery(char* mac, char* sensorModel) {}
void CLEARGRASSCGDK2Discovery(char* mac, char* sensorModel) {}
void CLEARGRASSCGPR1Discovery(char* mac, char* sensorModel) {}
void CLEARGRASSCGH1Discovery(char* mac, char* sensorModel) {}
void CLEARGRASSTRHKPADiscovery(char* mac, char* sensorModel) {}
void MiScaleDiscovery(char* mac, char* sensorModel) {}
void MiLampDiscovery(char* mac, char* sensorModel) {}
void MiBandDiscovery(char* mac, char* sensorModel) {}
void InkBirdDiscovery(char* mac, char* sensorModel) {}
void LYWSD03MMCDiscovery(char* mac, char* sensorModel) {}
void MHO_C401Discovery(char* mac, char* sensorModel) {}
void INodeEMDiscovery(char* mac, char* sensorModel) {}
void WS02Discovery(char* mac, char* sensorModel) {}
#  endif

#  ifdef ESP32
/*
       Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
       Ported to Arduino ESP32 by Evandro Copercini
    */
// core task implementation thanks to https://techtutorialsx.com/2017/05/09/esp32-running-code-on-a-specific-core/

//core on which the BLE detection task will run
static int taskCore = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  std::string convertServiceData(std::string deviceServiceData) {
    int serviceDataLength = (int)deviceServiceData.length();
    char spr[2 * serviceDataLength + 1];
    for (int i = 0; i < serviceDataLength; i++) sprintf(spr + 2 * i, "%.2x", (unsigned char)deviceServiceData[i]);
    spr[2 * serviceDataLength] = 0;
    Log.trace("Converted service data (%d) to %s" CR, serviceDataLength, spr);
    return spr;
  }

  void onResult(BLEAdvertisedDevice* advertisedDevice) {
    Log.trace(F("Creating BLE buffer" CR));
    JsonObject& BLEdata = getBTJsonObject();
    String mac_adress = advertisedDevice->getAddress().toString().c_str();
    mac_adress.toUpperCase();
    BLEdata.set("id", (char*)mac_adress.c_str());
    Log.notice(F("Device detected: %s" CR), (char*)mac_adress.c_str());
    BLEdevice* device = getDeviceByMac(BLEdata["id"].as<const char*>());

    if ((!oneWhite || isWhite(device)) && !isBlack(device)) { //if not black listed mac we go AND if we have no white mac or this mac is  white we go out
      if (advertisedDevice->haveName())
        BLEdata.set("name", (char*)advertisedDevice->getName().c_str());
      if (advertisedDevice->haveManufacturerData()) {
        char* manufacturerdata = BLEUtils::buildHexData(NULL, (uint8_t*)advertisedDevice->getManufacturerData().data(), advertisedDevice->getManufacturerData().length());
        Log.trace(F("Manufacturer Data: %s" CR), manufacturerdata);
        BLEdata.set("manufacturerdata", manufacturerdata);
        free(manufacturerdata);
      }
      if (advertisedDevice->haveRSSI())
        BLEdata.set("rssi", (int)advertisedDevice->getRSSI());
      if (advertisedDevice->haveTXPower())
        BLEdata.set("txpower", (int8_t)advertisedDevice->getTXPower());
      if (advertisedDevice->haveRSSI() && !publishOnlySensors && hassPresence) {
        hass_presence(BLEdata); // this device has an rssi and we don't want only sensors so in consequence we can use it for home assistant room presence component
      }
      if (advertisedDevice->haveServiceData()) {
        int serviceDataCount = advertisedDevice->getServiceDataCount();
        Log.trace(F("Get services data number: %d" CR), serviceDataCount);
        for (int j = 0; j < serviceDataCount; j++) {
          std::string service_data = convertServiceData(advertisedDevice->getServiceData(j));
          Log.trace(F("Service data: %s" CR), service_data.c_str());
          BLEdata.set("servicedata", (char*)service_data.c_str());
          std::string serviceDatauuid = advertisedDevice->getServiceDataUUID(j).toString();
          Log.trace(F("Service data UUID: %s" CR), (char*)serviceDatauuid.c_str());
          BLEdata.set("servicedatauuid", (char*)serviceDatauuid.c_str());
          process_bledata(BLEdata); // this will force to resolve all the service data
        }

        if (serviceDataCount > 1) {
          BLEdata.remove("servicedata");
          BLEdata.remove("servicedatauuid");

          int msglen = BLEdata.measureLength() + 1;
          char jsonmsg[msglen];
          char jsonmsgb[msglen];
          BLEdata.printTo(jsonmsgb, sizeof(jsonmsgb));
          for (int j = 0; j < serviceDataCount; j++) {
            strcpy(jsonmsg, jsonmsgb); // the parse _destroys_ the message buffer
            JsonObject& BLEdataLocal = getBTJsonObject(jsonmsg, j == 0); // note, that first time we will get here the BLEdata itself; haPresence for the first msg
            if (!BLEdataLocal.containsKey("id")) { // would crash without id
              Log.trace("Json parsing error for %s" CR, jsonmsgb);
              break;
            }
            std::string service_data = convertServiceData(advertisedDevice->getServiceData(j));
            std::string serviceDatauuid = advertisedDevice->getServiceDataUUID(j).toString();

            int last = atomic_load_explicit(&jsonBTBufferQueueLast, ::memory_order_seq_cst) % BTQueueSize;
            int size1 = jsonBTBufferQueue[last].buffer.size();
            BLEdataLocal.set("servicedata", (char*)service_data.c_str());
            int size2 = jsonBTBufferQueue[last].buffer.size();
            BLEdataLocal.set("servicedatauuid", (char*)serviceDatauuid.c_str());
            int size3 = jsonBTBufferQueue[last].buffer.size();
            Log.trace("Buffersize for %d : %d -> %d -> %d" CR, j, size1, size2, size3);
            PublishDeviceData(BLEdataLocal);
          }
        } else {
          PublishDeviceData(BLEdata, false); // easy case
        }
      } else {
        PublishDeviceData(BLEdata); // PublishDeviceData has its own logic whether it needs to publish the json or not
      }
    } else {
      Log.trace(F("Filtered mac device" CR));
    }
  }
};

/** 
 * BLEscan used to retrieve BLE advertized data from devices without connection
 */
void BLEscan() {
  disableCore0WDT();
  Log.notice(F("Scan begin" CR));
  BLEDevice::setScanDuplicateCacheSize(BLEScanDuplicateCacheSize);
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  MyAdvertisedDeviceCallbacks myCallbacks;
  pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
  pBLEScan->setActiveScan(ActiveBLEScan);
  pBLEScan->setInterval(BLEScanInterval);
  pBLEScan->setWindow(BLEScanWindow);
  BLEScanResults foundDevices = pBLEScan->start(Scan_duration / 1000, false);
  scanCount++;
  Log.notice(F("Found %d devices, scan number %d end deinit controller" CR), foundDevices.getCount(), scanCount);
  BLEDevice::deinit(true);
  enableCore0WDT();
}

/** 
 * Callback method to retrieve data from devices characteristics
 */
void notifyCB(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {
  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pBLERemoteCharacteristic->getUUID().toString().c_str());

    if (length == 5) {
      Log.trace(F("Device identified creating BLE buffer" CR));
      JsonObject& BLEdata = getBTJsonObject();
      String mac_adress = pBLERemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str();
      mac_adress.toUpperCase();
      for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
        BLEdevice* p = *it;
        if ((strcmp(p->macAdr, (char*)mac_adress.c_str()) == 0)) {
          if (p->sensorModel == LYWSD03MMC)
            BLEdata.set("model", "LYWSD03MMC");
          else if (p->sensorModel == MHO_C401)
            BLEdata.set("model", "MHO_C401");
        }
      }
      BLEdata.set("id", (char*)mac_adress.c_str());
      Log.trace(F("Device identified in CB: %s" CR), (char*)mac_adress.c_str());
      BLEdata.set("tempc", (float)((pData[0] | (pData[1] << 8)) * 0.01));
      BLEdata.set("tempf", (float)(convertTemp_CtoF((pData[0] | (pData[1] << 8)) * 0.01)));
      BLEdata.set("hum", (float)(pData[2]));
      BLEdata.set("volt", (float)(((pData[4] * 256) + pData[3]) / 1000.0));
      BLEdata.set("batt", (float)(((((pData[4] * 256) + pData[3]) / 1000.0) - 2.1) * 100));

      pubBT(BLEdata);
    } else {
      Log.notice(F("Device not identified" CR));
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }
  pBLERemoteCharacteristic->unsubscribe();
}

/** 
 * Connect to BLE devices and initiate the callbacks with a service/characteristic request
 */
void BLEconnect() {
  Log.notice(F("BLE Connect begin" CR));
  BLEDevice::init("");
  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    BLEdevice* p = *it;
    if (p->sensorModel == LYWSD03MMC || p->sensorModel == MHO_C401) {
      Log.trace(F("Model to connect found" CR));
      NimBLEClient* pClient;
      pClient = BLEDevice::createClient();
      BLEUUID serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
      BLEUUID charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");
      BLEAddress sensorAddress(p->macAdr);
      if (!pClient->connect(sensorAddress)) {
        Log.notice(F("Failed to find client: %s" CR), p->macAdr);
        NimBLEDevice::deleteClient(pClient);
      } else {
        BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
        if (!pRemoteService) {
          Log.notice(F("Failed to find service UUID: %s" CR), serviceUUID.toString().c_str());
          pClient->disconnect();
        } else {
          Log.trace(F("Found service: %s" CR), serviceUUID.toString().c_str());
          // Obtain a reference to the characteristic in the service of the remote BLE server.
          if (pClient->isConnected()) {
            Log.trace(F("Client isConnected, freeHeap: %d" CR), ESP.getFreeHeap());
            BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
            if (!pRemoteCharacteristic) {
              Log.notice(F("Failed to find characteristic UUID: %s" CR), charUUID.toString().c_str());
              pClient->disconnect();
            } else {
              if (pRemoteCharacteristic->canNotify()) {
                Log.trace(F("Registering notification" CR));
                pRemoteCharacteristic->subscribe(true, notifyCB);
                delay(BLE_CNCT_TIMEOUT);
                pClient->disconnect();
              } else {
                Log.notice(F("Failed registering notification" CR));
                pClient->disconnect();
              }
            }
          }
        }
      }
    }
  }
  Log.notice(F("BLE Connect end" CR));
}

void stopProcessing() {
  Log.notice(F("Stop BLE processing" CR));
  ProcessLock = true;
  delay(Scan_duration < 2000 ? Scan_duration : 2000);
}

void startProcessing() {
  Log.notice(F("Start BLE processing" CR));
  ProcessLock = false;
  vTaskResume(xCoreTaskHandle);
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
        BLEscan();
        // Launching a connect every BLEscanBeforeConnect
        if ((!(scanCount % BLEscanBeforeConnect) || scanCount == 1) && bleConnect)
          BLEconnect();
        dumpDevices();
      }
      if (lowpowermode) {
        lowPowerESP32();
        int scan = atomic_exchange_explicit(&forceBTScan, 0, ::memory_order_seq_cst); // is this enough, it will wait the full deepsleep...
        if (scan == 1) BTforceScan();
      } else {
        for (int interval = BLEinterval, waitms; interval > 0; interval -= waitms) {
          int scan = atomic_exchange_explicit(&forceBTScan, 0, ::memory_order_seq_cst);
          if (scan == 1) BTforceScan(); // should we break after this?
          delay(waitms = interval > 100 ? 100 : interval); // 100ms
        }
      }
    } else {
      Log.trace(F("BLE core task canceled by processLock" CR));
      vTaskSuspend(xCoreTaskHandle);
    }
  }
}

void lowPowerESP32() // low power mode
{
  Log.trace(F("Going to deep sleep for: %l s" CR), (BLEinterval / 1000));
  deepSleep(BLEinterval * 1000);
}

void deepSleep(uint64_t time_in_us) {
#    if defined(ZboardM5STACK) || defined(ZboardM5STICKC) || defined(ZboardM5STICKCP)
  sleepScreen();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)SLEEP_BUTTON, LOW);
#    endif

  Log.trace(F("Deactivating ESP32 components" CR));
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  adc_power_off();
  esp_wifi_stop();
  esp_deep_sleep(time_in_us);
}

void changelowpowermode(int newLowPowerMode) {
  Log.notice(F("Changing LOW POWER mode to: %d" CR), newLowPowerMode);
#    if defined(ZboardM5STACK) || defined(ZboardM5STICKC) || defined(ZboardM5STICKCP)
  if (lowpowermode == 2) {
#      ifdef ZboardM5STACK
    M5.Lcd.wakeup();
#      endif
#      if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP)
    M5.Axp.SetLDO2(true);
    M5.Lcd.begin();
#      endif
  }
  char lpm[2];
  sprintf(lpm, "%d", newLowPowerMode);
  M5Display("Changing LOW POWER mode to:", lpm, "");
#    endif
  lowpowermode = newLowPowerMode;
  preferences.begin(Gateway_Short_Name, false);
  preferences.putUInt("lowpowermode", lowpowermode);
  preferences.end();
}

void setupBT() {
  Log.notice(F("BLE scans interval: %d" CR), BLEinterval);
  Log.notice(F("BLE scans number before connect: %d" CR), BLEscanBeforeConnect);
  Log.notice(F("Publishing only BLE sensors: %T" CR), publishOnlySensors);
  Log.notice(F("minrssi: %d" CR), minRssi);
  Log.notice(F("Low Power Mode: %d" CR), lowpowermode);

  atomic_init(&forceBTScan, 0); // in theory, we don't need this
  atomic_init(&jsonBTBufferQueueNext, 0); // in theory, we don't need this
  atomic_init(&jsonBTBufferQueueLast, 0); // in theory, we don't need this

  // we setup a task with priority one to avoid conflict with other gateways
  xTaskCreatePinnedToCore(
      coreTask, /* Function to implement the task */
      "coreTask", /* Name of the task */
      10000, /* Stack size in words */
      NULL, /* Task input parameter */
      1, /* Priority of the task */
      &xCoreTaskHandle, /* Task handle. */
      taskCore); /* Core where the task should run */
  Log.trace(F("ZgatewayBT multicore ESP32 setup done " CR));
}

bool BTtoMQTT() { // for on demand BLE scans
  BLEscan();
}
#  else // arduino or ESP8266 working with HM10/11

#    include <SoftwareSerial.h>
#    define QUESTION_MSG "AT+DISA?"

SoftwareSerial softserial(BT_RX, BT_TX);

String returnedString((char*)0);
unsigned long timebt = 0;

// this struct define which parts of the hexadecimal chain we extract and what to do with these parts
// {"mac"}, {"typ"}, {"rsi"}, {"rdl"}, {"sty"}, {"rda"}
struct decompose d[6] = {{0, 12, true}, {12, 2, false}, {14, 2, false}, {16, 2, false}, {28, 4, true}, {32, 60, false}};

void setupBT() {
  Log.notice(F("BLE interval: %d" CR), BLEinterval);
  Log.notice(F("BLE scans number before connect: %d" CR), BLEscanBeforeConnect);
  Log.notice(F("Publishing only BLE sensors: %T" CR), publishOnlySensors);
  Log.notice(F("minrssi: %d" CR), minRssi);
  softserial.begin(HMSerialSpeed);
  softserial.print(F("AT+ROLE1" CR));
  delay(100);
  softserial.print(F("AT+IMME1" CR));
  delay(100);
  softserial.print(F("AT+RESET" CR));
  delay(100);
#    ifdef HM_BLUE_LED_STOP
  softserial.print(F("AT+PIO11" CR)); // When not connected (as in BLE mode) the LED is off. When connected the LED is solid on.
#    endif
  delay(100);
#    if defined(ESP8266)
  returnedString.reserve(512); //reserve memory space for BT Serial. (size should depend on available RAM)
#    endif
  Log.trace(F("ZgatewayBT HM1X setup done " CR));
}

bool BTtoMQTT() {
  //extract serial data from module in hexa format
  while (softserial.available() > 0) {
    int a = softserial.read();
    if (a < 16) {
      returnedString += "0";
    }
    returnedString += String(a, HEX);
  }

  if (millis() > (timebt + BLEinterval)) { //retrieving data
    timebt = millis();
    returnedString.remove(0); //init data string
    softserial.print(F(QUESTION_MSG)); //start new discovery
    return false;
  }

#    if defined(ESP8266)
  yield();
#    endif
  if (returnedString.length() > (BLEdelimiterLength + CRLR_Length)) { //packet has to be at least the (BLEdelimiter + 'CR LF') length
    Log.verbose(F("returnedString: %s" CR), (char*)returnedString.c_str());
    if (returnedString.equals(F(BLEEndOfDiscovery))) //OK+DISCE
    {
      returnedString.remove(0); //clear data string
      scanCount++;
      Log.notice(F("Scan number %d end " CR), scanCount);
      return false;
    }
    size_t pos = 0, eolPos = 0;
    while ((pos = returnedString.indexOf(F(BLEdelimiter))) != -1 && (eolPos = returnedString.indexOf(F(CRLR))) != -1) {
#    if defined(ESP8266)
      yield();
#    endif
      String token = returnedString.substring(pos + BLEdelimiterLength, eolPos); //capture a BT device frame
      returnedString.remove(0, eolPos + CRLR_Length); //remove frame from main buffer (including 'CR LF' chars)
      Log.trace(F("Token: %s" CR), token.c_str());
      if (token.length() > 32) { // we extract data only if we have size is at least the size of (MAC, TYPE, RSSI, and Rest Data Length)
        String mac = F("");
        mac.reserve(17);
        for (int i = d[0].start; i < (d[0].start + d[0].len); i += 2) {
          mac += token.substring((d[0].start + d[0].len) - i - 2, (d[0].start + d[0].len) - i);
          if (i < (d[0].start + d[0].len) - 2)
            mac += ":";
        }
        mac.toUpperCase();

        String rssiStr = token.substring(d[2].start, (d[2].start + d[2].len));
        int rssi = (int)strtol(rssiStr.c_str(), NULL, 16) - 256;

        String restDataLengthStr = token.substring(d[3].start, (d[3].start + d[3].len));
        int restDataLength = (int)strtol(restDataLengthStr.c_str(), NULL, 16) * 2;

        String restData = F("");
        if (restDataLength <= 60)
          restData = token.substring(d[5].start, (d[5].start + restDataLength));

        Log.trace(F("Creating BLE buffer" CR));
        JsonObject& BLEdata = getBTJsonObject();

        Log.trace(F("Id %s" CR), (char*)mac.c_str());
        BLEdata.set("id", (char*)mac.c_str());
        BLEdevice* device = getDeviceByMac((char*)mac.c_str());

        if (isBlack(device))
          return false; //if black listed mac we go out
        if (oneWhite && !isWhite(device))
          return false; //if we have at least one white mac and this mac is not white we go out

        BLEdata.set("rssi", (int)rssi);
        if (!publishOnlySensors && hassPresence)
          hass_presence(BLEdata); // this device has an rssi and we don't want only sensors so in consequence we can use it for home assistant room presence component
        Log.trace(F("Service data: %s" CR), restData.c_str());
        BLEdata.set("servicedata", restData.c_str());
        PublishDeviceData(BLEdata);
      }
    }
    return false;
  }
}
#  endif

void RemoveJsonPropertyIf(JsonObject& obj, char* key, bool condition) {
  if (condition) {
    Log.trace(F("Removing %s" CR), key);
    obj.remove(key);
  }
}

boolean valid_service_data(const char* data, int size) {
  for (int i = 0; i < size; ++i) {
    if (data[i] != 48) // 48 correspond to 0 in ASCII table
      return true;
  }
  return false;
}

// This function always should be called from the main core as it generates direct mqtt messages
void launchBTDiscovery() {
  if (newDevices == 0)
    return;
#  ifdef ESP32
  if (!semaphoreCreateOrUpdateDevice.take(1000, "launchBTDiscovery"))
    return;
  newDevices = 0;
  vector<BLEdevice*> localDevices = devices;
  semaphoreCreateOrUpdateDevice.give();
  for (vector<BLEdevice*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
#  else
  newDevices = 0;
  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
#  endif
    BLEdevice* p = *it;
    if (p->sensorModel != UNKNOWN_MODEL && !isDiscovered(p)) {
      String macWOdots = String(p->macAdr);
      macWOdots.replace(":", "");
      Log.trace(F("Launching discovery of %s" CR), p->macAdr);
      if (p->sensorModel == HHCCJCY01HHCC) MiFloraDiscovery((char*)macWOdots.c_str(), "HHCCJCY01HHCC");
      if (p->sensorModel == VEGTRUG) VegTrugDiscovery((char*)macWOdots.c_str(), "VEGTRUG");
      if (p->sensorModel == LYWSDCGQ) MiJiaDiscovery((char*)macWOdots.c_str(), "LYWSDCGQ");
      if (p->sensorModel == JQJCY01YM) FormalDiscovery((char*)macWOdots.c_str(), "JQJCY01YM");
      if (p->sensorModel == LYWSD02) LYWSD02Discovery((char*)macWOdots.c_str(), "LYWSD02");
      if (p->sensorModel == CGG1) CLEARGRASSTRHDiscovery((char*)macWOdots.c_str(), "CGG1");
      if (p->sensorModel == CGP1W) CLEARGRASSTRHKPADiscovery((char*)macWOdots.c_str(), "CGP1W");
      if (p->sensorModel == MUE4094RT) MiLampDiscovery((char*)macWOdots.c_str(), "MUE4094RT");
      if (p->sensorModel == CGDK2) CLEARGRASSCGDK2Discovery((char*)macWOdots.c_str(), "CGDK2");
      if (p->sensorModel == CGPR1) CLEARGRASSCGPR1Discovery((char*)macWOdots.c_str(), "CGPR1");
      if (p->sensorModel == CGH1) CLEARGRASSCGH1Discovery((char*)macWOdots.c_str(), "CGH1");
      if (p->sensorModel == CGD1) CLEARGRASSCGD1Discovery((char*)macWOdots.c_str(), "CGD1");
      if (p->sensorModel == WS02) WS02Discovery((char*)macWOdots.c_str(), "WS02");
      if (p->sensorModel == MIBAND) MiBandDiscovery((char*)macWOdots.c_str(), "MIBAND");
      if ((p->sensorModel == XMTZC04HM) ||
          (p->sensorModel == XMTZC05HM)) MiScaleDiscovery((char*)macWOdots.c_str(), "XMTZC0xHM");
      if (p->sensorModel == INKBIRD) InkBirdDiscovery((char*)macWOdots.c_str(), "INKBIRD");
      if (p->sensorModel == LYWSD03MMC || p->sensorModel == LYWSD03MMC_ATC || p->sensorModel == LYWSD03MMC_PVVX) LYWSD03MMCDiscovery((char*)macWOdots.c_str(), "LYWSD03MMC");
      if (p->sensorModel == MHO_C401) MHO_C401Discovery((char*)macWOdots.c_str(), "MHO_C401");
      if (p->sensorModel == INODE_EM) INodeEMDiscovery((char*)macWOdots.c_str(), "INODE_EM");
      p->isDisc = true; // we don't need the semaphore and all the search magic via createOrUpdateDevice
    } else {
      if (!isDiscovered(p)) {
        Log.trace(F("Device UNKNOWN_MODEL %s" CR), p->macAdr);
        p->isDisc = true;
      } else {
        Log.trace(F("Device already discovered %s" CR), p->macAdr);
      }
    }
  }
}

void PublishDeviceData(JsonObject& BLEdata, bool processBLEData) {
  if (abs((int)BLEdata["rssi"] | 0) < minRssi) { // process only the devices close enough
    if (processBLEData) process_bledata(BLEdata);
    if (!publishOnlySensors || BLEdata.containsKey("model") || BLEdata.containsKey("distance")) {
#  if !pubBLEServiceUUID
      RemoveJsonPropertyIf(BLEdata, "servicedatauuid", BLEdata.containsKey("servicedatauuid"));
#  endif
#  if !pubKnownBLEServiceData
      RemoveJsonPropertyIf(BLEdata, "servicedata", BLEdata.containsKey("model") && BLEdata.containsKey("servicedata"));
#  endif
      pubBT(BLEdata);
    }
  } else if (BLEdata.containsKey("distance")) {
    pubBT(BLEdata);
  } else {
    Log.trace(F("Low rssi, device filtered" CR));
  }
}

JsonObject& process_bledata(JsonObject& BLEdata) {
  const char* mac = BLEdata["id"].as<const char*>();
  BLEdevice* device = getDeviceByMac(mac);
  if (BLEdata.containsKey("servicedata")) {
    Log.trace(F("Checking BLE service data validity" CR));
    const char* service_data = (const char*)(BLEdata["servicedata"] | "");
    int service_len = strlen(service_data);
    if (valid_service_data(service_data, service_len)) {
      Log.trace(F("Searching BLE device data %s size %d" CR), service_data, strlen(service_data));
      Log.trace(F("Is it a mi flora ?" CR));
      if (strstr(service_data, "209800") != NULL) {
        Log.trace(F("mi flora data reading" CR));
        BLEdata.set("model", "HHCCJCY01HHCC");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, HHCCJCY01HHCC);
        return process_sensors(2, BLEdata);
      }
      Log.trace(F("Is it a vegtrug ?" CR));
      if (service_len > ServicedataMinLength && strstr(service_data, "20bc03") != NULL) {
        Log.trace(F("vegtrug data reading" CR));
        BLEdata.set("model", "VEGTRUG");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, VEGTRUG);
        return process_sensors(2, BLEdata);
      }
      Log.trace(F("Is it a LYWSDCGQ?" CR));
      if (service_len > ServicedataMinLength && strstr(service_data, "20aa01") != NULL) {
        Log.trace(F("LYWSDCGQ data reading" CR));
        BLEdata.set("model", "LYWSDCGQ");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, LYWSDCGQ);
        return process_sensors(0, BLEdata);
      }
      Log.trace(F("Is it a JQJCY01YM?" CR));
      if (service_len > ServicedataMinLength && strstr(service_data, "20df02") != NULL) {
        Log.trace(F("JQJCY01YM data reading" CR));
        BLEdata.set("model", "JQJCY01YM");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, JQJCY01YM);
        return process_sensors(0, BLEdata);
      }
      Log.trace(F("Is it a LYWSD02?" CR));
      if (service_len > ServicedataMinLength && strstr(service_data, "205b04") != NULL) {
        Log.trace(F("LYWSD02 data reading" CR));
        BLEdata.set("model", "LYWSD02");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, LYWSD02);
        return process_sensors(2, BLEdata);
      }
      Log.trace(F("Is it a MUE4094RT?" CR));
      if (strstr(service_data, "4030dd") != NULL) {
        Log.trace(F("MUE4094RT data reading" CR));
        BLEdata.set("model", "MUE4094RT");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, MUE4094RT);
        return process_milamp(BLEdata);
      }
      Log.trace(F("Is it a CGP1W?" CR));
      if (service_len > ServicedataMinLength && strncmp(service_data, "0809", 4) == 0) {
        Log.trace(F("CGP1W data reading" CR));
        BLEdata.set("model", "CGP1W");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, CGP1W);
        return process_cleargrass(BLEdata, true);
      }
      Log.trace(F("Is it a CGG1" CR));
      // One type of the advertising packet format started with 50204703 or 50304703, where 4703 is a type of a sensor
      // Another type of the advertising packet started with 0807 or 8816
      if ((service_len > ServicedataMinLength && strncmp(&service_data[2], "4703", 4) == 0) || (strncmp(service_data, "0807", 4) == 0) || (strncmp(service_data, "8816", 4) == 0)) {
        Log.trace(F("CGG1 data reading" CR));
        BLEdata.set("model", "CGG1");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, CGG1);
        return strncmp(&service_data[2], "4703", 4) == 0 ? process_sensors(0, BLEdata) : process_cleargrass(BLEdata, false);
      }
      Log.trace(F("Is it a CGD1?" CR));
      if ((service_len > ServicedataMinLength && (strstr(service_data, "080caf") != NULL || strstr(service_data, "080c09") != NULL)) || (service_len > ServicedataMinLength - 6 && strstr(service_data, "080cd0") != NULL)) {
        Log.trace(F("CGD1 data reading" CR));
        BLEdata.set("model", "CGD1");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, CGD1);
        return process_cleargrass(BLEdata, false);
      }
      Log.trace(F("Is it a CGDK2?" CR));
      if (service_len > ServicedataMinLength && strncmp(&service_data[0], "8810", 4) == 0) {
        Log.trace(F("CGDK2 data reading" CR));
        BLEdata.set("model", "CGDK2");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, CGDK2);
        return process_cleargrass(BLEdata, false);
      }
      Log.trace(F("Is it a CGPR1?" CR));
      if (service_len > ServicedataMinLength && strncmp(&service_data[0], "4812", 4) == 0 || strncmp(&service_data[0], "0812", 4) == 0) {
        Log.trace(F("CGPR1 data reading" CR));
        BLEdata.set("model", "CGPR1");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, CGPR1);
        return process_cgpr1(BLEdata);
      }
      Log.trace(F("Is it a CGH1?" CR));
      if (service_len > ServicedataMinLength && (strncmp(&service_data[0], "c804", 4) == 0 || strncmp(&service_data[0], "8804", 4) == 0 || strncmp(&service_data[0], "0804", 4) == 0 || strncmp(&service_data[0], "4804", 4) == 0)) {
        Log.trace(F("CGH1 data reading" CR));
        BLEdata.set("model", "CGH1");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, CGH1);
        return process_cgh1(BLEdata);
      }
      Log.trace(F("Is it a MHO_C401?" CR));
      if (strstr(service_data, "588703") != NULL) {
        Log.trace(F("MHO_C401 add to list for future connect" CR));
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, MHO_C401);
      }
      Log.trace(F("Is it a LYWSD03MMC?" CR));
      if (strstr(service_data, "585b05") != NULL) {
        Log.trace(F("LYWSD03MMC add to list for future connect" CR));
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, LYWSD03MMC);
      }
      Log.trace(F("Is it a custom (pvvx) LYWSD03MMC?" CR));
      if (service_len >= 30 && strncmp(service_data + 6, "38c1a4", 6) == 0) {
        Log.trace(F("LYWSD03MMC PVVX" CR));
        BLEdata.set("model", "LYWSD03MMC_PVVX");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, LYWSD03MMC_PVVX);
        return process_pvvx(BLEdata);
      }
      Log.trace(F("Is it a custom (atc1441) LYWSD03MMC?" CR));
      if (strstr(service_data, "a4c138") != NULL) {
        Log.trace(F("LYWSD03MMC ATC" CR));
        BLEdata.set("model", "LYWSD03MMC_ATC");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, LYWSD03MMC_ATC);
        return process_atc(BLEdata);
      }
      if (BLEdata.containsKey("servicedatauuid")) {
        const char* service_datauuid = (const char*)(BLEdata["servicedatauuid"] | "");
        Log.trace(F("servicedatauuid %s" CR), service_datauuid);
        Log.trace(F("Is it a MiBand?" CR));
        if (strstr(service_datauuid, "fee0") != NULL) {
          Log.trace(F("Mi Band data reading" CR));
          BLEdata.set("model", "MIBAND");
          if (device->sensorModel == -1)
            createOrUpdateDevice(mac, device_flags_init, MIBAND);
          return process_miband(BLEdata);
        }
        Log.trace(F("Is it a XMTZC04HM?" CR));
        if (strstr(service_datauuid, "181d") != NULL) {
          Log.trace(F("XMTZC04HM data reading" CR));
          BLEdata.set("model", "XMTZC04HM");
          if (device->sensorModel == -1)
            createOrUpdateDevice(mac, device_flags_init, XMTZC04HM);
          return process_scale_v1(BLEdata);
        }
        Log.trace(F("Is it a XMTZC05HM?" CR));
        if (strstr(service_datauuid, "181b") != NULL) {
          Log.trace(F("XMTZC05HM data reading" CR));
          BLEdata.set("model", "XMTZC05HM");
          if (device->sensorModel == -1)
            createOrUpdateDevice(mac, device_flags_init, XMTZC05HM);
          return process_scale_v2(BLEdata);
        }
      }
    } else {
      Log.trace(F("Non valid service data, removing it" CR));
      BLEdata.remove("servicedata");
    }

#  if !pubUnknownBLEServiceData
    Log.trace(F("Unknown service data, removing it" CR));
    BLEdata.remove("servicedata");
#  endif
  }

  if (BLEdata.containsKey("manufacturerdata")) {
    const char* manufacturerdata = (const char*)(BLEdata["manufacturerdata"] | "");
    Log.trace(F("manufacturerdata %s" CR), manufacturerdata);
    if (BLEdata.containsKey("name")) {
      const char* name = (const char*)(BLEdata["name"] | "");
      Log.trace(F("name %s" CR), name);
      Log.trace(F("Is it a INKBIRD?" CR));
      if (strcmp(name, "sps") == 0) {
        Log.trace(F("INKBIRD data reading" CR));
        BLEdata.set("model", "INKBIRD");
        if (device->sensorModel == -1)
          createOrUpdateDevice(mac, device_flags_init, INKBIRD);
        return process_inkbird(BLEdata);
      }
    }
    Log.trace(F("Is it a iNode Energy Meter?" CR));
    if (strlen(manufacturerdata) == 26 && ((long)value_from_hex_data(manufacturerdata, 0, 4, true) & 0xFFF9) == 0x8290) {
      Log.trace(F("iNode Energy Meter data reading" CR));
      BLEdata.set("model", "INODE_EM");
      if (device->sensorModel == -1)
        createOrUpdateDevice(mac, device_flags_init, INODE_EM);
      return process_inode_em(BLEdata);
    }
    Log.trace(F("Is it a WS02?" CR));
    if (strlen(manufacturerdata) >= 40 && (strstr(manufacturerdata, "100000001a11") != NULL)) {
      Log.trace(F("WS02 data reading data reading" CR));
      BLEdata.set("model", "WS02");
      if (device->sensorModel == -1)
        createOrUpdateDevice(mac, device_flags_init, WS02);
      return process_ws02(BLEdata);
    }
#  if !pubBLEManufacturerData
    Log.trace(F("Remove manufacturer data" CR));
    BLEdata.remove("manufacturerdata");
#  endif
  }

  return BLEdata;
}

JsonObject& process_sensors(int offset, JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();
  int data_length = 0;

  switch (servicedata[27 + offset]) {
    case '1':
    case '2':
    case '3':
    case '4':
      data_length = ((servicedata[27 + offset] - '0') * 2);
      Log.trace(F("Valid data_length: %d" CR), data_length);
      break;
    default:
      Log.trace(F("Invalid data_length, not enriching the device data" CR));
      return BLEdata;
  }

  double value = 9999;
  value = (double)value_from_hex_data(servicedata, 28 + offset, data_length, true);

  // Mi flora provides tem(perature), (earth) moi(sture), fer(tility) and lux (illuminance)
  // Mi Jia provides tem(perature), batt(erry) and hum(idity)
  // following the value of digit 23 + offset we determine the type of data we get from the sensor
  switch (servicedata[23 + offset]) {
    case '0':
      BLEdata.set("for", (double)value / 100);
      break;
    case '4':
      BLEdata.set("tempc", (double)value / 10);
      BLEdata.set("tempf", (double)convertTemp_CtoF(value / 10));
      break;
    case '6':
      BLEdata.set("hum", (double)value / 10);
      break;
    case '7':
      BLEdata.set("lux", (double)value);
      break;
    case '8':
      BLEdata.set("moi", (double)value);
      break;
    case '9':
      BLEdata.set("fer", (double)value);
      break;
    case 'a':
      BLEdata.set("batt", (double)value);
      break;
    case 'd':
      // temperature
      value = (double)value_from_hex_data(servicedata, 28 + offset, 4, true);
      BLEdata.set("tempc", (double)value / 10);
      BLEdata.set("tempf", (double)convertTemp_CtoF(value / 10));
      // humidity
      value = (double)value_from_hex_data(servicedata, 32 + offset, 4, true);
      BLEdata.set("hum", (double)value / 10);
      break;
    default:
      Log.trace(F("can't read values" CR));
  }

  return BLEdata;
}

JsonObject& process_scale_v1(JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();

  if (servicedata[0] == '2') { // stabilized
    double weight = 0;
    if (servicedata[1] == '2') { //kg
      weight = (double)value_from_hex_data(servicedata, 2, 4, true) / 200;
      BLEdata.set("unit", "kg");
    } else if (servicedata[1] == '3') { //lbs
      weight = (double)value_from_hex_data(servicedata, 2, 4, true) / 100;
      BLEdata.set("unit", "lbs");
    } else { //unknown unit
      BLEdata.set("unit", "unknown");
    }
    //Set Json value
    BLEdata.set("weight", (double)weight);
  }

  return BLEdata;
}

JsonObject& process_scale_v2(JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();

  double weight = (double)value_from_hex_data(servicedata, 22, 4, true) / 200;
  double impedance = (double)value_from_hex_data(servicedata, 18, 4, true);

  //Set Json values
  BLEdata.set("weight", (double)weight);
  BLEdata.set("impedance", (double)impedance);

  return BLEdata;
}

JsonObject& process_inkbird(JsonObject& BLEdata) {
  const char* manufacturerdata = BLEdata["manufacturerdata"].as<const char*>();

  double temperature = (double)value_from_hex_data(manufacturerdata, 0, 4, true) / 100;
  double humidity = (double)value_from_hex_data(manufacturerdata, 4, 4, true) / 100;
  double battery = (double)value_from_hex_data(manufacturerdata, 14, 2, true);

  //Set Json values
  BLEdata.set("tempc", (double)temperature);
  BLEdata.set("tempf", (double)convertTemp_CtoF(temperature));
  BLEdata.set("hum", (double)humidity);
  BLEdata.set("batt", (double)battery);

  return BLEdata;
}

JsonObject& process_miband(JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();

  double steps = (double)value_from_hex_data(servicedata, 0, 4, true);

  //Set Json value
  BLEdata.set("steps", (double)steps);

  return BLEdata;
}

JsonObject& process_milamp(JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();

  long darkness = (double)value_from_hex_data(servicedata, 8, 2, true);

  //Set Json value
  BLEdata.set("presence", (bool)"true");
  BLEdata.set("darkness", (long)darkness);

  return BLEdata;
}

JsonObject& process_cleargrass(JsonObject& BLEdata, boolean air) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();

  double value = 9999;
  // temperature
  value = (double)value_from_hex_data(servicedata, 20, 4, true);
  BLEdata.set("tempc", (double)value / 10);
  BLEdata.set("tempf", (double)convertTemp_CtoF(value / 10));
  // humidity
  value = (double)value_from_hex_data(servicedata, 24, 4, true);
  BLEdata.set("hum", (double)value / 10);
  if (air) {
    // air pressure
    value = (double)value_from_hex_data(servicedata, 32, 4, true);
    BLEdata.set("pres", (double)value / 100);
  }

  return BLEdata;
}

JsonObject& process_cgpr1(JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();
  int value = -1;
  if (strncmp(&servicedata[0], "0812", 4) == 0) { // lux
    value = value_from_hex_data(servicedata, 33, 4, true);
    if (value >= 0)
      BLEdata.set("lux", value);
  } else if (strncmp(&servicedata[0], "4812", 4) == 0) { // presence
    value = value_from_hex_data(servicedata, 21, 1, false);
    if (value == 0)
      BLEdata.set("pres", false);
    if (value == 1)
      BLEdata.set("pres", true);
  }

  return BLEdata;
}

JsonObject& process_cgh1(JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();
  int value = -1;
  if (strncmp(&servicedata[0], "0804", 4) == 0 || strncmp(&servicedata[0], "8804", 4) == 0) { // state
    value = value_from_hex_data(servicedata, 33, 1, false);
  } else if (strncmp(&servicedata[0], "4804", 4) == 0 || strncmp(&servicedata[0], "c804", 4) == 0) { // action
    value = value_from_hex_data(servicedata, 21, 1, false);
  }

  if (value == 0)
    BLEdata.set("open", true);
  if (value == 1)
    BLEdata.set("open", false);

  return BLEdata;
}

JsonObject& process_atc(JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();

  double temperature = (double)value_from_hex_data(servicedata, 12, 4, false) / 10;
  double humidity = (double)value_from_hex_data(servicedata, 16, 2, false);
  double battery = (double)value_from_hex_data(servicedata, 18, 2, false);
  double voltage = (double)value_from_hex_data(servicedata, 20, 4, false) / 1000;

  //Set Json values
  BLEdata.set("tempc", (double)temperature);
  BLEdata.set("tempf", (double)convertTemp_CtoF(temperature));
  BLEdata.set("hum", (double)humidity);
  BLEdata.set("batt", (double)battery);
  BLEdata.set("volt", (double)voltage);

  return BLEdata;
}

JsonObject& process_pvvx(JsonObject& BLEdata) {
  const char* servicedata = BLEdata["servicedata"].as<const char*>();

  double temperature = (double)value_from_hex_data(servicedata, 12, 4, true) / 100;
  double humidity = (double)value_from_hex_data(servicedata, 16, 4, true) / 100;
  double battery = (double)value_from_hex_data(servicedata, 24, 2, false);
  double voltage = (double)value_from_hex_data(servicedata, 20, 4, true) / 1000;

  //Set Json values
  BLEdata.set("tempc", (double)temperature);
  BLEdata.set("tempf", (double)convertTemp_CtoF(temperature));
  BLEdata.set("hum", (double)humidity);
  BLEdata.set("batt", (double)battery);
  BLEdata.set("volt", (double)voltage);

  return BLEdata;
}

JsonObject& process_inode_em(JsonObject& BLEdata) {
  const char* manufacturerdata = BLEdata["manufacturerdata"].as<const char*>();

  long impPerKWh = value_from_hex_data(manufacturerdata, 16, 4, true) & 0x3FFF;
  double power = ((double)value_from_hex_data(manufacturerdata, 4, 4, true) / impPerKWh) * 60000;
  double energy = (double)value_from_hex_data(manufacturerdata, 8, 8, true) / impPerKWh;
  long battery = ((value_from_hex_data(manufacturerdata, 20, 2, true) >> 4) - 2) * 10;

  //Set Json values
  BLEdata.set("power", (double)power);
  BLEdata.set("energy", (double)energy);
  BLEdata.set("batt", battery);

  return BLEdata;
}

JsonObject& process_ws02(JsonObject& BLEdata) {
  const char* manufacturerdata = BLEdata["manufacturerdata "].as<const char*>();

  double temperature = (double)value_from_hex_data(manufacturerdata, 20, 4, true) / 16;
  double humidity = (double)value_from_hex_data(manufacturerdata, 24, 4, true) / 16;
  double voltage = (double)value_from_hex_data(manufacturerdata, 16, 4, true) / 1000;

  //Set Json values
  BLEdata.set("tempc", (double)temperature);
  BLEdata.set("tempf", (double)convertTemp_CtoF(temperature));
  BLEdata.set("hum", (double)humidity);
  BLEdata.set("volt", (double)voltage);

  return BLEdata;
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
#  ifdef ESP32
    if (bleConnect)
      BLEconnect();
#  endif
  } else {
    Log.trace(F("Cannot launch scan due to other process running" CR));
  }
}

void MQTTtoBT(char* topicOri, JsonObject& BTdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoBTset)) {
    Log.trace(F("MQTTtoBT json set" CR));

    // Black list & white list set
    bool WorBupdated;
    WorBupdated |= updateWorB(BTdata, true);
    WorBupdated |= updateWorB(BTdata, false);

    if (WorBupdated) {
#  ifdef ESP32
      if (!semaphoreCreateOrUpdateDevice.take(1000, "dumpDevices")) {
        dumpDevices();
        semaphoreCreateOrUpdateDevice.give();
      }
#  else
      dumpDevices();
#  endif
    }

    // Scan interval set
    if (BTdata.containsKey("interval")) {
      Log.trace(F("BLE interval setup" CR));
      unsigned int interval = BTdata["interval"];
      if (interval == 0) {
#  ifdef ESP32
        atomic_store_explicit(&forceBTScan, 1, ::memory_order_seq_cst); // ask the other core to do the scan for us
#  else
        BTforceScan();
#  endif
      } else {
        Log.trace(F("Previous interval: %d ms" CR), BLEinterval);
        BLEinterval = interval;
        Log.notice(F("New interval: %d ms" CR), BLEinterval);
      }
    }
    // Number of scan before a connect set
    if (BTdata.containsKey("scanbcnct")) {
      Log.trace(F("BLE scans number before a connect setup" CR));
      Log.trace(F("Previous number: %d" CR), BLEscanBeforeConnect);
      BLEscanBeforeConnect = (unsigned int)BTdata["scanbcnct"];
      Log.notice(F("New scan number before connect: %d" CR), BLEscanBeforeConnect);
    }
    // publish all BLE devices discovered or  only the identified sensors (like temperature sensors)
    if (BTdata.containsKey("onlysensors")) {
      Log.trace(F("Do we publish only sensors" CR));
      Log.trace(F("Previous value: %T" CR), publishOnlySensors);
      publishOnlySensors = (bool)BTdata["onlysensors"];
      Log.notice(F("New value onlysensors: %T" CR), publishOnlySensors);
    }
#  ifdef ESP32
    // Attempts to connect to elligible devices or not
    if (BTdata.containsKey("bleconnect")) {
      Log.trace(F("Do we initiate a connection to retrieve data" CR));
      Log.trace(F("Previous value: %T" CR), bleConnect);
      bleConnect = (bool)BTdata["bleconnect"];
      Log.notice(F("New value bleConnect: %T" CR), bleConnect);
    }
    if (BTdata.containsKey("lowpowermode")) {
      changelowpowermode((int)BTdata["lowpowermode"]);
    }
#  endif
    // MinRSSI set
    if (BTdata.containsKey("minrssi")) {
      // storing Min RSSI for further use if needed
      Log.trace(F("Previous minrssi: %d" CR), minRssi);
      // set Min RSSI if present if not setting default value
      minRssi = abs((int)BTdata["minrssi"]);
      Log.notice(F("New minrssi: %d" CR), minRssi);
    }
    // Home Assistant presence message
    if (BTdata.containsKey("hasspresence")) {
      // storing Min RSSI for further use if needed
      Log.trace(F("Previous hasspresence: %T" CR), hassPresence);
      // set Min RSSI if present if not setting default value
      hassPresence = (bool)BTdata["hasspresence"];
      Log.notice(F("New hasspresence: %T" CR), hassPresence);
    }
  }
}
#endif
