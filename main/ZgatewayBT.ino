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

#ifdef ESP32
#include "FreeRTOS.h"
FreeRTOS::Semaphore semaphoreCreateOrUpdateDevice = FreeRTOS::Semaphore("createOrUpdateDevice");
  // Headers used for deep sleep functions
  #include <esp_wifi.h>
  #include <esp_bt.h>
  #include <esp_bt_main.h>
  #include <driver/adc.h>
#endif

#ifdef ZgatewayBT

#include <vector>
using namespace std;
vector<BLEdevice> devices;

static BLEdevice NO_DEVICE_FOUND = { {0,0,0,0,0,0,0,0,0,0,0,0}, false, false, false };
static bool oneWhite = false;

BLEdevice * getDeviceByMac(const char *mac)
{
  Log.trace(F("getDeviceByMac %s" CR), mac);

  for (vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p)
  {
    if ((strcmp(p->macAdr, mac) == 0))
    {
      return &(*p);
    }
  }
  return &NO_DEVICE_FOUND;
}

bool updateWorB(JsonObject &BTdata, bool isWhite)
{
  Log.trace(F("update WorB" CR));
  const char* jsonKey = isWhite ? "white-list" : "black-list";

  int size = BTdata[jsonKey].size();
  if (size == 0) 
    return false;

  for (int i = 0; i < size; i++)
  {
    const char *mac = BTdata[jsonKey][i];

    createOrUpdateDevice(mac, (isWhite ? device_flags_isWhiteL : device_flags_isBlackL));
  }
  
  return true;
}

void createOrUpdateDevice(const char *mac, uint8_t flags)
{
#ifdef ESP32
  if (!semaphoreCreateOrUpdateDevice.take(30000, "createOrUpdateDevice"))
    return;
#endif

  BLEdevice *device = getDeviceByMac(mac);
  if(device == &NO_DEVICE_FOUND)
  {
    Log.trace(F("add %s" CR), mac);
    //new device
    device = new BLEdevice();
    strcpy(device->macAdr, mac);
    device->isDisc = flags & device_flags_isDisc;
    device->isWhtL = flags & device_flags_isWhiteL;
    device->isBlkL = flags & device_flags_isBlackL;
    devices.push_back(*device);
  }
  else
  {
    Log.trace(F("update %s" CR), mac);
    
    if(flags & device_flags_isDisc)
    {
      device->isDisc = true;
    }

    if(flags & device_flags_isWhiteL || flags & device_flags_isBlackL)
    {
      device->isWhtL = flags & device_flags_isWhiteL;
      device->isBlkL = flags & device_flags_isBlackL;
    }
  }

  // update oneWhite flag
  oneWhite = oneWhite || device->isWhtL;

#ifdef ESP32
  semaphoreCreateOrUpdateDevice.give();
#endif
}

#define isWhite(device) device->isWhtL
#define isBlack(device) device->isBlkL
#define isDiscovered(device) device->isDisc

void dumpDevices()
{
  for (vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p)
  {
    Log.trace(F("macAdr %s" CR),p->macAdr);
    Log.trace(F("isDisc %d" CR),p->isDisc);
    Log.trace(F("isWhtL %d" CR),p->isWhtL);
    Log.trace(F("isBlkL %d" CR),p->isBlkL);
  }
}

void strupp(char *beg)
{
  while (*beg = toupper(*beg))
    ++beg;
}

