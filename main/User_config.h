/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send signals corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received signals
  
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
#ifndef user_config_h
#define user_config_h
/*-------------------VERSION----------------------*/
#define OMG_VERSION "version_tag"

/*-------------CONFIGURE WIFIMANAGER-------------(only ESP8266 & SONOFF RFBridge)*/
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
/*-------------DEFINE GATEWAY NAME BELOW IT CAN ALSO BE DEFINED IN platformio.ini----------------*/
#ifndef Gateway_Name
  #define Gateway_Name "OpenMQTTGateway"
#endif

#define Base_Topic "home/"

/*-------------DEFINE YOUR  NETWORK PARAMETERS BELOW----------------*/

//#define NetworkAdvancedSetup true //uncomment if you want to set advanced network parameters for arduino boards, not uncommented you can set the IP and mac only
#ifdef NetworkAdvancedSetup // for arduino boards advanced config
  #if defined(ESP8266)||defined(ESP32)
    const byte ip[] = { 192, 168, 1, 99 }; //ip adress of the gateway, already defined for arduino below
  #endif
  const byte gateway[] = { 0, 0, 0, 0 }; 
  const byte Dns[] = { 0, 0, 0, 0 }; 
  const byte subnet[] = { 255, 255, 255, 0 }; 
#endif

#if defined(ESP8266)||defined(ESP32)  // for nodemcu, weemos and esp8266
  //#define ESPWifiManualSetup true //uncomment you don't want to use wifimanager for your credential settings on ESP
#else // for arduino boards
  const byte ip[] = { 192, 168, 1, 99 }; 
  const byte mac[] = {  0xDE, 0xED, 0xBA, 0xFE, 0x54, 0x95 }; //W5100 ethernet shield mac adress
#endif

#if defined(ESPWifiManualSetup) // for nodemcu, weemos and esp8266
  #define wifi_ssid "wifi ssid"
  #define wifi_password "wifi password"
#endif

#define WifiManager_password "your_password" //this is going to be the WPA2-PSK password for the initial setup access point 
#define WifiManager_ssid Gateway_Name //this is the network name of the initial setup access point
#define WifiManager_ConfigPortalTimeOut 120
#define WifiManager_TimeOut 5

/*-------------DEFINE YOUR ADVANCED NETWORK PARAMETERS BELOW----------------*/
//#define MDNS_SD //uncomment if you  want to use mdns for discovering automatically your ip server, please note that MDNS with ESP32 can cause the BLE to not work
#define maxConnectionRetry 10 //maximum MQTT connection attempts before going to wifimanager setup if never connected once

//set minimum quality of signal so it ignores AP's under that quality
#define MinimumWifiSignalQuality 8

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
//MQTT Parameters definition
//#define mqtt_server_name "www.mqtt_broker.com" // instead of defining the server by its IP you can define it by its name, uncomment this line and set the correct MQTT server host name
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  #define parameters_size 20
  #define mqtt_topic_max_size 100
#else
  #define parameters_size 15
  #define mqtt_topic_max_size 50
#endif
char mqtt_user[parameters_size] = "your_username"; // not compulsory only if your broker needs authentication
char mqtt_pass[parameters_size] = "your_password"; // not compulsory only if your broker needs authentication
char mqtt_server[parameters_size] = "192.168.1.17";
char mqtt_port[6] = "1883";
char mqtt_topic[mqtt_topic_max_size] = Base_Topic;
char gateway_name[parameters_size * 2] = Gateway_Name;
//uncomment the line below to integrate msg value into the subject when receiving
//#define valueAsASubject true

#if defined(ESP8266)||defined(ESP32) 
  #include "esp_wifi.h"
  #define ATTEMPTS_BEFORE_BG  10 // Number of wifi connection attempts before going to BG protocol
  #define ATTEMPTS_BEFORE_B   20 // Number of wifi connection attempts before going to B protocol
#endif

// WIFI mode, uncomment to force a wifi mode, if not uncommented the ESP will connect without a mode forced
// if there is a reconnection issue it will try to connect with G mode and if not working with B mode
#ifdef ESP32
  uint8_t wifiProtocol = 0; // default mode, automatic selection
  //uint8_t wifiProtocol = WIFI_PROTOCOL_11B;
  //uint8_t wifiProtocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G; // can't have only one https://github.com/espressif/esp-idf/issues/702
  //uint8_t wifiProtocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N; // can't have only one https://github.com/espressif/esp-idf/issues/702
#elif ESP8266
  uint8_t wifiProtocol = 0; // default mode, automatic selection
  //uint8_t wifiProtocol = WIFI_PHY_MODE_11B;
  //uint8_t wifiProtocol = WIFI_PHY_MODE_11G;
  //uint8_t wifiProtocol = WIFI_PHY_MODE_11N;
#endif

/*-------------DEFINE THE MODULES YOU WANT BELOW----------------*/
//Addons and module management, uncomment the Z line corresponding to the module you want to use

