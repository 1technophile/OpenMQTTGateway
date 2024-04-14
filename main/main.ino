/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This program enables to:
 - receive MQTT data from a topic and send signal (RF, IR, BLE, GSM)  corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received signals (RF, IR, BLE, GSM)

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
#include "User_config.h"

// States of the gateway
// Wm setup
// Connected to MQTT
// Disconnected from Wifi
// Disconnected from MQTT
// Deep sleep
// Local OTA in progress
// Remote OTA in progress

// Macros and structure to enable the duplicates removing on the following gateways
#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)
// array to store previous received RFs, IRs codes and their timestamps
struct ReceivedSignal {
  SIGNAL_SIZE_UL_ULL value;
  uint32_t time;
};

ReceivedSignal receivedSignal[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};

#  define struct_size (sizeof(receivedSignal) / sizeof(ReceivedSignal))
#endif

//Time used to wait for an interval before checking system measures
unsigned long timer_sys_measures = 0;

// Time used to wait before system checkings
unsigned long timer_sys_checks = 0;

#define ARDUINOJSON_USE_LONG_LONG     1
#define ARDUINOJSON_ENABLE_STD_STRING 1

#include <queue>
int queueLength = 0;
unsigned long queueLengthSum = 0;
unsigned long blockedMessages = 0;
int maxQueueLength = 0;
#ifndef QueueSize
#  define QueueSize 18
#endif

/**
 * Deep-sleep for the ESP8266 & ESP32 we need some form of indicator that we have posted the measurements and am ready to deep sleep.
 * Set this to true in the sensor code after publishing the measurement.
 */
#if defined(DEEP_SLEEP_IN_US) || defined(ESP32_EXT0_WAKE_PIN)
bool ready_to_sleep = false;
#endif

#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <PubSubClient.h>

#include <string>

struct JsonBundle {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
};

std::queue<JsonBundle> jsonQueue;

#ifdef ESP32
// Mutex  to protect the queue
SemaphoreHandle_t xQueueMutex;
#endif

StaticJsonDocument<JSON_MSG_BUFFER> modulesBuffer;
JsonArray modules = modulesBuffer.to<JsonArray>();
bool ethConnected = false;

#ifndef ZgatewayGFSunInverter
// Arduino IDE compiles, it automatically creates all the header declarations for all the functions you have in your *.ino file.
// Unfortunately it ignores #if directives.
// This is a simple workaround for this problem.
struct GfSun2000Data {};
#endif

// Modules config inclusion
#if defined(ZwebUI) && defined(ESP32)
#  include "config_WebUI.h"
#endif
#if defined(ZgatewayRF) || defined(ZgatewayRF2) || defined(ZgatewayPilight) || defined(ZactuatorSomfy) || defined(ZgatewayRTL_433)
#  include "config_RF.h"
#endif
#ifdef ZgatewayWeatherStation
#  include "config_WeatherStation.h"
#endif
#ifdef ZgatewayGFSunInverter
#  include "config_GFSunInverter.h"
#endif
#ifdef ZgatewayLORA
#  include "config_LORA.h"
#endif
#ifdef ZgatewaySRFB
#  include "config_SRFB.h"
#endif
#ifdef ZgatewayBT
#  include "config_BT.h"
#endif
#ifdef ZgatewayIR
#  include "config_IR.h"
#endif
#ifdef Zgateway2G
#  include "config_2G.h"
#endif
#ifdef ZactuatorONOFF
#  include "config_ONOFF.h"
#endif
#ifdef ZsensorINA226
#  include "config_INA226.h"
#endif
#ifdef ZsensorHCSR501
#  include "config_HCSR501.h"
#endif
#ifdef ZsensorADC
#  include "config_ADC.h"
#endif
#ifdef ZsensorBH1750
#  include "config_BH1750.h"
#endif
#ifdef ZsensorMQ2
#  include "config_MQ2.h"
#endif
#ifdef ZsensorTEMT6000
#  include "config_TEMT6000.h"
#endif
#ifdef ZsensorTSL2561
#  include "config_TSL2561.h"
#endif
#ifdef ZsensorBME280
#  include "config_BME280.h"
#endif
#ifdef ZsensorHTU21
#  include "config_HTU21.h"
#endif
#ifdef ZsensorLM75
#  include "config_LM75.h"
#endif
#ifdef ZsensorAHTx0
#  include "config_AHTx0.h"
#endif
#ifdef ZsensorRN8209
#  include "config_RN8209.h"
#endif
#ifdef ZsensorHCSR04
#  include "config_HCSR04.h"
#endif
#ifdef ZsensorC37_YL83_HMRD
#  include "config_C37_YL83_HMRD.h"
#endif
#ifdef ZsensorDHT
#  include "config_DHT.h"
#endif
#ifdef ZsensorSHTC3
#  include "config_SHTC3.h"
#endif
#ifdef ZsensorDS1820
#  include "config_DS1820.h"
#endif
#ifdef ZgatewayRFM69
#  include "config_RFM69.h"
#endif
#ifdef ZsensorGPIOInput
#  include "config_GPIOInput.h"
#endif
#ifdef ZsensorGPIOKeyCode
#  include "config_GPIOKeyCode.h"
#endif
#ifdef ZsensorTouch
#  include "config_Touch.h"
#endif
#ifdef ZmqttDiscovery
#  include "config_mqttDiscovery.h"
#endif
#ifdef ZactuatorFASTLED
#  include "config_FASTLED.h"
#endif
#ifdef ZactuatorPWM
#  include "config_PWM.h"
#endif
#ifdef ZactuatorSomfy
#  include "config_Somfy.h"
#endif
#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
#  include "config_M5.h"
#endif
#if defined(ZdisplaySSD1306)
#  include "config_SSD1306.h"
#endif
#if defined(ZgatewayRS232)
#  include "config_RS232.h"
#endif
/*------------------------------------------------------------------------*/

void setupTLS(bool self_signed = false, uint8_t index = 0);

//adding this to bypass the problem of the arduino builder issue 50
void callback(char* topic, byte* payload, unsigned int length);

char mqtt_user[parameters_size] = MQTT_USER; // not compulsory only if your broker needs authentication
char mqtt_pass[parameters_size] = MQTT_PASS; // not compulsory only if your broker needs authentication
char mqtt_server[parameters_size] = MQTT_SERVER;
char mqtt_port[6] = MQTT_PORT;
char ota_pass[parameters_size] = gw_password;
#ifdef USE_MAC_AS_GATEWAY_NAME
#  undef WifiManager_ssid
#  undef ota_hostname
#  define MAC_NAME_MAX_LEN 30
char WifiManager_ssid[MAC_NAME_MAX_LEN];
char ota_hostname[MAC_NAME_MAX_LEN];
#endif
bool connectedOnce = false; //indicate if we have been connected once to MQTT
bool connected = false; //indicate whether we are currently connected. Used to detected re-connection
int failure_number_ntwk = 0; // number of failure connecting to network
int failure_number_mqtt = 0; // number of failure connecting to MQTT

unsigned long timer_led_measures = 0;
static void* eClient = nullptr;
static unsigned long last_ota_activity_millis = 0;
// Global struct to store live SYS configuration data
SYSConfig_s SYSConfig;

bool failSafeMode = false;
static bool mqtt_secure = MQTT_SECURE_DEFAULT;
static bool mqtt_cert_validate = MQTT_CERT_VALIDATE_DEFAULT;
static uint8_t mqtt_ss_index = MQTT_SECURE_SELF_SIGNED_INDEX_DEFAULT;
static String mqtt_cert = "";
static String ota_server_cert = "";

#ifdef ESP32
#  include <ArduinoOTA.h>
#  include <FS.h>
#  include <SPIFFS.h>
#  include <esp_task_wdt.h>
#  include <nvs.h>
#  include <nvs_flash.h>

bool BTProcessLock = true; // Process lock when we want to use a critical function like OTA for example, at start to true so as to wait for critical functions to be performed before BLE start
bool ProcessLock = false; // Process lock when we want to use a critical function like OTA for example

#  if !defined(NO_INT_TEMP_READING)
// ESP32 internal temperature reading
#    include <stdio.h>

#    include "rom/ets_sys.h"
#    include "soc/rtc_cntl_reg.h"
#    include "soc/sens_reg.h"
#  endif
#  ifdef ESP32_ETHERNET
#    include <ETH.h>
void WiFiEvent(WiFiEvent_t event);
#  endif

#  include <WiFiClientSecure.h>
#  include <WiFiMulti.h>
WiFiMulti wifiMulti;
#  include <WiFiManager.h>
#  ifdef MDNS_SD
#    include <ESPmDNS.h>
#  endif

#elif defined(ESP8266)
#  include <ArduinoOTA.h>
#  include <DNSServer.h>
#  include <ESP8266WebServer.h>
#  include <ESP8266WiFi.h>
#  include <ESP8266WiFiMulti.h>
#  include <FS.h>
#  include <WiFiManager.h>
X509List caCert;
#  if MQTT_SECURE_SELF_SIGNED_CLIENT
X509List* pClCert = nullptr;
PrivateKey* pClKey = nullptr;
#  endif
ESP8266WiFiMulti wifiMulti;
#  ifdef MDNS_SD
#    include <ESP8266mDNS.h>
#  endif

#else
#  include <Ethernet.h>
#endif

// client link to pubsub MQTT
PubSubClient client;

template <typename T> // Declared here to avoid pre-compilation issue (missing "template" in auto declaration by pio)
void Config_update(JsonObject& data, const char* key, T& var);
template <typename T>
void Config_update(JsonObject& data, const char* key, T& var) {
  if (data.containsKey(key)) {
    if (var != data[key].as<T>()) {
      var = data[key].as<T>();
      Log.notice(F("Config %s changed: %T" CR), key, data[key].as<T>());
    } else {
      Log.notice(F("Config %s unchanged: %T" CR), key, data[key].as<T>());
    }
  }
}

void revert_hex_data(const char* in, char* out, int l) {
  //reverting array 2 by 2 to get the data in good order
  int i = l - 2, j = 0;
  while (i != -2) {
    if (i % 2 == 0)
      out[j] = in[i + 1];
    else
      out[j] = in[i - 1];
    j++;
    i--;
  }
  out[l - 1] = '\0';
}

/**
 * Retrieve an unsigned long value from a char array extract representing hexadecimal data, reversed or not,
 * This value can represent a negative value if canBeNegative is set to true
 */
long value_from_hex_data(const char* service_data, int offset, int data_length, bool reverse, bool canBeNegative = true) {
  char data[data_length + 1];
  memcpy(data, &service_data[offset], data_length);
  data[data_length] = '\0';
  long value;
  if (reverse) {
    // reverse data order
    char rev_data[data_length + 1];
    revert_hex_data(data, rev_data, data_length + 1);
    value = strtol(rev_data, NULL, 16);
  } else {
    value = strtol(data, NULL, 16);
  }
  if (value > 65000 && data_length <= 4 && canBeNegative)
    value = value - 65535;
  Log.trace(F("value %D" CR), value);
  return value;
}

/*
 rounds a number to 2 decimal places
 example: round(3.14159) -> 3.14
*/
double round2(double value) {
  return (int)(value * 100 + 0.5) / 100.0;
}

/*
From an hexa char array ("A220EE...") to a byte array (half the size)
 */
bool _hexToRaw(const char* in, byte* out, int rawSize) {
  if (strlen(in) != rawSize * 2)
    return false;
  char tmp[3] = {0};
  for (unsigned char p = 0; p < rawSize; p++) {
    memcpy(tmp, &in[p * 2], 2);
    out[p] = strtol(tmp, NULL, 16);
  }
  return true;
}

/*
From a byte array to an hexa char array ("A220EE...", double the size)
 */
bool _rawToHex(byte* in, char* out, int rawSize) {
  for (unsigned char p = 0; p < rawSize; p++) {
    sprintf_P(&out[p * 2], PSTR("%02X" CR), in[p]);
  }
  return true;
}

char* ip2CharArray(IPAddress ip) { //from Nick Lee https://stackoverflow.com/questions/28119653/arduino-display-ethernet-localip
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}

bool to_bool(String const& s) { // thanks Chris Jester-Young from stackoverflow
  return s != "0";
}

/*
 * Publish a message depending on its origin
 *
*/
void pubMainCore(JsonObject& data) {
  if (data.containsKey("origin")) {
    pub((char*)data["origin"].as<const char*>(), data);
#ifdef ZgatewayBT
    if (data.containsKey("distance")) {
      String topic = String(mqtt_topic) + BTConfig.presenceTopic + String(gateway_name);
      Log.trace(F("Pub HA Presence %s" CR), topic.c_str());
      pub_custom_topic((char*)topic.c_str(), data, false);
    }
#endif
  } else {
    Log.error(F("No origin in JSON filtered" CR));
  }
}