#ifdef ZmqttDiscovery
void MiFloraDiscovery(char *mac)
{
#define MiFloraparametersCount 4
  Log.trace(F("MiFloraDiscovery" CR));
  char *MiFlorasensor[MiFloraparametersCount][8] = {
      {"sensor", "MiFlora-lux", mac, "illuminance", jsonLux, "", "", "lu"},
      {"sensor", "MiFlora-tem", mac, "temperature", jsonTemp, "", "", "°C"},
      {"sensor", "MiFlora-fer", mac, "", jsonFer, "", "", "µS/cm"},
      {"sensor", "MiFlora-moi", mac, "", jsonMoi, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiFlorasensor, MiFloraparametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void VegTrugDiscovery(char *mac)
{
#define VegTrugparametersCount 4
  Log.trace(F("VegTrugDiscovery" CR));
  char *VegTrugsensor[VegTrugparametersCount][8] = {
      {"sensor", "VegTrug-lux", mac, "illuminance", jsonLux, "", "", "lu"},
      {"sensor", "VegTrug-tem", mac, "temperature", jsonTemp, "", "", "°C"},
      {"sensor", "VegTrug-fer", mac, "", jsonFer, "", "", "µS/cm"},
      {"sensor", "VegTrug-moi", mac, "", jsonMoi, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  createDiscoveryFromList(mac, VegTrugsensor, VegTrugparametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void MiJiaDiscovery(char *mac)
{
#define MiJiaparametersCount 3
  Log.trace(F("MiJiaDiscovery" CR));
  char *MiJiasensor[MiJiaparametersCount][8] = {
      {"sensor", "MiJia-batt", mac, "battery", jsonBatt, "", "", "%"},
      {"sensor", "MiJia-tem", mac, "temperature", jsonTemp, "", "", "°C"},
      {"sensor", "MiJia-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  createDiscoveryFromList(mac, MiJiasensor, MiJiaparametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void LYWSD02Discovery(char *mac)
{
#define LYWSD02parametersCount 3
  Log.trace(F("LYWSD02Discovery" CR));
  char *LYWSD02sensor[LYWSD02parametersCount][8] = {
      {"sensor", "LYWSD02-batt", mac, "battery", jsonBatt, "", "", "V"},
      {"sensor", "LYWSD02-tem", mac, "temperature", jsonTemp, "", "", "°C"},
      {"sensor", "LYWSD02-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, LYWSD02sensor, LYWSD02parametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void CLEARGRASSTRHDiscovery(char *mac)
{
#define CLEARGRASSTRHparametersCount 3
  Log.trace(F("CLEARGRASSTRHDiscovery" CR));
  char *CLEARGRASSTRHsensor[CLEARGRASSTRHparametersCount][8] = {
      {"sensor", "CLEARGRASSTRH-batt", mac, "battery", jsonBatt, "", "", "V"},
      {"sensor", "CLEARGRASSTRH-tem", mac, "temperature", jsonTemp, "", "", "°C"},
      {"sensor", "CLEARGRASSTRH-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, CLEARGRASSTRHsensor, CLEARGRASSTRHparametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void CLEARGRASSCGD1Discovery(char *mac)
{
#define CLEARGRASSCGD1parametersCount 3
  Log.trace(F("CLEARGRASSCGD1Discovery" CR));
  char *CLEARGRASSCGD1sensor[CLEARGRASSCGD1parametersCount][8] = {
      {"sensor", "CLEARGRASSCGD1-batt", mac, "battery", jsonBatt, "", "", "V"},
      {"sensor", "CLEARGRASSCGD1-tem", mac, "temperature", jsonTemp, "", "", "°C"},
      {"sensor", "CLEARGRASSCGD1-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, CLEARGRASSCGD1sensor, CLEARGRASSCGD1parametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void CLEARGRASSTRHKPADiscovery(char *mac)
{
#define CLEARGRASSTRHKPAparametersCount 3
  Log.trace(F("CLEARGRASSTRHKPADiscovery" CR));
  char *CLEARGRASSTRHKPAsensor[CLEARGRASSTRHKPAparametersCount][8] = {
      {"sensor", "CLEARGRASSTRHKPA-pres", mac, "pressure", jsonPres, "", "", "kPa"},
      {"sensor", "CLEARGRASSTRHKPA-tem", mac, "temperature", jsonTemp, "", "", "°C"},
      {"sensor", "CLEARGRASSTRHKPA-hum", mac, "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, CLEARGRASSTRHKPAsensor, CLEARGRASSTRHKPAparametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void MiScaleDiscovery(char *mac)
{
#define MiScaleparametersCount 1
  Log.trace(F("MiScaleDiscovery" CR));
  char *MiScalesensor[MiScaleparametersCount][8] = {
      {"sensor", "MiScale-weight", mac, "", jsonWeight, "", "", "kg"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiScalesensor, MiScaleparametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void MiLampDiscovery(char *mac)
{
#define MiLampparametersCount 1
  Log.trace(F("MiLampDiscovery" CR));
  char *MiLampsensor[MiLampparametersCount][8] = {
      {"sensor", "MiLamp-presence", mac, "", jsonPresence, "", "", "d"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiLampsensor, MiLampparametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}

void MiBandDiscovery(char *mac)
{
#define MiBandparametersCount 1
  Log.trace(F("MiBandDiscovery" CR));
  char *MiBandsensor[MiBandparametersCount][8] = {
      {"sensor", "MiBand-steps", mac, "", jsonStep, "", "", "nb"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  createDiscoveryFromList(mac, MiBandsensor, MiBandparametersCount);
  createOrUpdateDevice(mac, device_flags_isDisc);
}
#else
void MiFloraDiscovery(char *mac){}
void VegTrugDiscovery(char *mac){}
void MiJiaDiscovery(char *mac){}
void LYWSD02Discovery(char *mac){}
void CLEARGRASSTRHDiscovery(char *mac){}
void CLEARGRASSCGD1Discovery(char *mac){}
void CLEARGRASSTRHKPADiscovery(char *mac){}
void MiScaleDiscovery(char *mac){}
void MiLampDiscovery(char *mac){}
void MiBandDiscovery(char *mac){}
#endif

#ifdef ESP32
/*
       Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
       Ported to Arduino ESP32 by Evandro Copercini
    */
// core task implementation thanks to https://techtutorialsx.com/2017/05/09/esp32-running-code-on-a-specific-core/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

//core on which the BLE detection task will run
static int taskCore = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Log.trace(F("Creating BLE buffer" CR));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject &BLEdata = jsonBuffer.createObject();
    String mac_adress = advertisedDevice.getAddress().toString().c_str();
    mac_adress.toUpperCase();
    BLEdata.set("id", (char *)mac_adress.c_str());
    mac_adress.replace(":", "");
    String mactopic = subjectBTtoMQTT + String("/") + mac_adress;
    char mac[mac_adress.length() + 1];
    mac_adress.toCharArray(mac, mac_adress.length() + 1);
    Log.notice(F("Device detected: %s" CR), mac);
    BLEdevice *device = getDeviceByMac(mac);

    if ((!oneWhite || isWhite(device)) && !isBlack(device))
    { //if not black listed mac we go AND if we have no white mac or this mac is  white we go out
      if (advertisedDevice.haveName())
        BLEdata.set("name", (char *)advertisedDevice.getName().c_str());
      #if pubBLEManufacturerData
      if (advertisedDevice.haveManufacturerData())
      {
        char* manufacturerdata = BLEUtils::buildHexData(NULL, (uint8_t*)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
        Log.trace(F("Manufacturer Data: %s" CR), manufacturerdata);
        BLEdata.set("manufacturerdata", manufacturerdata);
      }
      #endif
      if (advertisedDevice.haveRSSI())
        BLEdata.set("rssi", (int)advertisedDevice.getRSSI());
      if (advertisedDevice.haveTXPower())
        BLEdata.set("txpower", (int8_t)advertisedDevice.getTXPower());
      #ifdef subjectHomePresence
      if (advertisedDevice.haveRSSI())
        haRoomPresence(BLEdata); // this device has an rssi in consequence we can use it for home assistant room presence component
      #endif
      if (advertisedDevice.haveServiceData())
      {
        int serviceDataCount = advertisedDevice.getServiceDataCount();
        Log.trace(F("Get services data number: %d" CR), serviceDataCount);
        for (int j = 0; j < serviceDataCount; j++)
        {
          std::string serviceData = advertisedDevice.getServiceData(j);
          int serviceDataLength = serviceData.length();
          String returnedString = "";
          for (int i = 0; i < serviceDataLength; i++)
          {
            int a = serviceData[i];
            if (a < 16)
            {
              returnedString = returnedString + "0";
            }
            returnedString = returnedString + String(a, HEX);
          }
          char service_data[returnedString.length() + 1];
          returnedString.toCharArray(service_data, returnedString.length() + 1);
          service_data[returnedString.length()] = '\0';
          Log.trace(F("Service data: %s" CR), service_data);
          BLEdata.set("servicedata", service_data);
          std::string serviceDatauuid = advertisedDevice.getServiceDataUUID(j).toString();
          Log.trace(F("Service data UUID: %s" CR), (char *)serviceDatauuid.c_str());
          BLEdata.set("servicedatauuid", (char *)serviceDatauuid.c_str());
          PublishDeviceData(BLEdata);
        }
      }
      else
      {
        PublishDeviceData(BLEdata); // publish device even if there is no service data
      }
    }
    else
    {
      Log.trace(F("Filtered mac device" CR));
    }
  }
};

void BLEscan()
{
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
  Log.notice(F("Scan begin" CR));
  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan(); //create new scan
  MyAdvertisedDeviceCallbacks myCallbacks;
  pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  BLEScanResults foundDevices = pBLEScan->start(Scan_duration);
  Log.notice(F("Scan end, deinit controller" CR));
  esp_bt_controller_deinit();
}

void coreTask(void *pvParameters)
{
  while (true)
  {
    Log.trace(F("BT Task running on core: %d" CR), xPortGetCoreID());
    if (!low_power_mode)
      delay(BLEinterval);
    int n = 0;
    while (client.state() != 0 && n <= TimeBeforeMQTTconnect)
    {
      n++;
      Log.trace(F("Wait for MQTT on core: %d attempt: %d" CR), xPortGetCoreID(), n);
      delay(1000);
    }
    if(client.state() != 0 ) 
    {
      Log.warning(F("MQTT client disconnected no BLE scan" CR));
      delay(1000);
    }
    else 
    {
      pinMode(LOW_POWER_LED, OUTPUT);
      if (low_power_mode == 2)
        digitalWrite(LOW_POWER_LED, 1 - LOW_POWER_LED_OFF);
      BLEscan();
      //only change LOW_POWER_LED if low power mode is enabled
      if (low_power_mode)
        digitalWrite(LOW_POWER_LED, LOW_POWER_LED_OFF);
    }
    if(low_power_mode)
      lowPowerESP32();
  }
}

void lowPowerESP32() // low power mode
{
  Log.trace(F("Going to deep sleep for: %l s" CR), (BLEinterval / 1000));
  deepSleep(BLEinterval * 1000);
}

void deepSleep(uint64_t time_in_us)
{
  #if defined(ZboardM5STACK) || defined(ZboardM5STICKC)
  sleepScreen();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)SLEEP_BUTTON, LOW);
  #endif

  Log.trace(F("Deactivating ESP32 components" CR));
  esp_wifi_stop();
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  adc_power_off();
  esp_wifi_stop();
  esp_deep_sleep(time_in_us);
}

void changelow_power_mode(int newLowPowerMode)
{
  Log.notice(F("Changing LOW POWER mode to: %d" CR), newLowPowerMode);
  #if defined(ZboardM5STACK) || defined(ZboardM5STICKC)
  if (low_power_mode == 2)
  {
    #ifdef ZboardM5STACK
    M5.Lcd.wakeup();
    #endif
    #ifdef ZboardM5STICKC
    M5.Axp.SetLDO2(true);
    M5.Lcd.begin();
    #endif
  }
  char lpm[2];
  sprintf(lpm, "%d", newLowPowerMode);
  M5Display("Changing LOW POWER mode to:", lpm, "");
  #endif
  low_power_mode = newLowPowerMode;
  preferences.begin(Gateway_Short_Name, false);
  preferences.putUInt("low_power_mode", low_power_mode);
  preferences.end();
}

void setupBT()
{
  Log.notice(F("BLEinterval: %d" CR), BLEinterval);
  Log.notice(F("Minrssi: %d" CR), Minrssi);
  Log.notice(F("Low Power Mode: %d" CR), low_power_mode);

  // we setup a task with priority one to avoid conflict with other gateways
  xTaskCreatePinnedToCore(
      coreTask,   /* Function to implement the task */
      "coreTask", /* Name of the task */
      10000,      /* Stack size in words */
      NULL,       /* Task input parameter */
      1,          /* Priority of the task */
      NULL,       /* Task handle. */
      taskCore);  /* Core where the task should run */
  Log.trace(F("ZgatewayBT multicore ESP32 setup done " CR));
}

bool BTtoMQTT()
{ // for on demand BLE scans
  BLEscan();
}
#else // arduino or ESP8266 working with HM10/11

#include <SoftwareSerial.h>
#define QUESTION_MSG "AT+DISA?"

SoftwareSerial softserial(BT_RX, BT_TX);

String returnedString = "";
unsigned long timebt = 0;

// this struct define which parts of the hexadecimal chain we extract and what to do with these parts
struct decompose d[6] = {{"mac", 16, 12, true}, {"typ", 28, 2, false}, {"rsi", 30, 2, false}, {"rdl", 32, 2, false}, {"sty", 44, 4, true}, {"rda", 56, 60, false}};

void setupBT()
{
  BLEinterval = TimeBtw_Read;
  Minrssi = MinimumRSSI;
  Log.notice(F("BLEinterval: %d" CR), BLEinterval);
  Log.notice(F("Minrssi: %d" CR), Minrssi);
  softserial.begin(HMSerialSpeed);
  softserial.print(F("AT+ROLE1" CR));
  delay(100);
  softserial.print(F("AT+IMME1" CR));
  delay(100);
  softserial.print(F("AT+RESET" CR));
  delay(100);
  #ifdef HM_BLUE_LED_STOP
  softserial.print(F("AT+PIO11" CR)); // When not connected (as in BLE mode) the LED is off. When connected the LED is solid on.
  #endif
  delay(100);
  Log.trace(F("ZgatewayBT HM1X setup done " CR));
}

bool BTtoMQTT()
{
  //extract serial data from module in hexa format
  while (softserial.available() > 0)
  {
    int a = softserial.read();
    if (a < 16)
    {
      returnedString = returnedString + "0";
    }
    returnedString = returnedString + String(a, HEX);
  }

  if (millis() > (timebt + BLEinterval))
  { //retrieving data
    timebt = millis();
    #if defined(ESP8266)
    yield();
    #endif
    if (returnedString != "")
    {
      Log.verbose(F("returnedString: %s" CR), (char *)returnedString.c_str());
      size_t pos = 0;
      while ((pos = returnedString.lastIndexOf(BLEdelimiter)) != -1)
      {
        #if defined(ESP8266)
        yield();
        #endif
        String token = returnedString.substring(pos);
        returnedString.remove(pos, returnedString.length());
        char token_char[token.length() + 1];
        token.toCharArray(token_char, token.length() + 1);
        Log.trace(F("Token: %s" CR), token_char);
        if (token.length() > 60)
        { // we extract data only if we have infos (BLEdelimiter length + 4)
          for (int i = 0; i < 6; i++)
          {
            extract_char(token_char, d[i].extract, d[i].start, d[i].len, d[i].reverse, false);
            if (i == 3)
              d[5].len = (int)strtol(d[i].extract, NULL, 16) * 2; // extracting the length of the rest data
          }

          if ((strlen(d[0].extract)) == 12) // if a BLE device is detected we analyse it
          {
            Log.trace(F("Creating BLE buffer" CR));
            StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
            JsonObject &BLEdata = jsonBuffer.createObject();
            strupp(d[0].extract);

            BLEdevice *device = getDeviceByMac(d[0].extract);

            if (isBlack(device))
              return false; //if black listed mac we go out
            if (oneWhite && !isWhite(device))
              return false; //if we have at least one white mac and this mac is not white we go out
            #ifdef subjectHomePresence
            String HomePresenceId;
            for (int i = 0; i < 12; i++)
            {
              HomePresenceId += String(d[0].extract[i]);
              if (((i - 1) % 2 == 0) && (i != 11))
                HomePresenceId += ":";
            }
            Log.trace(F("HomePresenceId %s" CR),(char *)HomePresenceId.c_str());
            BLEdata.set("id", (char *)HomePresenceId.c_str());
            #endif
            int rssi = (int)strtol(d[2].extract, NULL, 16) - 256;
            BLEdata.set("rssi", (int)rssi);
            #ifdef subjectHomePresence
            haRoomPresence(BLEdata); // this device has an rssi in consequence we can use it for home assistant room presence component
            #endif
            Log.trace(F("Service data: %s" CR), d[5].extract);
            BLEdata.set("servicedata", d[5].extract);
            PublishDeviceData(BLEdata);
          }
        }
      }
      returnedString = ""; //init data string
      return false;
    }
    softserial.print(F(QUESTION_MSG));
    return false;
  }
  else
  {
    return false;
  }
}
#endif

void RemoveJsonPropertyIf(JsonObject &obj, char* key, bool condition)
{
  if(condition){
     Log.trace(F("Removing %s" CR), key);
     obj.remove(key);
  }
}

double value_from_service_data(const char *service_data, int offset, int data_length)
{
  char rev_data[data_length + 1];
  char data[data_length + 1];
  memcpy(rev_data, &service_data[offset], data_length);
  rev_data[data_length] = '\0';

  // reverse data order
  revert_hex_data(rev_data, data, data_length + 1);
  double value = strtol(data, NULL, 16);
  if (value > 65000 && data_length <= 4)
    value = value - 65535;
  Log.trace(F("value %D" CR),value);
  return value;
}

boolean valid_service_data(const char * data)
{
  int size = strlen(data);
  for (int i = 0; i < size; ++i) {
    if(data[i] != 48) // 48 correspond to 0 in ASCII table
      return true;
  }
  return false;
}

void launch_discovery(JsonObject &BLEdata, char * mac){
  BLEdevice *device = getDeviceByMac(mac);
  if (!isDiscovered(device) && BLEdata.containsKey("model"))
  {
    Log.trace(F("Launching discovery of %s" CR), mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "HHCCJCY01HHCC") == 0)   MiFloraDiscovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "VegTrug") == 0)         VegTrugDiscovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "LYWSDCGQ") == 0)        MiJiaDiscovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "LYWSD02") == 0)         LYWSD02Discovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "CGG1") == 0)            CLEARGRASSTRHDiscovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "CGP1W") == 0)           CLEARGRASSTRHKPADiscovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "MUE4094RT") == 0)       MiLampDiscovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "CGD1") == 0)            CLEARGRASSCGD1Discovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "MiBand") == 0)          MiBandDiscovery(mac);
    if(strcmp(BLEdata["model"].as<const char*>(), "XMTZC04HM") == 0 || 
      strcmp(BLEdata["model"].as<const char*>(), "XMTZC05HM") == 0)        MiScaleDiscovery(mac);
  }
  else
  {
    Log.trace(F("Device already discovered or model not detected" CR));
  }
}

void PublishDeviceData(JsonObject &BLEdata)
{
  if (abs((int)BLEdata["rssi"] | 0) < abs(Minrssi))
  { // process only the devices close enough
    JsonObject &BLEdataOut = process_bledata(BLEdata);
    String mac_adress = BLEdataOut["id"].as<const char*>();
    mac_adress.replace(":", "");
    char mac[mac_adress.length() + 1];
    mac_adress.toCharArray(mac, mac_adress.length() + 1);
    #ifdef ZmqttDiscovery
      launch_discovery(BLEdataOut, mac);
    #endif
    #if !pubBLEServiceUUID
      RemoveJsonPropertyIf(BLEdataOut, "servicedatauuid", BLEdataOut.containsKey("servicedatauuid"));
    #endif
    #if !pubKnownBLEServiceData
      RemoveJsonPropertyIf(BLEdataOut, "servicedata", BLEdataOut.containsKey("model") && BLEdataOut.containsKey("servicedata"));
    #endif
    String mactopic(mac);
    mactopic = subjectBTtoMQTT + String("/") + mactopic;
    pub((char *)mactopic.c_str(), BLEdataOut);
  }
  else
  {
    Log.trace(F("Low rssi, device filtered" CR));
  }
}

JsonObject& process_bledata(JsonObject &BLEdata){
  if(BLEdata.containsKey("servicedata"))
  {
    Log.trace(F("Checking BLE service data validity" CR));
    const char * service_data =  (const char *)(BLEdata["servicedata"]|"");
    if(valid_service_data(service_data))
    {
      Log.trace(F("Searching BLE device data %s size %d" CR), service_data, strlen(service_data));
      Log.trace(F("Is it a mi flora ?" CR));
      if (strstr(service_data, "209800") != NULL)
      {
        Log.trace(F("mi flora data reading" CR));
        BLEdata.set("model", "HHCCJCY01HHCC");

        return process_sensors(2, BLEdata);
      }
      Log.trace(F("Is it a vegtrug ?" CR));
      if (strstr(service_data, "20bc03") != NULL && strlen(service_data) > ServicedataMinLength)
      {
        Log.trace(F("vegtrug data reading" CR));
        BLEdata.set("model", "VegTrug");

        return process_sensors(2, BLEdata);
      }
      Log.trace(F("Is it a LYWSDCGQ?" CR));
      if (strstr(service_data, "20aa01") != NULL && strlen(service_data) > ServicedataMinLength)
      {
        Log.trace(F("LYWSDCGQ data reading" CR));
        BLEdata.set("model", "LYWSDCGQ");

        return process_sensors(0, BLEdata);
      }
      Log.trace(F("Is it a LYWSD02?" CR));
      if (strstr(service_data, "205b04") != NULL && strlen(service_data)  > ServicedataMinLength)
      {
        Log.trace(F("LYWSD02 data reading" CR));
        BLEdata.set("model", "LYWSD02");

        return process_sensors(2, BLEdata);
      }
      Log.trace(F("Is it a CGG1?" CR));
      if (strstr(service_data, "304703") != NULL && strlen(service_data)  > ServicedataMinLength)
      {
        Log.trace(F("CGG1 data reading method 1" CR));
        BLEdata.set("model", "CGG1");

        return process_sensors(0, BLEdata);
      }
      Log.trace(F("Is it a MUE4094RT?" CR));
      if (strstr(service_data, "4030dd") != NULL)
      {
        Log.trace(F("MUE4094RT data reading" CR));
        BLEdata.set("model", "MUE4094RT");

        return process_milamp(BLEdata);
      }
      Log.trace(F("Is it a CGP1W?" CR));
      if (strstr(service_data, "08094c") != NULL &&  strlen(service_data) > ServicedataMinLength)
      {
        Log.trace(F("CGP1W data reading" CR));
        BLEdata.set("model", "CGP1W");

        return process_cleargrass(BLEdata, true);
      }
      Log.trace(F("Is it a CGG1" CR));
      if (strstr(service_data, "080774") != NULL)
      {
        Log.trace(F("CGG1 method 2" CR));
        BLEdata.set("model", "CGG1");
        // no discovery as it is already available with method 1
        return process_cleargrass(BLEdata, false);
      }
      Log.trace(F("Is it a CGD1?" CR));
      if ((strstr(service_data, "080caf") != NULL || strstr(service_data, "080c09") != NULL)
      && (strlen(service_data) > ServicedataMinLength))
      {
        Log.trace(F("CGD1 data reading" CR));
        BLEdata.set("model", "CGD1");

        return process_cleargrass(BLEdata, false);
      }
      if(BLEdata.containsKey("servicedatauuid"))
      {
        const char * service_datauuid = (const char *)(BLEdata["servicedatauuid"]|"");
        Log.trace(F("servicedatauuid %s" CR), service_datauuid);
        Log.trace(F("Is it a MiBand?" CR));
        if (strstr(service_datauuid, "fee0") != NULL)
        {
          Log.trace(F("Mi Band data reading" CR));
          BLEdata.set("model", "MiBand");

          return process_miband(BLEdata);
        }
        Log.trace(F("Is it a XMTZC04HM?" CR));
        if (strstr(service_datauuid, "181d") != NULL)
        {
          Log.trace(F("XMTZC04HM data reading" CR));
          BLEdata.set("model", "XMTZC04HM");

          return process_scale_v1(BLEdata);
        }
        Log.trace(F("Is it a XMTZC05HM?" CR));
        if (strstr(service_datauuid, "181b") != NULL)
        {
          Log.trace(F("XMTZC05HM data reading" CR));
          BLEdata.set("model", "XMTZC05HM");;

          return process_scale_v2(BLEdata);
        }
      }
    }
    else
    {
      Log.trace(F("Non valid service data, removing it" CR));
      BLEdata.remove("servicedata");
    }

    #if !pubUnknownBLEServiceData
      Log.trace(F("Unknown service data, removing it" CR));
      BLEdata.remove("servicedata");
    #endif
  }

  return BLEdata;
}

JsonObject& process_sensors(int offset, JsonObject &BLEdata)
{
  const char * servicedata = BLEdata["servicedata"].as<const char *>();
  int data_length = 0;
  
  switch (servicedata[27 + offset])
  {
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
  value = value_from_service_data(servicedata, 28 + offset, data_length);

  // Mi flora provides tem(perature), (earth) moi(sture), fer(tility) and lux (illuminance)
  // Mi Jia provides tem(perature), batt(erry) and hum(idity)
  // following the value of digit 23 + offset we determine the type of data we get from the sensor
  switch (servicedata[23 + offset])
  {
  case '9':
    BLEdata.set("fer", (double)value);
    break;
  case '4':
    BLEdata.set("tem", (double)value / 10);
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
  case 'a':
    BLEdata.set("batt", (double)value);
    break;
  case 'd':
    // humidity
    value = value_from_service_data(servicedata, 28 + offset, 4);
    BLEdata.set("tem", (double)value / 10);
    // temperature
    value = value_from_service_data(servicedata, 32 + offset, 4);
    BLEdata.set("hum", (double)value / 10);
    break;
  default:
    Log.trace(F("can't read values" CR));
  }

  return BLEdata;
}

JsonObject& process_scale_v1(JsonObject &BLEdata)
{
  const char * servicedata = BLEdata["servicedata"].as<const char *>();

  double weight = value_from_service_data(servicedata, 2, 4) / 200;

  //Set Json value
  BLEdata.set("weight", (double)weight);

  return BLEdata;
}

JsonObject& process_scale_v2(JsonObject &BLEdata)
{
  const char * servicedata = BLEdata["servicedata"].as<const char *>();

  double weight = value_from_service_data(servicedata, 22, 4) / 200;
  double impedance = value_from_service_data(servicedata, 18, 4);

  //Set Json values
  BLEdata.set("weight", (double)weight);
  BLEdata.set("impedance", (double)impedance);

  return BLEdata;
}

JsonObject& process_miband(JsonObject &BLEdata)
{
  const char * servicedata = BLEdata["servicedata"].as<const char *>();

  double steps = value_from_service_data(servicedata, 0, 4);

  //Set Json value
  BLEdata.set("steps", (double)steps);

  return BLEdata;
}

JsonObject& process_milamp(JsonObject &BLEdata)
{
  const char * servicedata = BLEdata["servicedata"].as<const char *>();

  long darkness = value_from_service_data(servicedata, 8, 2);

  //Set Json value
  BLEdata.set("presence", (bool)"true");
  BLEdata.set("darkness", (long)darkness);

  return BLEdata;
}

JsonObject& process_cleargrass(JsonObject &BLEdata, boolean air)
{
  const char * servicedata = BLEdata["servicedata"].as<const char *>();

  double value = 9999;
  // humidity
  value = value_from_service_data(servicedata, 20, 4);
  BLEdata.set("tem", (double)value / 10);
  // temperature
  value = value_from_service_data(servicedata, 24, 4);
  BLEdata.set("hum", (double)value / 10);
  if (air)
  {
    // air pressure
    value = value_from_service_data(servicedata, 32, 4);
    BLEdata.set("pres", (double)value / 100);
  }

  return BLEdata;
}

#ifdef subjectHomePresence
void haRoomPresence(JsonObject &HomePresence)
{
  int BLErssi = HomePresence["rssi"];
  Log.trace(F("BLErssi %d" CR), BLErssi);
  int txPower = HomePresence["txpower"] | 0;
  if (txPower >= 0)
    txPower = -59; //if tx power is not found we set a default calibration value
  Log.trace(F("TxPower: %d" CR),txPower);
  double ratio = BLErssi * 1.0 / txPower;
  double distance;
  if (ratio < 1.0)
  {
    distance = pow(ratio, 10);
  }
  else
  {
    distance = (0.89976) * pow(ratio, 7.7095) + 0.111;
  }
  HomePresence["distance"] = distance;
  Log.trace(F("Ble distance %D" CR),distance);
  String topic = String(Base_Topic) + "home_presence/" + String(gateway_name);
  pub_custom_topic((char *)topic.c_str(), HomePresence, false);
}
#endif

void MQTTtoBT(char *topicOri, JsonObject &BTdata)
{ // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoBTset))
  {
    Log.trace(F("MQTTtoBT json set" CR));

    // Black list & white list set
    bool WorBupdated;
    WorBupdated |= updateWorB(BTdata, true);
    WorBupdated |= updateWorB(BTdata, false);

    if (WorBupdated)
      dumpDevices();

    // Scan interval set
    if (BTdata.containsKey("interval"))
    {
      Log.trace(F("BLE interval setup" CR));
      // storing BLE interval for further use if needed
      unsigned int prevBLEinterval = BLEinterval;
      Log.trace(F("Previous interval: %d ms" CR), BLEinterval);
      // set BLE interval if present if not setting default value
      BLEinterval = (unsigned int)BTdata["interval"];
      Log.notice(F("New interval: %d ms" CR), BLEinterval);
      if (BLEinterval == 0)
      {
        if (BTtoMQTT()) // as BLEinterval is = to 0 we can launch the loop and the scan will execute immediately
          Log.trace(F("Scan done" CR));
        BLEinterval = prevBLEinterval; // as 0 was just used as a command we recover previous scan duration
      }
    }
    // MinRSSI set
    if (BTdata.containsKey("minrssi"))
    {
      // storing Min RSSI for further use if needed
      Log.trace(F("Previous Minrssi: %d" CR), Minrssi);
      // set Min RSSI if present if not setting default value
      Minrssi = (unsigned int)BTdata["minrssi"];
      Log.notice(F("New Minrssi: %d" CR), Minrssi);
    }
    #ifdef ESP32
    if (BTdata.containsKey("low_power_mode"))
    {
      changelow_power_mode((int)BTdata["low_power_mode"]);
    }
    #endif
  }
}
#endif