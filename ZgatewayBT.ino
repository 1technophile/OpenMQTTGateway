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
#ifdef ZgatewayBT

#include <vector>
using namespace std;

vector<BLEdevice> devices;

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

    //Time used to wait for an interval before resending BLE infos
    unsigned long timeBLE= 0;
    
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
            if (advertisedDevice.haveName())              BLEdata.set("name", (char *)advertisedDevice.getName().c_str());
            if (advertisedDevice.haveManufacturerData())  BLEdata.set("manufacturerdata", (char *)advertisedDevice.getManufacturerData().c_str());
            if (advertisedDevice.haveRSSI())              BLEdata.set("rssi", (int) advertisedDevice.getRSSI());
            if (advertisedDevice.haveTXPower())           BLEdata.set("txpower", (int8_t) advertisedDevice.getTXPower());
            #ifdef subjectHomePresence
              if (advertisedDevice.haveRSSI()) haRoomPresence(BLEdata);// this device has an rssi in consequence we can use it for home assistant room presence component
            #endif
            if (advertisedDevice.haveServiceData()){

                char mac[mac_adress.length()+1];
                mac_adress.toCharArray(mac,mac_adress.length()+1);
                
                trc(F("Get service data "));

                std::string serviceData = advertisedDevice.getServiceData();
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
                  BLEdata.set("servicedatauuid", (char *)advertisedDevice.getServiceDataUUID().toString().c_str());
                #endif
                if((!oneWhite() || isWhite(mac)) && !isBlack(mac)){ //if not black listed mac we go AND if we have no white mac or this mac is  white we go out
                  pub((char *)mactopic.c_str(),BLEdata);
                  if (strstr(BLEdata["servicedatauuid"].as<char*>(),"fe95") != NULL){
                    trc("Processing BLE device data");
                    int pos = -1;
                    pos = strpos(service_data,"209800");
                    if (pos != -1){
                      trc(F("mi flora data reading"));
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) MiFloraDiscovery(mac);
                      #endif
                      process_data(pos - 24,service_data,mac);
                    }
                    pos = -1;
                    pos = strpos(service_data,"20aa01");
                    if (pos != -1){
                      trc(F("mi jia data reading"));
                      #ifdef ZmqttDiscovery
                        if(!isDiscovered(mac)) MiJiaDiscovery(mac);
                      #endif
                      process_data(pos - 26,service_data,mac);
                    }
                  }
                }
            }
          }
      };

    void setupBT(){
        #ifdef multiCore
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
        #else
          trc(F("ZgatewayBT singlecore ESP32 setup done "));
        #endif
    }
    
    #ifdef multiCore
    void coreTask( void * pvParameters ){
     
        String taskMessage = "BT Task running on core ";
        taskMessage = taskMessage + xPortGetCoreID();
     
        while(true){
            trc(taskMessage);
            delay(TimeBtw_Read);
            
            TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
            TIMERG0.wdt_feed=1;
            TIMERG0.wdt_wprotect=0;
            
            BLEDevice::init("");
            BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
            MyAdvertisedDeviceCallbacks myCallbacks;
            pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
            pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
            BLEScanResults foundDevices = pBLEScan->start(Scan_duration);
        }
    }
    #else
    boolean BTtoMQTT(){
      unsigned long now = millis();
      if (now > (timeBLE + TimeBtw_Read)) {
              timeBLE = now;
              BLEDevice::init("");
              BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
              MyAdvertisedDeviceCallbacks myCallbacks;
              pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
              pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
              BLEScanResults foundDevices = pBLEScan->start(Scan_duration);
              return true;
      }
      return false;
    }
    #endif
      
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
    
    boolean BTtoMQTT() {
    
      //extract serial data from module in hexa format
      while (softserial.available() > 0) {
          int a = softserial.read();
          if (a < 16) {
            returnedString = returnedString + "0";
          } 
            returnedString = returnedString + String(a,HEX);  
      }
    
      if (millis() > (timebt + TimeBtw_Read)) {//retriving data
          timebt = millis();
          #if defined(ESP8266)
            yield();
          #endif
          if (returnedString != "") {
            size_t pos = 0;
            while ((pos = returnedString.lastIndexOf(delimiter)) != -1) {
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
                  strupp(d[0].extract);
                  if(isBlack(d[0].extract)) return false; //if black listed mac we go out
                  if(oneWhite() && !isWhite(d[0].extract)) return false; //if we have at least one white mac and this mac is not white we go out
                  String topic = subjectBTtoMQTT + String(d[0].extract);
                  int rssi = (int)strtol(d[2].extract, NULL, 16) - 256;
                  BLEdata.set("rssi", (int)rssi);
                  #ifdef subjectHomePresence
                    haRoomPresence(BLEdata);// this device has an rssi in consequence we can use it for home assistant room presence component
                  #endif
                  String Service_data(d[5].extract);
                  Service_data = Service_data.substring(14);
                  #ifdef pubBLEServiceData
                    BLEdata.set("servicedata", (char *)Service_data.c_str());
                  #endif
                  pub((char *)topic.c_str(),BLEdata);
                  if (strcmp(d[4].extract, "fe95") == 0) {
                      int pos = -1;
                      pos = strpos(d[5].extract,"209800");
                      if (pos != -1) {
                        trc("mi flora data reading");
                        #ifdef ZmqttDiscovery
                          if(!isDiscovered(d[0].extract)) MiFloraDiscovery(d[0].extract);
                        #endif
                        boolean result = process_data(pos - 38,(char *)Service_data.c_str(),d[0].extract);
                      }
                      pos = -1;
                      pos = strpos(d[5].extract,"20aa01");
                      if (pos != -1){
                        trc("mi jia data reading");
                        #ifdef ZmqttDiscovery
                          if(!isDiscovered(d[0].extract)) MiJiaDiscovery(d[0].extract);
                        #endif
                        boolean result = process_data(pos - 40,(char *)Service_data.c_str(),d[0].extract);
                      }
                      return true;
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
    
    void strupp(char* beg)
    {
        while (*beg = toupper(*beg))
           ++beg;
    }

#endif

#ifdef ZmqttDiscovery
void MiFloraDiscovery(char * mac){
  #define MiFloraparametersCount 4
  trc(F("MiFloraDiscovery"));
  char * MiFlorasensor[MiFloraparametersCount][8] = {
     {"sensor", "MiFlora-lux", mac, "illuminance","{{ value_json.lux }}","", "", "lu"} ,
     {"sensor", "MiFlora-tem", mac,"","{{ value_json.tem }}","", "", "°C"} ,
     {"sensor", "MiFlora-fer", mac,"","{{ value_json.fer }}","", "", ""} ,
     {"sensor", "MiFlora-moi", mac,"","{{ value_json.moi }}","", "", "%"}
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
  devices.push_back(device);
}

void MiJiaDiscovery(char * mac){
  #define MiJiaparametersCount 3
  trc(F("MiJiaDiscovery"));
  char * MiJiasensor[MiJiaparametersCount][8] = {
     {"sensor", "MiJia-batt", mac, "illuminance","{{ value_json.batt }}","", "", "V"} ,
     {"sensor", "MiJia-tem", mac,"","{{ value_json.tem }}","", "", "°C"} ,
     {"sensor", "MiJia-hum", mac,"","{{ value_json.hum }}","", "", "%"}
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
  devices.push_back(device);
}
#endif

boolean process_data(int offset, char * rest_data, char * mac_adress){
  
  trc(F("Creating BLE buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& BLEdata = jsonBuffer.createObject();
  trc("rest_data");
  trc(rest_data);
  int data_length = 0;
  switch (rest_data[51 + offset]) {
    case '1' :
    case '2' :
    case '3' :
    case '4' :
        data_length = ((rest_data[51 + offset] - '0') * 2)+1;
        trc("data_length");
        trc(data_length);
    break;
    default:
        trc("can't read data_length");
    return false;
    }
    
  char rev_data[data_length];
  char data[data_length];
  memcpy( rev_data, &rest_data[52 + offset], data_length );
  rev_data[data_length] = '\0';
  
  // reverse data order
  revert_hex_data(rev_data, data, data_length);
  double value = strtol(data, NULL, 16);
  trc(value);
  char val[12];
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;

  // second value
  char val2[12];
  trc("rest_data");
  trc(rest_data);
  // Mi flora provides tem(perature), (earth) moi(sture), fer(tility) and lux (illuminance)
  // Mi Jia provides tem(perature), batt(erry) and hum(idity)
  // following the value of digit 47 we determine the type of data we get from the sensor
  switch (rest_data[47 + offset]) {
    case '9' :
          BLEdata.set("fer", (double)value);
    break;
    case '4' :
          if (value > 65000) value = value - 65535;
          BLEdata.set("tem", (double)value/10);
    break;
    case '6' :
          if (value > 65000) value = value - 65535;
          BLEdata.set("hum", (double)value/10);
    break;
    case '7' :
          BLEdata.set("lux", (double)value);
     break;
    case '8' :
          BLEdata.set("moi", (double)value);
     break;
     
    case 'a' : // batteryLevel
          BLEdata.set("batt", (double)value);
     break;

     case 'd' : // temp+hum
          char tempAr[8];
          // humidity
          memcpy(tempAr, data, 4);
          tempAr[4] = '\0';
          value = strtol(tempAr, NULL, 16);
          if (value > 65000) value = value - 65535;
          BLEdata.set("hum", (double)value/10);
          // temperature
          memcpy(tempAr, &data[4], 4);
          tempAr[4] = '\0';
          value = strtol(tempAr, NULL, 16);
          if (value > 65000) value = value - 65535;
          BLEdata.set("tem", (double)value/10);
     break;
    default:
    trc("can't read values");
    return false;
    }
    pub((char *)mactopic.c_str(),BLEdata);
    return true;
}

#ifdef subjectHomePresence
void haRoomPresence(JsonObject& HomePresence){
  int BLErssi = HomePresence["rssi"];
  trc(F("BLErssi"));
  trc(BLErssi);
  int txPower = HomePresence["txpower"]|0;
  if (txPower == 0)   txPower = -59; //if tx power is not found we set a default calibration value
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
    int WLsize =  BTdata["white-list"].size();
    if(WLsize > 0){
      for (int i = 0; i < WLsize; i++){
        const char * whiteMac = BTdata["white-list"][i];
        setWorBMac((char *)whiteMac, true); //TO DO catch mac adress > 12
      }
    }
    int BLsize =  BTdata["black-list"].size();
    if(BLsize > 0){
      for (int i = 0; i < BLsize; i++){
        const char * blackMac = BTdata["black-list"][i];
        setWorBMac((char *)blackMac,false); //TO DO catch mac adress > 12
      }
    }
    dumpDevices();
  }
}

void setWorBMac(char * mac , boolean isWhite){
  boolean foundMac = false;
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

boolean oneWhite(){
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
        if(p->isWhtL) return true;
  }
  return false;
}

boolean isWhite(char * mac){
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
    if((strcmp(p->macAdr,mac) == 0)){
        return p->isWhtL;
    }
  }
  return false;
}

boolean isBlack(char * mac){
  for(vector<BLEdevice>::iterator p = devices.begin(); p != devices.end(); ++p){
    if((strcmp(p->macAdr,mac) == 0)){
        return p->isBlkL;
    }
  }
  return false;
}

boolean isDiscovered(char * mac){
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
#endif
