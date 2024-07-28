/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
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

// Uncomment to use the MAC address first 4 digits in the format of 5566 as the suffix of the short gateway name.
// Any definition of Gateway_Name will be ignored. The Gateway_Short_name _ MAC will be used as the access point name.
//#define USE_MAC_AS_GATEWAY_NAME
#ifndef Gateway_Name
#  define Gateway_Name "OpenMQTTGateway"
#endif
#ifndef Gateway_Short_Name
#  define Gateway_Short_Name "OMG" // 3 characters maximum
#endif

#ifndef Base_Topic
#  define Base_Topic "home/"
#endif

/*-------------DEFINE YOUR NETWORK PARAMETERS BELOW----------------*/

//#define NetworkAdvancedSetup true //uncomment if you want to set advanced network parameters, not uncommented you can set the IP and MAC only
#ifdef NetworkAdvancedSetup
#  ifndef NET_IP
#    define NET_IP "192.168.1.99"
#  endif
#  ifndef NET_MASK
#    define NET_MASK "255.255.255.0"
#  endif
#  ifndef NET_GW
#    define NET_GW "192.168.1.1"
#  endif
#  ifndef NET_DNS
#    define NET_DNS "192.168.1.1"
#  endif
#endif

//#  define ESPWifiManualSetup true //uncomment you don't want to use wifimanager for your credential settings on ESP

//#define ESP32_ETHERNET=true // Uncomment to use Ethernet module on ESP32 Ethernet gateway and adapt the settings to your board below, the default parameter are for OLIMEX ESP32 gateway
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

//#define WM_PWD_FROM_MAC true // enable to set the password from the last 8 digits of the ESP MAC address for enhanced security, enabling this option requires to have access to the MAC address, either through a sticker or with serial monitoring
#ifndef WifiManager_ssid
#  define WifiManager_ssid Gateway_Name //this is the network name of the initial setup access point
#endif
#ifndef WifiManager_ConfigPortalTimeOut
#  define WifiManager_ConfigPortalTimeOut 240 //time in seconds for the setup portal to stay open, default 240s
#endif
#ifndef WiFi_TimeOut
#  define WiFi_TimeOut 30
#endif
#ifndef WM_DEBUG // WiFi Manager debug
#  define WM_DEBUG 1
#endif
//#define WIFIMNG_HIDE_MQTT_CONFIG //Uncomment so as to hide MQTT setting from Wifi manager page

/*-------------DEFINE YOUR ADVANCED NETWORK PARAMETERS BELOW----------------*/
//#define MDNS_SD //uncomment if you  want to use mDNS for discovering automatically your IP server, please note that mDNS with ESP32 can cause the BLE to not work
#define maxConnectionRetryNetwork 5 //maximum Wifi connection attempts with existing credential at start (used to bypass ESP32 issue on wifi connect)
#define maxRetryWatchDog          11 //maximum Wifi or MQTT re-connection attempts before restarting

//set minimum quality of signal so it ignores AP's under that quality
#define MinimumWifiSignalQuality 8

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#define parameters_size     65
#define mqtt_topic_max_size 150
#define mqtt_key_max_size   20
#ifdef MQTT_HTTPS_FW_UPDATE
#  ifndef CHECK_OTA_UPDATE
#    define CHECK_OTA_UPDATE true // enable to check for the presence of a new version for your environment on Github
#  endif
#endif

#ifndef JSON_MSG_BUFFER
#  if defined(ESP32)
#    define JSON_MSG_BUFFER 816 // adjusted to minimum size covering largest Theengs device JSON properties (RuuviTag_RAWv2)
#  elif defined(ESP8266)
#    define JSON_MSG_BUFFER 512 // Json message max buffer size, don't put 768 or higher it is causing unexpected behaviour on ESP8266, certificates handling with ESP8266 is not tested
#  endif
#  if MQTT_SECURE_DEFAULT
#    define JSON_MSG_BUFFER_MAX 2048 // Json message buffer size increased to handle certificate changes through MQTT, used for the queue and the coming MQTT messages
#  else
#    define JSON_MSG_BUFFER_MAX 898 // Minimum size for the cover MQTT discovery message
#  endif
#endif

#ifndef mqtt_max_payload_size
#  define mqtt_max_payload_size JSON_MSG_BUFFER_MAX + mqtt_topic_max_size + 10 // maximum size of the MQTT payload
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

