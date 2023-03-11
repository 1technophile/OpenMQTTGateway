#ifdef ESP32
#  include "User_config.h"
#  ifdef ZgatewayBT
#    include "ArduinoJson.h"
#    include "ArduinoLog.h"
#    include "ZgatewayBLEConnect.h"
#    define convertTemp_CtoF(c) ((c * 1.8) + 32)

extern bool ProcessLock;
extern std::vector<BLEdevice*> devices;

NimBLERemoteCharacteristic* zBLEConnect::getCharacteristic(const NimBLEUUID& service,
                                                           const NimBLEUUID& characteristic) {
  BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
  if (!m_pClient) {
    Log.error(F("No BLE client" CR));
  } else if (!m_pClient->isConnected() && !m_pClient->connect()) {
    Log.error(F("Connect to: %s failed" CR), m_pClient->getPeerAddress().toString().c_str());
  } else {
    BLERemoteService* pRemoteService = m_pClient->getService(service);
    if (!pRemoteService) {
      Log.notice(F("Failed to find service UUID: %s" CR), service.toString().c_str());
    } else {
      Log.trace(F("Found service: %s" CR), service.toString().c_str());
      Log.trace(F("Client isConnected, freeHeap: %d" CR), ESP.getFreeHeap());
      pRemoteCharacteristic = pRemoteService->getCharacteristic(characteristic);
      if (!pRemoteCharacteristic) {
        Log.notice(F("Failed to find characteristic UUID: %s" CR), characteristic.toString().c_str());
      }
    }
  }

  return pRemoteCharacteristic;
}

bool zBLEConnect::writeData(BLEAction* action) {
  NimBLERemoteCharacteristic* pChar = getCharacteristic(action->service, action->characteristic);
  if (pChar && (pChar->canWrite() || pChar->canWriteNoResponse())) {
    switch (action->value_type) {
      case BLE_VAL_HEX: {
        int len = action->value.length();
        if (len % 2) {
          Log.error(F("Invalid HEX value length" CR));
          return false;
        }

        std::vector<uint8_t> buf;
        for (auto i = 0; i < len; i += 2) {
          std::string temp = action->value.substr(i, 2);
          buf.push_back((uint8_t)strtoul(temp.c_str(), nullptr, 16));
        }
        return pChar->writeValue((const uint8_t*)&buf[0], buf.size(), !pChar->canWriteNoResponse());
      }
      case BLE_VAL_INT:
        return pChar->writeValue(strtol(action->value.c_str(), nullptr, 0), !pChar->canWriteNoResponse());
      case BLE_VAL_FLOAT:
        return pChar->writeValue(strtod(action->value.c_str(), nullptr), !pChar->canWriteNoResponse());
      default:
        return pChar->writeValue(action->value, !pChar->canWriteNoResponse());
    }
  }
  return false;
}

bool zBLEConnect::readData(BLEAction* action) {
  NimBLERemoteCharacteristic* pChar = getCharacteristic(action->service, action->characteristic);

  if (pChar && pChar->canRead()) {
    action->value = pChar->readValue();
    if (action->value != "") {
      return true;
    }
  }
  return false;
}

bool zBLEConnect::processActions(std::vector<BLEAction>& actions) {
  bool result = false;
  if (actions.size() > 0) {
    for (auto& it : actions) {
      if (NimBLEAddress(it.addr) == m_pClient->getPeerAddress()) {
        JsonObject BLEresult = getBTJsonObject();
        BLEresult["id"] = it.addr;
        BLEresult["service"] = it.service.toString();
        BLEresult["characteristic"] = it.characteristic.toString();

        if (it.write) {
          Log.trace(F("processing BLE write" CR));
          BLEresult["write"] = it.value;
          result = writeData(&it);
        } else {
          Log.trace(F("processing BLE read" CR));
          result = readData(&it);
          if (result) {
            switch (it.value_type) {
              case BLE_VAL_HEX: {
                char* pHex = NimBLEUtils::buildHexData(nullptr, (uint8_t*)it.value.c_str(), it.value.length());
                BLEresult["read"] = pHex;
                free(pHex);
                break;
              }
              case BLE_VAL_INT: {
                int ival = *(int*)it.value.data();
                BLEresult["read"] = ival;
                break;
              }
              case BLE_VAL_FLOAT: {
                float fval = *(double*)it.value.data();
                BLEresult["read"] = fval;
                break;
              }
              default:
                BLEresult["read"] = it.value.c_str();
                break;
            }
          }
        }

        it.complete = result;
        BLEresult["success"] = result;
        if (result || it.ttl <= 1) {
          pubBT(BLEresult);
        }
      }
    }
  }

  return result;
}