// Add a document to the queue
void enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc) {
  if (jsonDoc.size() == 0) {
    Log.error(F("Empty JSON, skipping" CR));
    return;
  }
  if (queueLength >= QueueSize) {
    Log.warning(F("%d Doc(s) in queue, doc blocked" CR), queueLength);
    blockedMessages++;
    return;
  }
  Log.trace(F("Enqueue JSON" CR));
  JsonBundle bundle;
  bundle.doc = jsonDoc;
  jsonQueue.push(bundle);
  Log.trace(F("Queue length: %d" CR), jsonQueue.size());
}

#ifdef ESP32
// Semaphore check before enqueueing a document
bool handleJsonEnqueue(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc, int timeout) {
  if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(timeout))) {
    enqueueJsonObject(jsonDoc);
    xSemaphoreGive(xQueueMutex);
    return true;
  } else {
    Log.error(F("xQueueMutex not taken" CR));
    blockedMessages++;
    return false;
  }
}
#else
bool handleJsonEnqueue(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc, int timeout) {
  enqueueJsonObject(jsonDoc);
  return true;
}
#endif

// Semaphore check before enqueueing a document with default timeout QueueSemaphoreTimeOutLoop
bool handleJsonEnqueue(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc) {
  return handleJsonEnqueue(jsonDoc, QueueSemaphoreTimeOutLoop);
}

/*
 * Add the jsonObject id as a topic to the jsonObject origin
 *
*/
void buildTopicFromId(JsonObject& Jsondata, const char* origin) {
  if (!Jsondata.containsKey("id")) {
    Log.error(F("No id in Jsondata" CR));
    return;
  }

  std::string topic = Jsondata["id"].as<std::string>();

  // Replace ":" in topic
  size_t pos = topic.find(":");
  while (pos != std::string::npos) {
    topic.erase(pos, 1);
    pos = topic.find(":", pos);
  }
#ifdef ZgatewayBT
  if (BTConfig.pubBeaconUuidForTopic && !BTConfig.extDecoderEnable && Jsondata.containsKey("model_id") && Jsondata["model_id"].as<std::string>() == "IBEACON") {
    if (Jsondata.containsKey("uuid")) {
      topic = Jsondata["uuid"].as<std::string>();
    } else {
      Log.error(F("No uuid in Jsondata" CR));
    }
  }

  if (BTConfig.extDecoderEnable && !Jsondata.containsKey("model"))
    topic = BTConfig.extDecoderTopic.c_str();
#endif
  std::string subjectStr(origin);
  topic = subjectStr + "/" + topic;

  Jsondata["origin"] = topic;

  Log.trace(F("Origin: %s" CR), Jsondata["origin"].as<const char*>());
}

// Empty the documents queue
void emptyQueue() {
  queueLength = jsonQueue.size();
  if (queueLength > maxQueueLength) {
    maxQueueLength = queueLength;
  }
  if (queueLength == 0) {
    return;
  }
  JsonBundle bundle;
  Log.trace(F("Dequeue JSON" CR));
  bundle = jsonQueue.front();
  jsonQueue.pop();
  JsonObject obj = bundle.doc.as<JsonObject>();
  pubMainCore(obj);
  queueLengthSum++;
}

/**
 * @brief Publish the payload on default MQTT topic.
 *
 * @param topicori suffix to add on default MQTT Topic
 * @param payload  the message to sends
 * @param retainFlag true if you what a retain
 */
void pub(const char* topicori, const char* payload, bool retainFlag) {
  String topic = String(mqtt_topic) + String(gateway_name) + String(topicori);
  pubMQTT(topic.c_str(), payload, retainFlag);
}

/**
 * @brief Publish the payload on default MQTT topic
 *
 * @param topicori suffix to add on default MQTT Topic
 * @param data The Json Object that represents the message
 */
void pub(const char* topicori, JsonObject& data) {
  String dataAsString = "";
  bool ret = sensor_Retain;
#if message_UTCtimestamp == true
  data["UTCtime"] = UTCtimestamp();
#endif
#if message_unixtimestamp == true
  data["unixtime"] = unixtimestamp();
#endif
  if (data.containsKey("retain") && data["retain"].is<bool>()) {
    ret = data["retain"];
    data.remove("retain");
  }
  if (data.containsKey("origin")) { // temporary, in the future use this instead of topicori
    data.remove("origin");
  }
  if (data.size() == 0) {
    Log.error(F("Empty JSON, not published" CR));
    return;
  }
  serializeJson(data, dataAsString);
  Log.notice(F("Send on %s msg %s" CR), topicori, dataAsString.c_str());
  String topic = String(mqtt_topic) + String(gateway_name) + String(topicori);
#if valueAsATopic
#  ifdef ZgatewayPilight
  String value = data["value"];
  String protocol = data["protocol"];
  if (value != "null" && value != 0) {
    topic = topic + "/" + protocol + "/" + value;
  }
#  else
  SIGNAL_SIZE_UL_ULL value = data["value"];
  if (value != 0) {
    topic = topic + "/" + toString(value);
  }
#  endif
#endif

#if jsonPublishing
  Log.trace(F("jsonPubl - ON" CR));
  pubMQTT(topic.c_str(), dataAsString.c_str(), ret);
  pubWebUI(topicori, data);
#endif

#if simplePublishing
  Log.trace(F("simplePub - ON" CR));
  // Loop through all the key-value pairs in obj
  for (JsonPair p : data) {
#  if defined(ESP8266)
    yield();
#  endif
    if (p.value().is<SIGNAL_SIZE_UL_ULL>() && strcmp(p.key().c_str(), "rssi") != 0) { //test rssi , bypass solution due to the fact that a int is considered as an SIGNAL_SIZE_UL_ULL
      if (strcmp(p.key().c_str(), "value") == 0) { // if data is a value we don't integrate the name into the topic
        pubMQTT(topic, p.value().as<SIGNAL_SIZE_UL_ULL>());
      } else { // if data is not a value we integrate the name into the topic
        pubMQTT(topic + "/" + String(p.key().c_str()), p.value().as<SIGNAL_SIZE_UL_ULL>());
      }
    } else if (p.value().is<int>()) {
      pubMQTT(topic + "/" + String(p.key().c_str()), p.value().as<int>());
    } else if (p.value().is<float>()) {
      pubMQTT(topic + "/" + String(p.key().c_str()), p.value().as<float>());
    } else if (p.value().is<char*>()) {
      pubMQTT(topic + "/" + String(p.key().c_str()), p.value().as<const char*>());
    }
  }
#endif
}

/**
 * @brief Publish the payload on default MQTT topic
 *
 * @param topicori suffix to add on default MQTT Topic
 * @param payload the message to sends
 */
void pub(const char* topicori, const char* payload) {
  String topic = String(mqtt_topic) + String(gateway_name) + String(topicori);
  pubMQTT(topic, payload);
}

/**
 * @brief Publish the payload on the topic with a retention
 *
 * @param topic  The topic where to publish
 * @param data   The Json Object that represents the message
 * @param retain true if you what a retain
 */
void pub_custom_topic(const char* topic, JsonObject& data, boolean retain) {
  String buffer = "";
  serializeJson(data, buffer);
  pubMQTT(topic, buffer.c_str(), retain);
}

/**
 * @brief Low level MQTT functions without retain
 *
 * @param topic  the topic
 * @param payload  the payload
 */
void pubMQTT(const char* topic, const char* payload) {
  pubMQTT(topic, payload, sensor_Retain);
}

/**
 * @brief Very Low level MQTT functions with retain Flag
 *
 * @param topic the topic
 * @param payload the payload
 * @param retainFlag  true if retain the retain Flag
 */
void pubMQTT(const char* topic, const char* payload, bool retainFlag) {
  if (client.connected()) {
    SendReceiveIndicatorON();
    Log.trace(F("[ OMG->MQTT ] topic: %s msg: %s " CR), topic, payload);
    client.publish(topic, payload, retainFlag);
  } else {
    Log.warning(F("Client not connected, aborting the publication" CR));
  }
}

void pubMQTT(String topic, const char* payload) {
  pubMQTT(topic.c_str(), payload);
}

void pubMQTT(const char* topic, unsigned long payload) {
  char val[11];
  sprintf(val, "%lu", payload);
  pubMQTT(topic, val);
}

void pubMQTT(const char* topic, unsigned long long payload) {
  char val[21];
  sprintf(val, "%llu", payload);
  pubMQTT(topic, val);
}

void pubMQTT(const char* topic, String payload) {
  pubMQTT(topic, payload.c_str());
}

void pubMQTT(String topic, String payload) {
  pubMQTT(topic.c_str(), payload.c_str());
}

void pubMQTT(String topic, int payload) {
  char val[12];
  sprintf(val, "%d", payload);
  pubMQTT(topic.c_str(), val);
}

void pubMQTT(String topic, unsigned long long payload) {
  char val[21];
  sprintf(val, "%llu", payload);
  pubMQTT(topic.c_str(), val);
}

void pubMQTT(String topic, float payload) {
  char val[12];
  dtostrf(payload, 3, 1, val);
  pubMQTT(topic.c_str(), val);
}

void pubMQTT(const char* topic, float payload) {
  char val[12];
  dtostrf(payload, 3, 1, val);
  pubMQTT(topic, val);
}

void pubMQTT(const char* topic, int payload) {
  char val[12];
  sprintf(val, "%d", payload);
  pubMQTT(topic, val);
}

void pubMQTT(const char* topic, unsigned int payload) {
  char val[12];
  sprintf(val, "%u", payload);
  pubMQTT(topic, val);
}

void pubMQTT(const char* topic, long payload) {
  char val[11];
  sprintf(val, "%ld", payload);
  pubMQTT(topic, val);
}

void pubMQTT(const char* topic, double payload) {
  char val[16];
  sprintf(val, "%f", payload);
  pubMQTT(topic, val);
}

void pubMQTT(String topic, unsigned long payload) {
  char val[11];
  sprintf(val, "%lu", payload);
  pubMQTT(topic.c_str(), val);
}

bool cmpToMainTopic(const char* topicOri, const char* toAdd) {
  // Is string "<mqtt_topic><gateway_name><toAdd>" equal to "<topicOri>"?
  // Compare first part with first chunk
  if (strncmp(topicOri, mqtt_topic, strlen(mqtt_topic)) != 0)
    return false;
  // Move pointer of sizeof chunk
  topicOri += strlen(mqtt_topic);
  // And so on...
  if (strncmp(topicOri, gateway_name, strlen(gateway_name)) != 0)
    return false;
  topicOri += strlen(gateway_name);
  if (strncmp(topicOri, toAdd, strlen(toAdd)) != 0)
    return false;
  return true;
}

void delayWithOTA(long waitMillis) {
  long waitStep = 100;
  for (long waitedMillis = 0; waitedMillis < waitMillis; waitedMillis += waitStep) {
#ifndef ESPWifiManualSetup
    checkButton(); // check if a reset of wifi/mqtt settings is asked
#endif
    ArduinoOTA.handle();
#if defined(ZwebUI) && defined(ESP32)
    WebUILoop();
#endif
#ifdef ESP32
    //esp_task_wdt_reset();
#endif
    delay(waitStep);
  }
}

void SYSConfig_init() {
#ifdef ZmqttDiscovery
  SYSConfig.discovery = true;
  SYSConfig.ohdiscovery = OpenHABDiscovery;
#endif
#ifdef RGB_INDICATORS
  SYSConfig.rgbbrightness = DEFAULT_ADJ_BRIGHTNESS;
#endif
}

void SYSConfig_fromJson(JsonObject& SYSdata) {
#ifdef ZmqttDiscovery
  Config_update(SYSdata, "disc", SYSConfig.discovery);
  Config_update(SYSdata, "ohdisc", SYSConfig.ohdiscovery);
#endif
#ifdef RGB_INDICATORS
  Config_update(SYSdata, "rgbb", SYSConfig.rgbbrightness);
#endif
}