#ifndef GeneralTimeOut
#  define GeneralTimeOut 20 // time out if a task is stuck in seconds (should be more than TimeBetweenReadingRN8209/1000) and more than 3 seconds, the WDT will reset the ESP, used also for MQTT connection
#endif
#ifndef QueueSemaphoreTimeOutTask
#  define QueueSemaphoreTimeOutTask 3000 // time out for semaphore retrieval from a task
#endif
#ifndef QueueSemaphoreTimeOutLoop
#  define QueueSemaphoreTimeOutLoop 100 // time out for semaphore retrieval from the loop
#endif

// Uncomment to use a device running TheengsGateway to decode BLE data. (https://github.com/theengs/gateway)
// Set the topic to the subscribe topic configured in the TheengGateway
// #define MQTTDecodeTopic "MQTTDecode"

#define ATTEMPTS_BEFORE_BG 10 // Number of wifi connection attempts before going to BG protocol
#define ATTEMPTS_BEFORE_B  20 // Number of wifi connection attempts before going to B protocol

#ifndef NTP_SERVER
#  define NTP_SERVER "pool.ntp.org"
#endif

#ifndef MQTT_SECURE_DEFAULT
#  define MQTT_SECURE_DEFAULT false
#endif

#ifndef MQTT_CERT_VALIDATE_DEFAULT
#  define MQTT_CERT_VALIDATE_DEFAULT false
#endif

#ifndef AWS_IOT
#  define AWS_IOT false
#endif

#ifndef MQTT_BROKER_MODE
#  define MQTT_BROKER_MODE false
#endif

#if MQTT_BROKER_MODE
// In MQTT broker mode the MQTT web config is not needed
#  define WIFIMNG_HIDE_MQTT_CONFIG true
#endif

#define GITHUB_OTA_SERVER_CERT_HASH "d4d211b4553af9fac371f24c2268d59d2b0fec6b9aa0fdbbde068f078d7daf86" // SHA256 fingerprint of the certificate used by the OTA server

#if AWS_IOT
// Enable the use of ALPN for AWS IoT Core with the port 443
const char* alpnProtocols[] = {"x-amzn-mqtt-ca", NULL};
#endif

//#  define MQTT_HTTPS_FW_UPDATE //uncomment to enable updating via MQTT message.

#ifdef MQTT_HTTPS_FW_UPDATE
// If used, this should be set to the root CA certificate of the server hosting the firmware.
#  ifdef PRIVATE_CERTS
#    include "certs/private_ota_cert.h"
#  else
#    include "certs/default_ota_cert.h"
#  endif

#  ifndef MQTT_HTTPS_FW_UPDATE_USE_PASSWORD
#    define MQTT_HTTPS_FW_UPDATE_USE_PASSWORD 1 // Set this to 0 if not using TLS connection to MQTT broker to prevent clear text passwords being sent.
#  endif
#  if DEVELOPMENTOTA
#    define OTA_JSON_URL "https://ota.openmqttgateway.com/binaries/dev/latest_version_dev.json" //OTA url used to discover new versions of the firmware from development nightly builds
#  else
#    define OTA_JSON_URL "https://ota.openmqttgateway.com/binaries/latest_version.json" //OTA url used to discover new versions of the firmware
#  endif
#  define ENTITY_PICTURE   "https://github.com/1technophile/OpenMQTTGateway/raw/development/docs/img/Openmqttgateway_logo_mini_margins.png"
#  define RELEASE_LINK_DEV "https://ota.openmqttgateway.com/binaries/dev/"
#  define RELEASE_LINK     "https://ota.openmqttgateway.com/binaries/"
#else
const char* OTAserver_cert = "";
#endif

#ifndef MQTT_SECURE_SIGNED_CLIENT
#  define MQTT_SECURE_SIGNED_CLIENT 0 // If using a signed certificate for the broker and using client certificate/key set this to true or 1
#endif

#ifndef CNT_DEFAULT_INDEX
#  define CNT_DEFAULT_INDEX 0 // Default set of connection parameters
#endif

#ifdef PRIVATE_CERTS
#  include "certs/private_client_cert.h"
#  include "certs/private_client_key.h"
#  include "certs/private_server_cert.h"
#else
#  include "certs/default_client_cert.h"
#  include "certs/default_client_key.h"
#  include "certs/default_server_cert.h"
#endif