/*-----------------------LYWSD03MMC && MHO_C401 HANDLING-----------------------*/
void LYWSD03MMC_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return; // unexpected notification
  }
  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());

    if (length == 5) {
      Log.trace(F("Device identified creating BLE buffer" CR));
      JsonObject BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      for (std::vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
        BLEdevice* p = *it;
        if ((strcmp(p->macAdr, (char*)mac_address.c_str()) == 0)) {
          if (p->sensorModel_id == BLEconectable::id::LYWSD03MMC)
            BLEdata["model"] = "LYWSD03MMC";
          else if (p->sensorModel_id == BLEconectable::id::MHO_C401)
            BLEdata["model"] = "MHO-C401";
        }
      }
      BLEdata["id"] = (char*)mac_address.c_str();
      Log.trace(F("Device identified in CB: %s" CR), (char*)mac_address.c_str());
      BLEdata["tempc"] = (float)((pData[0] | (pData[1] << 8)) * 0.01);
      BLEdata["tempf"] = (float)(convertTemp_CtoF((pData[0] | (pData[1] << 8)) * 0.01));
      BLEdata["hum"] = (float)(pData[2]);
      BLEdata["volt"] = (float)(((pData[4] * 256) + pData[3]) / 1000.0);
      BLEdata["batt"] = (float)(((((pData[4] * 256) + pData[3]) / 1000.0) - 2.1) * 100);

      pubBT(BLEdata);
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

void LYWSD03MMC_connect::publishData() {
  NimBLEUUID serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
  NimBLEUUID charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar && pChar->canNotify()) {
    Log.trace(F("Registering notification" CR));
    if (pChar->subscribe(true, std::bind(&LYWSD03MMC_connect::notifyCB, this,
                                         std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4))) {
      m_taskHandle = xTaskGetCurrentTaskHandle();
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
        m_taskHandle = nullptr;
      }
    } else {
      Log.notice(F("Failed registering notification" CR));
    }
  }
}

