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
/*-------------------VERSION----------------------*/
#define OMG_VERSION "0.8beta"

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
//MQTT Parameters definition
//#define mqtt_server_name "www.mqtt_broker.com" // instead of defining the server by its IP you can define it by its name, uncomment this line and set the correct MQTT server host name
char mqtt_user[20] = "your_username"; // not compulsory only if your broker needs authentication
char mqtt_pass[20] = "your_password"; // not compulsory only if your broker needs authentication
char mqtt_server[40] = "192.168.1.17";
char mqtt_port[6] = "1883";

#define Gateway_Name "OpenMQTTGateway"
#define Base_Topic "home/"
#define version_Topic  Base_Topic Gateway_Name "/version"
#define will_Topic  Base_Topic Gateway_Name "/LWT"
#define will_QoS 0
#define will_Retain true
#define will_Message "Offline"
#define Gateway_AnnouncementMsg "Online"

/*-------------DEFINE YOUR NETWORK PARAMETERS BELOW----------------*/

//#define ESPWifiManualSetup true //uncomment you don't want to use wifimanager for your credential settings on ESP
#define WifiManager_password "your_password"
//#define MDNS_SD //uncomment if you  want to use mdns for discovering automatically your ip server, please note that MDNS with ESP32 can cause the BLE to not work
//#define cleanFS true //uncomment if you want to clean the ESP memory and reenter your credentials
#define maxMQTTretry 4 //maximum MQTT connection attempts before going to wifi setup

//set minimum quality of signal so it ignores AP's under that quality
#define MinimumWifiSignalQuality 8

// Update these with values suitable for your network.
#if defined(ESP32) || defined(ESPWifiManualSetup) // for nodemcu, weemos and esp8266
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
#define ota_hostname Gateway_Name
#define ota_password "OTAPASSWORD"
#define ota_port 8266

/*-------------DEFINE PINs FOR STATUS LEDs----------------*/
#define led_receive 40
#define led_send 42
#define led_error 44

//      VCC   ------------D|-----------/\/\/\/\ -----------------  Arduino PIN
//                        LED       Resistor 270-510R

/*-------------DEFINE THE MODULES YOU WANT BELOW----------------*/
//Addons and module management, comment the Z line

#define ZgatewayRF     "RF"       //ESP8266, Arduino, ESP32
//#define ZgatewayRF315  "RF315"    //ESP8266, Arduino, ESP32
#define ZgatewayIR     "IR"       //ESP8266, Arduino,         Sonoff RF Bridge
#define ZgatewayBT     "BT"       //ESP8266, Arduino, ESP32
//#define ZgatewayRF2    "RF2"      //ESP8266, Arduino, ESP32
//#define ZgatewaySRFB   "SRFB"     //                          Sonoff RF Bridge
//#define Zgateway2G     "2G"       //ESP8266, Arduino, ESP32
//#define ZactuatorONOFF "ONOFF"    //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define ZsensorINA226  "INA226"   //ESP8266, Arduino, ESP32
//#define ZsensorHCSR501 "HCSR501"  //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define ZsensorADC     "ADC"      //ESP8266, Arduino, ESP32
//#define ZsensorBH1750  "BH1750"   //ESP8266, Arduino, ESP32
//#define ZsensorTSL2561 "TSL2561"  //ESP8266, Arduino, ESP32
//#define ZsensorBME280  "BME280"   //ESP8266, Arduino, ESP32
//#define ZsensorDHT     "DHT"      //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define ZgatewayRFM69  "RFM69"    //ESP8266, Arduino, ESP32

/*----------------------------OTHER PARAMETERS-----------------------------*/
/*-------------------CHANGING THEM IS NOT COMPULSORY-----------------------*/
/*----------------------------USER PARAMETERS-----------------------------*/
#ifdef ZgatewaySRFB
  #define SERIAL_BAUD 19200
#else
  #define SERIAL_BAUD 115200
#endif
/*--------------MQTT general topics-----------------*/
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#define subjectMQTTtoX  Base_Topic Gateway_Name "/commands/#"
#define subjectMultiGTWKey "toMQTT"

//variables to avoid duplicates
#define time_avoid_duplicate 3000 // if you want to avoid duplicate mqtt message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates

//uncomment to use multicore function of ESP32 for BLE
#ifdef ESP32
  #define multiCore //comment to don't use multicore function of ESP32 for BLE
#endif

#define TimeBetweenReadingSYS 120000 // time between system readings (like memory)
#define subjectSYStoMQTT  Base_Topic Gateway_Name "/SYStoMQTT"

/*-------------------ACTIVATE TRACES----------------------*/
#define TRACE 1  // 0= trace off 1 = trace on

