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

// Uncomment to use the MAC address first 4 digits in the format of 5566 as the suffix of the short gateway name.
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

//#define NetworkAdvancedSetup true //uncomment if you want to set advanced network parameters, not uncommented you can set the IP and MAC only
#ifdef NetworkAdvancedSetup
#  if defined(ESP8266) || defined(ESP32)
const byte ip[] = {192, 168, 1, 99}; //IP address of the gateway, already defined for arduino below
#  endif
const byte gateway[] = {0, 0, 0, 0};
const byte Dns[] = {0, 0, 0, 0};
const byte subnet[] = {255, 255, 255, 0};
#endif

#if defined(ESP8266) || defined(ESP32) // for nodemcu, weemos and esp8266
//#  define ESPWifiManualSetup true //uncomment you don't want to use wifimanager for your credential settings on ESP
#else // for arduino boards
const byte ip[] = {192, 168, 1, 99};
const byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0x54, 0x95}; //W5100 ethernet shield MAC address
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

//#define WM_PWD_FROM_MAC true // enable to set the password from the last 8 digits of the ESP MAC address for enhanced security, enabling this option requires to have access to the MAC address, either through a sticker or with serial monitoring
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
//#define MDNS_SD //uncomment if you  want to use mDNS for discovering automatically your IP server, please note that mDNS with ESP32 can cause the BLE to not work
#define maxConnectionRetry     10 //maximum MQTT connection attempts before going to wifimanager setup if never connected once
#define maxConnectionRetryWifi 5 //maximum Wifi connection attempts with existing credential at start (used to bypass ESP32 issue on wifi connect)
#define maxRetryWatchDog       11 //maximum Wifi or MQTT re-connection attempts before restarting

//set minimum quality of signal so it ignores AP's under that quality
#define MinimumWifiSignalQuality 8

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#  define parameters_size     60
#  define mqtt_topic_max_size 150
#  ifndef mqtt_max_packet_size
#    ifdef MQTT_HTTPS_FW_UPDATE
#      define mqtt_max_packet_size 2560
#    else
#      define mqtt_max_packet_size 1024
#    endif
#  endif
#else
#  define parameters_size      30
#  define mqtt_topic_max_size  75
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
// Uncomment to use a device running TheengsGateway to decode BLE data. (https://github.com/theengs/gateway)
// Set the topic to the subscribe topic configured in the TheengGateway
// #define MQTTDecodeTopic "MQTTDecode"

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

#  ifndef MQTT_CERT_VALIDATE_DEFAULT
#    define MQTT_CERT_VALIDATE_DEFAULT false
#  endif

#  ifndef AWS_IOT
#    define AWS_IOT false
#  endif

//#  define MQTT_HTTPS_FW_UPDATE //uncomment to enable updating via MQTT message.

#  ifdef MQTT_HTTPS_FW_UPDATE
// If used, this should be set to the root CA certificate of the server hosting the firmware.
// The certificate must be in PEM ascii format.
// The default certificate is for github.
const char* OTAserver_cert PROGMEM = R"EOF("
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
")EOF";

#    ifndef MQTT_HTTPS_FW_UPDATE_USE_PASSWORD
#      define MQTT_HTTPS_FW_UPDATE_USE_PASSWORD 1 // Set this to 0 if not using TLS connection to MQTT broker to prevent clear text passwords being sent.
#    endif
#    if DEVELOPMENTOTA
#      define OTA_JSON_URL "https://github.com/1technophile/OpenMQTTGateway/raw/gh-pages/dev/firmware_build/latest_version_dev.json" //OTA url used to discover new versions of the firmware from development nightly builds
#    else
#      define OTA_JSON_URL "https://github.com/1technophile/OpenMQTTGateway/raw/gh-pages/firmware_build/latest_version.json" //OTA url used to discover new versions of the firmware
#    endif
#    define ENTITY_PICTURE   "https://github.com/1technophile/OpenMQTTGateway/raw/development/docs/img/Openmqttgateway_logo_mini_margins.png"
#    define RELEASE_LINK_DEV "https://github.com/1technophile/OpenMQTTGateway/raw/gh-pages/dev/firmware_build/"
#    define RELEASE_LINK     "https://github.com/1technophile/OpenMQTTGateway/releases/download/"
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

/**
 * Deep-sleep for the ESP8266.
 * Set the wake pin.
 */