/*-----------------------DT24 HANDLING-----------------------*/
void DT24_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return; // unexpected notification
  }

  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());
    if (length == 20) {
      m_data.assign(pData, pData + length);
      return;
    } else if (m_data.size() == 20 && length == 16) {
      m_data.insert(m_data.end(), pData, pData + length);

      // DT24-BLE data format
      // https://github.com/NiceLabs/atorch-console/blob/master/docs/protocol-design.md#dc-meter-report
      // Data comes as two packets ( 20 and 16 ), and am only processing first
      Log.trace(F("Device identified creating BLE buffer" CR));
      JsonObject& BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      BLEdata["model"] = "DT24";
      BLEdata["id"] = (char*)mac_address.c_str();
      Log.trace(F("Device identified in CB: %s" CR), (char*)mac_address.c_str());
      BLEdata["volt"] = (float)(((m_data[4] * 256 * 256) + (m_data[5] * 256) + m_data[6]) / 10.0);
      BLEdata["current"] = (float)(((m_data[7] * 256 * 256) + (m_data[8] * 256) + m_data[9]) / 1000.0);
      BLEdata["power"] = (float)(((m_data[10] * 256 * 256) + (m_data[11] * 256) + m_data[12]) / 10.0);
      BLEdata["energy"] = (float)(((m_data[13] * 256 * 256 * 256) + (m_data[14] * 256 * 256) + (m_data[15] * 256) + m_data[16]) / 100.0);
      BLEdata["price"] = (float)(((m_data[17] * 256 * 256) + (m_data[18] * 256) + m_data[19]) / 100.0);
      BLEdata["tempc"] = (float)(m_data[24] * 256) + m_data[25];
      BLEdata["tempf"] = (float)(convertTemp_CtoF((m_data[24] * 256) + m_data[25]));

      pubBT(BLEdata);
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

void DT24_connect::publishData() {
  NimBLEUUID serviceUUID("ffe0");
  NimBLEUUID charUUID("ffe1");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar && pChar->canNotify()) {
    Log.trace(F("Registering notification" CR));
    if (pChar->subscribe(true, std::bind(&DT24_connect::notifyCB, this,
                                         std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4))) {
      m_taskHandle = xTaskGetCurrentTaskHandle();
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
        m_taskHandle = nullptr;
      }
    } else {
      Log.notice(F("Failed registering notification" CR));
    }
  }
}

/*-----------------------BM2 HANDLING-----------------------*/
void BM2_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return; // unexpected notification
  }

  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());
    if (length == 16) {
      Log.trace(F("Device identified creating BLE buffer" CR));
      JsonObject& BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      BLEdata["model"] = "BM2 Battery Monitor";
      BLEdata["id"] = (char*)mac_address.c_str();
      mbedtls_aes_context aes;
      mbedtls_aes_init(&aes);
      unsigned char output[16];
      unsigned char iv[16] = {};
      unsigned char key[16] = {
          108,
          101,
          97,
          103,
          101,
          110,
          100,
          255,
          254,
          49,
          56,
          56,
          50,
          52,
          54,
          54,
      };
      mbedtls_aes_setkey_dec(&aes, key, 128);
      mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, 16, iv, (uint8_t*)&pData[0], output);
      mbedtls_aes_free(&aes);
      float volt = ((output[2] | (output[1] << 8)) >> 4) / 100.0f;
      BLEdata["volt"] = volt;
      Log.trace(F("volt: %F" CR), volt);
      pubBT(BLEdata);
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

void BM2_connect::publishData() {
  NimBLEUUID serviceUUID("fff0");
  NimBLEUUID charUUID("fff4");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar && pChar->canNotify()) {
    Log.trace(F("Registering notification" CR));
    if (pChar->subscribe(true, std::bind(&BM2_connect::notifyCB, this,
                                         std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4))) {
      m_taskHandle = xTaskGetCurrentTaskHandle();
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
        m_taskHandle = nullptr;
      }
    } else {
      Log.notice(F("Failed registering notification" CR));
    }
  }
}

/*-----------------------HHCCJCY01HHCC HANDLING-----------------------*/
void HHCCJCY01HHCC_connect::publishData() {
  NimBLEUUID serviceUUID("00001204-0000-1000-8000-00805f9b34fb");
  NimBLEUUID charUUID("00001a00-0000-1000-8000-00805f9b34fb");
  NimBLEUUID charUUID2("00001a02-0000-1000-8000-00805f9b34fb");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar) {
    Log.trace(F("Read mode" CR));
    uint8_t buf[2] = {0xA0, 0x1F};
    pChar->writeValue(buf, 2, true);
    int batteryValue = -1;
    NimBLERemoteCharacteristic* pChar2 = getCharacteristic(serviceUUID, charUUID2);
    if (pChar2) {
      std::string value;
      value = pChar2->readValue();
      const char* val2 = value.c_str();
      batteryValue = val2[0];
      JsonObject& BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      BLEdata["model"] = "HHCCJCY01HHCC";
      BLEdata["id"] = (char*)mac_address.c_str();
      BLEdata["batt"] = (int)batteryValue;
      pubBT(BLEdata);
    } else {
      Log.notice(F("Failed getting characteristic" CR));
    }
  } else {
    Log.notice(F("Failed getting characteristic" CR));
  }
}

