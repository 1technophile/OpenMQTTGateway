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
SemaphoreHandle_t semaphoreCreateOrUpdateDevice;
SemaphoreHandle_t semaphoreBLEOperation;
QueueHandle_t BLEQueue;
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

#    include "ZgatewayBLEConnect.h"
#    include "soc/timer_group_reg.h"
#    include "soc/timer_group_struct.h"

#  endif

#  include <decoder.h>

#  include <vector>

using namespace std;

#  define device_flags_init     0 << 0
#  define device_flags_isDisc   1 << 0
#  define device_flags_isWhiteL 1 << 1
#  define device_flags_isBlackL 1 << 2
#  define device_flags_connect  1 << 3

TheengsDecoder decoder;

struct decompose {
  int start;
  int len;
  bool reverse;
};

#  ifdef ESP32
vector<BLEAction> BLEactions;
#  endif

vector<BLEdevice*> devices;
int newDevices = 0;

static BLEdevice NO_DEVICE_FOUND = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                    false,
                                    false,
                                    false,
                                    false,
                                    TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL};
static bool oneWhite = false;

int minRssi = abs(MinimumRSSI); //minimum rssi value

void pubBTMainCore(JsonObject& data, bool haPresenceEnabled = true) {
  if (abs((int)data["rssi"] | 0) < minRssi && data.containsKey("id")) {
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

#  ifdef ESP32
static TaskHandle_t xCoreTaskHandle;
static TaskHandle_t xProcBLETaskHandle;

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

#  else

JsonBundle jsonBTBuffer;

JsonObject& getBTJsonObject(const char* json, bool haPresenceEnabled) {
  return jsonBTBuffer.createObject();
}

void pubBT(JsonObject& data) {
  pubBTMainCore(data);
}

#  endif

bool ProcessLock = false; // Process lock when we want to use a critical function like OTA for example

BLEdevice* getDeviceByMac(const char* mac);
void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type = 0);

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
    createOrUpdateDevice(mac, (isWhite ? device_flags_isWhiteL : device_flags_isBlackL),
                         TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL);
  }

  return true;
}

void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type) {
#  ifdef ESP32
  if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(30000)) == pdFALSE) {
    Log.error(F("Semaphore NOT taken" CR));
    return;
  }
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
    device->connect = flags & device_flags_connect;
    device->macType = mac_type;
    device->sensorModel_id = model;
    devices.push_back(device);
    newDevices++;
  } else {
    Log.trace(F("update %s" CR), mac);

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

#  ifdef ESP32
  xSemaphoreGive(semaphoreCreateOrUpdateDevice);
#  endif
}

#  define isWhite(device)      device->isWhtL
#  define isBlack(device)      device->isBlkL
#  define isDiscovered(device) device->isDisc

void dumpDevices() {
  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    BLEdevice* p = *it;
    Log.trace(F("macAdr %s" CR), p->macAdr);
    Log.trace(F("macType %d" CR), p->macType);
    Log.trace(F("isDisc %d" CR), p->isDisc);
    Log.trace(F("isWhtL %d" CR), p->isWhtL);
    Log.trace(F("isBlkL %d" CR), p->isBlkL);
    Log.trace(F("connect %d" CR), p->connect);
    Log.trace(F("sensorModel_id %d" CR), p->sensorModel_id);
  }
}

void strupp(char* beg) {
  while ((*beg = toupper(*beg)))
    ++beg;
}