#include <string>

#if !MQTT_BROKER_MODE
struct ss_cnt_parameters {
  std::string server_cert;
  std::string client_cert;
  std::string client_key;
  std::string ota_server_cert;
  char mqtt_server[parameters_size];
  char mqtt_port[6];
  char mqtt_user[parameters_size];
  char mqtt_pass[parameters_size];
  bool isConnectionSecure;
  bool isCertValidate;
  bool validConnection;
};

// Index 0 is used for connection parameters provided in the build that can be overloaded by WiFi Manager/Onboarding/WebUI,MQTT
#  define CNT_DEFAULT_INDEX 0
// Index 1 and more are used for connection parameters provided at runtime by MQTT
#  define cnt_parameters_array_size 3

ss_cnt_parameters cnt_parameters_array[cnt_parameters_array_size] = {
    {ss_server_cert, ss_client_cert, ss_client_key, OTAserver_cert, MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS, MQTT_SECURE_DEFAULT, MQTT_CERT_VALIDATE_DEFAULT, false},
    {"", "", "", "", MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS, MQTT_SECURE_DEFAULT, MQTT_CERT_VALIDATE_DEFAULT, false},
    {"", "", "", "", MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASS, MQTT_SECURE_DEFAULT, MQTT_CERT_VALIDATE_DEFAULT, false}};
#endif

#define MIN_CERT_LENGTH 200 // Minimum length of a certificate to be considered valid

/**
 * Ext wake for Deep-sleep for the ESP32.
 * Set the wake pin state.
 */
#ifdef ESP32_EXT0_WAKE_PIN
#  ifndef ESP32_EXT0_WAKE_PIN_STATE
#    define ESP32_EXT0_WAKE_PIN_STATE 1
#  endif
#endif

#ifdef ESP32_EXT1_WAKE_PIN
#  ifndef ESP32_EXT1_WAKE_PIN_STATE
#    define ESP32_EXT1_WAKE_PIN_STATE 1
#  endif
#endif

#ifndef DEEP_SLEEP_IN_US
#  define DEEP_SLEEP_IN_US 60000000 // 1 minute
#endif

/*------------------DEEP SLEEP parameters ------------------*/
//DEFAULT_LOW_POWER_MODE DEACTIVATED low power mode can't be used on this build to prevent bricking devices that does not support low power mode
//DEFAULT_LOW_POWER_MODE ALWAYS_ON normal mode (no power consumption optimisations)
//DEFAULT_LOW_POWER_MODE INTERVAL to activate deep sleep with intervals and action wake up
//DEFAULT_LOW_POWER_MODE ACTION to activate deep sleep with action wake up
#ifndef DEFAULT_LOW_POWER_MODE
#  define DEFAULT_LOW_POWER_MODE DEACTIVATED
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
//#define ZsensorMQ2 "MQ2"  //ESP8266, Arduino, ESP32
//#define ZsensorTEMT6000 "TEMT6000"  //ESP8266
//#define ZsensorTSL2561 "TSL2561"  //ESP8266, Arduino, ESP32
//#define ZsensorBME280  "BME280"   //ESP8266, Arduino, ESP32
//#define ZsensorHTU21   "HTU21"    //ESP8266, Arduino, ESP32
//#define ZsensorLM75   "LM75"    //ESP8266, Arduino, ESP32
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
//#define ZgatewaySERIAL   "SERIAL"  //ESP8266, Arduino, ESP32

/*-------------DEFINE YOUR MQTT ADVANCED PARAMETERS BELOW----------------*/
#ifndef will_Topic
#  define will_Topic "/LWT"
#endif
#ifndef will_QoS
#  define will_QoS 0
#endif
#ifndef will_Retain
#  define will_Retain true
#endif
#ifndef sensor_Retain
#  define sensor_Retain false
#endif
#ifndef will_Message
#  define will_Message "offline"
#endif
#ifndef Gateway_AnnouncementMsg
#  define Gateway_AnnouncementMsg "online"
#endif

#ifndef jsonPublishing
#  define jsonPublishing true //define false if you don't want to use Json publishing (one topic for all the parameters)
#endif
//example home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4 {"rssi":-63,"servicedata":"fe0000000000000000000000000000000000000000"}
#ifndef jsonReceiving
#  define jsonReceiving true //define false if you don't want to use Json  reception analysis
#endif

