/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal
 - receive MQTT data from a topic and send IR signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received IR signal
 - publish MQTT data to a different topic related to BLE devices rssi signal
  
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

/*----------------------------USER PARAMETERS-----------------------------*/
#define SERIAL_BAUD   115200
/*-------------DEFINE YOUR WIRING TYPE BELOW----------------*/
// Choose between "I2C_Wiring" OR "Classic_Wiring" OR "RFM69_Wiring"
// Please Note: I2C Wiring,Classic Wiring and RFM69_Wiring uses different Pins for all Modules, see PIN definitions into the wiki
//#define I2C_Wiring // With Support for I2C Modules and the associated libraries BH1750 and BME280
//#define RFM69_Wiring // following file img/OpenMQTTGateway_IR_RF_RFM69.png
#define Classic_Wiring // following file img/OpenMQTTGateway_IR_RF_BT.png
/*-------------DEFINE YOUR NETWORK PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#define mqtt_server "192.168.1.17"
//#define mqtt_user "your_username" // not compulsory only if your broker needs authentication
#define mqtt_password "your_password" // not compulsory only if your broker needs authentication
#define mqtt_port 1883
#define Gateway_Name "OpenMQTTGateway"
#define will_Topic "home/OpenMQTTGateway/LWT"
#define will_QoS 0
#define will_Retain true
#define will_Message "Offline"
#define Gateway_AnnouncementMsg "Online"

/*-------------DEFINE YOUR NETWORK PARAMETERS BELOW----------------*/
// Update these with values suitable for your network.
#ifdef ESP8266 // for nodemcu, weemos and esp8266
  #define wifi_ssid "wifi ssid"
  #define wifi_password "wifi password"
#else // for arduino + W5100
  const byte mac[] = {  0xDE, 0xED, 0xBA, 0xFE, 0x54, 0x95 }; //W5100 ethernet shield mac adress
#endif

const byte ip[] = { 192, 168, 1, 99 }; //ip adress
// Advanced network config (optional) if you want to use these parameters uncomment line 158, 172 and comment line 171  of OpenMQTTGateway.ino
const byte gateway[] = { 192, 168, 1, 1 }; //ip adress
const byte Dns[] = { 192, 168, 1, 1 }; //ip adress
const byte subnet[] = { 255, 255, 255, 0 }; //ip adress

/*-------------DEFINE YOUR OTA PARAMETERS BELOW----------------*/
#define ota_hostname "OTAHOSTNAME"
#define ota_password "OTAPASSWORD"
#define ota_port 8266

/*-------------DEFINE THE MODULES YOU WANT BELOW----------------*/
//Addons and module management, comment the Z line and the config file if you don't use
#ifdef ESP8266 // for nodemcu, weemos and esp8266
  #define ZsensorDHT
  #include "config_DHT.h"
  #define ZgatewayRF
  #include "config_RF.h"
  #define ZgatewayRF2
  #ifdef RFM69_Wiring
    #define ZgatewayRFM69
  #include "config_RFM69.h"
  #endif
  #define ZgatewayIR
  #include "config_IR.h"
  #define ZgatewayBT
  #include "config_BT.h"
  #ifdef I2C_Wiring // to use the sensor below the gateway should wired with I2CWiring, see PIN DEFINITIONS below
    #define ZsensorBH1750
    #include "config_BH1750.h"
    #define ZsensorBME280
    #include "config_BME280.h"
  #endif
#else // for arduino + W5100
  #define ZgatewayRF
  #include "config_RF.h"
  #define ZgatewayRF2
  #ifdef RFM69_Wiring
    //#define ZgatewayRFM69 not tested
    //#include "config_RFM69.h"
  #endif
  #define ZgatewayIR
  #include "config_IR.h"
  #define ZgatewayBT
  #include "config_BT.h"
  //#define ZsensorDHT
  //#include "config_DHT.h"
  //#define ZsensorBH1750
  //#include "config_BH1750.h"
  //#define ZsensorBME280
  //#include "config_BME280.h"
  
#endif/*----------------------------OTHER PARAMETERS-----------------------------*/
/*-------------------CHANGING THEM IS NOT COMPULSORY-----------------------*/
/*--------------MQTT general topics-----------------*/
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#define subjectMQTTtoX "home/commands/#"
#define subjectMultiGTWKey "toMQTT"

/*-------------------ACTIVATE TRACES----------------------*/
#define TRACE 1  // 0= trace off 1 = trace on