#if defined(DEEP_SLEEP_IN_US) && defined(ESP8266)
#  ifndef ESP8266_DEEP_SLEEP_WAKE_PIN
#    define ESP8266_DEEP_SLEEP_WAKE_PIN D0
#  endif
#endif

/**
 * Ext wake for Deep-sleep for the ESP32.
 * Set the wake pin state.
 */
#ifdef ESP32_EXT0_WAKE_PIN
#  ifndef ESP32_EXT0_WAKE_PIN_STATE
#    define ESP32_EXT0_WAKE_PIN_STATE 1
#  endif
#endif

/*------------------DEEP SLEEP parameters ------------------*/
//DEFAULT_LOW_POWER_MODE -1 to normal mode, low power mode can't be used on this build
//DEFAULT_LOW_POWER_MODE 0 to normal mode (no power consumption optimisations)
//DEFAULT_LOW_POWER_MODE 1 to activate deep sleep
//DEFAULT_LOW_POWER_MODE 2 to activate deep sleep (LCD is turned OFF)
#ifdef ESP32
#  ifndef DEFAULT_LOW_POWER_MODE
#    define DEFAULT_LOW_POWER_MODE -1
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
#ifndef ota_password
#  define ota_password "OTAPASSWORD"
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
#    elif __AVR_ATmega2560__ //arduino mega
//#      define LED_SEND_RECEIVE 40
#    else //arduino uno/nano
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
#    elif __AVR_ATmega2560__ //arduino mega
//#      define LED_ERROR 42
#    else //arduino uno/nano
//#      define LED_ERROR 42
#    endif
#  endif
#  ifndef LED_ERROR_ON
#    define LED_ERROR_ON HIGH
#  endif
#  ifndef LED_INFO
#    ifdef ESP8266
//#      define LED_INFO 44
#    elif ESP32
//#      define LED_INFO 44
#    elif __AVR_ATmega2560__ //arduino mega
//#      define LED_INFO 44
#    else //arduino uno/nano
//#      define LED_INFO 44
#    endif
#  endif
#  ifndef LED_INFO_ON
#    define LED_INFO_ON HIGH
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

#  ifndef RGB_LED_POWER
#    define RGB_LED_POWER -1 // If the RGB Led is linked to GPIO pin for power define it here
#  endif
#  ifndef ANEOPIX_BRIGHTNESS
#    define ANEOPIX_BRIGHTNESS 20 // Set Default RGB brightness to approx 10% (0-255 scale)
#  endif
#  ifndef ANEOPIX_COLOR_SCHEME // allow for different color combinations
#    define ANEOPIX_COLOR_SCHEME 0
#  endif
// Allow to set LED used (for example thingpulse gateway has 4 we use them independently)
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
#  ifndef ANEOPIX_IND_DATA_GPIO2
// during boot the RGB LED is on to signal also reboots
#    define SetupIndicators()                             \
      pinMode(RGB_LED_POWER, OUTPUT);                     \
      digitalWrite(RGB_LED_POWER, HIGH);                  \
      leds.begin();                                       \
      leds.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_BOOT); \
      leds.show();
#  else
#    define SetupIndicators()            \
      pinMode(RGB_LED_POWER, OUTPUT);    \
      digitalWrite(RGB_LED_POWER, HIGH); \
      leds.begin();                      \
      leds2.begin();
#  endif
#  define ErrorIndicatorON()                              \
    leds.setPixelColor(ANEOPIX_ERROR_LED, ANEOPIX_ERROR); \
    leds.show();
#  define ErrorIndicatorOFF()                           \
    leds.setPixelColor(ANEOPIX_ERROR_LED, ANEOPIX_OFF); \
    leds.show();
#  define SendReceiveIndicatorON()                                     \
    leds.setPixelColor(ANEOPIX_SEND_RECEIVE_LED, ANEOPIX_SENDRECEIVE); \
    leds.show();
#  define SendReceiveIndicatorOFF()                            \
    leds.setPixelColor(ANEOPIX_SEND_RECEIVE_LED, ANEOPIX_OFF); \
    leds.show();
#  define InfoIndicatorON()                             \
    leds.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_INFO); \
    leds.show();
#  define InfoIndicatorOFF()                           \
    leds.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_OFF); \
    leds.show();
#  ifdef ANEOPIX_IND_DATA_GPIO2 // Used for relay power indicator
// For the critical ON indicator there is no method to turn it off, the only way is to unplug the device
// This enable to have persistence of the indicator to inform the user
#    define CriticalIndicatorON()                              \
      leds2.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_CRITICAL); \
      leds2.show();