#ifndef simplePublishing
#  define simplePublishing false //define true if you want to use simple publishing (one topic for one parameter)
#endif
//example
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/rssi -63.0
// home/OpenMQTTGateway_ESP32_DEVKIT/BTtoMQTT/4XXXXXXXXXX4/servicedata fe0000000000000000000000000000000000000000
#ifndef simpleReceiving
#  define simpleReceiving true //define false if you don't want to use old way reception analysis
#endif
#ifndef message_UTCtimestamp
#  define message_UTCtimestamp false //define true if you want messages to be timestamped in ISO8601 UTC format (e.g.: "UTCtime"="2023-12-26T19:10:20Z")
#endif
#ifndef message_unixtimestamp
#  define message_unixtimestamp false //define true if you want messages to have an unix timestamp (e.g.: "unixtime"=1679015107)
#endif

/*-------------DEFINE YOUR OTA PARAMETERS BELOW----------------*/
#ifndef ota_hostname
#  define ota_hostname Gateway_Name
#endif
#ifndef gw_password
#  define gw_password ""
#endif
#ifndef ota_port
#  define ota_port 8266
#endif
// timeout for OTA activities
// OTA upload with no activity in this period is considered inactive
// As long as OTA upload is considered "active", we avoid rebooting e.g.
// in case of failures connecting to MQTT
#ifndef ota_timeout_millis
#  define ota_timeout_millis 30000
#endif

/*-------------ERRORS, INFOS, SEND RECEIVE Display through LED----------------*/
#ifndef RGB_INDICATORS // Management of Errors, reception/emission and informations indicators with basic LED
/*-------------DEFINE PINs FOR STATUS LEDs----------------*/
#  ifndef LED_SEND_RECEIVE
#    ifdef ESP8266
//#      define LED_SEND_RECEIVE 40
#    elif ESP32
//#      define LED_SEND_RECEIVE 40
#    endif
#  endif
#  ifndef LED_SEND_RECEIVE_ON
#    define LED_SEND_RECEIVE_ON HIGH
#  endif
#  ifndef LED_ERROR
#    ifdef ESP8266
//#      define LED_ERROR 42
#    elif ESP32
//#      define LED_ERROR 42
#    endif
#  endif
#  ifndef LED_ERROR_ON
#    define LED_ERROR_ON HIGH
#  endif
#  ifndef RGB_LED_ON_ON
#    define RGB_LED_ON_ON HIGH
#  endif
#  ifndef LED_INFO
#    ifdef ESP8266
//#      define LED_INFO 44
#    elif ESP32
//#      define LED_INFO 44
#    endif
#  endif
#  ifndef LED_INFO_ON
#    define LED_INFO_ON HIGH
#  endif
#  ifdef RGB_LED_ON
#    define SetupIndicatorError() \
      pinMode(RGB_LED_ON, OUTPUT);
#    define ONIndicatorON() digitalWrite(RGB_LED_ON, RGB_LED_ON_ON)
#  else
#    define ONIndicatorON()
#  endif
#  ifdef LED_ERROR
#    define SetupIndicatorError() \
      pinMode(LED_ERROR, OUTPUT); \
      ErrorIndicatorOFF();
#    define ErrorIndicatorON()  digitalWrite(LED_ERROR, LED_ERROR_ON)
#    define ErrorIndicatorOFF() digitalWrite(LED_ERROR, !LED_ERROR_ON)
#  else
#    define SetupIndicatorError()
#    define ErrorIndicatorON()
#    define ErrorIndicatorOFF()
#  endif
#  ifdef LED_SEND_RECEIVE
#    define SetupIndicatorSendReceive()  \
      pinMode(LED_SEND_RECEIVE, OUTPUT); \
      SendReceiveIndicatorOFF();
#    define SendReceiveIndicatorON()  digitalWrite(LED_SEND_RECEIVE, LED_SEND_RECEIVE_ON)
#    define SendReceiveIndicatorOFF() digitalWrite(LED_SEND_RECEIVE, !LED_SEND_RECEIVE_ON)
#  else
#    define SetupIndicatorSendReceive()
#    define SendReceiveIndicatorON()
#    define SendReceiveIndicatorOFF()
#  endif
#  ifdef LED_INFO
#    define SetupIndicatorInfo() \
      pinMode(LED_INFO, OUTPUT); \
      InfoIndicatorOFF();