#if defined(ESP32)
void SYSConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  if (preferences.isKey("SYSConfig")) {
    auto error = deserializeJson(jsonBuffer, preferences.getString("SYSConfig", "{}"));
    preferences.end();
    if (error) {
      Log.error(F("SYS config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      return;
    }
    if (jsonBuffer.isNull()) {
      Log.warning(F("SYS config is null" CR));
      return;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    SYSConfig_fromJson(jo);
    Log.notice(F("SYS config loaded" CR));
  } else {
    preferences.end();
    Log.notice(F("SYS config not found" CR));
  }
}
#else // Function not available for ESP8266
void SYSConfig_load() {}
#endif

void connectMQTT() {
#ifndef ESPWifiManualSetup
  checkButton(); // check if a reset of wifi/mqtt settings is asked
#endif

  Log.warning(F("MQTT connection..." CR));
#ifdef ESP32
  //esp_task_wdt_add(NULL);
  //esp_task_wdt_reset();
#endif
  char topic[mqtt_topic_max_size];
  strcpy(topic, mqtt_topic);
  strcat(topic, gateway_name);
  strcat(topic, will_Topic);
  client.setBufferSize(mqtt_max_packet_size);
  client.setSocketTimeout(GeneralTimeOut - 1);
#if AWS_IOT
  if (client.connect(gateway_name, mqtt_user, mqtt_pass)) { // AWS doesn't support will topic for the moment
#else
  if (client.connect(gateway_name, mqtt_user, mqtt_pass, topic, will_QoS, will_Retain, will_Message)) {
#endif

    displayPrint("MQTT connected");
    Log.notice(F("Connected to broker" CR));
    failure_number_mqtt = 0;
    // Once connected, publish an announcement...
    pub(will_Topic, Gateway_AnnouncementMsg, will_Retain);
    //Subscribing to topic
    char topic2[mqtt_topic_max_size];
    strcpy(topic2, mqtt_topic);
    strcat(topic2, gateway_name);
    strcat(topic2, subjectMQTTtoX);
    if (client.subscribe(topic2)) {
#ifdef ZgatewayRF
      client.subscribe(subjectMultiGTWRF); // subject on which other OMG will publish, this OMG will store these msg and by the way don't republish them if they have been already published
#endif
#ifdef ZgatewayIR
      client.subscribe(subjectMultiGTWIR); // subject on which other OMG will publish, this OMG will store these msg and by the way don't republish them if they have been already published
#endif
      Log.trace(F("Subscription OK to the subjects %s" CR), topic2);
    }
  } else {
    failure_number_mqtt++; // we count the failure
    Log.warning(F("failure_number_mqtt: %d" CR), failure_number_mqtt);
    Log.warning(F("failed, rc=%d" CR), client.state());
#if defined(ESP32)
    if (mqtt_secure)
      Log.warning(F("failed, ssl error code=%d" CR), ((WiFiClientSecure*)eClient)->lastError(nullptr, 0));
#elif defined(ESP8266)
    if (mqtt_secure)
      Log.warning(F("failed, ssl error code=%d" CR), ((WiFiClientSecure*)eClient)->getLastSSLError());
#endif
    ErrorIndicatorON();
    delayWithOTA(5000);
    ErrorIndicatorOFF();
    delayWithOTA(5000);
    if (failure_number_mqtt > maxRetryWatchDog) {
      unsigned long millis_since_last_ota;
      while (
          // When
          // ...incomplete OTA in progress
          (last_ota_activity_millis != 0)
          // ...AND last OTA activity fairly recently
          && ((millis_since_last_ota = millis() - last_ota_activity_millis) < ota_timeout_millis)) {
        // ... We consider that OTA might be still active, and we sleep for a while, and giving
        // OTA chance to proceed (ArduinoOTA.handle())
        Log.warning(F("OTA might be still active (activity %d ms ago)" CR), millis_since_last_ota);
        ArduinoOTA.handle();
        delay(100);
      }
      ESPRestart(1);
    }
  }
}

// Callback function, when the gateway receive an MQTT value on the topics subscribed this function is called
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the original payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  Log.trace(F("Hey I got a callback %s" CR), topic);
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);
  // Conversion to a printable string
  p[length] = '\0';
  //launch the function to treat received data if this data concern OpenMQTTGateway
  if ((strstr(topic, subjectMultiGTWKey) != NULL) ||
      (strstr(topic, subjectGTWSendKey) != NULL) ||
      (strstr(topic, subjectMQTTtoSYSupdate) != NULL))
    receivingMQTT(topic, (char*)p);

  // Free the memory
  free(p);
}

#if defined(ESP32) && (defined(WifiGMode) || defined(WifiPower))
void setESPWifiProtocolTxPower() {
  //Reduce WiFi interference when using ESP32 using custom WiFi mode and tx power
  //https://github.com/espressif/arduino-esp32/search?q=WIFI_PROTOCOL_11G
  //https://www.letscontrolit.com/forum/viewtopic.php?t=671&start=20
#  if WifiGMode == true
  if (esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G) != ESP_OK) {
    Log.error(F("Failed to change WifiMode." CR));
  }
#  endif

#  if WifiGMode == false
  if (esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N) != ESP_OK) {
    Log.error(F("Failed to change WifiMode." CR));
  }
#  endif

  uint8_t getprotocol;
  esp_err_t err;
  err = esp_wifi_get_protocol(WIFI_IF_STA, &getprotocol);

  if (err != ESP_OK) {
    Log.notice(F("Could not get protocol!" CR));
  }
  if (getprotocol & WIFI_PROTOCOL_11N) {
    Log.notice(F("WiFi_Protocol_11n" CR));
  }
  if (getprotocol & WIFI_PROTOCOL_11G) {
    Log.notice(F("WiFi_Protocol_11g" CR));
  }
  if (getprotocol & WIFI_PROTOCOL_11B) {
    Log.notice(F("WiFi_Protocol_11b" CR));
  }

#  ifdef WifiPower
  Log.notice(F("Requested WiFi power level: %i" CR), WifiPower);
  WiFi.setTxPower(WifiPower);
#  endif
  Log.notice(F("Operating WiFi power level: %i" CR), WiFi.getTxPower());
}
#endif

#if defined(ESP8266) && (defined(WifiGMode) || defined(WifiPower))
void setESPWifiProtocolTxPower() {
#  if WifiGMode == true
  if (!wifi_set_phy_mode(PHY_MODE_11G)) {
    Log.error(F("Failed to change WifiMode." CR));
  }
#  endif

#  if WifiGMode == false
  if (!wifi_set_phy_mode(PHY_MODE_11N)) {
    Log.error(F("Failed to change WifiMode." CR));
  }
#  endif

  phy_mode_t getprotocol = wifi_get_phy_mode();
  if (getprotocol == PHY_MODE_11N) {
    Log.notice(F("WiFi_Protocol_11n" CR));
  }
  if (getprotocol == PHY_MODE_11G) {
    Log.notice(F("WiFi_Protocol_11g" CR));
  }
  if (getprotocol == PHY_MODE_11B) {
    Log.notice(F("WiFi_Protocol_11b" CR));
  }

#  ifdef WifiPower
  Log.notice(F("Requested WiFi power level: %i dBm" CR), WifiPower);

  int i_dBm = int(WifiPower * 4.0f);

  // i_dBm 82 == 20.5 dBm
  if (i_dBm > 82) {
    i_dBm = 82;
  } else if (i_dBm < 0) {
    i_dBm = 0;
  }

  system_phy_set_max_tpw((uint8_t)i_dBm);
#  endif
}
#endif

void setup() {
  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  Log.begin(LOG_LEVEL, &Serial);
  Log.notice(F(CR "************* WELCOME TO OpenMQTTGateway **************" CR));
#if defined(TRIGGER_GPIO) && !defined(ESPWifiManualSetup)
  pinMode(TRIGGER_GPIO, INPUT_PULLUP);
  checkButton();
#endif

  delay(100); //give time to start the flash and avoid issue when reading the preferences

  SYSConfig_init();
  SYSConfig_load();

  //setup LED status
  SetupIndicatorError();
  SetupIndicatorSendReceive();
  SetupIndicatorInfo();
  SetupIndicators(); // For RGB Leds

#ifdef ESP8266
#  ifndef ZgatewaySRFB // if we are not in sonoff rf bridge case we apply the ESP8266 GPIO optimization
  Serial.end();
  Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY); // enable on ESP8266 to free some pin
#  endif
#elif ESP32
  xQueueMutex = xSemaphoreCreateMutex();
#  if DEFAULT_LOW_POWER_MODE != -1 //  don't check preferences value if low power mode is not activated
  preferences.begin(Gateway_Short_Name, false);
  if (preferences.isKey("lowpowermode")) {
    lowpowermode = preferences.getUInt("lowpowermode", DEFAULT_LOW_POWER_MODE);
  } else {
    Log.notice(F("No lowpowermode config to load" CR));
  }
  preferences.end();
#  endif
#  if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
  setupM5();
#  endif
#  if defined(ZdisplaySSD1306)
  setupSSD1306();
  modules.add(ZdisplaySSD1306);
#  endif
#endif

  Log.notice(F("OpenMQTTGateway Version: " OMG_VERSION CR));

/**
 * Deep-sleep for the ESP8266 & ESP32 we need some form of indicator that we have posted the measurements and am ready to deep sleep.
 * When woken set back to false.
 */
#if defined(DEEP_SLEEP_IN_US) || defined(ESP32_EXT0_WAKE_PIN)
  ready_to_sleep = false;
#endif

#ifdef DEEP_SLEEP_IN_US
#  ifdef ESP8266
  Log.notice(F("Setting wake pin for deep sleep." CR));
  pinMode(ESP8266_DEEP_SLEEP_WAKE_PIN, WAKEUP_PULLUP);
#  endif
#  ifdef ESP32
  Log.notice(F("Setting duration for deep sleep." CR));
  if (esp_sleep_enable_timer_wakeup(DEEP_SLEEP_IN_US) != ESP_OK) {
    Log.error(F("Failed to set deep sleep duration." CR));
  }
#  endif
#endif

#ifdef ESP32_EXT0_WAKE_PIN
  Log.notice(F("Setting EXT0 Wakeup for deep sleep." CR));
  if (esp_sleep_enable_ext0_wakeup(ESP32_EXT0_WAKE_PIN, ESP32_EXT0_WAKE_PIN_STATE) != ESP_OK) {
    Log.error(F("Failed to set deep sleep EXT0 Wakeup." CR));
  }
#endif
#ifdef ESP32
  //esp_task_wdt_init(GeneralTimeOut, true); //enable panic so ESP32 restarts
#endif
/*
 The 2 modules below are not connection dependent so start them before the connectivity functions
 Note that the ONOFF module need to start after the RN8209 so that the overCurrent function is launched after the setup of the sensor
*/
#ifdef ZsensorRN8209
  setupRN8209();
  modules.add(ZsensorRN8209);
#endif
#ifdef ZactuatorONOFF
  setupONOFF();
  modules.add(ZactuatorONOFF);
#endif

#if defined(ESPWifiManualSetup)
  setup_wifi();
#else
  if (loadConfigFromFlash()) {
    Log.notice(F("Config loaded from flash" CR));
#  ifdef ESP32_ETHERNET
    setup_ethernet_esp32();
#  endif
    if (!failSafeMode && !ethConnected) setupwifi(false);
  } else {
#  ifdef ESP32_ETHERNET
    setup_ethernet_esp32();
#  endif
    Log.notice(F("No config in flash, launching wifi manager" CR));
    // In failSafeMode we don't want to setup wifi manager as it has already been done before
    if (!failSafeMode) setupwifi(false);
  }

#endif
  Log.trace(F("OpenMQTTGateway mac: %s" CR), WiFi.macAddress().c_str());
  Log.trace(F("OpenMQTTGateway ip: %s" CR), WiFi.localIP().toString().c_str());

  setOTA();

#if defined(ZwebUI) && defined(ESP32)
  WebUISetup();
  modules.add(ZwebUI);
#endif

  if (mqtt_secure) {
    eClient = new WiFiClientSecure;
    if (mqtt_cert_validate) {
      setupTLS(MQTT_SECURE_SELF_SIGNED, mqtt_ss_index);
    } else {
      WiFiClientSecure* sClient = (WiFiClientSecure*)eClient;
      sClient->setInsecure();
    }
  } else {
    eClient = new WiFiClient;
  }
  client.setClient(*(Client*)eClient);

#if defined(MDNS_SD)
  Log.trace(F("Connecting to MQTT by mDNS without MQTT hostname" CR));
  connectMQTTmdns();
#else
  uint16_t port = strtol(mqtt_port, NULL, 10);
  Log.trace(F("Port: %l" CR), port);
  Log.trace(F("Mqtt server: %s" CR), mqtt_server);
  client.setServer(mqtt_server, port);
#endif

  client.setCallback(callback);

  delay(1500);
#if defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRTL_433) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)
  setupCommonRF();
#endif
#ifdef ZsensorBME280
  setupZsensorBME280();
  modules.add(ZsensorBME280);
#endif
#ifdef ZsensorHTU21
  setupZsensorHTU21();
  modules.add(ZsensorHTU21);
#endif
#ifdef ZsensorLM75
  setupZsensorLM75();
  modules.add(ZsensorLM75);
#endif
#ifdef ZsensorAHTx0
  setupZsensorAHTx0();
  modules.add(ZsensorAHTx0);
#endif
#ifdef ZsensorBH1750
  setupZsensorBH1750();
  modules.add(ZsensorBH1750);
#endif
#ifdef ZsensorMQ2
  setupZsensorMQ2();
  modules.add(ZsensorMQ2);
#endif
#ifdef ZsensorTEMT6000
  setupZsensorTEMT6000();
  modules.add(ZsensorTEMT6000);
#endif
#ifdef ZsensorTSL2561
  setupZsensorTSL2561();
  modules.add(ZsensorTSL2561);
#endif
#ifdef Zgateway2G
  setup2G();
  modules.add(Zgateway2G);
