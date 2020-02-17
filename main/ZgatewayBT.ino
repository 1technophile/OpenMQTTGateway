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

#include <vector>
using namespace std;
vector<BLEdevice> devices;

static BLEdevice NO_DEVICE_FOUND = { NULL, false, false, false };

BLEdevice * getDeviceByMac(const char *mac)
{
  for (vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p)
  {
    if ((strcmp(p->macAdr, mac) == 0))
    {
      return &(*p);
    }
  }
  return &NO_DEVICE_FOUND;
}

bool updateWorB(JsonObject &BTdata, bool isWhite){
  const char* jsonKey = isWhite ? "white-list" : "black-list";

  int size = BTdata[jsonKey].size();
  if (size == 0) 
    return false;
  
  bool foundMac;
  for (int i = 0; i < size; i++)
  {
    const char *mac = BTdata[jsonKey][i];
    Log.trace(F("%s set: %s" CR), jsonKey, mac);
    
    BLEdevice *device = getDeviceByMac(mac);
    if(device == &NO_DEVICE_FOUND){
      //new device
      device = new BLEdevice();
      strcpy(device->macAdr, mac);
      device->isDisc = false;
      devices.push_back(*device);
    }
    
    device->isWhtL = isWhite;
    device->isBlkL = !isWhite;
  }
  
  return true;
}

bool oneWhite()
{
  for (vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p)
  {
    if (p->isWhtL)
      return true;
  }
  return false;
}

#define isWhite(mac) getDeviceByMac(mac)->isWhtL

bool isBlack(char *mac)
{
  for (vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p)
  {
    if ((strcmp(p->macAdr, mac) == 0))
    {
      return p->isBlkL;
    }
  }
  return false;
}

bool isDiscovered(char *mac)
{
  for (vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p)
  {
    if ((strcmp(p->macAdr, mac) == 0))
    {
      return p->isDisc;
    }
  }
  return false;
}