#    define InfoIndicatorON()  digitalWrite(LED_INFO, LED_INFO_ON)
#    define InfoIndicatorOFF() digitalWrite(LED_INFO, !LED_INFO_ON)
#  else
#    define SetupIndicatorInfo()
#    define InfoIndicatorON()
#    define InfoIndicatorOFF()
#  endif
#  define CriticalIndicatorON() // Not used
#  define PowerIndicatorON()    // Not used
#  define PowerIndicatorOFF()   // Not used
#  define SetupIndicators()     // Not used
#else // Management of Errors, reception/emission and informations indicators with RGB LED
#  include <Adafruit_NeoPixel.h>
#  ifndef ANEOPIX_IND_TYPE // needs library constants
#    define ANEOPIX_IND_TYPE NEO_GRB + NEO_KHZ800 // ws2812 and alike
#  endif
Adafruit_NeoPixel leds(ANEOPIX_IND_NUM_LEDS, ANEOPIX_IND_DATA_GPIO, ANEOPIX_IND_TYPE);
#  ifdef ANEOPIX_IND_DATA_GPIO2 // Only used for Critical Indicator
// assume the same LED type
Adafruit_NeoPixel leds2(ANEOPIX_IND_NUM_LEDS, ANEOPIX_IND_DATA_GPIO2, ANEOPIX_IND_TYPE);
#  endif
#  ifdef ANEOPIX_IND_DATA_GPIO3
// assume the same LED type
Adafruit_NeoPixel leds3(ANEOPIX_IND_NUM_LEDS, ANEOPIX_IND_DATA_GPIO3, ANEOPIX_IND_TYPE);
#  endif

#  ifndef RGB_LED_POWER
#    define RGB_LED_POWER -1 // If the RGB Led is linked to GPIO pin for power define it here
#  endif
#  ifndef ANEOPIX_BRIGHTNESS
#    define ANEOPIX_BRIGHTNESS 20 // Set Default maximum RGB brightness to approx 10% (0-255 scale)
#  endif
#  ifndef DEFAULT_ADJ_BRIGHTNESS
#    define DEFAULT_ADJ_BRIGHTNESS 255 // Set Default RGB adjustable brightness
#  endif
#  ifndef ANEOPIX_COLOR_SCHEME // allow for different color combinations
#    define ANEOPIX_COLOR_SCHEME 0
#  endif
// Allow to set LED used (for example thingpulse gateway has 4 we use them independently)
#  ifndef ANEOPIX_ON_LED
#    define ANEOPIX_ON_LED 0 // First Led
#  endif
#  ifndef ANEOPIX_INFO_LED
#    define ANEOPIX_INFO_LED 0 // First Led
#  endif
#  ifndef ANEOPIX_SEND_RECEIVE_LED
#    define ANEOPIX_SEND_RECEIVE_LED 0 // First Led
#  endif
#  ifndef ANEOPIX_ERROR_LED
#    define ANEOPIX_ERROR_LED 0 // First Led
#  endif
#  ifndef ANEOPIX_CRITICAL_LED
#    define ANEOPIX_CRITICAL_LED 0 // First Led
#  endif
// compile time calculation of color values
#  define ANEOPIX_RED     ((0xFF * ANEOPIX_BRIGHTNESS) >> 8) << 16
#  define ANEOPIX_RED_DIM ((0x3F * ANEOPIX_BRIGHTNESS) >> 8) << 16 // dimmed /4
#  define ANEOPIX_ORANGE  (((0xFF * ANEOPIX_BRIGHTNESS) >> 8) << 16) | \
                             (((0xA5 * ANEOPIX_BRIGHTNESS) >> 8) << 8)
#  define ANEOPIX_GOLD (((0xFF * ANEOPIX_BRIGHTNESS) >> 8) << 16) | \
                           (((0xD7 * ANEOPIX_BRIGHTNESS) >> 8) << 8)
