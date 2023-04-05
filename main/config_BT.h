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
extern void MQTTtoBT(char* topicOri, JsonObject& RFdata);
extern void emptyBTQueue();
extern void launchBTDiscovery(bool overrideDiscovery);
extern void stopProcessing();
extern void startProcessing();
extern void lowPowerESP32();
extern String stateBTMeasures(bool);

#ifdef ESP32
extern int btQueueBlocked;
extern int btQueueLengthSum;
extern int btQueueLengthCount;
#  include "NimBLEDevice.h"
#endif

/*-----------BT TOPICS & COMPILATION PARAMETERS-----------*/
#define subjectBTtoMQTT    "/BTtoMQTT"
#define subjectMQTTtoBTset "/commands/MQTTtoBT/config"
// Uncomment to send undecoded device data to another gateway device for decoding
// #define MQTTDecodeTopic    "undecoded"
#ifndef UseExtDecoder
#  ifdef MQTTDecodeTopic
#    define UseExtDecoder true
#  else
#    define UseExtDecoder false
#  endif
#endif
#ifndef MQTTDecodeTopic
#  define MQTTDecodeTopic "undecoded"
#endif

#ifndef AttemptBLEConnect
#  define AttemptBLEConnect true //do we by default attempt a BLE connection to sensors with ESP32
#endif

#ifndef BLE_FILTER_CONNECTABLE
#  define BLE_FILTER_CONNECTABLE 0 // Sets whether to filter publishing of scanned devices that require a connection.
#endif // Setting this to 1 prevents overwriting the publication of the device connection data with the advertised data (Recommended for use with OpenHAB).

#define MinimumRSSI -100 //default minimum rssi value, all the devices below -100 will not be reported

#ifndef Scan_duration
#  define Scan_duration 10000 //define the duration for a scan; in milliseconds
#endif
#ifndef MinScanDuration
#  define MinScanDuration 1000 //minimum duration for a scan; in milliseconds
#endif
#ifndef BLEScanInterval
#  define BLEScanInterval 52 // How often the scan occurs / switches channels; in milliseconds,
#endif
#ifndef BLEScanWindow
#  define BLEScanWindow 30 // How long to scan during the interval; in milliseconds.
#endif
#ifndef AdaptiveBLEScan
#  define AdaptiveBLEScan true // Sets adaptive scanning, this will automatically decide on the best passive and active scanning intervals
#endif
#ifndef TimeBtwActive
#  define TimeBtwActive 55555 //define default time between two BLE active scans when general passive scanning is selected; in milliseconds
#endif
#ifndef MinTimeBtwScan
#  define MinTimeBtwScan 100 //define the time between two scans; in milliseconds
#endif
#ifndef TimeBtwConnect
#  define TimeBtwConnect 3600000 //define default time between BLE connection attempt (not used for immediate actions); in milliseconds
#endif
#ifndef PresenceAwayTimer
#  define PresenceAwayTimer 120000 //define the time between Offline Status update for the sensors
#endif

#ifndef BLEScanDuplicateCacheSize
#  define BLEScanDuplicateCacheSize 200
#endif
#ifndef TimeBtwRead
#  define TimeBtwRead 55555 //define default time between 2 scans; in milliseconds
#endif

#ifndef PublishOnlySensors
#  define PublishOnlySensors false //false if we publish all BLE devices discovered or true only the identified sensors (like temperature sensors)
#endif

#ifndef PublishRandomMACs
#  define PublishRandomMACs false //false to not publish devices which randomly change their MAC addresses
#endif

#ifndef HassPresence
#  define HassPresence false //false if we publish into Home Assistant presence topic
#endif

#ifndef BTQueueSize
#  define BTQueueSize 4 // lockless queue size for multi core cases (ESP32 currently)
#endif

#define HMSerialSpeed 9600 // Communication speed with the HM module, softwareserial doesn't support 115200
//#define HM_BLUE_LED_STOP true //uncomment to stop the blue led light of HM1X

#define BLEdelimiter       "4f4b2b444953413a" // OK+DISA:
#define BLEEndOfDiscovery  "4f4b2b4449534345" // OK+DISCE
#define BLEdelimiterLength 16
#define CRLR               "0d0a"
#define CRLR_Length        4
#define BLE_CNCT_TIMEOUT   3000

