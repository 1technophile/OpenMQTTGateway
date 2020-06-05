/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the bluetooth low energy gateway (beacons detection)
  
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
*/
#ifndef config_BT_h
#define config_BT_h

extern void setupBT();
extern bool BTtoMQTT();  
extern void MQTTtoBT(char * topicOri, JsonObject& RFdata);
/*----------------------BT topics & parameters-------------------------*/
#define subjectBTtoMQTT  "/BTtoMQTT"
#define subjectMQTTtoBTset  "/commands/MQTTtoBT/config"
#define MinimumRSSI -100 //default minimum rssi value, all the devices below -90 will not be reported
#define Scan_duration 10 //define the time for a scan --WARNING-- changing this value can lead to instability on ESP32
#define HM-10
#define HMSerialSpeed 9600 // Communication speed with the HM module, softwareserial doesn't support 115200
//#define HM-11 // uncomment this line if you use HM-11 and comment the line above
//#define HM_BLUE_LED_STOP true //uncomment to stop the blue led light of HM1X
#define BLEdelimiter "4f4b2b444953413a" // OK+DISA:
#define ServicedataMinLength 30

#ifndef TimeBtw_Read
  #define TimeBtw_Read 55555 //define default time between 2 scans
#endif

#ifndef pubKnownBLEServiceData
  #define pubKnownBLEServiceData false // define true if you want to publish service data belonging to recognised sensors
#endif

#ifndef pubUnknownBLEServiceData
  #define pubUnknownBLEServiceData true // define false if you don't want to publish service data to unrecognised sensors (in case you are having too heavy service data) https://github.com/1technophile/OpenMQTTGateway/issues/318#issuecomment-446064707
#endif

#ifndef pubBLEManufacturerData
  #define pubBLEManufacturerData false // define true if you want to publish the manufacturer's data (sometimes contains characters that aren't valid with receiving client)
#endif

#ifndef pubBLEServiceUUID
  #define pubBLEServiceUUID false // define true if you want to publish the service UUID data
#endif

/*-------------------HOME ASSISTANT ROOM PRESENCE ----------------------*/
// if not commented Home presence integration with HOME ASSISTANT is activated
#define subjectHomePresence "home_presence/" // will send Home Assistant room presence message to this topic (first part is same for all rooms, second is room name)

unsigned int BLEinterval = TimeBtw_Read; //time between 2 scans
int Minrssi = MinimumRSSI; //minimum rssi value

struct BLEdevice{
  char macAdr[13];
  bool isDisc;
  bool isWhtL;
  bool isBlkL;
};

#define device_flags_isDisc 1 << 0
#define device_flags_isWhiteL 1 << 1
#define device_flags_isBlackL 1 << 2

struct decompose{
  char subject[4];
  int start;
  int len;
  bool reverse;
  char extract[60];
};
     
/*-------------------PIN DEFINITIONS----------------------*/
#if !defined(BT_RX) || !defined(BT_TX)
  #ifdef ESP8266
    #define BT_RX 13 //D7 ESP8266 RX connect HM-10 or 11 TX
    #define BT_TX 12 //D6 ESP8266 TX connect HM-10 or 11 RX
  #elif defined(ESP32)
    #define BT_RX 18 // not tested
    #define BT_TX 19 // not tested
  #else
    #define BT_RX 5 //arduino RX connect HM-10 or 11 TX
    #define BT_TX 6 //arduino TX connect HM-10 or 11 RX
  #endif
#endif

#endif