#endif
#ifdef ZgatewayIR
  setupIR();
  modules.add(ZgatewayIR);
#endif
#ifdef ZgatewayLORA
  setupLORA();
  modules.add(ZgatewayLORA);
#endif
#ifdef ZgatewayRF
  modules.add(ZgatewayRF);
#  define ACTIVE_RECEIVER ACTIVE_RF
#endif
#ifdef ZgatewayRF2
  modules.add(ZgatewayRF2);
#  ifdef ACTIVE_RECEIVER
#    undef ACTIVE_RECEIVER
#  endif
#  define ACTIVE_RECEIVER ACTIVE_RF2
#endif
#ifdef ZgatewayPilight
  modules.add(ZgatewayPilight);
#  ifdef ACTIVE_RECEIVER
#    undef ACTIVE_RECEIVER
#  endif
#  define ACTIVE_RECEIVER ACTIVE_PILIGHT
#endif
#ifdef ZgatewayWeatherStation
  setupWeatherStation();
  modules.add(ZgatewayWeatherStation);
#endif
#ifdef ZgatewayGFSunInverter
  setupGFSunInverter();
  modules.add(ZgatewayGFSunInverter);
#endif
#ifdef ZgatewaySRFB
  setupSRFB();
  modules.add(ZgatewaySRFB);
#endif
#ifdef ZgatewayBT
  setupBT();
  modules.add(ZgatewayBT);
#endif
#ifdef ZgatewayRFM69
  setupRFM69();
  modules.add(ZgatewayRFM69);
#endif
#ifdef ZsensorINA226
  setupINA226();
  modules.add(ZsensorINA226);
#endif
#ifdef ZsensorHCSR501
  setupHCSR501();
  modules.add(ZsensorHCSR501);
#endif
#ifdef ZsensorHCSR04
  setupHCSR04();
  modules.add(ZsensorHCSR04);
#endif
#ifdef ZsensorGPIOInput
  setupGPIOInput();
  modules.add(ZsensorGPIOInput);
#endif
#ifdef ZsensorGPIOKeyCode
  setupGPIOKeyCode();
  modules.add(ZsensorGPIOKeyCode);
#endif
#ifdef ZactuatorFASTLED
  setupFASTLED();
  modules.add(ZactuatorFASTLED);
#endif
#ifdef ZactuatorPWM
  setupPWM();
  modules.add(ZactuatorPWM);
#endif
#ifdef ZactuatorSomfy
#  ifdef ACTIVE_RECEIVER
#    undef ACTIVE_RECEIVER
#  endif
#  define ACTIVE_RECEIVER ACTIVE_NONE
  setupSomfy();
  modules.add(ZactuatorSomfy);
#endif
#ifdef ZsensorDS1820
  setupZsensorDS1820();
  modules.add(ZsensorDS1820);
#endif
#ifdef ZsensorADC
  setupADC();
  modules.add(ZsensorADC);
#endif
#ifdef ZsensorTouch
  setupTouch();
  modules.add(ZsensorTouch);
#endif
#ifdef ZsensorC37_YL83_HMRD
  setupZsensorC37_YL83_HMRD();
  modules.add(ZsensorC37_YL83_HMRD);
#endif
#ifdef ZsensorDHT
  setupDHT();
  modules.add(ZsensorDHT);
#endif
#ifdef ZgatewayRS232
  setupRS232();
  modules.add(ZgatewayRS232);
#endif
#ifdef ZsensorSHTC3
  setupSHTC3();
#endif
#ifdef ZgatewayRTL_433
#  ifdef ACTIVE_RECEIVER
#    undef ACTIVE_RECEIVER
#  endif
#  define ACTIVE_RECEIVER ACTIVE_RTL
  setupRTL_433();
  modules.add(ZgatewayRTL_433);
#endif
  Log.trace(F("mqtt_max_packet_size: %d" CR), mqtt_max_packet_size);

  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);
  Log.notice(F("OpenMQTTGateway modules: %s" CR), jsonChar);
  Log.notice(F("************** Setup OpenMQTTGateway end **************" CR));
}

// Bypass for ESP not reconnecting automaticaly the second time https://github.com/espressif/arduino-esp32/issues/2501
bool wifi_reconnect_bypass() {
#if defined(ESP32) && defined(USE_BLUFI)
  extern bool omg_blufi_ble_connected;
  if (omg_blufi_ble_connected) {
    Log.notice(F("BLUFI is connected, bypassing wifi reconnect" CR));
    return true;
  }
#endif
  uint8_t wifi_autoreconnect_cnt = 0;
#ifdef ESP32
  while (WiFi.status() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryNetwork) {
#else
  while (WiFi.waitForConnectResult() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryNetwork) {
#endif
    Log.notice(F("Attempting Wifi connection with saved AP: %d" CR), wifi_autoreconnect_cnt);

    WiFi.begin();
#if defined(WifiGMode) || defined(WifiPower)
    setESPWifiProtocolTxPower();
#endif
    delay(1000);
    wifi_autoreconnect_cnt++;
  }
  if (wifi_autoreconnect_cnt < maxConnectionRetryNetwork) {
    return true;
  } else {
    return false;
  }
}

void setOTA() {
  // Port defaults to 8266
  ArduinoOTA.setPort(ota_port);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ota_hostname);

  // No authentication by default
  ArduinoOTA.setPassword(ota_pass);

  ArduinoOTA.onStart([]() {
    Log.trace(F("Start OTA, lock other functions" CR));
    ErrorIndicatorON();
    SendReceiveIndicatorON();
    last_ota_activity_millis = millis();
#ifdef ESP32
    ProcessLock = true;
#  ifdef ZgatewayBT
    stopProcessing();
#  endif
#endif
    lpDisplayPrint("OTA in progress");
  });
  ArduinoOTA.onEnd([]() {
    Log.trace(F("\nOTA done" CR));
    last_ota_activity_millis = 0;
    ErrorIndicatorOFF();
    SendReceiveIndicatorOFF();
    lpDisplayPrint("OTA done");
    ESPRestart(6);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.trace(F("Progress: %u%%\r" CR), (progress / (total / 100)));
#ifdef ESP32
    //esp_task_wdt_reset();
#endif
    last_ota_activity_millis = millis();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    last_ota_activity_millis = millis();
    ErrorIndicatorOFF();
    SendReceiveIndicatorOFF();
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Log.error(F("Auth Failed" CR));
    else if (error == OTA_BEGIN_ERROR)
      Log.error(F("Begin Failed" CR));
    else if (error == OTA_CONNECT_ERROR)
      Log.error(F("Connect Failed" CR));
    else if (error == OTA_RECEIVE_ERROR)
      Log.error(F("Receive Failed" CR));
    else if (error == OTA_END_ERROR)
      Log.error(F("End Failed" CR));
    ESPRestart(6);
  });
  ArduinoOTA.begin();
}

void setupTLS(bool self_signed, uint8_t index) {
  configTime(0, 0, NTP_SERVER);
  WiFiClientSecure* sClient = (WiFiClientSecure*)eClient;
#if MQTT_SECURE_SELF_SIGNED
  if (self_signed) {
    Log.notice(F("Using self signed cert index %u" CR), index);
#  if defined(ESP32)
    sClient->setCACert(certs_array[index].server_cert);
#    if AWS_IOT
    if (strcmp(mqtt_port, "443") == 0) {
      sClient->setAlpnProtocols(alpnProtocols);
    }
#    endif
#    if MQTT_SECURE_SELF_SIGNED_CLIENT
    sClient->setCertificate(certs_array[index].client_cert);
    sClient->setPrivateKey(certs_array[index].client_key);
#    endif
#  elif defined(ESP8266)
    caCert.append(certs_array[index].server_cert);
    sClient->setTrustAnchors(&caCert);
    sClient->setBufferSizes(512, 512);
#    if MQTT_SECURE_SELF_SIGNED_CLIENT
    if (pClCert != nullptr) {
      delete pClCert;
    }
    if (pClKey != nullptr) {
      delete pClKey;
    }
    pClCert = new X509List(certs_array[index].client_cert);
    pClKey = new PrivateKey(certs_array[index].client_key);
    sClient->setClientRSACert(pClCert, pClKey);
#    endif
#  endif
  } else
#endif
  {
    if (mqtt_cert.length() > 0) {
#if defined(ESP32)
      sClient->setCACert(mqtt_cert.c_str());
    } else {
      sClient->setCACert(certificate);
    }
#elif defined(ESP8266)
      caCert.append(mqtt_cert.c_str());
    } else {
      caCert.append(certificate);
    }
    sClient->setTrustAnchors(&caCert);
    sClient->setBufferSizes(512, 512);
#endif
  }
}

/*
  Reboot for Reason Codes
  1 - Repeated MQTT Connection Failure
  2 - Repeated WiFi Connection Failure
  3 - Failed WiFiManager configuration portal
  4 - BLE Scan watchdog
  5 - User requested reboot
  6 - OTA Update
  7 - Parameters changed
  8 - not enough memory to pursue
*/
void ESPRestart(byte reason) {
  delay(1000);
  StaticJsonDocument<128> jsonBuffer;
  JsonObject jsondata = jsonBuffer.to<JsonObject>();
  jsondata["reason"] = reason;
  jsondata["retain"] = true;
  jsondata["uptime"] = uptime();
  pub(subjectLOGtoMQTT, jsondata);
  Log.warning(F("Rebooting for reason code %d" CR), reason);
#if defined(ESP32)
  ESP.restart();
#elif defined(ESP8266)
  ESP.reset();
#endif
}

#if defined(ESPWifiManualSetup)
void setup_wifi() {
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(wifi_ssid, wifi_password);
  Log.trace(F("Connecting to %s" CR), wifi_ssid);
#  ifdef wifi_ssid1
  wifiMulti.addAP(wifi_ssid1, wifi_password1);
  Log.trace(F("Connecting to %s" CR), wifi_ssid1);
#  endif
  delay(10);

  // We start by connecting to a WiFi network

#  ifdef NetworkAdvancedSetup
  IPAddress ip_adress;
  IPAddress gateway_adress;
  IPAddress subnet_adress;
  IPAddress dns_adress;
  ip_adress.fromString(NET_IP);
  gateway_adress.fromString(NET_GW);
  subnet_adress.fromString(NET_MASK);
  dns_adress.fromString(NET_DNS);

  if (!WiFi.config(ip_adress, gateway_adress, subnet_adress, dns_adress)) {
    Log.error(F("Wifi STA Failed to configure" CR));
  }

#  endif

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Log.trace(F("." CR));
    failure_number_ntwk++;
#  if defined(ESP32) && defined(ZgatewayBT)
    if (lowpowermode) {
      if (failure_number_ntwk > maxConnectionRetryNetwork) {
        lowPowerESP32();
      }
    } else {
      if (failure_number_ntwk > maxRetryWatchDog) {
        ESPRestart(2);
      }
    }
#  else
    if (failure_number_ntwk > maxRetryWatchDog) {
      ESPRestart(2);
    }
#  endif
  }
  Log.notice(F("WiFi ok with manual config credentials" CR));
  displayPrint("Wifi connected");
}

#else

WiFiManager wifiManager;

//flag for saving data
bool shouldSaveConfig = false;
//do we have been connected once to MQTT

//callback notifying us of the need to save config
void saveConfigCallback() {
  Log.trace(F("Should save config" CR));
  shouldSaveConfig = true;
}

#  ifdef TRIGGER_GPIO
/**
 * Identify a long button press to trigger a reset or a failsafe mode
* */
void blockingWaitForReset() {
  if (digitalRead(TRIGGER_GPIO) == LOW) {
    delay(50);
    if (digitalRead(TRIGGER_GPIO) == LOW) {
      Log.trace(F("Trigger button Pressed" CR));
      delay(3000); // reset delay hold
      if (digitalRead(TRIGGER_GPIO) == LOW) {
        Log.trace(F("Button Held" CR));
// Switching off the relay during reset or failsafe operations
#    ifdef ZactuatorONOFF
        uint8_t level = digitalRead(ACTUATOR_ONOFF_GPIO);
        if (level == ACTUATOR_ON) {
          ActuatorTrigger();
        }
#    endif
#    ifdef ESP32
        //esp_task_wdt_delete(NULL);
#    endif
        InfoIndicatorOFF();
        SendReceiveIndicatorOFF();
        // Checking if the flash has already been erased to identify if we erase it or go into failsafe mode
        // going to failsafe mode is done by doing a long button press from a state where the flash has already been erased
        if (SPIFFS.begin()) {
          Log.trace(F("mounted file system" CR));
          if (SPIFFS.exists("/config.json")) {
            Log.notice(F("Erasing ESP Config, restarting" CR));
            setupwifi(true);
          }
        }
        delay(30000);
        if (digitalRead(TRIGGER_GPIO) == LOW) {
          Log.notice(F("Going into failsafe mode without peripherals" CR));
          // Failsafe mode enable to connect to Wifi or change the firmware without the peripherals setup
          failSafeMode = true;
          setupwifi(false);
        }
      }
    }
  }
}