#  ifdef ZmqttDiscovery
void DT24Discovery(const char* mac, const char* sensorModel_id) {
#    define DT24parametersCount 6
  Log.trace(F("DT24Discovery" CR));
  const char* DT24sensor[DT24parametersCount][9] = {
      {"sensor", "DT24-volt", mac, "power", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "DT24-amp", mac, "power", jsonCurrent, "", "", "A", stateClassMeasurement},
      {"sensor", "DT24-watt", mac, "power", jsonPower, "", "", "W", stateClassMeasurement},
      {"sensor", "DT24-watt-hour", mac, "power", jsonEnergy, "", "", "kWh", stateClassMeasurement},
      {"sensor", "DT24-price", mac, "", jsonMsg, "", "", "", stateClassNone},
      {"sensor", "DT24-temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, DT24sensor, DT24parametersCount, "DT24", "ATorch", sensorModel_id);
}

void LYWSD03MMCDiscovery(const char* mac, const char* sensorModel) {
#    define LYWSD03MMCparametersCount 4
  Log.trace(F("LYWSD03MMCDiscovery" CR));
  const char* LYWSD03MMCsensor[LYWSD03MMCparametersCount][9] = {
      {"sensor", "LYWSD03MMC-batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "LYWSD03MMC-volt", mac, "", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "LYWSD03MMC-temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "LYWSD03MMC-hum", mac, "humidity", jsonHum, "", "", "%", stateClassMeasurement}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, LYWSD03MMCsensor, LYWSD03MMCparametersCount, "LYWSD03MMC", "Xiaomi", sensorModel);
}

void MHO_C401Discovery(const char* mac, const char* sensorModel) {
#    define MHO_C401parametersCount 4
  Log.trace(F("MHO_C401Discovery" CR));
  const char* MHO_C401sensor[MHO_C401parametersCount][9] = {
      {"sensor", "MHO_C401-batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "MHO_C401-volt", mac, "", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "MHO_C401-temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "MHO_C401-hum", mac, "humidity", jsonHum, "", "", "%", stateClassMeasurement}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MHO_C401sensor, MHO_C401parametersCount, "MHO_C401", "Xiaomi", sensorModel);
}

#  else
void LYWSD03MMCDiscovery(const char* mac, const char* sensorModel) {}
void MHO_C401Discovery(const char* mac, const char* sensorModel) {}
void DT24Discovery(const char* mac, const char* sensorModel_id) {}
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
  void onResult(BLEAdvertisedDevice* advertisedDevice) {
    if (xQueueSend(BLEQueue, &advertisedDevice, 0) != pdTRUE) {
      Log.error(F("BLEQueue full" CR));
    }
  }
};

std::string convertServiceData(std::string deviceServiceData) {
  int serviceDataLength = (int)deviceServiceData.length();
  char spr[2 * serviceDataLength + 1];
  for (int i = 0; i < serviceDataLength; i++) sprintf(spr + 2 * i, "%.2x", (unsigned char)deviceServiceData[i]);
  spr[2 * serviceDataLength] = 0;
  Log.trace("Converted service data (%d) to %s" CR, serviceDataLength, spr);
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
      BLEdata["mac_type"] = advertisedDevice->getAddress().getType();
      Log.notice(F("Device detected: %s" CR), (char*)mac_adress.c_str());
      BLEdevice* device = getDeviceByMac(BLEdata["id"].as<const char*>());

#    if BLE_FILTER_CONNECTABLE
      if (device->connect) {
        Log.notice(F("Filtered connectable device" CR));
        continue;
      }
#    endif

      if ((!oneWhite || isWhite(device)) && !isBlack(device)) { //if not black listed mac we go AND if we have no white mac or this mac is  white we go out
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
        if (advertisedDevice->haveRSSI() && !publishOnlySensors && hassPresence) {
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
        Log.trace(F("Filtered mac device" CR));
      }
    }
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
  disableCore0WDT();
  Log.notice(F("Scan begin" CR));
  BLEScan* pBLEScan = BLEDevice::getScan();
  MyAdvertisedDeviceCallbacks myCallbacks;
  pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
  pBLEScan->setActiveScan(ActiveBLEScan);
  pBLEScan->setInterval(BLEScanInterval);
  pBLEScan->setWindow(BLEScanWindow);
  BLEScanResults foundDevices = pBLEScan->start(Scan_duration / 1000, false);
  scanCount++;
  Log.notice(F("Found %d devices, scan number %d end" CR), foundDevices.getCount(), scanCount);
  enableCore0WDT();
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
          } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {
            HHCCJCY01HHCC_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
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
                    p->sensorModel_id != BLEconectable::id::MHO_C401) {
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
        if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
          BLEscan();
          // Launching a connect every BLEscanBeforeConnect
          if ((!(scanCount % BLEscanBeforeConnect) || scanCount == 1) && bleConnect)
            BLEconnect();
          dumpDevices();
          xSemaphoreGive(semaphoreBLEOperation);
        } else {
          Log.error(F("Failed to start scan - BLE busy" CR));
        }
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
#    if defined(ZboardM5STACK) || defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  sleepScreen();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)SLEEP_BUTTON, LOW);
#    endif

  Log.trace(F("Deactivating ESP32 components" CR));
  BLEDevice::deinit(true);
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  // Ignore the deprecated warning, this call is necessary here.
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  adc_power_off();
#    pragma GCC diagnostic pop
  esp_wifi_stop();
  esp_deep_sleep(time_in_us);
}