unsigned long scanCount = 0;

#ifndef pubBLEAdvData
#  define pubBLEAdvData false // define true if you want to publish all advertisement data
#endif

#ifndef useBeaconUuidForTopic
#  define useBeaconUuidForTopic false // define true to use iBeacon UUID as topic, instead of sender (random) MAC address
#endif

/*--------------HOME ASSISTANT ROOM PRESENCE--------------*/
#define subjectHomePresence "presence/" // will send Home Assistant room presence message to this topic (first part is same for all rooms, second is room name)

#ifndef useBeaconUuidForPresence
#  define useBeaconUuidForPresence false // //define true to use iBeacon UUID as for presence, instead of sender MAC (random) address
#endif

/*----------------CONFIGURABLE PARAMETERS-----------------*/
struct BTConfig_s {
  bool bleConnect; // Attempt a BLE connection to sensors with ESP32
  bool adaptiveScan;
  unsigned long intervalActiveScan; // Time between 2 active scans when generally passive scanning
  unsigned long BLEinterval; // Time between 2 scans
  unsigned long intervalConnect; // Time between 2 connects
  unsigned long scanDuration; // Duration for a scan; in milliseconds
  bool pubOnlySensors; // Publish only the identified sensors (like temperature sensors)
  bool pubRandomMACs; // Publish devices which randomly change their MAC address
  bool presenceEnable; // Publish into Home Assistant presence topic
  String presenceTopic; // Home Assistant presence topic to publish on
  bool presenceUseBeaconUuid; // Use iBeacon UUID as for presence, instead of sender MAC (random) address
  int minRssi; // Minimum rssi value, all the devices below will not be reported
  bool extDecoderEnable; // Send undecoded device data to another gateway device for decoding
  String extDecoderTopic; // Topic to send undecoded device data on
  bool filterConnectable; // Sets whether to filter publishing of scanned devices that require a connection.
  bool pubAdvData; // Publish advertisement data
  bool pubBeaconUuidForTopic; // Use iBeacon UUID as topic, instead of sender (random) MAC address
  bool ignoreWBlist; // Disable Whitelist & Blacklist
  unsigned long presenceAwayTimer; //Timer that trigger a tracker state as offline if not seen
};

// Global struct to store live BT configuration data
extern BTConfig_s BTConfig;

/*-------------------PIN DEFINITIONS----------------------*/
#if !defined(BT_RX) || !defined(BT_TX)
#  ifdef ESP8266
#    define BT_RX 13 //D7 ESP8266 RX connect HM-10 or 11 TX
#    define BT_TX 12 //D6 ESP8266 TX connect HM-10 or 11 RX
#  elif defined(ESP32)
#    define BT_RX 18 // not tested
#    define BT_TX 19 // not tested
#  else
#    define BT_RX 5 //arduino RX connect HM-10 or 11 TX
#    define BT_TX 6 //arduino TX connect HM-10 or 11 RX
#  endif
#endif

/*---------------INTERNAL USE: DO NOT MODIFY--------------*/
#ifdef ESP32
enum ble_val_type {
  BLE_VAL_STRING = 0,
  BLE_VAL_HEX,
  BLE_VAL_INT,
  BLE_VAL_FLOAT,
};

struct BLEAction {
  std::string value;
  char addr[18];
  int addr_type;
  NimBLEUUID service;
  NimBLEUUID characteristic;
  bool write;
  bool complete;
  uint8_t ttl;
  ble_val_type value_type;
};
#endif

struct BLEdevice {
  char macAdr[18];
  int macType;
  bool isDisc;
  bool isWhtL;
  bool isBlkL;
  bool connect;
  int sensorModel_id;
  unsigned long lastUpdate;
};

class BLEconectable {
public:
  enum id {
    MIN = 1000,
    LYWSD03MMC,
    MHO_C401,
    DT24_BLE,
    BM2,
    XMWSDJ04MMC,
    MAX,
  };
};

JsonObject& getBTJsonObject(const char* json = NULL, bool haPresenceEnabled = true);

#endif