/**
 * Check if button is pressed so as to reset the credentials and parameters stored into the flash
* */
void checkButton() {
  unsigned long timeFromStart = millis();
  // Identify if the reset button is pushed at start
  if (timeFromStart < TimeToResetAtStart) {
    blockingWaitForReset();
  } else { // When we are not at start we either check the button as a regular input (ZsensorGPIOInput used) or for a reset
#    if defined(INPUT_GPIO) && defined(ZsensorGPIOInput) && INPUT_GPIO == TRIGGER_GPIO
    MeasureGPIOInput();
#    else
    blockingWaitForReset();
#    endif
  }
}
#  else
void checkButton() {}
#  endif

void saveConfig() {
  Log.trace(F("saving config" CR));
  DynamicJsonDocument json(512 + ota_server_cert.length() + mqtt_cert.length());
  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["mqtt_user"] = mqtt_user;
  json["mqtt_pass"] = mqtt_pass;
  json["mqtt_topic"] = mqtt_topic;
  json["gateway_name"] = gateway_name;
  json["mqtt_broker_secure"] = mqtt_secure;
  json["mqtt_broker_cert"] = mqtt_cert;
  json["mqtt_ss_index"] = mqtt_ss_index;
  json["ota_server_cert"] = ota_server_cert;
  json["ota_pass"] = ota_pass;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Log.error(F("failed to open config file for writing" CR));
  }

  serializeJson(json, configFile);
  configFile.close();
}

bool loadConfigFromFlash() {
  Log.trace(F("mounting FS..." CR));
  bool result = false;

  if (SPIFFS.begin()) {
    Log.trace(F("mounted file system" CR));
  } else {
    Log.warning(F("failed to mount FS -> formating" CR));
    SPIFFS.format();
    if (SPIFFS.begin())
      Log.trace(F("mounted file system after formating" CR));
  }
  if (SPIFFS.exists("/config.json")) {
    //file exists, reading and loading
    Log.trace(F("reading config file" CR));
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      Log.trace(F("opened config file" CR));
      DynamicJsonDocument json(configFile.size() * 2);
      auto error = deserializeJson(json, configFile);
      if (error) {
        Log.error(F("deserialize config failed: %s, buffer capacity: %u" CR), error.c_str(), json.capacity());
      }
      if (!json.isNull()) {
        Log.trace(F("\nparsed json, size: %u" CR), json.memoryUsage());
        if (json.containsKey("mqtt_server"))
          strcpy(mqtt_server, json["mqtt_server"]);
        if (json.containsKey("mqtt_port"))
          strcpy(mqtt_port, json["mqtt_port"]);
        if (json.containsKey("mqtt_user"))
          strcpy(mqtt_user, json["mqtt_user"]);
        if (json.containsKey("mqtt_pass"))
          strcpy(mqtt_pass, json["mqtt_pass"]);
        if (json.containsKey("mqtt_topic"))
          strcpy(mqtt_topic, json["mqtt_topic"]);
        if (json.containsKey("mqtt_broker_secure"))
          mqtt_secure = json["mqtt_broker_secure"].as<bool>();
        if (json.containsKey("mqtt_broker_cert"))
          mqtt_cert = json["mqtt_broker_cert"].as<const char*>();
        if (json.containsKey("mqtt_ss_index"))
          mqtt_ss_index = json["mqtt_ss_index"].as<uint8_t>();
        if (json.containsKey("gateway_name"))
          strcpy(gateway_name, json["gateway_name"]);
        if (json.containsKey("ota_pass")) {
          strcpy(ota_pass, json["ota_pass"]);
#  ifdef WM_PWD_FROM_MAC // From ESP Mac Address, last 8 digits as the password
          // Compare the existing ota_pass if ota_pass = OTAPASSWORD then replace with the last 8 digits of the mac address
          // This enable user migrating from previous version to have the same WiFi portal password as previously unless they changed it
          if (strcmp(ota_pass, "OTAPASSWORD") == 0) {
            String s = WiFi.macAddress();
            sprintf(ota_pass, "%.2s%.2s%.2s%.2s",
                    s.c_str() + 6, s.c_str() + 9, s.c_str() + 12, s.c_str() + 15);
          }
#  endif
        }
        if (json.containsKey("ota_server_cert"))
          ota_server_cert = json["ota_server_cert"].as<const char*>();
        result = true;
      } else {
        Log.warning(F("failed to load json config" CR));
      }
      configFile.close();
    }
  } else {
    Log.notice(F("no config file found defining default values" CR));
#  ifdef USE_MAC_AS_GATEWAY_NAME
    String s = WiFi.macAddress();
    sprintf(gateway_name, "%.2s%.2s%.2s%.2s%.2s%.2s",
            s.c_str(), s.c_str() + 3, s.c_str() + 6, s.c_str() + 9, s.c_str() + 12, s.c_str() + 15);
#  endif
#  ifdef WM_PWD_FROM_MAC // From ESP Mac Address, last 8 digits as the password
    sprintf(ota_pass, "%.2s%.2s%.2s%.2s",
            s.c_str() + 6, s.c_str() + 9, s.c_str() + 12, s.c_str() + 15);
#  endif
  }

  return result;
}

void setupwifi(bool reset_settings) {
  delay(10);
  WiFi.mode(WIFI_STA);

  if (reset_settings)
    eraseAndRestart();

#  if defined(ESP32) && defined(USE_BLUFI)
  if (!wifi_reconnect_bypass()) {
    startBlufi();
    return;
  }
#  endif

#  ifdef USE_MAC_AS_GATEWAY_NAME
  String s = WiFi.macAddress();
  snprintf(WifiManager_ssid, MAC_NAME_MAX_LEN, "%s_%.2s%.2s", Gateway_Short_Name, s.c_str(), s.c_str() + 3);
  strcpy(ota_hostname, WifiManager_ssid);
  Log.notice(F("OTA Hostname: %s.local" CR), ota_hostname);
#  endif

  wifiManager.setDebugOutput(WM_DEBUG);

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
#  ifndef WIFIMNG_HIDE_MQTT_CONFIG
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, parameters_size, " minlength='1' maxlength='64' required");
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6, " minlength='1' maxlength='5' required");
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, parameters_size, " maxlength='64'");
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", MQTT_PASS, parameters_size, " input type='password' maxlength='64'");
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt base topic", mqtt_topic, mqtt_topic_max_size, " minlength='1' maxlength='64' required");
  WiFiManagerParameter custom_mqtt_secure("secure", "mqtt secure", "1", 2, mqtt_secure ? "type=\"checkbox\" checked" : "type=\"checkbox\"");
  WiFiManagerParameter custom_mqtt_cert("cert", "<br/>mqtt broker cert", mqtt_cert.c_str(), 4096);
  WiFiManagerParameter custom_gateway_name("name", "gateway name", gateway_name, parameters_size, " minlength='1' maxlength='64' required");
  WiFiManagerParameter custom_ota_pass("ota", "gateway password", ota_pass, parameters_size, " input type='password' minlength='8' maxlength='64' required");
#  endif
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around

  wifiManager.setConnectTimeout(WiFi_TimeOut);
  //Set timeout before going to portal
  wifiManager.setConfigPortalTimeout(WifiManager_ConfigPortalTimeOut);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

//set static IP
#  ifdef NetworkAdvancedSetup
  Log.trace(F("Adv wifi cfg" CR));
  IPAddress ip_adress;
  IPAddress gateway_adress;
  IPAddress subnet_adress;
  IPAddress dns_adress;
  ip_adress.fromString(NET_IP);
  gateway_adress.fromString(NET_GW);
  subnet_adress.fromString(NET_MASK);
  dns_adress.fromString(NET_DNS);
  wifiManager.setSTAStaticIPConfig(ip_adress, gateway_adress, subnet_adress, dns_adress);
#  endif

#  ifndef WIFIMNG_HIDE_MQTT_CONFIG
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_secure);
  wifiManager.addParameter(&custom_mqtt_cert);
  wifiManager.addParameter(&custom_gateway_name);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.addParameter(&custom_ota_pass);
#  endif
  //set minimum quality of signal so it ignores AP's under that quality
  wifiManager.setMinimumSignalQuality(MinimumWifiSignalQuality);

  if (SPIFFS.begin()) {
    Log.trace(F("mounted file system" CR));
    // Check if the config file exists and prevent the portal from showing if yes
    // Showing the portal if the config file exist would enable access to the configuration data and to the ESP update page
    // This is a security risk if an attacker has access to the gateway password
    if (SPIFFS.exists("/config.json")) {
      wifiManager.setEnableConfigPortal(false);
    }
  }

#  ifdef ESP32_ETHERNET
  wifiManager.setBreakAfterConfig(true); // If ethernet is used, we don't want to block the connection by keeping the portal up
#  endif

  if (!wifi_reconnect_bypass()) // if we didn't connect with saved credential we start Wifimanager web portal
  {
#  ifdef ESP32
    if (lowpowermode < 2) {
      displayPrint("Connect your phone to WIFI AP:", WifiManager_ssid, ota_pass);
    } else { // in case of low power mode we put the ESP to sleep again if we didn't get connected (typical in case the wifi is down)
#    ifdef ZgatewayBT
      lowPowerESP32();
#    endif
    }
#  endif

    InfoIndicatorON();
    ErrorIndicatorON();
    Log.notice(F("Connect your phone to WIFI AP: %s with PWD: %s" CR), WifiManager_ssid, ota_pass);

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect(WifiManager_ssid, ota_pass)) {
      if (!ethConnected) { // If we are using ethernet, we don't want to restart the ESP
        Log.warning(F("failed to connect and hit timeout" CR));
        delay(3000);

#  ifdef ESP32
        /* Workaround for bug in arduino core that causes the AP to become unsecure on reboot */
        esp_wifi_set_mode(WIFI_MODE_AP);
        esp_wifi_start();
        wifi_config_t conf;
        esp_wifi_get_config(WIFI_IF_AP, &conf);
        conf.ap.ssid_hidden = 1;
        esp_wifi_set_config(WIFI_IF_AP, &conf);
#  endif

        //restart and try again
        ESPRestart(3);
      }
    }
    InfoIndicatorOFF();
    ErrorIndicatorOFF();
  }

  displayPrint("Wifi connected");

  if (shouldSaveConfig) {
    //read updated parameters
#  ifndef WIFIMNG_HIDE_MQTT_CONFIG
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    // Check if the MQTT password field contains the default value
    if (strcmp(custom_mqtt_pass.getValue(), MQTT_PASS) != 0) {
      // If it's not the default password, update the MQTT password
      strcpy(mqtt_pass, custom_mqtt_pass.getValue());
    }
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    if (mqtt_topic[strlen(mqtt_topic) - 1] != '/' && strlen(mqtt_topic) < parameters_size) {
      strcat(mqtt_topic, "/");
    }

    strcpy(gateway_name, custom_gateway_name.getValue());
    strcpy(ota_pass, custom_ota_pass.getValue());
    mqtt_secure = *custom_mqtt_secure.getValue();

    int cert_len = strlen(custom_mqtt_cert.getValue());
    if (cert_len) {
      char* cert_in = (char*)custom_mqtt_cert.getValue();
      while (*cert_in == ' ' || *cert_in == '\t') {
        cert_in++;
      }

      char* cert_begin = cert_in;
      while (*cert_in != NULL) {
        if (*cert_in == ' ' && (strncmp((cert_in + 1), "CERTIFICATE", 11) != 0)) {
          *cert_in = '\n';
        }
        cert_in++;
      }

      mqtt_cert = cert_begin;
    }
#  endif
    //save the custom parameters to FS
    saveConfig();
  }
}
#  ifdef ESP32_ETHERNET
void setup_ethernet_esp32() {
  bool ethBeginSuccess = false;
  WiFi.onEvent(WiFiEvent);
#    ifdef NetworkAdvancedSetup
  IPAddress ip_adress;
  IPAddress gateway_adress;
  IPAddress subnet_adress;
  IPAddress dns_adress;
  ip.fromString(NET_IP);
  gateway.fromString(NET_GW);
  subnet.fromString(NET_MASK);
  Dns.fromString(NET_DNS);

  Log.trace(F("Adv eth cfg" CR));
  ETH.config(ip, gateway, subnet, Dns);
  ethBeginSuccess = ETH.begin();
#    else
  Log.trace(F("Spl eth cfg" CR));
  ethBeginSuccess = ETH.begin();
#    endif
  Log.trace(F("Connecting to Ethernet" CR));
  while (!ethConnected && failure_number_ntwk <= maxConnectionRetryNetwork) {
    delay(500);
    Log.trace(F("." CR));
    failure_number_ntwk++;
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Log.trace(F("Ethernet Started" CR));
      ETH.setHostname(gateway_name);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Log.notice(F("Ethernet Connected" CR));
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Log.trace(F("OpenMQTTGateway MAC: %s" CR), ETH.macAddress().c_str());
      Log.trace(F("OpenMQTTGateway IP: %s" CR), ETH.localIP().toString().c_str());
      Log.trace(F("OpenMQTTGateway link speed: %d Mbps" CR), ETH.linkSpeed());
      ethConnected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Log.error(F("Ethernet Disconnected" CR));
      ethConnected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Log.error(F("Ethernet Stopped" CR));
      ethConnected = false;
      break;
    default:
      break;
  }
}
#  endif
#endif