void changelowpowermode(int newLowPowerMode) {
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
      10000, /* Stack size in bytes */
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
        BLEdata["id"] = (const char*)mac.c_str();
        BLEdevice* device = getDeviceByMac((char*)mac.c_str());

        if (isBlack(device))
          return false; //if black listed mac we go out
        if (oneWhite && !isWhite(device))
          return false; //if we have at least one white mac and this mac is not white we go out

        BLEdata["rssi"] = (int)rssi;
        if (!publishOnlySensors && hassPresence)
          hass_presence(BLEdata); // this device has an rssi and we don't want only sensors so in consequence we can use it for home assistant room presence component
        Log.trace(F("Service data: %s" CR), restData.c_str());
        BLEdata["servicedata"] = restData.c_str();
        PublishDeviceData(BLEdata);
      }
    }
    return false;
  }
}
#  endif

void RemoveJsonPropertyIf(JsonObject& obj, const char* key, bool condition) {
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

#  ifdef ZmqttDiscovery
// This function always should be called from the main core as it generates direct mqtt messages
void launchBTDiscovery() {
  if (newDevices == 0)
    return;
#    ifdef ESP32
  if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(1000)) == pdFALSE) {
    Log.error(F("Semaphore NOT taken" CR));
    return;
  }
  newDevices = 0;
  vector<BLEdevice*> localDevices = devices;
  xSemaphoreGive(semaphoreCreateOrUpdateDevice);
  for (vector<BLEdevice*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
#    else
  newDevices = 0;
  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
#    endif
    BLEdevice* p = *it;
    Log.trace(F("Device mac %s" CR), p->macAdr);
    if (p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL &&
        p->sensorModel_id < BLEconectable::id::MIN && !isDiscovered(p)) {
      String macWOdots = String(p->macAdr);
      macWOdots.replace(":", "");
      Log.trace(F("Looking for Model_id: %d" CR), p->sensorModel_id);
      std::string properties = decoder.getTheengProperties(p->sensorModel_id);
      Log.trace(F("properties: %s" CR), properties.c_str());
      std::string brand = decoder.getTheengAttribute(p->sensorModel_id, "brand");
      std::string model = decoder.getTheengAttribute(p->sensorModel_id, "model");
      std::string model_id = decoder.getTheengAttribute(p->sensorModel_id, "model_id");
      if (!properties.empty()) {
        StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
        deserializeJson(jsonBuffer, properties);
        for (JsonPair prop : jsonBuffer["properties"].as<JsonObject>()) {
          Log.trace("Key: %s", prop.key().c_str());
          Log.trace("Unit: %s", prop.value()["unit"].as<const char*>());
          Log.trace("Name: %s", prop.value()["name"].as<const char*>());
          String discovery_topic = String(subjectBTtoMQTT) + "/" + macWOdots;
          String entity_name = String(model_id.c_str()) + "-" + String(prop.key().c_str());
          String unique_id = macWOdots + "-" + entity_name;
#    if OpenHABDiscovery
          String value_template = "{{ value_json." + String(prop.key().c_str()) + "}}";
#    else
          String value_template = "{{ value_json." + String(prop.key().c_str()) + " | is_defined }}";
#    endif
          createDiscovery("sensor",
                          discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                          will_Topic, prop.value()["name"], value_template.c_str(),
                          "", "", prop.value()["unit"],
                          0, "", "", false, "",
                          model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                          stateClassMeasurement);
        }
      } else {
        // Discovery of sensors from which we retrieve data only by connect
        if (p->sensorModel_id == BLEconectable::id::DT24_BLE) {
          DT24Discovery(macWOdots.c_str(), "DT24-BLE");
        }
        if (p->sensorModel_id == BLEconectable::id::LYWSD03MMC) {
          LYWSD03MMCDiscovery(macWOdots.c_str(), "LYWSD03MMC");
        }
        if (p->sensorModel_id == BLEconectable::id::MHO_C401) {
          MHO_C401Discovery(macWOdots.c_str(), "MHO-C401");
        }
      }
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
#  endif

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
#  if !pubBLEManufacturerData
      RemoveJsonPropertyIf(BLEdata, "manufacturerdata", BLEdata.containsKey("model") && BLEdata.containsKey("manufacturerdata"));
#  endif
      pubBT(BLEdata);
    } else {
#  if !pubUnknownBLEServiceData
      Log.trace(F("Unknown service data, removing it" CR));
      RemoveJsonPropertyIf(BLEdata, "servicedata", BLEdata.containsKey("servicedata"));
#  endif
#  if !pubUnknownBLEManufacturerData
      RemoveJsonPropertyIf(BLEdata, "manufacturerdata", BLEdata.containsKey("model") && BLEdata.containsKey("manufacturerdata"));
#  endif
    }
  } else if (BLEdata.containsKey("distance")) {
    pubBT(BLEdata);
  } else {
    Log.trace(F("Low rssi, device filtered" CR));
  }
}