//#define ZgatewayRF     "RF"       //ESP8266, Arduino, ESP32
//#define ZgatewayIR     "IR"       //ESP8266, Arduino,         Sonoff RF Bridge
//#define ZgatewayLORA   "LORA"       //ESP8266, Arduino, ESP32
//#define ZgatewayPilight "Pilight" //ESP8266, Arduino, ESP32
//#define ZgatewayWeatherStation "WeatherStation" //ESP8266, Arduino, ESP32
//#define ZgatewayBT     "BT"       //ESP8266, ESP32
//#define ZgatewayRF2    "RF2"      //ESP8266, Arduino, ESP32
//#define ZgatewaySRFB   "SRFB"     //                          Sonoff RF Bridge
//#define Zgateway2G     "2G"       //ESP8266, Arduino, ESP32
//#define ZgatewayRFM69  "RFM69"    //ESP8266, Arduino, ESP32
//#define ZactuatorONOFF "ONOFF"    //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define ZsensorINA226  "INA226"   //ESP8266, Arduino, ESP32
//#define ZsensorHCSR04  "HCSR04"   //ESP8266, Arduino, ESP32
//#define ZsensorHCSR501 "HCSR501"  //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define ZsensorADC     "ADC"      //ESP8266, Arduino, ESP32
//#define ZsensorBH1750  "BH1750"   //ESP8266, Arduino, ESP32
//#define ZsensorTSL2561 "TSL2561"  //ESP8266, Arduino, ESP32
//#define ZsensorBME280  "BME280"   //ESP8266, Arduino, ESP32
//#define ZsensorDHT     "DHT"      //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define ZsensorDS1820  "DS1820"   //ESP8266, Arduino, ESP32
//#define ZsensorGPIOKeyCode "GPIOKeyCode" //ESP8266, Arduino, ESP32
//#define ZsensorGPIOInput "GPIOInput" //ESP8266, Arduino, ESP32
//#define ZmqttDiscovery "HADiscovery"//ESP8266, Arduino, ESP32, Sonoff RF Bridge
//#define ZactuatorFASTLED "FASTLED"  //ESP8266, Arduino, ESP32, Sonoff RF Bridge

/*-------------DEFINE YOUR MQTT ADVANCED PARAMETERS BELOW----------------*/
#define version_Topic  "/version"
#define will_Topic  "/LWT"
#define will_QoS 0
#define will_Retain true
#define will_Message "offline"
#define Gateway_AnnouncementMsg "online"

#define jsonPublishing true //comment if you don't want to use Json  publishing  (one topic for all the parameters)
//example home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4 {"rssi":-63,"servicedata":"fe0000000000000000000000000000000000000000"}
#define jsonReceiving true //comment if you don't want to use Json  reception analysis

//#define simplePublishing true //comment if you don't want to use simple publishing (one topic for one parameter)
//example 
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/rssi -63.0
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/servicedata fe0000000000000000000000000000000000000000
//#define simpleReceiving true //comment if you don't want to use old way reception analysis

/*-------------DEFINE YOUR OTA PARAMETERS BELOW----------------*/
#define ota_hostname Gateway_Name
#define ota_password "OTAPASSWORD"
#define ota_port 8266

/*-------------DEFINE PINs FOR STATUS LEDs----------------*/
#ifndef led_receive
  #ifdef ESP8266
    #define led_receive 40
  #elif ESP32
    #define led_receive 40
  #elif __AVR_ATmega2560__ //arduino mega
    #define led_receive 40
  #else //arduino uno/nano
    #define led_receive 40
  #endif
#endif
#ifndef led_send
  #ifdef ESP8266
    #define led_send 42
  #elif ESP32
    #define led_send 42
  #elif __AVR_ATmega2560__ //arduino mega
    #define led_send 42
  #else //arduino uno/nano
    #define led_send 42
  #endif
#endif
#ifndef led_info
  #ifdef ESP8266
    #define led_info 44
  #elif ESP32
    #define led_info 44
  #elif __AVR_ATmega2560__ //arduino mega
    #define led_info 44
  #else //arduino uno/nano
    #define led_info 44
  #endif
#endif

#ifndef TRIGGER_PIN
    #ifdef ESP8266
        #define TRIGGER_PIN 14 // pin D5 as full reset button (long press >10s)
    #elif ESP32
        #define TRIGGER_PIN 0 // boot button as full reset button (long press >10s)
    #endif
#endif

//      VCC   ------------D|-----------/\/\/\/\ -----------------  Arduino PIN
//                        LED       Resistor 270-510R

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
#define subjectMQTTtoX      "/commands/#"
#define subjectMultiGTWKey  "toMQTT"
#define subjectGTWSendKey   "MQTTto"

// key used for launching commands to the gateway
#define restartCmd          "restart"
#define eraseCmd            "erase"

// define if we concatenate the values into the topic
//#define valueAsASubject true

//variables to avoid duplicates
#define time_avoid_duplicate 3000 // if you want to avoid duplicate mqtt message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates

#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  #define JSON_MSG_BUFFER 512 // Json message max buffer size, don't put 1024 or higher it is causing unexpected behaviour on ESP8266
  #define ARDUINOJSON_USE_LONG_LONG 1
#else // boards with smaller memory
  #define JSON_MSG_BUFFER 64 // Json message max buffer size, don't put 1024 or higher it is causing unexpected behaviour on ESP8266
#endif

#define TimeBetweenReadingSYS 120000 // time between system readings (like memory)
#define subjectSYStoMQTT  "/SYStoMQTT"
#define subjectMQTTtoSYSset "/commands/MQTTtoSYS/config"

/*-------------------DEFINE LOG LEVEL----------------------*/
#define LOG_LEVEL LOG_LEVEL_VERBOSE

#endif