#if defined(MDNS_SD)
void connectMQTTmdns() {
  Log.trace(F("Browsing for MQTT service" CR));
  int n = MDNS.queryService("mqtt", "tcp");
  if (n == 0) {
    Log.error(F("no services found" CR));
  } else {
    Log.trace(F("%d service(s) found" CR), n);
    for (int i = 0; i < n; ++i) {
      Log.trace(F("Service %d %s found" CR), i, MDNS.hostname(i).c_str());
      Log.trace(F("IP %s Port %d" CR), MDNS.IP(i).toString().c_str(), MDNS.port(i));
    }
    if (n == 1) {
      Log.trace(F("One MQTT server found setting parameters" CR));
      client.setServer(MDNS.IP(0), int(MDNS.port(0)));
    } else {
      Log.error(F("Several MQTT servers found, please deactivate mDNS and set your default server" CR));
    }
  }
}
#endif

void loop() {
#ifndef ESPWifiManualSetup
  checkButton(); // check if a reset of wifi/mqtt settings is asked
#endif

#ifdef ESP32
  //esp_task_wdt_reset();
#endif

  unsigned long now = millis();

  // Switch off of the LED after TimeLedON
  if (now > (timer_led_measures + (TimeLedON * 1000))) {
    timer_led_measures = millis();
    InfoIndicatorOFF();
    SendReceiveIndicatorOFF();
  }

  if (ethConnected || WiFi.status() == WL_CONNECTED) {
    if (ethConnected && WiFi.status() == WL_CONNECTED) {
      WiFi.disconnect(); // we disconnect the wifi as we are connected to ethernet
    }
    ArduinoOTA.handle();
    failure_number_ntwk = 0;
#if defined(ZwebUI) && defined(ESP32)
    WebUILoop();
#endif
    if (client.loop()) { // MQTT client is still connected
      InfoIndicatorON();
      failure_number_ntwk = 0;
      // We have just re-connected if connected was previously false
      bool justReconnected = !connected;
#ifdef ZmqttDiscovery
      // Deactivate autodiscovery after DiscoveryAutoOffTimer.
      // Exception: when discovery_republish_on_reconnect is enabled, we never never automatically disable discovery
      if (!discovery_republish_on_reconnect && SYSConfig.discovery && (now > lastDiscovery + DiscoveryAutoOffTimer))
        SYSConfig.discovery = false;
      // at first connection we publish the discovery payloads
      // or, when we have just re-connected (only when discovery_republish_on_reconnect is enabled)
      bool publishDiscovery = SYSConfig.discovery && (!connectedOnce || (discovery_republish_on_reconnect && justReconnected));
      if (publishDiscovery) {
        pubMqttDiscovery();
      }
#endif
      connectedOnce = true;
      connected = true;
      if (now > (timer_sys_measures + (TimeBetweenReadingSYS * 1000)) || !timer_sys_measures) {
        timer_sys_measures = millis();
        stateMeasures();
#ifdef ZgatewayBT
        stateBTMeasures(false);
#endif
#ifdef ZactuatorONOFF
        stateONOFFMeasures();
#endif
#ifdef ZdisplaySSD1306
        stateSSD1306Display();
#endif
#ifdef ZgatewayLORA
        stateLORAMeasures();
#endif
#if defined(ZgatewayRTL_433) || defined(ZgatewayPilight) || defined(ZgatewayRF) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)
        stateRFMeasures();
#endif
#if defined(ZwebUI) && defined(ESP32)
        stateWebUIStatus();
#endif
      }
      if (now > (timer_sys_checks + (TimeBetweenCheckingSYS * 1000)) || !timer_sys_checks) {
#if message_UTCtimestamp || message_unixtimestamp
        syncNTP();
#endif
        if (!timer_sys_checks) { // Update check at start up only
#if defined(ESP32) && defined(MQTT_HTTPS_FW_UPDATE)
          checkForUpdates();
#endif
#ifdef ZgatewayBT
          BTProcessLock = !BTConfig.enabled; // Release BLE processes at start if enabled
#endif
        }

        timer_sys_checks = millis();
      }
      emptyQueue();
#ifdef ZsensorBME280
      MeasureTempHumAndPressure(); //Addon to measure Temperature, Humidity, Pressure and Altitude with a Bosch BME280/BMP280
#endif
#ifdef ZsensorHTU21
      MeasureTempHum(); //Addon to measure Temperature, Humidity, of a HTU21 sensor
#endif
#ifdef ZsensorLM75
      MeasureTemp(); //Addon to measure Temperature of an LM75 sensor
#endif
#ifdef ZsensorAHTx0
      MeasureAHTTempHum(); //Addon to measure Temperature, Humidity, of an 'AHTx0' sensor
#endif
#ifdef ZsensorHCSR04
      MeasureDistance(); //Addon to measure distance with a HC-SR04
#endif
#ifdef ZsensorBH1750
      MeasureLightIntensity(); //Addon to measure Light Intensity with a BH1750
#endif
#ifdef ZsensorMQ2
      MeasureGasMQ2();
#endif
#ifdef ZsensorTEMT6000
      MeasureLightIntensityTEMT6000();
#endif
#ifdef ZsensorTSL2561
      MeasureLightIntensityTSL2561();
#endif
#ifdef ZsensorC37_YL83_HMRD
      MeasureC37_YL83_HMRDWater(); //Addon for leak detection with a C-37 YL-83 H-MRD
#endif
#ifdef ZsensorDHT
      MeasureTempAndHum(); //Addon to measure the temperature with a DHT
#endif
#ifdef ZsensorSHTC3
      MeasureTempAndHum(); //Addon to measure the temperature with a DHT
#endif
#ifdef ZsensorDS1820
      MeasureDS1820Temp(); //Addon to measure the temperature with DS1820 sensor(s)
#endif
#ifdef ZsensorINA226
      MeasureINA226();
#endif
#ifdef ZsensorHCSR501
      MeasureHCSR501();
#endif
#ifdef ZsensorGPIOInput
      MeasureGPIOInput();
#endif
#ifdef ZsensorGPIOKeyCode
      MeasureGPIOKeyCode();
#endif
#ifdef ZsensorADC
      MeasureADC(); //Addon to measure the analog value of analog pin
#endif
#ifdef ZsensorTouch
      MeasureTouch();
#endif
#ifdef ZgatewayLORA
      LORAtoMQTT();
#  ifdef ZmqttDiscovery
      if (SYSConfig.discovery)
        launchLORADiscovery(publishDiscovery);
#  endif
#endif
#ifdef ZgatewayRF
      RFtoMQTT();
#endif
#ifdef ZgatewayRF2
      RF2toMQTT();
#endif
#ifdef ZgatewayWeatherStation
      ZgatewayWeatherStationtoMQTT();
#endif
#ifdef ZgatewayGFSunInverter
      ZgatewayGFSunInverterMQTT();
#endif
#ifdef ZgatewayPilight
      PilighttoMQTT();
#endif
#ifdef ZgatewayBT
#  ifdef ZmqttDiscovery
      if (SYSConfig.discovery)
        launchBTDiscovery(publishDiscovery);
#  endif
#endif
#ifdef ZgatewaySRFB
      SRFBtoMQTT();
#endif
#ifdef ZgatewayIR
      IRtoMQTT();
#endif
#ifdef Zgateway2G
      if (_2GtoMQTT())
        Log.trace(F("2GtoMQTT OK" CR));
#endif
#ifdef ZgatewayRFM69
      if (RFM69toMQTT())
        Log.trace(F("RFM69toMQTT OK" CR));
#endif
#ifdef ZgatewayRS232
      RS232toMQTT();
#endif
#ifdef ZactuatorFASTLED
      FASTLEDLoop();
#endif
#ifdef ZactuatorPWM
      PWMLoop();
#endif
#ifdef ZgatewayRTL_433
      RTL_433Loop();
#  ifdef ZmqttDiscovery
      if (SYSConfig.discovery)
        launchRTL_433Discovery(publishDiscovery);
#  endif
#endif

    } else {
      // MQTT disconnected
      connected = false;
      connectMQTT();
    }
  } else { // disconnected from network
#ifdef ESP32
    //esp_task_wdt_reset();
#endif
    connected = false;
    Log.warning(F("Network disconnected" CR));
    ErrorIndicatorON();
    delay(2000); // add a delay to avoid ESP32 crash and reset
#ifdef ESP32
    //esp_task_wdt_reset();
#endif
    ErrorIndicatorOFF();
    delay(2000);
    wifi_reconnect_bypass();
  }
// Function that doesn't need an active connection
#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
  loopM5();
#endif
#if defined(ZdisplaySSD1306)
  loopSSD1306();
#endif

/**
 * Deep-sleep for the ESP8266 & ESP32 - e.g. DEEP_SLEEP_IN_US 30000000 for 30 seconds / wake by ESP32_EXT0_WAKE_PIN.
 * Everything is off and (almost) all execution state is lost.
 */
#if defined(DEEP_SLEEP_IN_US) || defined(ESP32_EXT0_WAKE_PIN)
  if (ready_to_sleep) {
    delay(250); //Give some time for last MQTT messages to be sent
#  ifdef DEEP_SLEEP_IN_US
    Log.notice(F("Entering deep sleep for %l us." CR), DEEP_SLEEP_IN_US);
#  endif
#  ifdef ESP32_EXT0_WAKE_PIN
    Log.notice(F("Entering deep sleep, EXT0 Wakeup by pin : %l." CR), ESP32_EXT0_WAKE_PIN);
#  endif
#  ifdef ESP8266
    ESP.deepSleep(DEEP_SLEEP_IN_US);
#  endif
#  ifdef ESP32
    esp_deep_sleep_start();
#  endif
  }
#endif
}

/**
 * Calculate uptime and take into account the millis() rollover
 */
unsigned long uptime() {
  static unsigned long lastUptime = 0;
  static unsigned long uptimeAdd = 0;
  unsigned long uptime = millis() / 1000 + uptimeAdd;
  if (uptime < lastUptime) {
    uptime += 4294967;
    uptimeAdd += 4294967;
  }
  lastUptime = uptime;
  return uptime;
}

/**
 * Calculate internal ESP32 temperature
 */
#if defined(ESP32) && !defined(NO_INT_TEMP_READING)
float intTemperatureRead() {
  SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
  SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
  CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
  CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
  SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
  SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
  ets_delay_us(100);
  SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
  ets_delay_us(5);
  float temp_f = (float)GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
  float temp_c = (temp_f - 32) / 1.8;
  return temp_c;
}
#endif

void syncNTP() {
  configTime(0, 0, NTP_SERVER);
  time_t now = time(nullptr);
  uint8_t count = 0;
  Log.trace(F("Waiting for NTP time sync" CR));
  while ((now < 8 * 3600 * 2) && count++ < 60) {
    delay(500);
    now = time(nullptr);
  }

  if (count >= 60) {
    Log.error(F("Unable to update - invalid time" CR));
    return;
  }
}

int unixtimestamp() {
  return time(nullptr);
}

String UTCtimestamp() {
  time_t now;
  time(&now);
  char buffer[sizeof "yyyy-MM-ddThh:mm:ssZ"];
  strftime(buffer, sizeof buffer, "%FT%TZ", gmtime(&now));
  return buffer;
}

/*
 Erase flash and restart the ESP
*/
void eraseAndRestart() {
  Log.trace(F("Formatting requested, result: %d" CR), SPIFFS.format());

#if defined(ESP8266)
  WiFi.disconnect(true);
#  ifndef ESPWifiManualSetup
  wifiManager.resetSettings();
#  endif
  delay(5000);
  ESP.reset();
#else
  //esp_task_wdt_delete(NULL);
  nvs_flash_erase();
  ESP.restart();
#endif
}