void process_bledata(JsonObject& BLEdata) {
  const char* mac = BLEdata["id"].as<const char*>();
  int model_id = decoder.decodeBLEJson(BLEdata);
  int mac_type = BLEdata["mac_type"].as<int>();
  if (model_id >= 0) { // Broadcaster devices
    Log.trace(F("Decoder found device: %s" CR), BLEdata["model_id"].as<const char*>());
    if (model_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {
      createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type); // Device that broadcast and can be connected
    } else {
      createOrUpdateDevice(mac, device_flags_init, model_id, mac_type);
    }
  } else if (BLEdata.containsKey("name")) { // Connectable devices
    std::string name = BLEdata["name"];
    if (name.compare("LYWSD03MMC") == 0)
      model_id = BLEconectable::id::LYWSD03MMC;
    else if (name.compare("DT24-BLE") == 0)
      model_id = BLEconectable::id::DT24_BLE;
    else if (name.compare("MHO-C401") == 0)
      model_id = BLEconectable::id::MHO_C401;

    if (model_id > 0) {
      Log.trace(F("Connectable device found: %s" CR), name.c_str());
      createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type);
    }
  } else {
    Log.trace(F("No device found " CR));
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
#  ifdef ESP32
    if (bleConnect)
      BLEconnect();
#  endif
  } else {
    Log.trace(F("Cannot launch scan due to other process running" CR));
  }
}

void MQTTtoBTAction(JsonObject& BTdata) {
#  ifdef ESP32
  BLEAction action;
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

  if (BTdata.containsKey("immediate") && BTdata["immediate"].as<bool>()) {
    // Immediate action; we need to prevent the normal connection action and stop scanning
    ProcessLock = true;
    NimBLEScan* pScan = NimBLEDevice::getScan();
    if (pScan->isScanning()) {
      pScan->stop();
    }

    if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(5000)) == pdTRUE) {
      // swap the vectors so only this device is processed
      std::vector<BLEdevice*> dev_swap;
      dev_swap.push_back(getDeviceByMac(action.addr));
      std::swap(devices, dev_swap);

      std::vector<BLEAction> act_swap;
      act_swap.push_back(action);
      std::swap(BLEactions, act_swap);

      // Unlock here to allow the action to be performed
      ProcessLock = false;
      BLEconnect();
      // back to normal
      std::swap(devices, dev_swap);
      std::swap(BLEactions, act_swap);
      // If we stopped the scheduled connect for this action, do the scheduled now
      if ((!(scanCount % BLEscanBeforeConnect) || scanCount == 1) && bleConnect)
        BLEconnect();
      xSemaphoreGive(semaphoreBLEOperation);
    } else {
      Log.error(F("BLE busy - command not sent" CR));
      JsonObject result = getBTJsonObject();
      result["id"] = action.addr;
      result["success"] = false;
      pubBT(result);
    }
  } else {
    BLEactions.push_back(action);
  }
#  endif
}

void MQTTtoBT(char* topicOri, JsonObject& BTdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoBTset)) {
    Log.trace(F("MQTTtoBT json set" CR));

    // Black list & white list set
    bool WorBupdated;
    WorBupdated = updateWorB(BTdata, true);
    WorBupdated |= updateWorB(BTdata, false);

    if (WorBupdated) {
#  ifdef ESP32
      if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(1000)) == pdTRUE) {
        dumpDevices();
        xSemaphoreGive(semaphoreCreateOrUpdateDevice);
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

    MQTTtoBTAction(BTdata);
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
