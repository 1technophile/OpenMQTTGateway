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
#ifndef OMG_VERSION
#  define OMG_VERSION "version_tag"
#endif

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
#  define Gateway_Name "OpenMQTTGateway"
#endif
#ifndef Gateway_Short_Name
#  define Gateway_Short_Name "OMG"
#endif

#ifndef Base_Topic
#  define Base_Topic "home/"
#endif

/*-------------DEFINE YOUR NETWORK PARAMETERS BELOW----------------*/

//#define NetworkAdvancedSetup true //uncomment if you want to set advanced network parameters for arduino boards, not uncommented you can set the IP and mac only
#ifdef NetworkAdvancedSetup // for arduino boards advanced config
#  if defined(ESP8266) || defined(ESP32)
const byte ip[] = {192, 168, 1, 99}; //ip adress of the gateway, already defined for arduino below
#  endif
const byte gateway[] = {0, 0, 0, 0};
const byte Dns[] = {0, 0, 0, 0};
const byte subnet[] = {255, 255, 255, 0};
#endif

#if defined(ESP8266) || defined(ESP32) // for nodemcu, weemos and esp8266
//#  define ESPWifiManualSetup true //uncomment you don't want to use wifimanager for your credential settings on ESP
#else // for arduino boards
const byte ip[] = {192, 168, 1, 99};
const byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0x54, 0x95}; //W5100 ethernet shield mac adress
#endif

//#define ESP32_ETHERNET=true // Uncomment to use Ethernet module on OLIMEX ESP32 Ethernet gateway

#if defined(ESPWifiManualSetup) // for nodemcu, weemos and esp8266
#  ifndef wifi_ssid
#    define wifi_ssid "wifi ssid"
#  endif
#  ifndef wifi_password
#    define wifi_password "wifi password"
#  endif
#endif

#ifndef WifiManager_password
#  define WifiManager_password "your_password" //this is going to be the WPA2-PSK password for the initial setup access point
#endif
#ifndef WifiManager_ssid
#  define WifiManager_ssid Gateway_Name //this is the network name of the initial setup access point
#endif
#ifndef WifiManager_ConfigPortalTimeOut
#  define WifiManager_ConfigPortalTimeOut 120
#endif
#ifndef WifiManager_TimeOut
#  define WifiManager_TimeOut 5
#endif

/*-------------DEFINE YOUR ADVANCED NETWORK PARAMETERS BELOW----------------*/
//#define MDNS_SD //uncomment if you  want to use mdns for discovering automatically your ip server, please note that MDNS with ESP32 can cause the BLE to not work
#define maxConnectionRetry     10 //maximum MQTT connection attempts before going to wifimanager setup if never connected once
#define maxConnectionRetryWifi 5 //maximum Wifi connection attempts with existing credential at start (used to bypass ESP32 issue on wifi connect)

//set minimum quality of signal so it ignores AP's under that quality
#define MinimumWifiSignalQuality 8

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#  define parameters_size      30
#  define mqtt_topic_max_size  100
#  define mqtt_max_packet_size 1024
#else
#  define parameters_size      15
#  define mqtt_topic_max_size  50
#  define mqtt_max_packet_size 128
#endif

// activate the use of TLS for secure connection to the MQTT broker
// MQTT_SERVER must be set to the Common Name (CN) of the broker's certificate
//#define SECURE_CONNECTION

#ifdef SECURE_CONNECTION
#  define MQTT_DEFAULT_PORT "8883"
#else
#  define MQTT_DEFAULT_PORT "1883"
#endif

#ifndef MQTT_USER
#  define MQTT_USER "your_username"
#endif
#ifndef MQTT_PASS
#  define MQTT_PASS "your_password"
#endif
#ifndef MQTT_SERVER
#  define MQTT_SERVER "192.168.1.17"
#endif
#ifndef MQTT_PORT
#  define MQTT_PORT MQTT_DEFAULT_PORT
#endif

#ifdef SECURE_CONNECTION
#  if defined(ESP8266) || defined(ESP32)
#    if defined(ESP32)
#      define CERT_ATTRIBUTE
#    elif defined(ESP8266)
#      define CERT_ATTRIBUTE PROGMEM
#    endif

