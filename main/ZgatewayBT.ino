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

void setWorBMac(char * mac , bool isWhite){
  bool foundMac = false;
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
    if((strcmp(p->macAdr,mac) == 0)){
        p->isWhtL = isWhite;
        p->isBlkL = !isWhite;
        foundMac = true;
    }
  }
  if (!foundMac) {
    BLEdevice device;
    strcpy( device.macAdr,mac );
    device.isDisc = false;
    device.isWhtL = isWhite;
    device.isBlkL = !isWhite;
    devices.push_back(device);
  }
}

bool oneWhite(){
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
        if(p->isWhtL) return true;
  }
  return false;
}

bool isWhite(char * mac){
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
    if((strcmp(p->macAdr,mac) == 0)){
        return p->isWhtL;
    }
  }
  return false;
}

bool isBlack(char * mac){
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
    if((strcmp(p->macAdr,mac) == 0)){
        return p->isBlkL;
    }
  }
  return false;
}

bool isDiscovered(char * mac){
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
    if((strcmp(p->macAdr,mac) == 0)){
        return p->isDisc;
    }
  }
  return false;
}

void dumpDevices(){
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
    trc(p->macAdr);
    trc(p->isDisc);
    trc(p->isWhtL);
    trc(p->isBlkL);
  }
}

void strupp(char* beg)
{
    while (*beg = toupper(*beg))
        ++beg;
}

