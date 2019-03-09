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
#define OMG_VERSION "0.9beta"

/*-------------CONFIGURE WIFIMANAGER-------------*/
/*
 * The following parameters are set during the WifiManager setup process:
 * - wifi_ssid
 * - wifi_password
 * - mqtt_user
 * - mqtt_pass
 * - mqtt_server
 * - mqtt_port
 * 
 * To completely disable WifiManager, define ESPWifiManualSetup.
 * If you do so, please don't forget to set these variables before compiling
 * 
 * Otherwise you can provide these credentials on the web interface after connecting 
 * to the access point with your password (SSID: WifiManager_ssid, password: WifiManager_password)
 */

#if defined(ESP8266)  // for nodemcu, weemos and esp8266
  //#define ESPWifiManualSetup true //uncomment you don't want to use wifimanager for your credential settings on ESP
#endif

#define WifiManager_password "your_password" //this is going to be the WPA2-PSK password for the initial setup access point 
#define WifiManager_ssid "OpenMQTTGateway" //this is the network name of the initial setup access point
#define WifiManager_ConfigPortalTimeOut 120

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
//MQTT Parameters definition
//#define mqtt_server_name "www.mqtt_broker.com" // instead of defining the server by its IP you can define it by its name, uncomment this line and set the correct MQTT server host name
char mqtt_user[20] = "your_username"; // not compulsory only if your broker needs authentication
char mqtt_pass[30] = "your_password"; // not compulsory only if your broker needs authentication
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
//#define MDNS_SD //uncomment if you  want to use mdns for discovering automatically your ip server, please note that MDNS with ESP32 can cause the BLE to not work
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

// these values are only used if no dhcp configuration is available
const byte ip[] = { 192, 168, 1, 99 }; //ip adress
// Advanced network config (optional) if you want to use these parameters uncomment line 158, 172 and comment line 171  of OpenMQTTGateway.ino
const byte gateway[] = { 192, 168, 1, 1 }; //ip adress
const byte Dns[] = { 192, 168, 1, 1 }; //ip adress
const byte subnet[] = { 255, 255, 255, 0 }; //ip adress

/*-------------DEFINE YOUR OTA PARAMETERS BELOW----------------*/
#define ota_hostname "OTAHOSTNAME"
#define ota_password "OTAPASSWORD"
#define ota_port 8266
/*-------------DEFINE PINs FOR STATUS LEDs----------------*/
#ifdef ESP8266
  #define led_receive 40
  #define led_send 42
  #define led_info 44
#elif ESP32
  #define led_receive 40
  #define led_send 42
  #define led_info 44
#elif __AVR_ATmega2560__ //arduino mega
  #define led_receive 40
  #define led_send 42
  #define led_info 44
#else //arduino uno/nano
  #define led_receive 40
  #define led_send 42
  #define led_info 44
#endif

//      VCC   ------------D|-----------/\/\/\/\ -----------------  Arduino PIN
//                        LED       Resistor 270-510R

/*-------------DEFINE THE MODULES YOU WANT BELOW----------------*/
//Addons and module management, comment the Z line and the config file if you don't use
#if defined(ESP8266) // for nodemcu, weemos, esp8266, and Arduino mega
  #define ZgatewayRF "RF"
  //#define ZgatewayLORA "LORA" (not tested yet)
  #define ZgatewayRF315 "RF315"
  #define ZgatewaySRFB "SRFB"
  #define ZgatewayRF2 "RF2"
  #define ZgatewayIR "IR"
  #define ZgatewayBT "BT"
  #define Zgateway2G "2G"
  #define ZgatewayPilight "Pilight"
  #define ZactuatorONOFF "ONOFF"
  #define ZsensorINA226 "INA226"
  #define ZsensorHCSR501 "HCSR501"
  #define ZsensorADC "ADC"
  #define ZsensorBH1750 "BH1750"
  #define ZsensorBME280 "BME280"
  #define ZsensorTSL2561 "TSL2561"
  #define ZsensorDHT "DHT"// If you uncomment this you can't use I2C due to the fact that I2C use also D1
  #define ZgatewayRFM69 "RFM69"// If you uncomment this you can't use RF and BT due to the fact that RF use also D8 and BT use also D6/D7
  #define ZsensorGPIOInput "GPIOInput"
  #define ZsensorGPIOKeyCode "GPIOKeyCode"
  #define ZmqttDiscovery "HADiscovery"//ESP8266, Arduino, EPS32, Sonoff RF Bridge
  #define jsonPublishing true
  #define simplePublishing true