#    define PowerIndicatorON()                             \
      leds2.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_INFO); \
      leds2.show();
#    define PowerIndicatorOFF()                           \
      leds2.setPixelColor(ANEOPIX_INFO_LED, ANEOPIX_OFF); \
      leds2.show();
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
#define subjectMQTTtoX     "/commands/#"
#define subjectMultiGTWKey "toMQTT"
#define subjectGTWSendKey  "MQTTto"

// key used for launching commands to the gateway
#define restartCmd "restart"
#define eraseCmd   "erase"
#define statusCmd  "status"

#ifndef valueAsATopic
#  define valueAsATopic false // define true to integrate msg value into the subject when receiving
#endif

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

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)
// variable to avoid duplicates
#  ifndef time_avoid_duplicate
#    define time_avoid_duplicate 3000 // if you want to avoid duplicate MQTT message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates
#  endif
#endif

#define TimeBetweenReadingSYS        120 // time between (s) system readings (like memory)
#define TimeBetweenCheckingSYS       3600 // time between (s) system checkings (like updates)
#define TimeLedON                    1 // time LED are ON
#define InitialMQTTConnectionTimeout 10 // time estimated (s) before the board is connected to MQTT
#define subjectSYStoMQTT             "/SYStoMQTT" // system parameters
#define subjectRLStoMQTT             "/RLStoMQTT" // latest release information
#define subjectMQTTtoSYSset          "/commands/MQTTtoSYS/config"
#define subjectMQTTtoSYSupdate       "/commands/MQTTtoSYS/firmware_update"
#define TimeToResetAtStart           5000 // Time we allow the user at start for the reset command by button press
/*-------------------DEFINE LOG LEVEL----------------------*/
#ifndef LOG_LEVEL
#  define LOG_LEVEL LOG_LEVEL_NOTICE
#endif

/*-------------------ESP32 Wifi band and tx power ---------------------*/
//Certain sensors are sensitive to ESP32 Wifi which can cause interference with their normal operation
//For example it can cause false triggers on a PIR HC-SR501
//It is reccomended to change Wifi BAND to G and reduce tx power level to 11dBm
//Since the WiFi protocol is persisted in the flash of the ESP32 you have to run at least once with `WiFiGMode` defined false to get Band N back.
#ifdef ESP32
#  ifndef WifiGMode
//#    define WifiGMode                 true
#  endif
#  ifndef WifiPower
//#    define WifiPower                 WIFI_POWER_11dBm
#  endif
#endif

/*-----------PLACEHOLDERS FOR WebUI DISPLAY--------------*/
#define pubWebUI(...) // display the published message onto the OLED display

/*-----------PLACEHOLDERS FOR OLED/LCD DISPLAY--------------*/
// The real definitions are in config_M5.h / config_SSD1306.h
#define pubOled(...)        // display the published message onto the OLED display
#define displayPrint(...)   // only print if not in low power mode
#define lpDisplayPrint(...) // print in low power mode

/*----------- SHARED WITH OMG MODULES --------------*/

char mqtt_topic[parameters_size + 1] = Base_Topic;
char gateway_name[parameters_size + 1] = Gateway_Name;

void connectMQTT();
#ifndef ESPWifiManualSetup
void saveMqttConfig();
#endif

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

#ifdef ZmqttDiscovery
bool disc = true; // Auto discovery with Home Assistant convention
unsigned long lastDiscovery = 0; // Time of the last discovery to trigger automaticaly to off after DiscoveryAutoOffTimer
#endif

#if defined(ESP8266) || defined(ESP32)
#  include <vector>
// Flags definition for white list, black list, discovery management
#  define device_flags_init     0 << 0
#  define device_flags_isDisc   1 << 0
#  define device_flags_isWhiteL 1 << 1
#  define device_flags_isBlackL 1 << 2
#  define device_flags_connect  1 << 3
#  define isWhite(device)       device->isWhtL
#  define isBlack(device)       device->isBlkL
#  define isDiscovered(device)  device->isDisc
#endif

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)
bool isAduplicateSignal(SIGNAL_SIZE_UL_ULL);
void storeSignalValue(SIGNAL_SIZE_UL_ULL);
#endif

#define convertTemp_CtoF(c) ((c * 1.8) + 32)
#define convertTemp_FtoC(f) ((f - 32) * 5 / 9)

#endif
