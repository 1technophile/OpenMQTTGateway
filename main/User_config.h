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

// Uncomment to use the mac address in the format of 112233445566 as the gateway name
// Any definition of Gateway_Name will be ignored. The Gateway_Short_name _ MAC will be used as the access point name.
//#define USE_MAC_AS_GATEWAY_NAME
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

//#define NetworkAdvancedSetup true //uncomment if you want to set advanced network parameters, not uncommented you can set the IP and mac only
#ifdef NetworkAdvancedSetup
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

//#define ESP32_ETHERNET=true // Uncomment to use Ethernet module on ESP32 Ethernet gateway and adapt the settings to your board below, the default parameter are for OLIMEX ESP32 Gateway
#ifdef ESP32_ETHERNET
#  ifndef ETH_PHY_ADDR
#    define ETH_PHY_ADDR 0
#  endif
#  ifndef ETH_PHY_TYPE
#    define ETH_PHY_TYPE ETH_PHY_LAN8720
#  endif
#  ifndef ETH_PHY_POWER
#    define ETH_PHY_POWER 12
#  endif
#  ifndef ETH_PHY_MDC
#    define ETH_PHY_MDC 23
#  endif
#  ifndef ETH_PHY_MDIO
#    define ETH_PHY_MDIO 18
#  endif
#  ifndef ETH_CLK_MODE
#    define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#  endif
#endif

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
#ifndef WM_DEBUG_LEVEL
#  define WM_DEBUG_LEVEL 1 // valid values are: DEBUG_ERROR = 0, DEBUG_NOTIFY = 1, DEBUG_VERBOSE = 2, DEBUG_DEV = 3, DEBUG_MAX = 4
#endif
//#define WIFIMNG_HIDE_MQTT_CONFIG //Uncomment so as to hide MQTT setting from Wifi manager page

/*-------------DEFINE YOUR ADVANCED NETWORK PARAMETERS BELOW----------------*/
//#define MDNS_SD //uncomment if you  want to use mdns for discovering automatically your ip server, please note that MDNS with ESP32 can cause the BLE to not work
#define maxConnectionRetry     10 //maximum MQTT connection attempts before going to wifimanager setup if never connected once
#define maxConnectionRetryWifi 5 //maximum Wifi connection attempts with existing credential at start (used to bypass ESP32 issue on wifi connect)
#define maxRetryWatchDog       11 //maximum Wifi or mqtt re-connection attempts before restarting

//set minimum quality of signal so it ignores AP's under that quality
#define MinimumWifiSignalQuality 8

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#  define parameters_size     60
#  define mqtt_topic_max_size 100
#  ifndef mqtt_max_packet_size
#    ifdef MQTT_HTTPS_FW_UPDATE
#      define mqtt_max_packet_size 2560
#    else
#      define mqtt_max_packet_size 1024
#    endif
#  endif
#else
#  define parameters_size      30
#  define mqtt_topic_max_size  50
#  define mqtt_max_packet_size 128
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
#  define MQTT_PORT "1883"
#endif

#if defined(ESP8266) || defined(ESP32)
// The root ca certificate used for validating the MQTT broker
// The certificate must be in PEM ascii format
const char* certificate PROGMEM = R"EOF("
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
")EOF";

#  define ATTEMPTS_BEFORE_BG 10 // Number of wifi connection attempts before going to BG protocol
#  define ATTEMPTS_BEFORE_B  20 // Number of wifi connection attempts before going to B protocol

#  ifndef NTP_SERVER
#    define NTP_SERVER "pool.ntp.org"
#  endif

#  ifndef MQTT_SECURE_DEFAULT
#    define MQTT_SECURE_DEFAULT false
#  endif

#  ifndef AWS_IOT
#    define AWS_IOT false
#  endif

//#  define MQTT_HTTPS_FW_UPDATE //uncomment to enable updating via mqtt message.

#  ifdef MQTT_HTTPS_FW_UPDATE
// If used, this should be set to the root CA certificate of the server hosting the firmware.
// The certificate must be in PEM ascii format.
// The default certificate is for github.
const char* OTAserver_cert PROGMEM = R"EOF("
-----BEGIN CERTIFICATE-----
MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j
ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL
MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3
LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug
RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm
+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW
PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM
xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB
Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3
hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg
EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF
MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA
FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec
nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z
eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF
hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2
Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe
vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep
+OkuE6N36B9K
-----END CERTIFICATE-----
")EOF";

#    ifndef MQTT_HTTPS_FW_UPDATE_USE_PASSWORD
#      define MQTT_HTTPS_FW_UPDATE_USE_PASSWORD 1 // Set this to 0 if not using TLS connection to MQTT broker to prevent clear text passwords being sent.
#    endif
#  endif

#  ifndef MQTT_SECURE_SELF_SIGNED
#    define MQTT_SECURE_SELF_SIGNED 0
#  endif

#  ifndef MQTT_SECURE_SELF_SIGNED_CLIENT
#    define MQTT_SECURE_SELF_SIGNED_CLIENT 1 // If using a self signed certificate for the broker and not using client certificates set this to false or 0
#  endif

#  ifndef MQTT_SECURE_SELF_SIGNED_INDEX_DEFAULT
#    define MQTT_SECURE_SELF_SIGNED_INDEX_DEFAULT 0
#  endif

#  if MQTT_SECURE_SELF_SIGNED
const char* ss_server_cert PROGMEM = R"EOF("
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
")EOF";

const char* ss_client_cert PROGMEM = R"EOF("
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
")EOF";

const char* ss_client_key PROGMEM = R"EOF("
-----BEGIN RSA PRIVATE KEY-----
...
-----END RSA PRIVATE KEY-----
")EOF";