#  define ANEOPIX_GREEN     ((0xFF * ANEOPIX_BRIGHTNESS) >> 8) << 8
#  define ANEOPIX_GREEN_DIM ((0x3F * ANEOPIX_BRIGHTNESS) >> 8) << 8 // dimmed /4
#  define ANEOPIX_AQUA      (((0xFF * ANEOPIX_BRIGHTNESS) >> 8) << 8) | \
                           (0xFF * ANEOPIX_BRIGHTNESS) >> 8
#  define ANEOPIX_BLUE     (0xFF * ANEOPIX_BRIGHTNESS) >> 8
#  define ANEOPIX_BLUE_DIM (0x3F * ANEOPIX_BRIGHTNESS) >> 8 // dimmed /4
#  define ANEOPIX_BLACK    0

#  if ANEOPIX_COLOR_SCHEME == 0
// original color combination remains default
#    define ANEOPIX_INFO        ANEOPIX_GREEN
#    define ANEOPIX_ERROR       ANEOPIX_ORANGE
#    define ANEOPIX_SENDRECEIVE ANEOPIX_BLUE
#    define ANEOPIX_CRITICAL    ANEOPIX_RED // second led
#    define ANEOPIX_POWER       ANEOPIX_GREEN // second led
#    define ANEOPIX_BOOT        ANEOPIX_BLACK // unused
#    define ANEOPIX_OFF         ANEOPIX_BLACK
// color combinations tested for good visibility of onboard leds
#  elif ANEOPIX_COLOR_SCHEME == 1
#    define ANEOPIX_INFO        ANEOPIX_GREEN_DIM // dimmed green info background
#    define ANEOPIX_ERROR       ANEOPIX_RED_DIM
#    define ANEOPIX_SENDRECEIVE ANEOPIX_GOLD // bright gold  = sending
#    define ANEOPIX_CRITICAL    ANEOPIX_BLACK // unused
#    define ANEOPIX_POWER       ANEOPIX_BLACK // unused
#    define ANEOPIX_BOOT        ANEOPIX_AQUA
#    define ANEOPIX_OFF         ANEOPIX_BLACK
#  else
#    define ANEOPIX_INFO        ANEOPIX_BLUE_DIM // dimmed blue info background
#    define ANEOPIX_ERROR       ANEOPIX_RED_DIM
#    define ANEOPIX_SENDRECEIVE ANEOPIX_GOLD // bright gold  = sending
#    define ANEOPIX_CRITICAL    ANEOPIX_BLACK // unused
#    define ANEOPIX_POWER       ANEOPIX_BLACK // unused
#    define ANEOPIX_BOOT        ANEOPIX_AQUA
#    define ANEOPIX_OFF         ANEOPIX_BLACK
#  endif
#  if !defined(ANEOPIX_IND_DATA_GPIO2) && !defined(ANEOPIX_IND_DATA_GPIO3)
// during boot the RGB LED is on to signal also reboots
#    define SetupIndicators()                             \
      if (RGB_LED_POWER > -1) {                           \
        pinMode(RGB_LED_POWER, OUTPUT);                   \
        digitalWrite(RGB_LED_POWER, HIGH);                \
      }                                                   \
      leds.begin();                                       \
      leds.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_BOOT); \
      leds.show();
#  elif defined(ANEOPIX_IND_DATA_GPIO2) && !defined(ANEOPIX_IND_DATA_GPIO3)
#    define SetupIndicators()              \
      if (RGB_LED_POWER > -1) {            \
        pinMode(RGB_LED_POWER, OUTPUT);    \
        digitalWrite(RGB_LED_POWER, HIGH); \
      }                                    \
      leds.begin();                        \
      leds2.begin();
#  else
#    define SetupIndicators()              \
      if (RGB_LED_POWER > -1) {            \
        pinMode(RGB_LED_POWER, OUTPUT);    \
        digitalWrite(RGB_LED_POWER, HIGH); \
      }                                    \
      leds.begin();                        \
      leds2.begin();                       \
      leds3.begin();
#  endif
#  ifndef RGB_LED_ERROR
#    define RGB_LED_ERROR leds
#  endif
#  ifndef RGB_LED_SR
#    define RGB_LED_SR leds
#  endif
#  ifndef RGB_LED_INFO
#    define RGB_LED_INFO leds
#  endif
#  ifndef RGB_LED_ON
#    define RGB_LED_ON leds
#  endif
#  define ONIndicatorON()                                    \
    RGB_LED_ON.setPixelColor(ANEOPIX_ON_LED, ANEOPIX_POWER); \
    RGB_LED_ON.setBrightness(SYSConfig.rgbbrightness);       \
    RGB_LED_ON.show();
