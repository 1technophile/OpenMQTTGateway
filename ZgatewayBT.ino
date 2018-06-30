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

    //Time used to wait for an interval before resending BLE infos
    unsigned long timeBLE= 0;
    
    //core on which the BLE detection task will run
    static int taskCore = 0;
      
    class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
          void onResult(BLEAdvertisedDevice advertisedDevice) {
            String mac_adress = advertisedDevice.getAddress().toString().c_str();
            mac_adress.replace(":","");
            mac_adress.toUpperCase();
            String mactopic = subjectBTtoMQTT + mac_adress;
            if (advertisedDevice.haveName()){
                trc(F("Get Name "));
                String nameBLE = advertisedDevice.getName().c_str();
                trc(nameBLE);
                client.publish((char *)(mactopic + "/name").c_str(),(char *)nameBLE.c_str());
            }
            if (advertisedDevice.haveManufacturerData()){
                trc(F("Get ManufacturerData "));
                String ManufacturerData = advertisedDevice.getManufacturerData().c_str();
                trc(ManufacturerData);
                client.publish((char *)(mactopic + "/ManufacturerData").c_str(),(char *)ManufacturerData.c_str());
            }
            if (advertisedDevice.haveRSSI()){
              trc(F("Get RSSI "));       
              String rssi = String(advertisedDevice.getRSSI());
              String rssitopic = mactopic + subjectBTtoMQTTrssi;
              trc(rssitopic + " " + rssi);
              client.publish((char *)rssitopic.c_str(),(char *)rssi.c_str());
            }
            if (advertisedDevice.haveTXPower()){
              trc(F("Get TXPower "));       
              int8_t TXPower = advertisedDevice.getTXPower();
              trc(TXPower);
              char cTXPower[5];
              sprintf(cTXPower, "%d", TXPower);
              client.publish((char *)(mactopic + "/tx").c_str(),cTXPower);
            }
            if (advertisedDevice.haveServiceData()){
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
                trc(returnedString);
                                
                trc(F("Get service data UUID"));
                BLEUUID serviceDataUUID = advertisedDevice.getServiceDataUUID();
                trc(serviceDataUUID.toString().c_str());

                if (strstr(serviceDataUUID.toString().c_str(),"fe95") != NULL){
                  trc("Processing BLE device data");
                  char service_data[returnedString.length()+1];
                  returnedString.toCharArray(service_data,returnedString.length()+1);
                  service_data[returnedString.length()] = '\0';
                  char mac[mac_adress.length()+1];
                  mac_adress.toCharArray(mac,mac_adress.length()+1);
                  if (strstr(service_data,"209800") != NULL) {
                    trc("mi flora data reading");
                    boolean result = process_data(-22,service_data,mac);
                  }
                  if (strstr(service_data,"20aa01") != NULL){
                    trc("mi jia data reading");
                    boolean result = process_data(-24,service_data,mac);
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
      if (now > (timeBLE + TimeBtw_Read)) {//retriving value of temperature and humidity of the box from DHT every xUL
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
  trc(F("ZgatewayBT HM1X setup done "));
}

#ifdef ZgatewayBT_v6xx
#define QUESTION_MSG "AT+DISA?"
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
          trc(token);
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
                strupp(d[0].extract);
                String mactopic(d[0].extract);
                trc(mactopic);
                mactopic = subjectBTtoMQTT + mactopic + subjectBTtoMQTTrssi;
                int rssi = (int)strtol(d[2].extract, NULL, 16) - 256;
                char val[12];
                sprintf(val, "%d", rssi);
                client.publish((char *)mactopic.c_str(),val);
                if (strcmp(d[4].extract, "fe95") == 0) 
                  if (strstr(d[5].extract,"209800") != NULL) {
                    trc("mi flora data reading");
                    boolean result = process_data(0,d[5].extract,d[0].extract);
                  }
                  if (strstr(d[5].extract,"20aa01") != NULL){
                    trc("mi jia data reading");
                    boolean result = process_data(-2,d[5].extract,d[0].extract);
                  }
                return true;
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

#ifndef ZgatewayBT_v6xx
#define QUESTION_MSG "AT+DISI?"
boolean BTtoMQTT() {
  while (softserial.available() > 0) {
     #if defined(ESP8266)
      yield();
     #endif
    String discResult = softserial.readString();
    if (discResult.indexOf(STRING_MSG)>=0){
      discResult.replace(RESPONSE_MSG,"");
      discResult.replace(RESP_END_MSG,"");
      float device_number = discResult.length()/78.0;
      if (device_number == (int)device_number){ // to avoid publishing partial values we detect if the serial data has been fully read = a multiple of 78
        trc(F("Sending BT data to MQTT"));
        #if defined(ESP8266)
          yield();
        #endif
        for (int i=0;i<(int)device_number;i++){
             String onedevice = discResult.substring(0,78);
             onedevice.replace(STRING_MSG,"");
             String mac = onedevice.substring(53,65);
             String rssi = onedevice.substring(66,70);
             String mactopic = subjectBTtoMQTT + mac + subjectBTtoMQTTRSSI;
             trc(mactopic + " " + rssi);
             client.publish((char *)mactopic.c_str(),(char *)rssi.c_str());
             discResult = discResult.substring(78);
          }
          return true;
        }
      }
    if (discResult.indexOf(SETUP_MSG)>=0)
    {
      trc(F("Connection OK to HM-10"));
    }
  }
  if (millis() > (timebt + TimeBtw_Read)) {//retriving value of adresses and rssi
       timebt = millis();
       #if defined(ESP8266)
        yield();
       #endif
       softserial.print(F(QUESTION_MSG));
       
  }
  return false;
}
#endif
#endif

boolean process_data(int offset, char * rest_data, char * mac_adress){
  
  int data_length = 0;
  switch (rest_data[51 + offset]) {
    case '1' :
    case '2' :
    case '3' :
    case '4' :
        data_length = ((rest_data[51 + offset] - '0') * 2)+1;
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
  
 
  // Mi flora provides tem(perature), (earth) moi(sture), fer(tility) and lux (illuminance)
  // Mi Jia provides tem(perature) and hum(idity)
  // following the value of digit 47 we determine the type of data we get from the sensor
  switch (rest_data[47 + offset]) {
    case '9' :
          mactopic = mactopic + subjectBTtoMQTTfer;
          dtostrf(value,0,0,val);
    break;
    case '4' :
          mactopic = mactopic + subjectBTtoMQTTtem;
          if (value > 65000) value = value - 65535;
          dtostrf(value/10,3,1,val); // temp has to be divided by 10
    break;
    case '6' :
          mactopic = mactopic + subjectBTtoMQTThum;
          if (value > 65000) value = value - 65535;
          dtostrf(value/10,3,1,val); // hum has to be divided by 10
    break;
    case '7' :
          mactopic = mactopic + subjectBTtoMQTTlux;
          dtostrf(value,0,0,val);
     break;
    case '8' :
          mactopic = mactopic + subjectBTtoMQTTmoi;
          dtostrf(value,0,0,val);
     break;
    default:
    trc("can't read values");
    return false;
    }
    client.publish((char *)mactopic.c_str(),val);;
    trc(val);
    return true;
  }

#endif