// The root ca certificate used for validating the MQTT broker
// The certificate must be in PEM ascii format
const char* certificate CERT_ATTRIBUTE = R"EOF("
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
")EOF";

// specify a NTP server here or else the NTP server from DHCP is used
//#    define NTP_SERVER "pool.ntp.org"
#  else
#    error "only ESP8266 and ESP32 support SECURE_CONNECTION with TLS"
#  endif
#endif

#if defined(ESP8266) || defined(ESP32)
#  define ATTEMPTS_BEFORE_BG 10 // Number of wifi connection attempts before going to BG protocol
#  define ATTEMPTS_BEFORE_B  20 // Number of wifi connection attempts before going to B protocol
#endif

/*------------------DEEP SLEEP parameters ------------------*/
//DEFAULT_LOW_POWER_MODE 0 to normal mode (no power consumption optimisations)
//DEFAULT_LOW_POWER_MODE 1 to activate deep sleep
//DEFAULT_LOW_POWER_MODE 2 to activate deep sleep (LCD is turned OFF)
#ifdef ESP32
#  ifndef DEFAULT_LOW_POWER_MODE
#    define DEFAULT_LOW_POWER_MODE 0
#  endif
int low_power_mode = DEFAULT_LOW_POWER_MODE;
#endif

// WIFI mode, uncomment to force a wifi mode, if not uncommented the ESP will connect without a mode forced
// if there is a reconnection issue it will try to connect with G mode and if not working with B mode
#ifdef ESP32
#  include "esp_wifi.h"
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
//#define ZsensorHTU21   "HTU21"    //ESP8266, Arduino, ESP32
//#define ZsensorDHT     "DHT"      //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define ZsensorDS1820  "DS1820"   //ESP8266, Arduino, ESP32
//#define ZsensorGPIOKeyCode "GPIOKeyCode" //ESP8266, Arduino, ESP32
//#define ZsensorGPIOInput "GPIOInput" //ESP8266, Arduino, ESP32
//#define ZmqttDiscovery "HADiscovery"//ESP8266, Arduino, ESP32, Sonoff RF Bridge
//#define ZactuatorFASTLED "FASTLED" //ESP8266, Arduino, ESP32, Sonoff RF Bridge
//#define ZboardM5STICKC "M5StickC"
//#define ZboardM5STACK  "ZboardM5STACK"
//#define ZradioCC1101   "CC1101"   //ESP8266, ESP32
//#define ZactuatorPWM   "PWM"      //ESP8266, ESP32

/*-------------DEFINE YOUR MQTT ADVANCED PARAMETERS BELOW----------------*/
#ifndef version_Topic
#  define version_Topic "/version"
#endif
#ifndef will_Topic
#  define will_Topic "/LWT"
#endif
#ifndef will_QoS
#  define will_QoS 0
#endif
#ifndef will_Retain
#  define will_Retain true
#endif
#ifndef will_Message
#  define will_Message "offline"
#endif
#ifndef Gateway_AnnouncementMsg
#  define Gateway_AnnouncementMsg "online"
#endif

#ifndef jsonPublishing
#  define jsonPublishing true //comment if you don't want to use Json  publishing  (one topic for all the parameters)
#endif
//example home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4 {"rssi":-63,"servicedata":"fe0000000000000000000000000000000000000000"}
#ifndef jsonReceiving
#  define jsonReceiving true //comment if you don't want to use Json  reception analysis
#endif

//#define simplePublishing true //comment if you don't want to use simple publishing (one topic for one parameter)
//example
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/rssi -63.0
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/servicedata fe0000000000000000000000000000000000000000
//#define simpleReceiving true //comment if you don't want to use old way reception analysis

/*-------------DEFINE YOUR OTA PARAMETERS BELOW----------------*/
#ifndef ota_hostname
#  define ota_hostname Gateway_Name
#endif
#ifndef ota_password
#  define ota_password "OTAPASSWORD"
#endif
#ifndef ota_port
#  define ota_port 8266
#endif