#  define ErrorIndicatorON()                                       \
    RGB_LED_ERROR.setPixelColor(ANEOPIX_ERROR_LED, ANEOPIX_ERROR); \
    RGB_LED_ERROR.setBrightness(SYSConfig.rgbbrightness);          \
    RGB_LED_ERROR.show();
#  define ErrorIndicatorOFF()                                    \
    RGB_LED_ERROR.setPixelColor(ANEOPIX_ERROR_LED, ANEOPIX_OFF); \
    RGB_LED_ERROR.show();
#  define SendReceiveIndicatorON()                                           \
    RGB_LED_SR.setPixelColor(ANEOPIX_SEND_RECEIVE_LED, ANEOPIX_SENDRECEIVE); \
    RGB_LED_SR.setBrightness(SYSConfig.rgbbrightness);                       \
    RGB_LED_SR.show();
#  define SendReceiveIndicatorOFF()                                  \
    RGB_LED_SR.setPixelColor(ANEOPIX_SEND_RECEIVE_LED, ANEOPIX_OFF); \
    RGB_LED_SR.show();
#  define InfoIndicatorON()                                     \
    RGB_LED_INFO.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_INFO); \
    RGB_LED_INFO.setBrightness(SYSConfig.rgbbrightness);        \
    RGB_LED_INFO.show();
#  define InfoIndicatorOFF()                                   \
    RGB_LED_INFO.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_OFF); \
    RGB_LED_INFO.show();
#  ifdef ANEOPIX_IND_DATA_GPIO2 // Used for relay power indicator
// For the critical ON indicator there is no method to turn it off, the only way is to unplug the device
// This enable to have persistence of the indicator to inform the user
#    ifndef LED_CRITICAL
#      define LED_CRITICAL leds2
#    endif
#    ifndef LED_POWER
#      define LED_POWER leds2
#    endif
#    define CriticalIndicatorON()                                     \
      LED_CRITICAL.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_CRITICAL); \
      LED_CRITICAL.setBrightness(255);                                \
      LED_CRITICAL.show();
#    define PowerIndicatorON()                                 \
      LED_POWER.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_INFO); \
      LED_POWER.setBrightness(SYSConfig.rgbbrightness);        \
      LED_POWER.show();
#    define PowerIndicatorOFF()                               \
      LED_POWER.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_OFF); \
      LED_POWER.show();
#  endif
#  define SetupIndicatorInfo()
#  define SetupIndicatorSendReceive()
#  define SetupIndicatorError()
#endif

#ifdef ESP8266
//#  define TRIGGER_GPIO 14 // pin D5 as full reset button (long press >10s)
#elif ESP32
//#  define TRIGGER_GPIO 0 // boot button as full reset button (long press >10s)
//#  define NO_INT_TEMP_READING true //Define if we don't want internal temperature reading for the ESP32
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
#ifndef subjectMQTTtoX
#  define subjectMQTTtoX "/commands/#"
#endif
#ifndef subjectMultiGTWKey
#  define subjectMultiGTWKey "toMQTT"
#endif
#ifndef subjectGTWSendKey
#  define subjectGTWSendKey "MQTTto"
#endif

// key used for launching commands to the gateway
#define restartCmd "restart"
#define eraseCmd   "erase"
#define statusCmd  "status"

#ifndef valueAsATopic
#  define valueAsATopic false // define true to integrate msg value into the subject when receiving
#endif

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)
// variable to avoid duplicates
#  ifndef time_avoid_duplicate
#    define time_avoid_duplicate 3000 // if you want to avoid duplicate MQTT message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates
#  endif
#endif