String stateMeasures() {
  StaticJsonDocument<JSON_MSG_BUFFER> SYSdata;

  SYSdata["uptime"] = uptime();

  SYSdata["version"] = OMG_VERSION;
#ifdef RGB_INDICATORS
  SYSdata["rgbb"] = SYSConfig.rgbbrightness;
#endif
#ifdef ZmqttDiscovery
  SYSdata["disc"] = SYSConfig.discovery;
  SYSdata["ohdisc"] = SYSConfig.ohdiscovery;
#endif
  SYSdata["env"] = ENV_NAME;
  uint32_t freeMem;
  uint32_t minFreeMem;
  freeMem = ESP.getFreeHeap();
#ifdef ZgatewayRTL_433
  // Some RTL_433 decoders have memory leak, this is a temporary workaround
  if (freeMem < MinimumMemory) {
    Log.error(F("Not enough memory %d, restarting" CR), freeMem);
    ESPRestart(8);
  }
#endif
  SYSdata["freemem"] = freeMem;
  SYSdata["mqttp"] = mqtt_port;
  SYSdata["mqtts"] = mqtt_secure;
  SYSdata["msgprc"] = queueLengthSum;
  SYSdata["msgblck"] = blockedMessages;
  SYSdata["maxq"] = maxQueueLength;
#ifdef ESP32
  minFreeMem = ESP.getMinFreeHeap();
  SYSdata["minmem"] = minFreeMem;
#  ifndef NO_INT_TEMP_READING
  SYSdata["tempc"] = round2(intTemperatureRead());
#  endif
  SYSdata["freestck"] = uxTaskGetStackHighWaterMark(NULL);
#endif

  SYSdata["eth"] = ethConnected;
  if (ethConnected) {
#ifdef ESP32_ETHERNET
    SYSdata["mac"] = (char*)ETH.macAddress().c_str();
    SYSdata["ip"] = ip2CharArray(ETH.localIP());
    ETH.fullDuplex() ? SYSdata["fd"] = (bool)"true" : SYSdata["fd"] = (bool)"false";
    SYSdata["linkspeed"] = (int)ETH.linkSpeed();
#endif
  } else {
    SYSdata["rssi"] = (long)WiFi.RSSI();
    SYSdata["SSID"] = (char*)WiFi.SSID().c_str();
    SYSdata["BSSID"] = (char*)WiFi.BSSIDstr().c_str();
    SYSdata["ip"] = ip2CharArray(WiFi.localIP());
    SYSdata["mac"] = (char*)WiFi.macAddress().c_str();
  }

#ifdef ZgatewayBT
#  ifdef ESP32
  SYSdata["lowpowermode"] = (int)lowpowermode;
#  endif
#endif
#ifdef ZboardM5STACK
  M5.Power.begin();
  SYSdata["m5battlevel"] = (int8_t)M5.Power.getBatteryLevel();
  SYSdata["m5ischarging"] = (bool)M5.Power.isCharging();
  SYSdata["m5ischargefull"] = (bool)M5.Power.isChargeFull();
#endif
#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  M5.Axp.EnableCoulombcounter();
  SYSdata["m5batvoltage"] = (float)M5.Axp.GetBatVoltage();
  SYSdata["m5batcurrent"] = (float)M5.Axp.GetBatCurrent();
  SYSdata["m5vinvoltage"] = (float)M5.Axp.GetVinVoltage();
  SYSdata["m5vincurrent"] = (float)M5.Axp.GetVinCurrent();
  SYSdata["m5vbusvoltage"] = (float)M5.Axp.GetVBusVoltage();
  SYSdata["m5vbuscurrent"] = (float)M5.Axp.GetVBusCurrent();
  SYSdata["m5tempaxp"] = (float)M5.Axp.GetTempInAXP192();
  SYSdata["m5batpower"] = (float)M5.Axp.GetBatPower();
  SYSdata["m5batchargecurrent"] = (float)M5.Axp.GetBatChargeCurrent();
  SYSdata["m5apsvoltage"] = (float)M5.Axp.GetAPSVoltage();
#endif
  SYSdata["modules"] = modules;

  SYSdata["origin"] = subjectSYStoMQTT;
  handleJsonEnqueue(SYSdata);
  pubOled(subjectSYStoMQTT, SYSdata);

  char jsonChar[100];
  serializeJson(modules, jsonChar, 99);

  String _modules = jsonChar;

  _modules.replace(",", ", ");
  _modules.replace("[", "");
  _modules.replace("]", "");
  _modules.replace("\"", "'");

  SYSdata["modules"] = _modules.c_str();

  String output;
  serializeJson(SYSdata, output);
  return output;
}

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)
/**
 * Store signal values from RF, IR, SRFB or Weather stations so as to avoid duplicates
 */
void storeSignalValue(SIGNAL_SIZE_UL_ULL MQTTvalue) {
  unsigned long now = millis();
  // find oldest value of the buffer
  int o = getMin();
  Log.trace(F("Min ind: %d" CR), o);
  // replace it by the new one
  receivedSignal[o].value = MQTTvalue;
  receivedSignal[o].time = now;

  // Casting "receivedSignal[o].value" to (unsigned long) because ArduinoLog doesn't support uint64_t for ESP's
  Log.trace(F("store code : %u / %u" CR), (unsigned long)receivedSignal[o].value, receivedSignal[o].time);
  Log.trace(F("Col: val/timestamp" CR));
  for (int i = 0; i < struct_size; i++) {
    Log.trace(F("mem code : %u / %u" CR), (unsigned long)receivedSignal[i].value, receivedSignal[i].time);
  }
}

/**
 * get oldest time index from the values array from RF, IR, SRFB or Weather stations so as to avoid duplicates
 */
int getMin() {
  unsigned int minimum = receivedSignal[0].time;
  int minindex = 0;
  for (int i = 1; i < struct_size; i++) {
    if (receivedSignal[i].time < minimum) {
      minimum = receivedSignal[i].time;
      minindex = i;
    }
  }
  return minindex;
}

/**
 * Check if signal values from RF, IR, SRFB or Weather stations are duplicates
 */
bool isAduplicateSignal(SIGNAL_SIZE_UL_ULL value) {
  Log.trace(F("isAdupl?" CR));
  for (int i = 0; i < struct_size; i++) {
    if (receivedSignal[i].value == value) {
      unsigned long now = millis();
      if (now - receivedSignal[i].time < time_avoid_duplicate) { // change
        Log.trace(F("no pub. dupl" CR));
        return true;
      }
    }
  }
  return false;
}
#endif

void receivingMQTT(char* topicOri, char* datacallback) {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jsondata = jsonBuffer.to<JsonObject>();
  auto error = deserializeJson(jsonBuffer, datacallback);
  if (error) {
    Log.error(F("deserialize MQTT data failed: %s" CR), error.c_str());
    return;
  }

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation)
  if (strstr(topicOri, subjectMultiGTWKey) != NULL) { // storing received value so as to avoid publishing this value if it has been already sent by this or another OpenMQTTGateway
    SIGNAL_SIZE_UL_ULL data = jsondata.isNull() ? STRTO_UL_ULL(datacallback, NULL, 10) : jsondata["value"];
    if (data != 0 && !isAduplicateSignal(data)) {
      storeSignalValue(data);
    }
  }
#endif

  if (!jsondata.isNull()) { // json object ok -> json decoding
    // log the received json
    String buffer = "";
    serializeJson(jsondata, buffer);
    Log.notice(F("[ MQTT->OMG ]: %s" CR), buffer.c_str());

#ifdef ZgatewayPilight // ZgatewayPilight is only defined with json publishing due to its numerous parameters
    MQTTtoPilight(topicOri, jsondata);
#endif
#if defined(ZgatewayRTL_433) || defined(ZgatewayPilight) || defined(ZgatewayRF) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)
    MQTTtoRFset(topicOri, jsondata);
#endif
#if jsonReceiving
#  ifdef ZgatewayLORA
    MQTTtoLORA(topicOri, jsondata);
#  endif
#  ifdef ZgatewayRF
    MQTTtoRF(topicOri, jsondata);
#  endif
#  ifdef ZgatewayRF2
    MQTTtoRF2(topicOri, jsondata);
#  endif
#  ifdef Zgateway2G
    MQTTto2G(topicOri, jsondata);
#  endif
#  ifdef ZgatewaySRFB
    MQTTtoSRFB(topicOri, jsondata);
#  endif
#  ifdef ZgatewayIR
    MQTTtoIR(topicOri, jsondata);
#  endif
#  ifdef ZgatewayRFM69
    MQTTtoRFM69(topicOri, jsondata);
#  endif
#  ifdef ZgatewayBT
    MQTTtoBT(topicOri, jsondata);
#  endif
#  ifdef ZactuatorFASTLED
    MQTTtoFASTLED(topicOri, jsondata);
#  endif
#  ifdef ZactuatorPWM
    MQTTtoPWM(topicOri, jsondata);
#  endif
#  if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
    MQTTtoM5(topicOri, jsondata);
#  endif
#  if defined(ZdisplaySSD1306)
    MQTTtoSSD1306(topicOri, jsondata);
#  endif
#  ifdef ZactuatorONOFF
    MQTTtoONOFF(topicOri, jsondata);
#  endif
#  ifdef ZactuatorSomfy
    MQTTtoSomfy(topicOri, jsondata);
#  endif
#  ifdef ZgatewayRS232
    MQTTtoRS232(topicOri, jsondata);
#  endif
#  ifdef MQTT_HTTPS_FW_UPDATE
    MQTTHttpsFWUpdate(topicOri, jsondata);
#  endif
#  if defined(ZwebUI) && defined(ESP32)
    MQTTtoWebUI(topicOri, jsondata);
#  endif
#endif
    SendReceiveIndicatorON();

    MQTTtoSYS(topicOri, jsondata);
  } else { // not a json object --> simple decoding
#if simpleReceiving
#  ifdef ZgatewayLORA
    MQTTtoLORA(topicOri, datacallback);
#  endif
#  ifdef ZgatewayRF
    MQTTtoRF(topicOri, datacallback);
#  endif
#  ifdef ZgatewayRF315
    MQTTtoRF315(topicOri, datacallback);
#  endif
#  ifdef ZgatewayRF2
    MQTTtoRF2(topicOri, datacallback);
#  endif
#  ifdef Zgateway2G
    MQTTto2G(topicOri, datacallback);
#  endif
#  ifdef ZgatewaySRFB
    MQTTtoSRFB(topicOri, datacallback);
#  endif
#  ifdef ZgatewayRFM69
    MQTTtoRFM69(topicOri, datacallback);
#  endif
#  ifdef ZactuatorFASTLED
    MQTTtoFASTLED(topicOri, datacallback);
#  endif
#endif
#ifdef ZactuatorONOFF
    MQTTtoONOFF(topicOri, datacallback);
#endif
  }
}

#ifdef MQTT_HTTPS_FW_UPDATE
String latestVersion;
#  ifdef ESP32
#    include <HTTPClient.h>

#    include "zzHTTPUpdate.h"

#    if CHECK_OTA_UPDATE
/**
 * Check on a server the latest version information to build a releaseLink
 * The release link will be used when the user trigger an OTA update command
 * Only available for ESP32
 */
bool checkForUpdates() {
  Log.notice(F("Update check, free heap: %d"), ESP.getFreeHeap());
  HTTPClient http;
  http.setTimeout((GeneralTimeOut - 1) * 1000); // -1 to avoid WDT
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(OTA_JSON_URL, OTAserver_cert);
  int httpCode = http.GET();
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jsondata = jsonBuffer.to<JsonObject>();

  if (httpCode > 0) { //Check for the returning code
    String payload = http.getString();
    auto error = deserializeJson(jsonBuffer, payload);
    if (error) {
      Log.error(F("Deserialize MQTT data failed: %s" CR), error.c_str());
    }
    Log.trace(F("HttpCode %d" CR), httpCode);
    Log.trace(F("Payload %s" CR), payload.c_str());
  } else {
    Log.error(F("Error on HTTP request"));
  }
  http.end(); //Free the resources
  Log.notice(F("Update check done, free heap: %d"), ESP.getFreeHeap());
  if (jsondata.containsKey("latest_version")) {
    jsondata["installed_version"] = OMG_VERSION;
    jsondata["entity_picture"] = ENTITY_PICTURE;
    if (!jsondata.containsKey("release_summary"))
      jsondata["release_summary"] = "";
    latestVersion = jsondata["latest_version"].as<String>();
    jsondata["origin"] = subjectRLStoMQTT;
    jsondata["retain"] = true;
    handleJsonEnqueue(jsondata);

    Log.trace(F("Update file found on server" CR));
    return true;
  } else {
    Log.trace(F("No update file found on server" CR));
    return false;
  }
}

#    else
bool checkForUpdates() {
  return false;
}
#    endif
#  elif ESP8266
#    include <ESP8266httpUpdate.h>
#  endif