struct ss_certs {
  const char* server_cert;
  const char* client_cert;
  const char* client_key;
};

struct ss_certs certs_array[2] = {
    {ss_server_cert, ss_client_cert, ss_client_key},
    {ss_server_cert, ss_client_cert, ss_client_key}};

static_assert(MQTT_SECURE_SELF_SIGNED_INDEX_DEFAULT < (sizeof(certs_array) / sizeof(ss_certs)),
              "Invalid MQTT self signed default index");
#  endif
#endif

/*------------------DEEP SLEEP parameters ------------------*/
//DEFAULT_LOW_POWER_MODE 0 to normal mode (no power consumption optimisations)
//DEFAULT_LOW_POWER_MODE 1 to activate deep sleep
//DEFAULT_LOW_POWER_MODE 2 to activate deep sleep (LCD is turned OFF)
#ifdef ESP32
#  ifndef DEFAULT_LOW_POWER_MODE
#    define DEFAULT_LOW_POWER_MODE 0
#  endif
int lowpowermode = DEFAULT_LOW_POWER_MODE;
#endif

/*-------------DEFINE THE MODULES YOU WANT BELOW----------------*/
//Addons and module management, uncomment the Z line corresponding to the module you want to use

//#define ZgatewayRF     "RF"       //ESP8266, Arduino, ESP32
//#define ZgatewayIR     "IR"       //ESP8266, Arduino,         Sonoff RF Bridge
//#define ZgatewayLORA   "LORA"       //ESP8266, Arduino, ESP32
//#define ZgatewayPilight "Pilight" //ESP8266, Arduino, ESP32
//#define ZgatewayWeatherStation "WeatherStation" //ESP8266, Arduino, ESP32
//#define ZgatewayGFSunInverter "GFSunInverter"   //ESP32
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
//#define ZboardM5STICKCP "M5StickCP"
//#define ZboardM5STACK  "M5STACK"
//#define ZboardM5TOUGH  "M5TOUGH"
//#define ZradioCC1101   "CC1101"   //ESP8266, ESP32
//#define ZactuatorPWM   "PWM"      //ESP8266, ESP32
//#define ZsensorSHTC3 "SHTC3" //ESP8266, Arduino, ESP32,  Sonoff RF Bridge
//#define ZactuatorSomfy "Somfy"    //ESP8266, Arduino, ESP32
//#define ZgatewayRS232   "RS232"  //ESP8266, Arduino, ESP32

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
#ifndef LED_SEND_RECEIVE
#  ifdef ESP8266
#    define LED_SEND_RECEIVE 40
#  elif ESP32
#    define LED_SEND_RECEIVE 40
#  elif __AVR_ATmega2560__ //arduino mega
#    define LED_SEND_RECEIVE 40
#  else //arduino uno/nano
#    define LED_SEND_RECEIVE 40
#  endif
#endif
#ifndef LED_SEND_RECEIVE_ON
#  define LED_SEND_RECEIVE_ON HIGH
#endif
#ifndef LED_ERROR
#  ifdef ESP8266
#    define LED_ERROR 42
#  elif ESP32
#    define LED_ERROR 42
#  elif __AVR_ATmega2560__ //arduino mega
#    define LED_ERROR 42
#  else //arduino uno/nano
#    define LED_ERROR 42
#  endif
#endif
#ifndef LED_ERROR_ON
#  define LED_ERROR_ON HIGH
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
#  ifndef SERIAL_BAUD
#    define SERIAL_BAUD 115200
#  endif
#endif
/*--------------MQTT general topics-----------------*/
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#define subjectMQTTtoX     "/commands/#"
#define subjectMultiGTWKey "toMQTT"
#define subjectGTWSendKey  "MQTTto"
#define subjectFWUpdate    "firmware_update"

// key used for launching commands to the gateway
#define restartCmd "restart"
#define eraseCmd   "erase"
#define statusCmd  "status"

// uncomment the line below to integrate msg value into the subject when receiving
//#define valueAsASubject true

#if defined(ESP32)
#  define JSON_MSG_BUFFER    768
#  define SIGNAL_SIZE_UL_ULL uint64_t
#  define STRTO_UL_ULL       strtoull
#elif defined(ESP8266)
#  define JSON_MSG_BUFFER    512 // Json message max buffer size, don't put 768 or higher it is causing unexpected behaviour on ESP8266
#  define SIGNAL_SIZE_UL_ULL uint64_t
#  define STRTO_UL_ULL       strtoull
#elif defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#  define JSON_MSG_BUFFER    512 // Json message max buffer size, don't put 1024 or higher
#  define SIGNAL_SIZE_UL_ULL uint64_t
#  define STRTO_UL_ULL       strtoul
#else // boards with smaller memory
#  define JSON_MSG_BUFFER    64 // Json message max buffer size, don't put 1024 or higher
#  define SIGNAL_SIZE_UL_ULL uint32_t
#  define STRTO_UL_ULL       strtoul
#endif

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation)
// variable to avoid duplicates
#  ifndef time_avoid_duplicate
#    define time_avoid_duplicate 3000 // if you want to avoid duplicate mqtt message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates
#  endif
#endif

#define TimeBetweenReadingSYS        120 // time between (s) system readings (like memory)
#define TimeLedON                    1 // time LED are ON
#define InitialMQTTConnectionTimeout 10 // time estimated (s) before the board is connected to MQTT
#define subjectSYStoMQTT             "/SYStoMQTT"
#define subjectMQTTtoSYSset          "/commands/MQTTtoSYS/config"

/*-------------------DEFINE LOG LEVEL----------------------*/
#ifndef LOG_LEVEL
#  define LOG_LEVEL LOG_LEVEL_NOTICE
#endif

#endif
