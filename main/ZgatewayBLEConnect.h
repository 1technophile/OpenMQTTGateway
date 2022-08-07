#ifndef zBLEConnect_h
#define zBLEConnect_h

#ifdef ESP32
#  include "ArduinoJson.h"
#  include "NimBLEDevice.h"
#  include "config_BT.h"

extern void pubBT(JsonObject& data);

class ClientCallbacks : public NimBLEClientCallbacks {
  uint32_t onPassKeyRequest() {
    return m_passkey;
  }

  bool onConfirmPIN(uint32_t pass_key) {
    return true;
  }

  friend class zBLEConnect;
  uint32_t m_passkey;
};

class zBLEConnect {
public:
  NimBLEClient* m_pClient;
  ClientCallbacks m_callbacks;
  TaskHandle_t m_taskHandle = nullptr;
  zBLEConnect(NimBLEAddress& addr) {
    m_pClient = NimBLEDevice::createClient(addr);
    m_pClient->setConnectTimeout(5);
    m_pClient->setClientCallbacks(&m_callbacks, false);
  }
  virtual ~zBLEConnect() { NimBLEDevice::deleteClient(m_pClient); }
  virtual bool writeData(BLEAction* action);
  virtual bool readData(BLEAction* action);
  virtual bool processActions(std::vector<BLEAction>& actions);
  virtual void publishData() {}
  virtual NimBLERemoteCharacteristic* getCharacteristic(const NimBLEUUID& service,
                                                        const NimBLEUUID& characteristic,
                                                        bool secure = false,
                                                        uint32_t passkey = 123456);
};

class LYWSD03MMC_connect : public zBLEConnect {
  void notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);

public:
  LYWSD03MMC_connect(NimBLEAddress& addr) : zBLEConnect(addr) {}
  void publishData() override;
};

class DT24_connect : public zBLEConnect {
  std::vector<uint8_t> m_data;
  void notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);

public:
  DT24_connect(NimBLEAddress& addr) : zBLEConnect(addr) {}
  void publishData() override;
};

class GENERIC_connect : public zBLEConnect {
  std::vector<uint8_t> m_data;

public:
  GENERIC_connect(NimBLEAddress& addr) : zBLEConnect(addr) {}
};

class HHCCJCY01HHCC_connect : public zBLEConnect {
  std::vector<uint8_t> m_data;
  void notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);

public:
  HHCCJCY01HHCC_connect(NimBLEAddress& addr) : zBLEConnect(addr) {}
  void publishData() override;
};

class XMWSDJ04MMC_connect : public zBLEConnect {
  std::vector<uint8_t> m_data;
  void notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);

public:
  XMWSDJ04MMC_connect(NimBLEAddress& addr) : zBLEConnect(addr) {}
  void publishData() override;
};

class SBS1_connect : public zBLEConnect {
  uint8_t m_notifyVal;
  void notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify);

public:
  SBS1_connect(NimBLEAddress& addr) : zBLEConnect(addr) {}
  bool processActions(std::vector<BLEAction>& actions) override;
};

#endif //ESP32
#endif //zBLEConnect_h