#elif ESP32
  #define ZgatewayRF "RF"
  #define ZgatewayLORA "LORA"
  #define ZgatewayRF "RF315"
  #define ZgatewayRF2 "RF2"
  //#define Zgateway2G (not tested yet)
  #define ZgatewayIR "IR"
  #define ZgatewayBT "BT"
  #define ZactuatorONOFF "ONOFF"
  #define ZsensorINA226  "INA226"   //ESP8266, Arduino, ESP32
  #define ZsensorHCSR501 "HCSR501"  //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
  #define ZsensorADC     "ADC"      //ESP8266, Arduino, ESP32
  #define ZsensorBH1750  "BH1750"   //ESP8266, Arduino, ESP32
  #define ZsensorBME280  "BME280"   //ESP8266, Arduino, ESP32
  #define ZsensorTSL2561 "TSL2561"
  #define ZsensorDHT "DHT"
  #define ZgatewayRFM69 "RFM69"
  #define ZsensorGPIOInput "GPIOInput"
  #define ZsensorGPIOKeyCode "GPIOKeyCode"
  #define ZmqttDiscovery "HADiscovery"//ESP8266, Arduino, EPS32, Sonoff RF Bridge
  #define jsonPublishing true
  #define simplePublishing true
#elif defined(__AVR_ATmega1280__)
  #define ZgatewayRF "RF"
  //#define ZgatewayLORA "LORA" (not tested yet)
  #define ZgatewayRF315 "RF315"
  #define ZgatewaySRFB "SRFB"
  #define ZgatewayRF2 "RF2"
  #define ZgatewayIR "IR"
  #define ZgatewayBT "BT"
  #define Zgateway2G "2G"
  //#define ZgatewayPilight "Pilight"
  #define ZactuatorONOFF "ONOFF"
  #define ZsensorINA226 "INA226"
  #define ZsensorHCSR501 "HCSR501"
  #define ZsensorADC "ADC"
  #define ZsensorBH1750 "BH1750"
  #define ZsensorBME280 "BME280"
  #define ZsensorTSL2561 "TSL2561"
  #define ZsensorDHT "DHT"// If you uncomment this you can't use I2C due to the fact that I2C use also D1
  #define ZgatewayRFM69 "RFM69"// If you uncomment this you can't use RF and BT due to the fact that RF use also D8 and BT use also D6/D7
  #define ZsensorGPIOInput "GPIOInput"
  #define ZsensorGPIOKeyCode "GPIOKeyCode"
  #define ZmqttDiscovery "HADiscovery"//ESP8266, Arduino, EPS32, Sonoff RF Bridge
  #define jsonPublishing true
  #define simplePublishing true
#else // for arduino Uno
  #define ZgatewayRF "RF"
  //#define ZgatewayLORA "LORA" (not tested yet)
  //#define Zgateway2G  (not tested yet)
  //#define ZgatewayRF2 // too big for UNO
  //#define ZgatewayIR
  //#define ZgatewayBT
  //#define ZactuatorONOFF
  //#define ZsensorINA226
  //#define ZsensorDHT
  //#define ZsensorBH1750
  //#define ZsensorBME280
  //#define ZsensorHCSR501
  //#define ZsensorADC
  //#define ZgatewayRFM69 //not tested
  //#define ZsensorGPIOInput //not tested
  //#define ZsensorGPIOKeyCode "GPIOKeyCode" //not tested
  #define jsonPublishing true
  //#define simplePublishing true
#endif
/*----------------------------OTHER PARAMETERS-----------------------------*/
#ifdef ZgatewaySRFB
  #define SERIAL_BAUD 19200
#else
  #define SERIAL_BAUD 115200
#endif
/*-------------------CHANGING THEM IS NOT COMPULSORY-----------------------*/
/*--------------MQTT general topics-----------------*/
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#define subjectMQTTtoX  Base_Topic Gateway_Name "/commands/#"
#define subjectMultiGTWKey "toMQTT"
#define subjectGTWSendKey "MQTTto"

#define valueAsASubject true

//variables to avoid duplicates
#define time_avoid_duplicate 3000 // if you want to avoid duplicate mqtt message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates

#ifdef ESP32
  //#define multiCore //uncomment to use multicore function of ESP32 for BLE
#endif
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  #define JSON_MSG_BUFFER 512 // Json message max buffer size, don't put 1024 or higher it is causing unexpected behaviour on ESP8266
#else // boards with smaller memory
  #define JSON_MSG_BUFFER 64 // Json message max buffer size, don't put 1024 or higher it is causing unexpected behaviour on ESP8266
#endif
#define TimeBetweenReadingSYS 30000 // time between system readings (like memory)
#define subjectSYStoMQTT  Base_Topic Gateway_Name "/SYStoMQTT"
/*-------------------ACTIVATE TRACES----------------------*/
#define TRACE 0  // 0= trace off 1 = trace on