/*-------------DEFINE PINs FOR STATUS LEDs----------------*/
#ifndef LED_RECEIVE
#  ifdef ESP8266
#    define LED_RECEIVE 40
#  elif ESP32
#    define LED_RECEIVE 40
#  elif __AVR_ATmega2560__ //arduino mega
#    define LED_RECEIVE 40
#  else //arduino uno/nano
#    define LED_RECEIVE 40
#  endif
#endif
#ifndef LED_RECEIVE_ON
#  define LED_RECEIVE_ON HIGH
#endif
#ifndef LED_SEND
#  ifdef ESP8266
#    define LED_SEND 42
#  elif ESP32
#    define LED_SEND 42
#  elif __AVR_ATmega2560__ //arduino mega
#    define LED_SEND 42
#  else //arduino uno/nano
#    define LED_SEND 42
#  endif
#endif
#ifndef LED_SEND_ON
#  define LED_SEND_ON HIGH
#endif
#ifndef LED_INFO
#  ifdef ESP8266
#    define LED_INFO 44
#  elif ESP32
#    define LED_INFO 44
#  elif __AVR_ATmega2560__ //arduino mega
#    define LED_INFO 44
#  else //arduino uno/nano
#    define LED_INFO 44
#  endif
#endif
#ifndef LED_INFO_ON
#  define LED_INFO_ON HIGH
#endif

#ifdef ESP8266
//#  define TRIGGER_GPIO 14 // pin D5 as full reset button (long press >10s)
#elif ESP32
//#  define TRIGGER_GPIO 0 // boot button as full reset button (long press >10s)
#endif

//      VCC   ------------D|-----------/\/\/\/\ -----------------  Arduino PIN
//                        LED       Resistor 270-510R

/*----------------------------OTHER PARAMETERS-----------------------------*/
/*-------------------CHANGING THEM IS NOT COMPULSORY-----------------------*/
/*----------------------------USER PARAMETERS-----------------------------*/
#ifdef ZgatewaySRFB
#  define SERIAL_BAUD 19200
#else
#  define SERIAL_BAUD 115200
#endif
/*--------------MQTT general topics-----------------*/
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#define subjectMQTTtoX     "/commands/#"
#define subjectMultiGTWKey "toMQTT"
#define subjectGTWSendKey  "MQTTto"

// key used for launching commands to the gateway
#define restartCmd "restart"
#define eraseCmd   "erase"

// uncomment the line below to integrate msg value into the subject when receiving
//#define valueAsASubject true

#if defined(ESP8266) || defined(ESP32)
#  define JSON_MSG_BUFFER    512 // Json message max buffer size, don't put 1024 or higher it is causing unexpected behaviour on ESP8266
#  define SIGNAL_SIZE_UL_ULL uint64_t
#  define STRTO_UL_ULL       strtoull
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#  define JSON_MSG_BUFFER    512 // Json message max buffer size, don't put 1024 or higher it is causing unexpected behaviour on ESP8266
#  define SIGNAL_SIZE_UL_ULL uint64_t
#  define STRTO_UL_ULL       strtoul
#else // boards with smaller memory
#  define JSON_MSG_BUFFER    64 // Json message max buffer size, don't put 1024 or higher it is causing unexpected behaviour on ESP8266
#  define SIGNAL_SIZE_UL_ULL uint32_t
#  define STRTO_UL_ULL       strtoul
#endif

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation)
// variable to avoid duplicates
#  define time_avoid_duplicate 3000 // if you want to avoid duplicate mqtt message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates
#endif

#define TimeBetweenReadingSYS        120 // time between (s) system readings (like memory)
#define TimeLedON                    0.5 // time LED are ON
#define InitialMQTTConnectionTimeout 10 // time estimated (s) before the board is connected to MQTT
#define subjectSYStoMQTT             "/SYStoMQTT"
#define subjectMQTTtoSYSset          "/commands/MQTTtoSYS/config"

/*-------------------DEFINE LOG LEVEL----------------------*/
#define LOG_LEVEL LOG_LEVEL_NOTICE

#endif