#ifdef ZmqttDiscovery
void MiFloraDiscovery(char * mac){
  #define MiFloraparametersCount 4
  trc(F("MiFloraDiscovery"));
  char * MiFlorasensor[MiFloraparametersCount][8] = {
     {"sensor", "MiFlora-lux", mac, "illuminance","{{ value_json.lux | is_defined }}","", "", "lu"} ,
     {"sensor", "MiFlora-tem", mac,"temperature","{{ value_json.tem | is_defined }}","", "", "°C"} ,
     {"sensor", "MiFlora-fer", mac,"","{{ value_json.fer | is_defined }}","", "", "µS/cm"} ,
     {"sensor", "MiFlora-moi", mac,"","{{ value_json.moi | is_defined }}","", "", "%"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<MiFloraparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(MiFlorasensor[i][1]);
   String discovery_topic = String(subjectBTtoMQTT) + String(mac);
   String unique_id = String(mac) + "-" + MiFlorasensor[i][1];
   createDiscovery(MiFlorasensor[i][0],
                    (char *)discovery_topic.c_str(), MiFlorasensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiFlorasensor[i][3], MiFlorasensor[i][4],
                    MiFlorasensor[i][5], MiFlorasensor[i][6], MiFlorasensor[i][7],
                    0,"","",true,"");
  }
  BLEdevice device;
  strcpy( device.macAdr, mac );
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void MiJiaDiscovery(char * mac){
  #define MiJiaparametersCount 3
  trc(F("MiJiaDiscovery"));
  char * MiJiasensor[MiJiaparametersCount][8] = {
     {"sensor", "MiJia-batt", mac, "battery","{{ value_json.batt | is_defined }}","", "", "%"} ,
     {"sensor", "MiJia-tem", mac,"temperature","{{ value_json.tem | is_defined }}","", "", "°C"} ,
     {"sensor", "MiJia-hum", mac,"humidity","{{ value_json.hum | is_defined }}","", "", "%"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<MiJiaparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(MiJiasensor[i][1]);
   String discovery_topic = String(subjectBTtoMQTT) + String(mac);
   String unique_id = String(mac) + "-" + MiJiasensor[i][1];
   createDiscovery(MiJiasensor[i][0],
                    (char *)discovery_topic.c_str(), MiJiasensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiJiasensor[i][3], MiJiasensor[i][4],
                    MiJiasensor[i][5], MiJiasensor[i][6], MiJiasensor[i][7],
                    0,"","",true,"");
  }
  BLEdevice device;
  strcpy( device.macAdr, mac );
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void LYWSD02Discovery(char * mac){
  #define LYWSD02parametersCount 3
  trc(F("LYWSD02Discovery"));
  char * LYWSD02sensor[LYWSD02parametersCount][8] = {
     {"sensor", "LYWSD02-batt", mac, "battery","{{ value_json.batt | is_defined }}","", "", "V"} ,
     {"sensor", "LYWSD02-tem", mac,"temperature","{{ value_json.tem | is_defined }}","", "", "°C"} ,
     {"sensor", "LYWSD02-hum", mac,"humidity","{{ value_json.hum | is_defined }}","", "", "%"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<LYWSD02parametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(LYWSD02sensor[i][1]);
   String discovery_topic = String(subjectBTtoMQTT) + String(mac);
   String unique_id = String(mac) + "-" + LYWSD02sensor[i][1];
   createDiscovery(LYWSD02sensor[i][0],
                    (char *)discovery_topic.c_str(), LYWSD02sensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, LYWSD02sensor[i][3], LYWSD02sensor[i][4],
                    LYWSD02sensor[i][5], LYWSD02sensor[i][6], LYWSD02sensor[i][7],
                    0,"","",true,"");
  }
  BLEdevice device;
  strcpy( device.macAdr, mac );
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void CLEARGRASSTRHDiscovery(char * mac){
  #define CLEARGRASSTRHparametersCount 3
  trc(F("CLEARGRASSTRHDiscovery"));
  char * CLEARGRASSTRHsensor[CLEARGRASSTRHparametersCount][8] = {
     {"sensor", "CLEARGRASSTRH-batt", mac, "battery","{{ value_json.batt | is_defined }}","", "", "V"} ,
     {"sensor", "CLEARGRASSTRH-tem", mac,"temperature","{{ value_json.tem | is_defined }}","", "", "°C"} ,
     {"sensor", "CLEARGRASSTRH-hum", mac,"humidity","{{ value_json.hum | is_defined }}","", "", "%"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<CLEARGRASSTRHparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(CLEARGRASSTRHsensor[i][1]);
   String discovery_topic = String(subjectBTtoMQTT) + String(mac);
   String unique_id = String(mac) + "-" + CLEARGRASSTRHsensor[i][1];
   createDiscovery(CLEARGRASSTRHsensor[i][0],
                    (char *)discovery_topic.c_str(), CLEARGRASSTRHsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, CLEARGRASSTRHsensor[i][3], CLEARGRASSTRHsensor[i][4],
                    CLEARGRASSTRHsensor[i][5], CLEARGRASSTRHsensor[i][6], CLEARGRASSTRHsensor[i][7],
                    0,"","",true,"");
  }
  BLEdevice device;
  strcpy( device.macAdr, mac );
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void CLEARGRASSTRHKPADiscovery(char * mac){
  #define CLEARGRASSTRHKPAparametersCount 3
  trc(F("CLEARGRASSTRHKPADiscovery"));
  char * CLEARGRASSTRHKPAsensor[CLEARGRASSTRHKPAparametersCount][8] = {
     {"sensor", "CLEARGRASSTRHKPA-pres", mac, "pressure","{{ value_json.pres | is_defined }}","", "", "kPa"} ,
     {"sensor", "CLEARGRASSTRHKPA-tem", mac,"temperature","{{ value_json.tem | is_defined }}","", "", "°C"} ,
     {"sensor", "CLEARGRASSTRHKPA-hum", mac,"humidity","{{ value_json.hum | is_defined }}","", "", "%"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<CLEARGRASSTRHKPAparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(CLEARGRASSTRHKPAsensor[i][1]);
   String discovery_topic = String(subjectBTtoMQTT) + String(mac);
   String unique_id = String(mac) + "-" + CLEARGRASSTRHKPAsensor[i][1];
   createDiscovery(CLEARGRASSTRHKPAsensor[i][0],
                    (char *)discovery_topic.c_str(), CLEARGRASSTRHKPAsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, CLEARGRASSTRHKPAsensor[i][3], CLEARGRASSTRHKPAsensor[i][4],
                    CLEARGRASSTRHKPAsensor[i][5], CLEARGRASSTRHKPAsensor[i][6], CLEARGRASSTRHKPAsensor[i][7],
                    0,"","",true,"");
  }
  BLEdevice device;
  strcpy( device.macAdr, mac );
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}


void MiScaleDiscovery(char * mac){
  #define MiScaleparametersCount 1
  trc(F("MiScaleDiscovery"));
  char * MiScalesensor[MiScaleparametersCount][8] = {
     {"sensor", "MiScale-weight", mac, "weight","{{ value_json.weight | is_defined }}","", "", "kg"} ,
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<MiScaleparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(MiScalesensor[i][1]);
   String discovery_topic = String(subjectBTtoMQTT) + String(mac);
   String unique_id = String(mac) + "-" + MiScalesensor[i][1];
   createDiscovery(MiScalesensor[i][0],
                    (char *)discovery_topic.c_str(), MiScalesensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiScalesensor[i][3], MiScalesensor[i][4],
                    MiScalesensor[i][5], MiScalesensor[i][6], MiScalesensor[i][7],
                    0,"","",true,"");
  }
  BLEdevice device;
  strcpy( device.macAdr, mac );
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void MiLampDiscovery(char * mac){
  #define MiLampparametersCount 1
  trc(F("MiLampDiscovery"));
  char * MiLampsensor[MiLampparametersCount][8] = {
     {"sensor", "MiLamp-presence", mac, "presence","{{ value_json.presence}}","", "", "d"} ,
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<MiLampparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(MiLampsensor[i][1]);
   String discovery_topic = String(subjectBTtoMQTT) + String(mac);
   String unique_id = String(mac) + "-" + MiLampsensor[i][1];
   createDiscovery(MiLampsensor[i][0],
                    (char *)discovery_topic.c_str(), MiLampsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiLampsensor[i][3], MiLampsensor[i][4],
                    MiLampsensor[i][5], MiLampsensor[i][6], MiLampsensor[i][7],
                    0,"","",true,"");
  }
  BLEdevice device;
  strcpy( device.macAdr, mac );
  device.isDisc = true;
  device.isWhtL = false;
  device.isBlkL = false;
  devices.push_back(device);
}

void MiBandDiscovery(char * mac){
  #define MiBandparametersCount 1
  trc(F("MiBandDiscovery"));
  char * MiBandsensor[MiBandparametersCount][8] = {
     {"sensor", "MiBand-steps", mac, "steps","{{ value_json.steps | is_defined }}","", "", "nb"} ,
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<MiBandparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(MiBandsensor[i][1]);
   String discovery_topic = String(subjectBTtoMQTT) + String(mac);
   String unique_id = String(mac) + "-" + MiBandsensor[i][1];
   createDiscovery(MiBandsensor[i][0],
                    (char *)discovery_topic.c_str(), MiBandsensor[i][1], (char *)unique_id.c_str(),
                    will_Topic, MiBandsensor[i][3], MiBandsensor[i][4],
                    MiBandsensor[i][5], MiBandsensor[i][6], MiBandsensor[i][7],
                    0,"","",true,"");
  }
  BLEdevice device;
  strcpy( device.macAdr, mac );
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
      
    class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
      void onResult(BLEAdvertisedDevice advertisedDevice) {
        trc(F("Creating BLE buffer"));
        StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
        JsonObject& BLEdata = jsonBuffer.createObject();
        String mac_adress = advertisedDevice.getAddress().toString().c_str();
        BLEdata.set("id", (char *)mac_adress.c_str());
        mac_adress.replace(":","");
        mac_adress.toUpperCase();
        String mactopic = subjectBTtoMQTT + mac_adress;
        char mac[mac_adress.length()+1];
        mac_adress.toCharArray(mac,mac_adress.length()+1);
        trc("device detected");
        trc(mac);
        if((!oneWhite() || isWhite(mac)) && !isBlack(mac)){ //if not black listed mac we go AND if we have no white mac or this mac is  white we go out
          if (advertisedDevice.haveName())              BLEdata.set("name", (char *)advertisedDevice.getName().c_str());
          if (advertisedDevice.haveManufacturerData())  BLEdata.set("manufacturerdata", (char *)advertisedDevice.getManufacturerData().c_str());
          if (advertisedDevice.haveRSSI())              BLEdata.set("rssi", (int) advertisedDevice.getRSSI());
          if (advertisedDevice.haveTXPower())           BLEdata.set("txpower", (int8_t) advertisedDevice.getTXPower());
          #ifdef subjectHomePresence
            if (advertisedDevice.haveRSSI()) haRoomPresence(BLEdata);// this device has an rssi in consequence we can use it for home assistant room presence component
          #endif
          if (advertisedDevice.haveServiceData()){
              trc(F("Get services data :"));
              int serviceDataCount = advertisedDevice.getServiceDataCount();
              trc(serviceDataCount);
              for (int j = 0; j < serviceDataCount;j++){
                std::string serviceData = advertisedDevice.getServiceData(j);               
                int serviceDataLength = serviceData.length();
                String returnedString = "";
                for (int i=0; i<serviceDataLength; i++)
                {
                  int a = serviceData[i];
                  if (a < 16) {
                    returnedString = returnedString + "0";
                  } 
                  returnedString = returnedString + String(a,HEX);  
                }
                char service_data[returnedString.length()+1];
                returnedString.toCharArray(service_data,returnedString.length()+1);
                service_data[returnedString.length()] = '\0';
                #ifdef pubBLEServiceData
                  BLEdata.set("servicedata", service_data);  
                #endif
                BLEdata.set("servicedatauuid", (char *)advertisedDevice.getServiceDataUUID(j).toString().c_str());
                if(abs((int)BLEdata["rssi"]|0) < abs(Minrssi)) { // publish only the devices close enough
                  pub((char *)mactopic.c_str(),BLEdata);
                  if (strstr(BLEdata["servicedatauuid"].as<char*>(),"fe95") != NULL ){ //Mi FLora, Mi jia, Cleargrass Method 1, LYWDS02
                    trc("Processing BLE device data");
                    int pos = -1;
                    pos = strpos(service_data,"209800");
                    if (pos != -1){
                      trc(F("mi flora data reading"));
                      //example "servicedata":"71209800bc63b6658d7cc40d0910023200"
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) MiFloraDiscovery(mac);
                      #endif
                      process_sensors(pos - 24,service_data,mac);
                    }
                    pos = -1;
                    pos = strpos(service_data,"20aa01");
                    if (pos != -1){
                      trc(F("mi jia data reading"));
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) MiJiaDiscovery(mac);
                      #endif
                      process_sensors(pos - 26,service_data,mac);
                    }
                    pos = -1;
                    pos = strpos(service_data,"205b04");
                    if (pos != -1){
                      trc(F("LYWSD02 data reading"));
                      //example "servicedata":"70205b04b96ab883c8593f09041002e000"
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) LYWSD02Discovery(mac);
                      #endif
                      process_sensors(pos - 24,service_data,mac);
                    }
                    pos = -1;
                    pos = strpos(service_data,"304703");
                    if (pos != -1){
                      trc(F("ClearGrass T RH data reading method 1"));
                      //example "servicedata":"5030470340743e10342d58041002d6000a100164"
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) CLEARGRASSTRHDiscovery(mac);
                      #endif
                      process_sensors(pos - 26,service_data,mac);
                    }
                    pos = -1;
                    pos = strpos(service_data,"4030dd");
                    if (pos != -1){
                      trc(F("Mi Lamp data reading"));
                      //example "servicedata":4030DD031D0300010100
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) MiLampDiscovery(mac);
                      #endif
                      process_milamp(service_data,mac);
                    }
                  }
                  if (strstr(BLEdata["servicedatauuid"].as<char*>(),"181d") != NULL){ // Mi Scale V1
                      trc(F("Mi Scale V1 data reading"));
                      //example "servicedata":"a2ac2be307060207122b" /"a28039e3070602070e28"
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) MiScaleDiscovery(mac);
                      #endif
                      process_scale_v1(service_data,mac);
                  }
                  if (strstr(BLEdata["servicedatauuid"].as<char*>(),"181b") != NULL){ // Mi Scale V2
                      trc(F("Mi Scale V2 data reading"));
                      //example "servicedata":02c4e1070b1e13050c00002607 / 02a6e20705150a251df401443e /02a6e20705180c0d04d701943e
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) MiScaleDiscovery(mac);
                      #endif
                      process_scale_v2(service_data,mac);
                  }
                  if (strstr(BLEdata["servicedatauuid"].as<char*>(),"fee0") != NULL){ // Mi Band //0000fee0-0000-1000-8000-00805f9b34fb // ESP32 only
                      trc(F("Mi Band data reading"));
                      //example "servicedata":a21e0000
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) MiBandDiscovery(mac);
                      #endif
                      process_miband(service_data,mac);
                  }
                  if (strstr(BLEdata["servicedata"].as<char*>(),"08094c") != NULL){ // Clear grass with air pressure//08094c0140342d580104d8000c020702612702015a
                      trc(F("Clear grass data with air pressure reading"));
                      //example "servicedata":08094c0140342d580104 c400 2402 0702 5d27 02015a
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) CLEARGRASSTRHKPADiscovery(mac);
                      #endif
                      process_cleargrass_air(service_data,mac);
                  }
                  if (strstr(BLEdata["servicedata"].as<char*>(),"080774") != NULL){ // Clear grass standard method 2/0807743e10342d580104c3002c0202012a
                      trc(F("Clear grass data reading method 2"));
                      //example "servicedata":0807743e10342d580104 c300 2c02 02012a
                      // no discovery as it is already available with method 1
                      process_cleargrass(service_data,mac);
                  }
                }else{
                  trc("Low rssi, device filtered");
                }
              }
          }else{
            if(abs((int)BLEdata["rssi"]|0) < abs(Minrssi)) { // publish only the devices close enough
              pub((char *)mactopic.c_str(),BLEdata); // publish device even if there is no service data
            }else{
              trc("Low rssi, device filtered");
            }
          }
        }else{
          trc(F("Filtered mac device"));
        }
      }
    };

    void BLEscan(){
            
      TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
      TIMERG0.wdt_feed=1;
      TIMERG0.wdt_wprotect=0;
      trc(F("Scan begin"));
      BLEDevice::init("");
      BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
      MyAdvertisedDeviceCallbacks myCallbacks;
      pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
      pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
      BLEScanResults foundDevices = pBLEScan->start(Scan_duration);
      trc(F("Scan end, deinit controller"));
      esp_bt_controller_deinit();
    }   

    void coreTask( void * pvParameters ){

    String taskMessage = "BT Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();

      while(true){
          trc(taskMessage);
          delay(BLEinterval);
          BLEscan();
      }
    }
  
    void setupBT(){
      BLEinterval = TimeBtw_Read;
      Minrssi = MinimumRSSI;
      trc(F("BLEinterval btw scans"));
      trc(BLEinterval);
      trc(F("Minrssi"));
      trc(Minrssi);
      // we setup a task with priority one to avoid conflict with other gateways
      xTaskCreatePinnedToCore(
                        coreTask,   /* Function to implement the task */
                        "coreTask", /* Name of the task */
                        10000,      /* Stack size in words */
                        NULL,       /* Task input parameter */
                        1,          /* Priority of the task */
                        NULL,       /* Task handle. */
                        taskCore);  /* Core where the task should run */
        trc(F("ZgatewayBT multicore ESP32 setup done "));
    }   

    bool BTtoMQTT(){ // for on demand BLE scans
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
    struct decompose d[6] = {{"mac",16,12,true},{"typ",28,2,false},{"rsi",30,2,false},{"rdl",32,2,false},{"sty",44,4,true},{"rda",34,60,false}};
    
    void setupBT() {
      BLEinterval = TimeBtw_Read;
      Minrssi = MinimumRSSI;
      trc(F("BLEinterval btw scans"));
      trc(BLEinterval);
      trc(F("Minrssi"));
      trc(Minrssi);
      softserial.begin(9600);
      softserial.print(F("AT+ROLE1"));
      delay(100);
      softserial.print(F("AT+IMME1"));
      delay(100);
      softserial.print(F("AT+RESET"));
      delay(100);
      #ifdef HM_BLUE_LED_STOP
        softserial.print(F("AT+PIO11")); // When not connected (as in BLE mode) the LED is off. When connected the LED is solid on.
      #endif
      delay(100);
      trc(F("ZgatewayBT HM1X setup done "));
    }
    
    bool BTtoMQTT() {
    
      //extract serial data from module in hexa format
      while (softserial.available() > 0) {
          int a = softserial.read();
          if (a < 16) {
            returnedString = returnedString + "0";
          } 
            returnedString = returnedString + String(a,HEX);  
      }
    
      if (millis() > (timebt + BLEinterval)) {//retriving data
          timebt = millis();
          #if defined(ESP8266)
            yield();
          #endif
          if (returnedString != "") {
            size_t pos = 0;
            while ((pos = returnedString.lastIndexOf(BLEdelimiter)) != -1) {
              #if defined(ESP8266)
                yield();
              #endif
              String token = returnedString.substring(pos);
              returnedString.remove(pos,returnedString.length() );
              char token_char[token.length()+1];
              token.toCharArray(token_char, token.length()+1);
              trc(token);
              if ( token.length() > 60){// we extract data only if we have detailled infos
                for(int i =0; i<6;i++)
                {
                  extract_char(token_char,d[i].extract,d[i].start, d[i].len ,d[i].reverse, false);
                  if (i == 3) d[5].len = (int)strtol(d[i].extract, NULL, 16) * 2; // extracting the length of the rest data
                }
      
                if((strlen(d[0].extract)) == 12) // if a mac adress is detected we publish it
                {
                  trc(F("Creating BLE buffer"));
                  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
                  JsonObject& BLEdata = jsonBuffer.createObject();
                  strupp(d[0].extract);
                  if(isBlack(d[0].extract)) return false; //if black listed mac we go out
                  if(oneWhite() && !isWhite(d[0].extract)) return false; //if we have at least one white mac and this mac is not white we go out
                  #ifdef subjectHomePresence
                    String HomePresenceId;
                    for (int i = 0; i<12; i++){
                      HomePresenceId += String(d[0].extract[i]);
                      if(((i-1) % 2 == 0) && (i!=11)) HomePresenceId += ":";
                    }
                    trc(F("HomePresenceId"));      
                    trc(HomePresenceId);
                    BLEdata.set("id", (char *)HomePresenceId.c_str());
                  #endif
                  String topic = subjectBTtoMQTT + String(d[0].extract);
                  int rssi = (int)strtol(d[2].extract, NULL, 16) - 256;
                  BLEdata.set("rssi", (int)rssi);
                  #ifdef subjectHomePresence
                    haRoomPresence(BLEdata);// this device has an rssi in consequence we can use it for home assistant room presence component
                  #endif
                  String service_data(d[5].extract);
                  service_data = service_data.substring(14);
                  #ifdef pubBLEServiceData
                    BLEdata.set("servicedata", (char *)service_data.c_str());
                  #endif
                  if(abs((int)BLEdata["rssi"]|0) < abs(Minrssi)) { // publish only the devices close enough
                    pub((char *)topic.c_str(),BLEdata);
                    int pos = -1;
                    pos = strpos(d[5].extract,"209800");
                    if (pos != -1) {
                      trc("mi flora data reading");
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(d[0].extract)) MiFloraDiscovery(d[0].extract);
                      #endif
                      bool result = process_sensors(pos - 38,(char *)service_data.c_str(),d[0].extract);
                    }
                    pos = -1;
                    pos = strpos(d[5].extract,"20aa01");
                    //example "servicedata":"5020aa0194dfaa33342d580d1004e3002c02"
                    if (pos != -1){
                      trc("mi jia data reading");
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(d[0].extract)) MiJiaDiscovery(d[0].extract);
                      #endif
                      bool result = process_sensors(pos - 40,(char *)service_data.c_str(),d[0].extract);
                    }
                    pos = -1;
                    pos = strpos(d[5].extract,"205b04");
                    //example "servicedata":"141695fe70205b0461298882c8593f09061002d002"
                    if (pos != -1){
                      trc("LYWSD02 data reading");
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(d[0].extract)) LYWSD02Discovery(d[0].extract);
                      #endif
                      bool result = process_sensors(pos - 38,(char *)service_data.c_str(),d[0].extract);
                    }
                    pos = -1;
                    pos = strpos(d[5].extract,"304703");
                    if (pos != -1){
                      trc("CLEARGRASSTRH data reading method 1");
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(d[0].extract)) CLEARGRASSTRHDiscovery(d[0].extract);
                      #endif
                      bool result = process_sensors(pos - 40,(char *)service_data.c_str(),d[0].extract);
                    }
                    pos = -1;
                    pos = strpos(d[5].extract,"e30706");
                    if (pos != -1){
                      trc("Mi Scale data reading");
                      //example "servicedata":"a2ac2be307060207122b" /"a28039e3070602070e28"
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(d[0].extract)) MiScaleDiscovery(d[0].extract);
                      #endif
                      bool result = process_scale_v1((char *)service_data.c_str(),d[0].extract);
                    }
                    pos = -1;
                    pos = strpos(d[5].extract,"4030dd");
                    if (pos != -1){
                      trc(F("Mi Lamp data reading"));
                      //example "servicedata":4030dd31d0300010100
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(d[0].extract)) MiLampDiscovery(d[0].extract);
                      #endif
                      process_milamp((char *)service_data.c_str(),d[0].extract);
                    }
                    pos = -1;
                    pos = strpos(d[5].extract,"08094c"); // Clear grass with air pressure//08094c0140342d580104d8000c020702612702015a
                    if (pos != -1){
                      trc(F("Clear grass data with air pressure reading"));
                      //example "servicedata":08094c0140342d580104 c400 2402 0702 5d27 02015a
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(d[0].extract)) CLEARGRASSTRHKPADiscovery(d[0].extract);
                      #endif
                      process_cleargrass_air((char *)service_data.c_str(),d[0].extract);
                    }
                    pos = -1;
                    pos = strpos(d[5].extract,"080774"); // Clear grass standard method 2/0807743e10342d580104c3002c0202012a
                    if (pos != -1){
                      trc(F("Clear grass data reading method 2"));
                      //example "servicedata":0807743e10342d580104 c300 2c02 02012a
                      // no discovery as it is already available with method 1
                      process_cleargrass((char *)service_data.c_str(),d[0].extract);
                    }
                    return true;
                  }else{
                    trc("Low rssi, device filtered");
                  }
                }
              }
            }
            returnedString = ""; //init data string
            return false;
          }
          softserial.print(F(QUESTION_MSG));
          return false;
      }else{   
        return false;
      }
    }