void MQTTHttpsFWUpdate(char* topicOri, JsonObject& HttpsFwUpdateData) {
  if (strstr(topicOri, subjectMQTTtoSYSupdate) != NULL) {
    const char* version = HttpsFwUpdateData["version"] | "latest";
    if (version && ((strlen(version) != strlen(OMG_VERSION)) || strcmp(version, OMG_VERSION) != 0)) {
      const char* url = HttpsFwUpdateData["url"];
      String systemUrl;
      if (url) {
        if (!strstr((url + (strlen(url) - 5)), ".bin")) {
          Log.error(F("Invalid firmware extension" CR));
          return;
        }
#  if MQTT_HTTPS_FW_UPDATE_USE_PASSWORD > 0
        const char* pwd = HttpsFwUpdateData["password"];
        if (pwd) {
          if (strcmp(pwd, ota_pass) != 0) {
            Log.error(F("Invalid OTA password" CR));
            return;
          }
        } else {
          Log.error(F("No password sent" CR));
          return;
        }
#  endif
#  ifdef ESP32
      } else if (strcmp(version, "latest") == 0) {
        systemUrl = RELEASE_LINK + latestVersion + "/" + ENV_NAME + "-firmware.bin";
        url = systemUrl.c_str();
        Log.notice(F("Using system OTA url with latest version %s" CR), url);
      } else if (strcmp(version, "dev") == 0) {
        systemUrl = String(RELEASE_LINK_DEV) + ENV_NAME + "-firmware.bin";
        url = systemUrl.c_str();
        Log.notice(F("Using system OTA url with dev version %s" CR), url);
      } else if (version[0] == 'v') {
        systemUrl = String(RELEASE_LINK) + version + "/" + ENV_NAME + "-firmware.bin";
        url = systemUrl.c_str();
        Log.notice(F("Using system OTA url with defined version %s" CR), url);
#  endif
      } else {
        Log.error(F("Invalid URL" CR));
        return;
      }
#  ifdef ESP32
      ProcessLock = true;
      //esp_task_wdt_delete(NULL); // Stop task watchdog during update
#    ifdef ZgatewayBT
      stopProcessing();
#    endif
#  endif
      Log.warning(F("Starting firmware update" CR));

      SendReceiveIndicatorON();
      ErrorIndicatorON();
      StaticJsonDocument<JSON_MSG_BUFFER> jsondata;
      jsondata["release_summary"] = "Update in progress ...";
      jsondata["origin"] = subjectRLStoMQTT;
      handleJsonEnqueue(jsondata);

      const char* ota_cert = HttpsFwUpdateData["server_cert"];
      if (!ota_cert && !strstr(url, "http:")) {
        if (ota_server_cert.length() > 0) {
          Log.notice(F("Using stored cert" CR));
          ota_cert = ota_server_cert.c_str();
        } else {
          Log.notice(F("Using config cert" CR));
          ota_cert = OTAserver_cert;
        }
      }

      t_httpUpdate_return result = HTTP_UPDATE_FAILED;
      if (strstr(url, "http:")) {
        Log.notice(F("Http update" CR));
        WiFiClient update_client;
#  ifdef ESP32
        httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        result = httpUpdate.update(update_client, url);
#  elif ESP8266
        ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        result = ESPhttpUpdate.update(update_client, url);
#  endif

      } else {
        WiFiClientSecure update_client;
        if (mqtt_secure) {
          client.disconnect();
          update_client = *(WiFiClientSecure*)eClient;
        } else {
          syncNTP();
        }

#  ifdef ESP32
        update_client.setCACert(ota_cert);
        update_client.setTimeout(12);
        httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        httpUpdate.rebootOnUpdate(false);
        result = httpUpdate.update(update_client, url);
#  elif ESP8266
        caCert.append(ota_cert);
        update_client.setTrustAnchors(&caCert);
        update_client.setTimeout(12000);
        ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        ESPhttpUpdate.rebootOnUpdate(false);
        result = ESPhttpUpdate.update(update_client, url);
#  endif
      }

      switch (result) {
        case HTTP_UPDATE_FAILED:
#  ifdef ESP32
          Log.error(F("HTTP_UPDATE_FAILED Error (%d): %s\n" CR), httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
#  elif ESP8266
          Log.error(F("HTTP_UPDATE_FAILED Error (%d): %s\n" CR), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#  endif
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Log.notice(F("HTTP_UPDATE_NO_UPDATES" CR));
          break;

        case HTTP_UPDATE_OK:
          Log.notice(F("HTTP_UPDATE_OK" CR));
          jsondata["release_summary"] = "Update success !";
          jsondata["installed_version"] = latestVersion;
          jsondata["origin"] = subjectRLStoMQTT;
          handleJsonEnqueue(jsondata);
          ota_server_cert = ota_cert;
#  ifndef ESPWifiManualSetup
          saveConfig();
#  endif
          ESPRestart(6);
          break;
      }

      SendReceiveIndicatorOFF();
      ErrorIndicatorOFF();

      ESPRestart(6);
    }
  }
}
#endif

void MQTTtoSYS(char* topicOri, JsonObject& SYSdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoSYSset)) {
    bool restartESP = false;
    Log.trace(F("MQTTtoSYS json" CR));
    if (SYSdata.containsKey("cmd")) {
      const char* cmd = SYSdata["cmd"];
      Log.notice(F("Command: %s" CR), cmd);
      if (strstr(cmd, restartCmd) != NULL) { //restart
        ESPRestart(5);
      } else if (strstr(cmd, eraseCmd) != NULL) { //erase and restart
#ifndef ESPWifiManualSetup
        setupwifi(true);
#endif
      } else if (strstr(cmd, statusCmd) != NULL) { //erase and restart
        stateMeasures();
      }
    }
#ifdef RGB_INDICATORS
    if (SYSdata.containsKey("rgbb") && SYSdata["rgbb"].is<float>()) {
      if (SYSdata["rgbb"] >= 0 && SYSdata["rgbb"] <= 255) {
        SYSConfig.rgbbrightness = round2(SYSdata["rgbb"]);
        leds.setBrightness(SYSConfig.rgbbrightness);
        leds.show();
#  ifdef ZactuatorONOFF
        updatePowerIndicator();
#  endif
        Log.notice(F("RGB brightness: %d" CR), SYSConfig.rgbbrightness);
        stateMeasures();
      } else {
        Log.error(F("RGB brightness value invalid - ignoring command" CR));
      }
    }
#endif
#ifdef ZmqttDiscovery
    if (SYSdata.containsKey("ohdisc") && SYSdata["ohdisc"].is<bool>()) {
      SYSConfig.ohdiscovery = SYSdata["ohdisc"];
      Log.notice(F("OpenHAB discovery: %T" CR), SYSConfig.ohdiscovery);
      stateMeasures();
    }
#endif
    if (SYSdata.containsKey("wifi_ssid") && SYSdata.containsKey("wifi_pass")) {
#ifdef ESP32
      ProcessLock = true;
#  ifdef ZgatewayBT
      stopProcessing();
#  endif
#endif
      String prev_ssid = WiFi.SSID();
      String prev_pass = WiFi.psk();
      client.disconnect();
      WiFi.disconnect(true);

      Log.warning(F("Attempting connection to new AP %s" CR), (const char*)SYSdata["wifi_ssid"]);
      WiFi.begin((const char*)SYSdata["wifi_ssid"], (const char*)SYSdata["wifi_pass"]);
#if defined(WifiGMode) || defined(WifiPower)
      setESPWifiProtocolTxPower();
#endif
      WiFi.waitForConnectResult(WiFi_TimeOut * 1000);

      if (WiFi.status() != WL_CONNECTED) {
        Log.error(F("Failed to connect to new AP; falling back" CR));
        WiFi.disconnect(true);
        WiFi.begin(prev_ssid.c_str(), prev_pass.c_str());
#if defined(WifiGMode) || defined(WifiPower)
        setESPWifiProtocolTxPower();
#endif
      }
      restartESP = true;
    }

    bool disconnectClient = false; // Trigger client.disconnet if a user/password change doesn't

    if (SYSdata.containsKey("mqtt_topic") || SYSdata.containsKey("gateway_name") || SYSdata.containsKey("gw_pass")) {
      if (SYSdata.containsKey("mqtt_topic")) {
        strncpy(mqtt_topic, SYSdata["mqtt_topic"], parameters_size);
      }
      if (SYSdata.containsKey("gateway_name")) {
        strncpy(gateway_name, SYSdata["gateway_name"], parameters_size);
      }
      if (SYSdata.containsKey("gw_pass")) {
        strncpy(ota_pass, SYSdata["gw_pass"], parameters_size);
        restartESP = true;
      }
#ifndef ESPWifiManualSetup
      saveConfig();
#endif
      disconnectClient = true; // trigger reconnect in loop using the new topic/name
    }

#ifdef MQTTsetMQTT
    if (SYSdata.containsKey("mqtt_user") && SYSdata.containsKey("mqtt_pass")) {
      bool update_server = false;
      bool secure_connect = SYSdata["mqtt_secure"].as<bool>();
      void* prev_client = nullptr;
      bool use_ss_cert = SYSdata.containsKey("mqtt_cert_index");
      uint8_t cert_index = mqtt_ss_index;

      if (SYSdata.containsKey("mqtt_server") && SYSdata.containsKey("mqtt_port")) {
        if (!SYSdata.containsKey("mqtt_secure")) {
          Log.error(F("mqtt_server provided without mqtt_secure defined - ignoring command" CR));
          return;
        }
#  if MQTT_SECURE_SELF_SIGNED
        if (use_ss_cert) {
          cert_index = SYSdata["mqtt_cert_index"].as<uint8_t>();
          if (cert_index >= sizeof(certs_array) / sizeof(ss_certs)) {
            Log.error(F("mqtt_cert_index invalid - ignoring command" CR));
            return;
          }
        }
#  endif

#  ifdef ESP32
        ProcessLock = true;
#    ifdef ZgatewayBT
        stopProcessing();
#    endif
#  endif
        disconnectClient = false;
        client.disconnect();
        update_server = true;
        if (secure_connect != mqtt_secure) {
          prev_client = eClient;
          if (!mqtt_secure) {
            eClient = new WiFiClientSecure;
          } else {
            Log.warning(F("Switching to unsecure MQTT broker" CR));
            eClient = new WiFiClient;
          }

          client.setClient(*(Client*)eClient);
        }

        if (secure_connect) {
          setupTLS(use_ss_cert, cert_index);
        }

        client.setServer(SYSdata["mqtt_server"].as<const char*>(), SYSdata["mqtt_port"].as<unsigned int>());
      } else {
#  ifdef ESP32
        ProcessLock = true;
#    ifdef ZgatewayBT
        stopProcessing();
#    endif
#  endif
        disconnectClient = false;
        client.disconnect();
      }

      String prev_user = mqtt_user;
      String prev_pass = mqtt_pass;
      strcpy(mqtt_user, SYSdata["mqtt_user"]);
      strcpy(mqtt_pass, SYSdata["mqtt_pass"]);

      connectMQTT();

      if (client.connected()) {
        if (update_server) {
          strcpy(mqtt_server, SYSdata["mqtt_server"]);
          strcpy(mqtt_port, SYSdata["mqtt_port"]);
          mqtt_ss_index = cert_index;
          if (prev_client != nullptr) {
            mqtt_secure = !mqtt_secure;
            delete prev_client;
          }
        }
#  ifndef ESPWifiManualSetup
        saveConfig();
#  endif
      } else {
        if (update_server) {
          if (prev_client != nullptr) {
            delete eClient;
            eClient = prev_client;
            client.setClient(*(Client*)eClient);
          }
          uint16_t port = strtol(mqtt_port, NULL, 10);
          client.setServer(mqtt_server, port);
        }
        strcpy(mqtt_user, prev_user.c_str());
        strcpy(mqtt_pass, prev_pass.c_str());
        if (mqtt_secure) {
          setupTLS(MQTT_SECURE_SELF_SIGNED, mqtt_ss_index);
        }
        connectMQTT();
      }
      restartESP = true;
    }
#endif

    if (disconnectClient) {
      client.disconnect();
    }

#ifdef ZmqttDiscovery
    if (SYSdata.containsKey("disc")) {
      if (SYSdata["disc"].is<bool>()) {
        if (SYSdata["disc"] == true && SYSConfig.discovery == false)
          lastDiscovery = millis();
        SYSConfig.discovery = SYSdata["disc"];
        stateMeasures();
        if (SYSConfig.discovery)
          pubMqttDiscovery();
      } else {
        Log.error(F("Discovery command not a boolean" CR));
      }
      Log.notice(F("Discovery state: %T" CR), SYSConfig.discovery);
    }
#  ifdef ESP32
    if (SYSdata.containsKey("save") && SYSdata["save"].as<bool>()) {
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject jo = jsonBuffer.to<JsonObject>();
      jo["disc"] = SYSConfig.discovery;
      jo["ohdisc"] = SYSConfig.ohdiscovery;
#    ifdef RGB_INDICATORS
      jo["rgbb"] = SYSConfig.rgbbrightness;
#    endif
      // Save config into NVS (non-volatile storage)
      String conf = "";
      serializeJson(jsonBuffer, conf);
      preferences.begin(Gateway_Short_Name, false);
      int result = preferences.putString("SYSConfig", conf);
      preferences.end();
      Log.notice(F("SYS Config_save: %s, result: %d" CR), conf.c_str(), result);
    }
#  endif
#endif
    if (restartESP) {
      ESPRestart(7);
    }
  }
}

#if valueAsATopic && !defined(ZgatewayPilight)
String toString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

#else

String toString(uint32_t input) {
  String result = String(input);

  return result;
}
#endif