void dumpDevices()
{
  for (vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p)
  {
    Log.trace(F("macAdr %s" CR),p->macAdr);
    Log.trace(F("isDisc %s" CR),p->isDisc);
    Log.trace(F("isWhtL %s" CR),p->isWhtL);
    Log.trace(F("isBlkL %s" CR),p->isBlkL);
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
      {"sensor", "MiFlora-lux", mac, "illuminance", "{{ value_json.lux | is_defined }}", "", "", "lu"},
      {"sensor", "MiFlora-tem", mac, "temperature", "{{ value_json.tem | is_defined }}", "", "", "°C"},
      {"sensor", "MiFlora-fer", mac, "", "{{ value_json.fer | is_defined }}", "", "", "µS/cm"},
      {"sensor", "MiFlora-moi", mac, "", "{{ value_json.moi | is_defined }}", "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < MiFloraparametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),MiFlorasensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + MiFlorasensor[i][1];
    createDiscovery(MiFlorasensor[i][0],
                    (char *)discovery_topic.c_str(), MiFlorasensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiFlorasensor[i][3], MiFlorasensor[i][4],
                    MiFlorasensor[i][5], MiFlorasensor[i][6], MiFlorasensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void VegTrugDiscovery(char *mac)
{
#define VegTrugparametersCount 4
  Log.trace(F("VegTrugDiscovery" CR));
  char *VegTrugsensor[VegTrugparametersCount][8] = {
      {"sensor", "VegTrug-lux", mac, "illuminance", "{{ value_json.lux | is_defined }}", "", "", "lu"},
      {"sensor", "VegTrug-tem", mac, "temperature", "{{ value_json.tem | is_defined }}", "", "", "°C"},
      {"sensor", "VegTrug-fer", mac, "", "{{ value_json.fer | is_defined }}", "", "", "µS/cm"},
      {"sensor", "VegTrug-moi", mac, "", "{{ value_json.moi | is_defined }}", "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < VegTrugparametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),VegTrugsensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + VegTrugsensor[i][1];
    createDiscovery(VegTrugsensor[i][0],
                    (char *)discovery_topic.c_str(), VegTrugsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, VegTrugsensor[i][3], VegTrugsensor[i][4],
                    VegTrugsensor[i][5], VegTrugsensor[i][6], VegTrugsensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void MiJiaDiscovery(char *mac)
{
#define MiJiaparametersCount 3
  Log.trace(F("MiJiaDiscovery" CR));
  char *MiJiasensor[MiJiaparametersCount][8] = {
      {"sensor", "MiJia-batt", mac, "battery", "{{ value_json.batt | is_defined }}", "", "", "%"},
      {"sensor", "MiJia-tem", mac, "temperature", "{{ value_json.tem | is_defined }}", "", "", "°C"},
      {"sensor", "MiJia-hum", mac, "humidity", "{{ value_json.hum | is_defined }}", "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < MiJiaparametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),MiJiasensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + MiJiasensor[i][1];
    createDiscovery(MiJiasensor[i][0],
                    (char *)discovery_topic.c_str(), MiJiasensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiJiasensor[i][3], MiJiasensor[i][4],
                    MiJiasensor[i][5], MiJiasensor[i][6], MiJiasensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void LYWSD02Discovery(char *mac)
{
#define LYWSD02parametersCount 3
  Log.trace(F("LYWSD02Discovery" CR));
  char *LYWSD02sensor[LYWSD02parametersCount][8] = {
      {"sensor", "LYWSD02-batt", mac, "battery", "{{ value_json.batt | is_defined }}", "", "", "V"},
      {"sensor", "LYWSD02-tem", mac, "temperature", "{{ value_json.tem | is_defined }}", "", "", "°C"},
      {"sensor", "LYWSD02-hum", mac, "humidity", "{{ value_json.hum | is_defined }}", "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < LYWSD02parametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),LYWSD02sensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + LYWSD02sensor[i][1];
    createDiscovery(LYWSD02sensor[i][0],
                    (char *)discovery_topic.c_str(), LYWSD02sensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, LYWSD02sensor[i][3], LYWSD02sensor[i][4],
                    LYWSD02sensor[i][5], LYWSD02sensor[i][6], LYWSD02sensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void CLEARGRASSTRHDiscovery(char *mac)
{
#define CLEARGRASSTRHparametersCount 3
  Log.trace(F("CLEARGRASSTRHDiscovery" CR));
  char *CLEARGRASSTRHsensor[CLEARGRASSTRHparametersCount][8] = {
      {"sensor", "CLEARGRASSTRH-batt", mac, "battery", "{{ value_json.batt | is_defined }}", "", "", "V"},
      {"sensor", "CLEARGRASSTRH-tem", mac, "temperature", "{{ value_json.tem | is_defined }}", "", "", "°C"},
      {"sensor", "CLEARGRASSTRH-hum", mac, "humidity", "{{ value_json.hum | is_defined }}", "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < CLEARGRASSTRHparametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),CLEARGRASSTRHsensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + CLEARGRASSTRHsensor[i][1];
    createDiscovery(CLEARGRASSTRHsensor[i][0],
                    (char *)discovery_topic.c_str(), CLEARGRASSTRHsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, CLEARGRASSTRHsensor[i][3], CLEARGRASSTRHsensor[i][4],
                    CLEARGRASSTRHsensor[i][5], CLEARGRASSTRHsensor[i][6], CLEARGRASSTRHsensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void CLEARGRASSCGD1Discovery(char *mac)
{
#define CLEARGRASSCGD1parametersCount 3
  Log.trace(F("CLEARGRASSCGD1Discovery" CR));
  char *CLEARGRASSCGD1sensor[CLEARGRASSCGD1parametersCount][8] = {
      {"sensor", "CLEARGRASSCGD1-batt", mac, "battery", "{{ value_json.batt | is_defined }}", "", "", "V"},
      {"sensor", "CLEARGRASSCGD1-tem", mac, "temperature", "{{ value_json.tem | is_defined }}", "", "", "°C"},
      {"sensor", "CLEARGRASSCGD1-hum", mac, "humidity", "{{ value_json.hum | is_defined }}", "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < CLEARGRASSCGD1parametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),CLEARGRASSCGD1sensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + CLEARGRASSCGD1sensor[i][1];
    createDiscovery(CLEARGRASSCGD1sensor[i][0],
                    (char *)discovery_topic.c_str(), CLEARGRASSCGD1sensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, CLEARGRASSCGD1sensor[i][3], CLEARGRASSCGD1sensor[i][4],
                    CLEARGRASSCGD1sensor[i][5], CLEARGRASSCGD1sensor[i][6], CLEARGRASSCGD1sensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void CLEARGRASSTRHKPADiscovery(char *mac)
{
#define CLEARGRASSTRHKPAparametersCount 3
  Log.trace(F("CLEARGRASSTRHKPADiscovery" CR));
  char *CLEARGRASSTRHKPAsensor[CLEARGRASSTRHKPAparametersCount][8] = {
      {"sensor", "CLEARGRASSTRHKPA-pres", mac, "pressure", "{{ value_json.pres | is_defined }}", "", "", "kPa"},
      {"sensor", "CLEARGRASSTRHKPA-tem", mac, "temperature", "{{ value_json.tem | is_defined }}", "", "", "°C"},
      {"sensor", "CLEARGRASSTRHKPA-hum", mac, "humidity", "{{ value_json.hum | is_defined }}", "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < CLEARGRASSTRHKPAparametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),CLEARGRASSTRHKPAsensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + CLEARGRASSTRHKPAsensor[i][1];
    createDiscovery(CLEARGRASSTRHKPAsensor[i][0],
                    (char *)discovery_topic.c_str(), CLEARGRASSTRHKPAsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, CLEARGRASSTRHKPAsensor[i][3], CLEARGRASSTRHKPAsensor[i][4],
                    CLEARGRASSTRHKPAsensor[i][5], CLEARGRASSTRHKPAsensor[i][6], CLEARGRASSTRHKPAsensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void MiScaleDiscovery(char *mac)
{
#define MiScaleparametersCount 1
  Log.trace(F("MiScaleDiscovery" CR));
  char *MiScalesensor[MiScaleparametersCount][8] = {
      {"sensor", "MiScale-weight", mac, "weight", "{{ value_json.weight | is_defined }}", "", "", "kg"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < MiScaleparametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),MiScalesensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + MiScalesensor[i][1];
    createDiscovery(MiScalesensor[i][0],
                    (char *)discovery_topic.c_str(), MiScalesensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiScalesensor[i][3], MiScalesensor[i][4],
                    MiScalesensor[i][5], MiScalesensor[i][6], MiScalesensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void MiLampDiscovery(char *mac)
{
#define MiLampparametersCount 1
  Log.trace(F("MiLampDiscovery" CR));
  char *MiLampsensor[MiLampparametersCount][8] = {
      {"sensor", "MiLamp-presence", mac, "presence", "{{ value_json.presence}}", "", "", "d"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < MiLampparametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),MiLampsensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + MiLampsensor[i][1];
    createDiscovery(MiLampsensor[i][0],
                    (char *)discovery_topic.c_str(), MiLampsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiLampsensor[i][3], MiLampsensor[i][4],
                    MiLampsensor[i][5], MiLampsensor[i][6], MiLampsensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void MiBandDiscovery(char *mac)
{
#define MiBandparametersCount 1
  Log.trace(F("MiBandDiscovery" CR));
  char *MiBandsensor[MiBandparametersCount][8] = {
      {"sensor", "MiBand-steps", mac, "steps", "{{ value_json.steps | is_defined }}", "", "", "nb"},
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < MiBandparametersCount; i++)
  {
    Log.trace(F("CreateDiscoverySensor %s" CR),MiBandsensor[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + MiBandsensor[i][1];
    createDiscovery(MiBandsensor[i][0],
                    (char *)discovery_topic.c_str(), MiBandsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiBandsensor[i][3], MiBandsensor[i][4],
                    MiBandsensor[i][5], MiBandsensor[i][6], MiBandsensor[i][7],
                    0, "", "", false, "");
  }
  BLEdevice device;
  strcpy(device.macAdr, mac);
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

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
    BLEdata.set("id", (char *)mac_adress.c_str());
    mac_adress.replace(":", "");
    mac_adress.toUpperCase();
    String mactopic = subjectBTtoMQTT + String("/") + mac_adress;
    char mac[mac_adress.length() + 1];
    mac_adress.toCharArray(mac, mac_adress.length() + 1);
    Log.notice(F("Device detected: %s" CR),mac);
    if ((!oneWhite() || isWhite(mac)) && !isBlack(mac))
    { //if not black listed mac we go AND if we have no white mac or this mac is  white we go out
      if (advertisedDevice.haveName())
        BLEdata.set("name", (char *)advertisedDevice.getName().c_str());
      if (advertisedDevice.haveManufacturerData())
        BLEdata.set("manufacturerdata", (char *)advertisedDevice.getManufacturerData().c_str());
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
        Log.trace(F("Get services data number: %d" CR),serviceDataCount);
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
          Log.notice(F("Service data: %s" CR),service_data);
          #ifdef pubBLEServiceData
          BLEdata.set("servicedata", service_data);
          #endif
          BLEdata.set("servicedatauuid", (char *)advertisedDevice.getServiceDataUUID(j).toString().c_str());
          if (abs((int)BLEdata["rssi"] | 0) < abs(Minrssi))
          { // publish only the devices close enough
            pub((char *)mactopic.c_str(), BLEdata);
            if (strstr(BLEdata["servicedatauuid"].as<char *>(), "fe95") != NULL)
            { //Mi FLora, Mi jia, Cleargrass Method 1, LYWSD02, VegTrug
              Log.trace(F("Processing BLE device data" CR));
              int pos = -1;
              pos = strpos(service_data, "209800");
              if (pos != -1)
              {
                Log.trace(F("mi flora data reading" CR));
                //example "servicedata":"71209800bc63b6658d7cc40d0910023200"
                #ifdef ZmqttDiscovery
                if (!isDiscovered(mac))
                  MiFloraDiscovery(mac);
                #endif
                process_sensors(pos - 24, service_data, mac);
              }
              pos = -1;
              pos = strpos(service_data, "20bc03");
              if (pos != -1)
              {
                Log.trace(F("vegtrug data reading" CR));
                //example "servicedata":"7120bc0399c309688d7cc40d0910020000"
                #ifdef ZmqttDiscovery
                if (!isDiscovered(mac))
                  VegTrugDiscovery(mac);
                #endif
                process_sensors(pos - 24, service_data, mac);
              }
              pos = -1;
              pos = strpos(service_data, "20aa01");
              if (pos != -1)
              {
                Log.trace(F("mi jia data reading" CR));
                #ifdef ZmqttDiscovery
                if (!isDiscovered(mac))
                  MiJiaDiscovery(mac);
                #endif
                process_sensors(pos - 26, service_data, mac);
              }
              pos = -1;
              pos = strpos(service_data, "205b04");
              if (pos != -1)
              {
                Log.trace(F("LYWSD02 data reading" CR));
                //example "servicedata":"70205b04b96ab883c8593f09041002e000"
                #ifdef ZmqttDiscovery
                if (!isDiscovered(mac))
                  LYWSD02Discovery(mac);
                #endif
                process_sensors(pos - 24, service_data, mac);
              }
              pos = -1;
              pos = strpos(service_data, "304703");
              if (pos != -1)
              {
                Log.trace(F("ClearGrass T RH data reading method 1" CR));
                //example "servicedata":"5030470340743e10342d58041002d6000a100164"
                #ifdef ZmqttDiscovery
                if (!isDiscovered(mac))
                  CLEARGRASSTRHDiscovery(mac);
                #endif
                process_sensors(pos - 26, service_data, mac);
              }
              pos = -1;
              pos = strpos(service_data, "4030dd");
              if (pos != -1)
              {
                Log.trace(F("Mi Lamp data reading" CR));
                //example "servicedata":4030DD031D0300010100
                #ifdef ZmqttDiscovery
                if (!isDiscovered(mac))
                  MiLampDiscovery(mac);
                #endif
                process_milamp(service_data, mac);
              }
            }
            if (strstr(BLEdata["servicedatauuid"].as<char *>(), "181d") != NULL)
            { // Mi Scale V1
              Log.trace(F("Mi Scale V1 data reading" CR));
              //example "servicedata":"a2ac2be307060207122b" /"a28039e3070602070e28"
              #ifdef ZmqttDiscovery
              if (!isDiscovered(mac))
                MiScaleDiscovery(mac);
              #endif
              process_scale_v1(service_data, mac);
            }
            if (strstr(BLEdata["servicedatauuid"].as<char *>(), "181b") != NULL)
            { // Mi Scale V2
              Log.trace(F("Mi Scale V2 data reading" CR));
              //example "servicedata":02c4e1070b1e13050c00002607 / 02a6e20705150a251df401443e /02a6e20705180c0d04d701943e
              #ifdef ZmqttDiscovery
              if (!isDiscovered(mac))
                MiScaleDiscovery(mac);
              #endif
              process_scale_v2(service_data, mac);
            }
            if (strstr(BLEdata["servicedatauuid"].as<char *>(), "fee0") != NULL)
            { // Mi Band //0000fee0-0000-1000-8000-00805f9b34fb // ESP32 only
              Log.trace(F("Mi Band data reading" CR));
              //example "servicedata":a21e0000
              #ifdef ZmqttDiscovery
              if (!isDiscovered(mac))
                MiBandDiscovery(mac);
              #endif
              process_miband(service_data, mac);
            }
            if (strstr(BLEdata["servicedata"].as<char *>(), "08094c") != NULL)
            { // Clear grass with air pressure//08094c0140342d580104d8000c020702612702015a
              Log.trace(F("Clear grass data with air pressure reading" CR));
              //example "servicedata":08094c0140342d580104 c400 2402 0702 5d27 02015a
              #ifdef ZmqttDiscovery
              if (!isDiscovered(mac))
                CLEARGRASSTRHKPADiscovery(mac);
              #endif
              process_cleargrass_air(service_data, mac);
            }
            if (strstr(BLEdata["servicedata"].as<char *>(), "080774") != NULL)
            { // Clear grass standard method 2/0807743e10342d580104c3002c0202012a
              Log.trace(F("Clear grass data reading method 2" CR));
              //example "servicedata":0807743e10342d580104 c300 2c02 02012a
              // no discovery as it is already available with method 1
              process_cleargrass(service_data, mac);
            }
            if (strstr(BLEdata["servicedata"].as<char *>(), "080caf") != NULL)
            { // Clear grass CGD1 080caffd50342d580104c900a102
              Log.trace(F("Clear grass CGD1 data reading" CR));
              //example "servicedata":080caffd50342d580104 c900 a102
              #ifdef ZmqttDiscovery
              if (!isDiscovered(mac))
                CLEARGRASSCGD1Discovery(mac);
              #endif
              process_cleargrass(service_data, mac);
            }
          }
          else
          {
            Log.trace(F("Low rssi, device filtered" CR));
          }
        }
      }
      else
      {
        if (abs((int)BLEdata["rssi"] | 0) < abs(Minrssi))
        {                                         // publish only the devices close enough
          pub((char *)mactopic.c_str(), BLEdata); // publish device even if there is no service data
        }
        else
        {
          Log.trace(F("Low rssi, device filtered" CR));
        }
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
    Log.trace(F('BT Task running on core: %d'),xPortGetCoreID());
    delay(BLEinterval);
    if (client.state() == 0)
    {
      BLEscan();
    }
    else
    {
      Log.warning(F("MQTT client disconnected no BLE scan" CR));
      delay(1000);
    }
  }
}

void setupBT()
{
  BLEinterval = TimeBtw_Read;
  Minrssi = MinimumRSSI;
  Log.notice(F("BLEinterval: %d" CR),BLEinterval);
  Log.notice(F("Minrssi: %d" CR),Minrssi);
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

#define STRING_MSG "OK+DISC:"
#define QUESTION_MSG "AT+DISA?"
#define RESPONSE_MSG "OK+DISIS"
#define RESP_END_MSG "OK+DISCE"
#define SETUP_MSG "OK+RESET"

SoftwareSerial softserial(BT_RX, BT_TX);

String returnedString = "";
unsigned long timebt = 0;

// this struct define which parts of the hexadecimal chain we extract and what to do with these parts
struct decompose d[6] = {{"mac", 16, 12, true}, {"typ", 28, 2, false}, {"rsi", 30, 2, false}, {"rdl", 32, 2, false}, {"sty", 44, 4, true}, {"rda", 34, 60, false}};

void setupBT()
{
  BLEinterval = TimeBtw_Read;
  Minrssi = MinimumRSSI;
  Log.notice(F("BLEinterval: %d" CR),BLEinterval);
  Log.notice(F("Minrssi: %d" CR),Minrssi);
  softserial.begin(9600);
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
  { //retriving data
    timebt = millis();
    #if defined(ESP8266)
    yield();
    #endif
    if (returnedString != "")
    {
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
        Log.trace(F("Token: %s" CR),token_char);
        if (token.length() > 60)
        { // we extract data only if we have detailled infos
          for (int i = 0; i < 6; i++)
          {
            extract_char(token_char, d[i].extract, d[i].start, d[i].len, d[i].reverse, false);
            if (i == 3)
              d[5].len = (int)strtol(d[i].extract, NULL, 16) * 2; // extracting the length of the rest data
          }

          if ((strlen(d[0].extract)) == 12) // if a mac adress is detected we publish it
          {
            Log.trace(F("Creating BLE buffer" CR));
            StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
            JsonObject &BLEdata = jsonBuffer.createObject();
            strupp(d[0].extract);
            if (isBlack(d[0].extract))
              return false; //if black listed mac we go out
            if (oneWhite() && !isWhite(d[0].extract))
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
            String topic = subjectBTtoMQTT + String("/") + String(d[0].extract);
            int rssi = (int)strtol(d[2].extract, NULL, 16) - 256;
            BLEdata.set("rssi", (int)rssi);
            #ifdef subjectHomePresence
            haRoomPresence(BLEdata); // this device has an rssi in consequence we can use it for home assistant room presence component
            #endif
            String service_data(d[5].extract);
            Log.notice(F("Service data: %s" CR),d[5].extract);
            service_data = service_data.substring(14);
            #ifdef pubBLEServiceData
            BLEdata.set("servicedata", (char *)service_data.c_str());
            #endif
            if (abs((int)BLEdata["rssi"] | 0) < abs(Minrssi))
            { // publish only the devices close enough
              pub((char *)topic.c_str(), BLEdata);
              int pos = -1;
              pos = strpos(d[5].extract, "209800");
              if (pos != -1)
              {
                Log.trace(F("mi flora data reading" CR));
                #ifdef ZmqttDiscovery
                if (!isDiscovered(d[0].extract))
                  MiFloraDiscovery(d[0].extract);
                #endif
                bool result = process_sensors(pos - 38, (char *)service_data.c_str(), d[0].extract);
              }
              pos = -1;
              pos = strpos(d[5].extract, "20aa01");
              //example "servicedata":"5020aa0194dfaa33342d580d1004e3002c02"
              if (pos != -1)
              {
                Log.trace(F("mi jia data reading" CR));
                #ifdef ZmqttDiscovery
                if (!isDiscovered(d[0].extract))
                  MiJiaDiscovery(d[0].extract);
                #endif
                bool result = process_sensors(pos - 40, (char *)service_data.c_str(), d[0].extract);
              }
              pos = -1;
              pos = strpos(d[5].extract, "205b04");
              //example "servicedata":"141695fe70205b0461298882c8593f09061002d002"
              if (pos != -1)
              {
                Log.trace(F("LYWSD02 data reading" CR));
                #ifdef ZmqttDiscovery
                if (!isDiscovered(d[0].extract))
                  LYWSD02Discovery(d[0].extract);
                #endif
                bool result = process_sensors(pos - 38, (char *)service_data.c_str(), d[0].extract);
              }
              pos = -1;
              pos = strpos(d[5].extract, "304703");
              if (pos != -1)
              {
                Log.trace(F("CLEARGRASSTRH data reading method 1" CR));
                #ifdef ZmqttDiscovery
                if (!isDiscovered(d[0].extract))
                  CLEARGRASSTRHDiscovery(d[0].extract);
                #endif
                bool result = process_sensors(pos - 40, (char *)service_data.c_str(), d[0].extract);
              }
              pos = -1;
              pos = strpos(d[5].extract, "e30706");
              if (pos != -1)
              {
                Log.trace(F("Mi Scale data reading" CR));
                //example "servicedata":"a2ac2be307060207122b" /"a28039e3070602070e28"
                #ifdef ZmqttDiscovery
                if (!isDiscovered(d[0].extract))
                  MiScaleDiscovery(d[0].extract);
                #endif
                bool result = process_scale_v1((char *)service_data.c_str(), d[0].extract);
              }
              pos = -1;
              pos = strpos(d[5].extract, "4030dd");
              if (pos != -1)
              {
                Log.trace(F("Mi Lamp data reading" CR));
                //example "servicedata":4030dd31d0300010100
                #ifdef ZmqttDiscovery
                if (!isDiscovered(d[0].extract))
                  MiLampDiscovery(d[0].extract);
                #endif
                process_milamp((char *)service_data.c_str(), d[0].extract);
              }
              pos = -1;
              pos = strpos(d[5].extract, "08094c"); // Clear grass with air pressure//08094c0140342d580104d8000c020702612702015a
              if (pos != -1)
              {
                Log.trace(F("Clear grass data with air pressure reading" CR));
                //example "servicedata":08094c0140342d580104 c400 2402 0702 5d27 02015a
                #ifdef ZmqttDiscovery
                if (!isDiscovered(d[0].extract))
                  CLEARGRASSTRHKPADiscovery(d[0].extract);
                #endif
                process_cleargrass_air((char *)service_data.c_str(), d[0].extract);
              }
              pos = -1;
              pos = strpos(d[5].extract, "080774"); // Clear grass standard method 2/0807743e10342d580104c3002c0202012a
              if (pos != -1)
              {
                Log.trace(F("Clear grass data reading method 2" CR));
                //example "servicedata":0807743e10342d580104 c300 2c02 02012a
                // no discovery as it is already available with method 1
                process_cleargrass((char *)service_data.c_str(), d[0].extract);
              }
              pos = strpos(d[5].extract, "080caf"); // Clear grass CGD1 08094c0140342d580104d8000c020702612702015a
              if (pos != -1)
              {
                Log.trace(F("Clear grass data CGD1" CR));
                //example "servicedata":080caffd50342d580104 c900 a102
                #ifdef ZmqttDiscovery
                if (!isDiscovered(d[0].extract))
                  CLEARGRASSCGD1Discovery(d[0].extract);
                #endif
                process_cleargrass((char *)service_data.c_str(), d[0].extract);
              }

              return true;
            }
            else
            {
              Log.trace(F("Low rssi, device filtered" CR));
            }
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

double value_from_service_data(char *service_data, int offset, int data_length)
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

bool process_sensors(int offset, char *rest_data, char *mac_adress)
{

  Log.trace(F("Creating BLE buffer" CR));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &BLEdata = jsonBuffer.createObject();
  int data_length = 0;
  
  switch (rest_data[51 + offset])
  {
  case '1':
  case '2':
  case '3':
  case '4':
    data_length = ((rest_data[51 + offset] - '0') * 2);
    Log.trace(F("Valid data_length: %d" CR), data_length);
    break;
  default:
    Log.trace(F("Invalid data_length" CR));
    return false;
  }

  double value = 9999;
  value = value_from_service_data(rest_data, 52 + offset, data_length);

  // Mi flora provides tem(perature), (earth) moi(sture), fer(tility) and lux (illuminance)
  // Mi Jia provides tem(perature), batt(erry) and hum(idity)
  // following the value of digit 47 we determine the type of data we get from the sensor
  switch (rest_data[47 + offset])
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
    value = value_from_service_data(rest_data, 52 + offset, 4);
    BLEdata.set("tem", (double)value / 10);
    // temperature
    value = value_from_service_data(rest_data, 56 + offset, 4);
    BLEdata.set("hum", (double)value / 10);
    break;
  default:
    Log.trace(F("can't read values" CR));
    return false;
  }
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + String("/") + mactopic;
  pub((char *)mactopic.c_str(), BLEdata);
  return true;
}

bool process_scale_v1(char *rest_data, char *mac_adress)
{
  //example "servicedata":"a2ac2be307060207122b" /"a28039e3070602070e28"
  Log.trace(F("Creating BLE buffer" CR));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &BLEdata = jsonBuffer.createObject();

  double weight = value_from_service_data(rest_data, 2, 4) / 200;

  //Set Json value
  BLEdata.set("weight", (double)weight);

  // Publish weight
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + String("/") + mactopic;
  pub((char *)mactopic.c_str(), BLEdata);
  return true;
}

bool process_scale_v2(char *rest_data, char *mac_adress)
{
  //example "servicedata":"a2ac2be307060207122b" /"a28039e3070602070e28"
  Log.trace(F("Creating BLE buffer" CR));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &BLEdata = jsonBuffer.createObject();

  double weight = value_from_service_data(rest_data, 22, 4) / 200;
  double impedance = value_from_service_data(rest_data, 18, 4);

  //Set Json values
  BLEdata.set("weight", (double)weight);
  BLEdata.set("impedance", (double)impedance);

  // Publish weight
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + String("/") + mactopic;
  pub((char *)mactopic.c_str(), BLEdata);
  return true;
}

bool process_miband(char *rest_data, char *mac_adress)
{
  //example "servicedata":"a21e0000"
  Log.trace(F("Creating BLE buffer" CR));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &BLEdata = jsonBuffer.createObject();

  double steps = value_from_service_data(rest_data, 0, 4);

  //Set Json value
  BLEdata.set("steps", (double)steps);

  // Publish weight
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + String("/") + mactopic;
  pub((char *)mactopic.c_str(), BLEdata);
  return true;
}

bool process_milamp(char *rest_data, char *mac_adress)
{
  //example "servicedata":"4030dd31d0300010100"
  Log.trace(F("Creating BLE buffer" CR));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &BLEdata = jsonBuffer.createObject();

  long darkness = value_from_service_data(rest_data, 8, 2);

  //Set Json value
  BLEdata.set("presence", (bool)"true");
  BLEdata.set("darkness", (long)darkness);

  // Publish
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + String("/") + mactopic;
  pub((char *)mactopic.c_str(), BLEdata);
  return true;
}

bool process_cleargrass_air(char *rest_data, char *mac_adress)
{
  //example "servicedata":"08094c0140342d580104c400240207025d2702015a"
  //decoding "08094c0140342d580104 temp:c400 hum:2402 0702 pressure:5d27 02015a"
  Log.trace(F("Creating BLE buffer" CR));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &BLEdata = jsonBuffer.createObject();

  double value = 9999;
  // humidity
  value = value_from_service_data(rest_data, 20, 4);
  BLEdata.set("tem", (double)value / 10);
  // temperature
  value = value_from_service_data(rest_data, 24, 4);
  BLEdata.set("hum", (double)value / 10);
  // air pressure
  value = value_from_service_data(rest_data, 32, 4);
  BLEdata.set("pres", (double)value / 100);

  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + String("/") + mactopic;
  pub((char *)mactopic.c_str(), BLEdata);
  return true;
}

bool process_cleargrass(char *rest_data, char *mac_adress)
{
  //example "servicedata":0807743e10342d580104 c300 2c02 02012a
  Log.trace(F("Creating BLE buffer" CR));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &BLEdata = jsonBuffer.createObject();

  double value = 9999;
  // humidity
  value = value_from_service_data(rest_data, 20, 4);
  BLEdata.set("tem", (double)value / 10);
  // temperature
  value = value_from_service_data(rest_data, 24, 4);
  BLEdata.set("hum", (double)value / 10);

  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + String("/") + mactopic;
  pub((char *)mactopic.c_str(), BLEdata);
  return true;
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
  }
}

#endif