/*-----------------------XMWSDJ04MMC HANDLING-----------------------*/
void XMWSDJ04MMC_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return; // unexpected notification
  }
  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());

    if (length == 6) {
      Log.trace(F("Device identified creating BLE buffer" CR));
      JsonObject BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      BLEdata["model"] = "XMWSDJ04MMC";
      BLEdata["id"] = (char*)mac_address.c_str();
      Log.trace(F("Device identified in CB: %s" CR), (char*)mac_address.c_str());
      BLEdata["tempc"] = (float)((pData[0] | (pData[1] << 8)) * 0.1);
      BLEdata["tempf"] = (float)(convertTemp_CtoF((pData[0] | (pData[1] << 8)) * 0.1));
      BLEdata["hum"] = (float)((pData[2] | (pData[3] << 8)) * 0.1);
      BLEdata["volt"] = (float)((pData[4] | (pData[5] << 8)) / 1000.0);
      BLEdata["batt"] = (float)((((pData[4] | (pData[5] << 8)) / 1000.0) - 2.1) * 100);

      pubBT(BLEdata);
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

void XMWSDJ04MMC_connect::publishData() {
  NimBLEUUID serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
  NimBLEUUID charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar && pChar->canNotify()) {
    Log.trace(F("Registering notification" CR));
    if (pChar->subscribe(true, std::bind(&XMWSDJ04MMC_connect::notifyCB, this,
                                         std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4))) {
      m_taskHandle = xTaskGetCurrentTaskHandle();
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
        m_taskHandle = nullptr;
      }
    } else {
      Log.notice(F("Failed registering notification" CR));
    }
  }
}

/*-----------------------SBS1 HANDLING-----------------------*/
void SBS1_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return; // unexpected notification
  }
  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());

    if (length) {
      m_notifyVal = *pData;
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

bool SBS1_connect::processActions(std::vector<BLEAction>& actions) {
  NimBLEUUID serviceUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
  NimBLEUUID charUUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");
  NimBLEUUID notifyCharUUID("cba20003-224d-11e6-9fb8-0002a5d5c51b");
  static byte ON[] = {0x57, 0x01, 0x01};
  static byte OFF[] = {0x57, 0x01, 0x02};

  bool result = false;
  if (actions.size() > 0) {
    for (auto& it : actions) {
      if (NimBLEAddress(it.addr) == m_pClient->getPeerAddress()) {
        NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);
        NimBLERemoteCharacteristic* pNotifyChar = getCharacteristic(serviceUUID, notifyCharUUID);

        if (it.write && pChar && pNotifyChar) {
          Log.trace(F("processing Switchbot %s" CR), it.value.c_str());
          if (pNotifyChar->subscribe(true,
                                     std::bind(&SBS1_connect::notifyCB,
                                               this, std::placeholders::_1, std::placeholders::_2,
                                               std::placeholders::_3, std::placeholders::_4),
                                     true)) {
            if (it.value == "on") {
              result = pChar->writeValue(ON, 3, false);
            } else {
              result = pChar->writeValue(OFF, 3, false);
            }

            if (result) {
              m_taskHandle = xTaskGetCurrentTaskHandle();
              if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
                m_taskHandle = nullptr;
              }
              result = m_notifyVal == 0x01;
            }
          }
        }

        it.complete = result;
        if (result || it.ttl <= 1) {
          JsonObject BLEresult = getBTJsonObject();
          BLEresult["id"] = it.addr;
          BLEresult["state"] = result ? it.value : it.value == "on" ? "off" : "on";
          pubBT(BLEresult);
        }
      }
    }
  }

  return result;
}

#  endif //ZgatewayBT
#endif //ESP32