#endif

double value_from_service_data(char * service_data, int offset, int data_length){
  char rev_data[data_length+1];
  char data[data_length+1];
  memcpy( rev_data, &service_data[offset], data_length );
  rev_data[data_length] = '\0';
  
  // reverse data order
  revert_hex_data(rev_data, data, data_length+1);
  double value = strtol(data, NULL, 16);
  if (value > 65000 && data_length <= 4) value = value - 65535;
  trc(value);
  return value;
}

bool process_sensors(int offset, char * rest_data, char * mac_adress){
  
  trc(F("Creating BLE buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& BLEdata = jsonBuffer.createObject();
  trc("rest_data");
  trc(rest_data);
  int data_length = 0;
  trc("data_length");
  trc(rest_data[51 + offset]);
  switch (rest_data[51 + offset]) {
    case '1' :
    case '2' :
    case '3' :
    case '4' :
        data_length = ((rest_data[51 + offset] - '0') * 2);
        trc("valid data_length");
    break;
    default:
        trc("invalid data_length");
    return false;
    }
  
  double value = 9999;
  value = value_from_service_data(rest_data, 52 + offset, data_length);

  // Mi flora provides tem(perature), (earth) moi(sture), fer(tility) and lux (illuminance)
  // Mi Jia provides tem(perature), batt(erry) and hum(idity)
  // following the value of digit 47 we determine the type of data we get from the sensor
  switch (rest_data[47 + offset]) {
    case '9' :
          BLEdata.set("fer", (double)value);
    break;
    case '4' :
          BLEdata.set("tem", (double)value/10);
    break;
    case '6' :
          BLEdata.set("hum", (double)value/10);
    break;
    case '7' :
          BLEdata.set("lux", (double)value);
     break;
    case '8' :
          BLEdata.set("moi", (double)value);
     break;
     
    case 'a' :
          BLEdata.set("batt", (double)value);
     break;

     case 'd' :
          // humidity
          value = value_from_service_data(rest_data, 52 + offset, 4);
          BLEdata.set("tem", (double)value/10);
          // temperature
          value = value_from_service_data(rest_data, 56 + offset, 4);
          BLEdata.set("hum", (double)value/10);
     break;
    default:
    trc("can't read values");
    return false;
    }
    String mactopic(mac_adress);
    mactopic = subjectBTtoMQTT + mactopic;
    pub((char *)mactopic.c_str(),BLEdata);
    return true;
}

bool process_scale_v1(char * rest_data, char * mac_adress){
  //example "servicedata":"a2ac2be307060207122b" /"a28039e3070602070e28"
  trc("rest_data");
  trc(rest_data);

  trc(F("Creating BLE buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& BLEdata = jsonBuffer.createObject();

  double weight = value_from_service_data(rest_data, 2, 4)/200;

  //Set Json value
  BLEdata.set("weight", (double)weight);

  // Publish weight
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;
  pub((char *)mactopic.c_str(),BLEdata);
  return true;
}

bool process_scale_v2(char * rest_data, char * mac_adress){
  //example "servicedata":"a2ac2be307060207122b" /"a28039e3070602070e28"
  trc("rest_data");
  trc(rest_data);

  trc(F("Creating BLE buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& BLEdata = jsonBuffer.createObject();

  double weight = value_from_service_data(rest_data, 22, 4)/200;
  double impedance = value_from_service_data(rest_data, 18, 4);

  //Set Json values
  BLEdata.set("weight", (double)weight);
  BLEdata.set("impedance", (double)impedance);

  // Publish weight
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;
  pub((char *)mactopic.c_str(),BLEdata);
  return true;
}

bool process_miband(char * rest_data, char * mac_adress){
  //example "servicedata":"a21e0000"
  trc("rest_data");
  trc(rest_data);

  trc(F("Creating BLE buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& BLEdata = jsonBuffer.createObject();

  double steps = value_from_service_data(rest_data, 0, 4);

  //Set Json value
  BLEdata.set("steps", (double)steps);

  // Publish weight
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;
  pub((char *)mactopic.c_str(),BLEdata);
  return true;
}

bool process_milamp(char * rest_data, char * mac_adress){
  //example "servicedata":"4030dd31d0300010100"
  trc("rest_data");
  trc(rest_data);

  trc(F("Creating BLE buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& BLEdata = jsonBuffer.createObject();

  long darkness = value_from_service_data(rest_data, 8, 2);

  //Set Json value
  BLEdata.set("presence", (bool)"true");
  BLEdata.set("darkness", (long)darkness);

  // Publish
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;
  pub((char *)mactopic.c_str(),BLEdata);
  return true;
}

bool process_cleargrass_air(char * rest_data, char * mac_adress){
  //example "servicedata":"08094c0140342d580104c400240207025d2702015a"
  //decoding "08094c0140342d580104 temp:c400 hum:2402 0702 pressure:5d27 02015a"
  trc("rest_data");
  trc(rest_data);

  trc(F("Creating BLE buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& BLEdata = jsonBuffer.createObject();

  double value = 9999;
  // humidity
  value = value_from_service_data(rest_data, 20, 4);
  BLEdata.set("tem", (double)value/10);
  // temperature
  value = value_from_service_data(rest_data, 24, 4);
  BLEdata.set("hum", (double)value/10);
  // air pressure
  value = value_from_service_data(rest_data, 32, 4);
  BLEdata.set("pres", (double)value/100);

  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;
  pub((char *)mactopic.c_str(),BLEdata);
  return true;
}


bool process_cleargrass(char * rest_data, char * mac_adress){
  //example "servicedata":0807743e10342d580104 c300 2c02 02012a
  trc("rest_data");
  trc(rest_data);

  trc(F("Creating BLE buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& BLEdata = jsonBuffer.createObject();

  double value = 9999;
  // humidity
  value = value_from_service_data(rest_data, 20, 4);
  BLEdata.set("tem", (double)value/10);
  // temperature
  value = value_from_service_data(rest_data, 24, 4);
  BLEdata.set("hum", (double)value/10);

  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;
  pub((char *)mactopic.c_str(),BLEdata);
  return true;
}

#ifdef subjectHomePresence
void haRoomPresence(JsonObject& HomePresence){
  int BLErssi = HomePresence["rssi"];
  trc(F("BLErssi"));
  trc(BLErssi);
  int txPower = HomePresence["txpower"]|0;
  if (txPower >= 0)   txPower = -59; //if tx power is not found we set a default calibration value
  trc(F("txPower"));
  trc(txPower);
  double ratio = BLErssi*1.0/txPower;
  double distance;
  if (ratio < 1.0) {
    distance = pow(ratio,10);
  }else{
    distance = (0.89976)* pow(ratio,7.7095) + 0.111;  
  }
  HomePresence["distance"] = distance;
  trc(F("BLE DISTANCE :"));
  trc(distance);
  pub(subjectHomePresence,HomePresence);
}
#endif

void MQTTtoBT(char * topicOri, JsonObject& BTdata) { // json object decoding
 if (strcmp(topicOri,subjectMQTTtoBTset) == 0){
    trc(F("MQTTtoBT json set"));

    // Black list & white list set
    int WLsize =  BTdata["white-list"].size();
    if(WLsize > 0){
      trc(F("WL set"));
      for (int i = 0; i < WLsize; i++){
        const char * whiteMac = BTdata["white-list"][i];
        setWorBMac((char *)whiteMac, true);
      }
    }
    int BLsize =  BTdata["black-list"].size();
    if(BLsize > 0){
      trc(F("BL set"));
      for (int i = 0; i < BLsize; i++){
        const char * blackMac = BTdata["black-list"][i];
        setWorBMac((char *)blackMac,false);
      }
    }
    if (BLsize > 0 || WLsize > 0) dumpDevices();

    // Scan interval set
    if (BTdata.containsKey("interval")){
      trc(F("BLE interval"));
      // storing BLE interval for further use if needed
      unsigned int prevBLEinterval = BLEinterval;
      trc("previous interval");
      trc(BLEinterval); 
      // set BLE interval if present if not setting default value
      BLEinterval = (unsigned int)BTdata["interval"];
      trc("new interval");
      trc(BLEinterval);
      if (BLEinterval == 0) {
        if(BTtoMQTT())// as BLEinterval is = to 0 we can launch the loop and the scan will execute immediately
        trc(F("Scan done")); 
        BLEinterval = prevBLEinterval; // as 0 was just used as a command we recover previous scan duration
      }
    }
    // MinRSSI set
    if (BTdata.containsKey("minrssi")){
      trc(F("Min RSSI"));
      // storing Min RSSI for further use if needed
      trc("previous Minrssi");
      trc(Minrssi); 
      // set Min RSSI if present if not setting default value
      Minrssi = (unsigned int)BTdata["minrssi"];
      trc("new Minrssi");
      trc(Minrssi);
    }
  }
}

#endif