#ifndef TimeBetweenReadingSYS
#  define TimeBetweenReadingSYS 120 // time between (s) system readings (like memory)
#endif
#define TimeBetweenCheckingSYS       3600 // time between (s) system checkings (like updates)
#define TimeLedON                    1 // time LED are ON
#define InitialMQTTConnectionTimeout 10 // time estimated (s) before the board is connected to MQTT
#define subjectSYStoMQTT             "/SYStoMQTT" // system parameters
#define subjectLOGtoMQTT             "/LOGtoMQTT" // log informations
#define subjectRLStoMQTT             "/RLStoMQTT" // latest release information
#define subjectMQTTtoSYSset          "/commands/MQTTtoSYS/config"
#define subjectMQTTtoSYSupdate       "/commands/MQTTtoSYS/firmware_update"
#define TimeToResetAtStart           5000 // Time we allow the user at start for the reset command by button press
/*-------------------DEFINE LOG LEVEL----------------------*/
#ifndef LOG_LEVEL
#  define LOG_LEVEL LOG_LEVEL_NOTICE
#endif

/*-------------------ESP Wifi band and tx power ---------------------*/
//Certain sensors are sensitive to Wifi which can cause interference with their normal operation
//For example it can cause false triggers on a PIR HC-SR501
//It is reccomended to change Wifi BAND to G and reduce tx power level to 11dBm
//Since the WiFi protocol is persisted in the flash of the ESP you have to run at least once with `WiFiGMode` defined false to get Band N back.
#ifndef WifiGMode
//#    define WifiGMode                 true
#endif
#ifndef WifiPower
//#    define WifiPower                 WIFI_POWER_11dBm //When using an ESP32
//#    define WifiPower                 11 //When using an ESP8266
#endif

/*-----------PLACEHOLDERS FOR WebUI DISPLAY--------------*/
#define pubWebUI(...) // display the published message onto the WebUI display

/*-----------PLACEHOLDERS FOR OLED/LCD DISPLAY--------------*/
// The real definitions are in config_M5.h / config_SSD1306.h
#define pubOled(...)        // display the published message onto the OLED display
#define displayPrint(...)   // only print if not in low power mode
#define lpDisplayPrint(...) // print in low power mode

/*----------- SHARED WITH OMG MODULES --------------*/

char mqtt_topic[parameters_size + 1] = Base_Topic;
char gateway_name[parameters_size + 1] = Gateway_Name;

void connectMQTT();

unsigned long uptime();
bool cmpToMainTopic(const char*, const char*);
void pub(const char*, const char*, bool);
// void pub(const char*, JsonObject&);
void pub(const char*, const char*);
// void pub_custom_topic(const char*, JsonObject&, boolean);

#if defined(ESP32)
#  include <Preferences.h>
Preferences preferences;
#endif

unsigned long lastDiscovery = 0; // Time of the last discovery to trigger automaticaly to off after DiscoveryAutoOffTimer
#ifndef DEFAULT_DISCOVERY
#  define DEFAULT_DISCOVERY true
#endif

#include <vector>
// Flags definition for white list, black list, discovery management
#define device_flags_init     0 << 0
#define device_flags_isDisc   1 << 0
#define device_flags_isWhiteL 1 << 1
#define device_flags_isBlackL 1 << 2
#define device_flags_connect  1 << 3
#define isWhite(device)       device->isWhtL
#define isBlack(device)       device->isBlkL
#define isDiscovered(device)  device->isDisc

enum PowerMode { DEACTIVATED = -1,
                 ALWAYS_ON,
                 INTERVAL,
                 ACTION };

/*--------------------Minimum freeHeap--------------------*/
// Below this parameter we trigger a restart, this avoid stuck boards like seen in https://github.com/1technophile/OpenMQTTGateway/issues/1693
#define MinimumMemory 40000

/*----------------CONFIGURABLE PARAMETERS-----------------*/
struct SYSConfig_s {
  bool XtoMQTT; // if true the gateway will publish the received data on the MQTT broker
  bool offline;
  bool discovery; // HA discovery convention
  bool ohdiscovery; // OH discovery specificities
#ifdef RGB_INDICATORS
  int rgbbrightness; // brightness of the RGB LED
#endif
  enum PowerMode powerMode;
};

#ifndef DEFAULT_XtoMQTT
#  define DEFAULT_XtoMQTT true
#endif
#ifndef DEFAULT_OFFLINE
#  define DEFAULT_OFFLINE false
#endif

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)
bool isAduplicateSignal(uint64_t);
void storeSignalValue(uint64_t);
#endif

// Origin topics
#define subjectBTtoMQTT "/BTtoMQTT"

#define convertTemp_CtoF(c) ((c * 1.8) + 32)
#define convertTemp_FtoC(f) ((f - 32) * 5 / 9)

#endif
