/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
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

// Macros and structure to enable the duplicates removing on the following gateways
#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation)
// array to store previous received RFs, IRs codes and their timestamps
struct ReceivedSignal {
  SIGNAL_SIZE_UL_ULL value;
  uint32_t time;
};
#  if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#    define struct_size 12
ReceivedSignal receivedSignal[struct_size] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
#  else // boards with smaller memory
#    define struct_size 4
ReceivedSignal receivedSignal[struct_size] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
#  endif
#endif

#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
//Time used to wait for an interval before checking system measures
unsigned long timer_sys_measures = 0;
#  define ARDUINOJSON_USE_LONG_LONG 1
#endif

#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <PubSubClient.h>

StaticJsonBuffer<JSON_MSG_BUFFER> modulesBuffer;
JsonArray& modules = modulesBuffer.createArray();

// Modules config inclusion
#if defined(ZgatewayRF) || defined(ZgatewayRF2) || defined(ZgatewayPilight) || defined(ZactuatorSomfy)
#  include "config_RF.h"
#endif
#ifdef ZgatewayWeatherStation
#  include "config_WeatherStation.h"
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
#ifdef ZsensorTSL2561
#  include "config_TSL2561.h"
#endif
#ifdef ZsensorBME280
#  include "config_BME280.h"
#endif
#ifdef ZsensorHTU21
#  include "config_HTU21.h"
#endif
#ifdef ZsensorAHTx0
#  include "config_AHTx0.h"
#endif
#ifdef ZsensorHCSR04
#  include "config_HCSR04.h"
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
#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
#  include "config_M5.h"
#endif
#if defined(ZgatewayRS232)
#  include "config_RS232.h"
#endif
/*------------------------------------------------------------------------*/

//adding this to bypass the problem of the arduino builder issue 50
void callback(char* topic, byte* payload, unsigned int length);

char mqtt_user[parameters_size] = MQTT_USER; // not compulsory only if your broker needs authentication
char mqtt_pass[parameters_size * 2] = MQTT_PASS; // not compulsory only if your broker needs authentication
char mqtt_server[parameters_size] = MQTT_SERVER;
char mqtt_port[6] = MQTT_PORT;
char mqtt_topic[mqtt_topic_max_size] = Base_Topic;
char gateway_name[parameters_size * 2] = Gateway_Name;

bool connectedOnce = false; //indicate if we have been connected once to MQTT
int failure_number_ntwk = 0; // number of failure connecting to network
int failure_number_mqtt = 0; // number of failure connecting to MQTT
#ifdef ZmqttDiscovery
bool disc = true; // Auto discovery with Home Assistant convention
#endif
unsigned long timer_led_measures = 0;

#ifdef ESP32
#  include <ArduinoOTA.h>
#  include <FS.h>
#  include <SPIFFS.h>
#  include <esp_wifi.h>

#  ifdef ESP32_ETHERNET
#    define ETH_CLK_MODE  ETH_CLOCK_GPIO17_OUT
#    define ETH_PHY_POWER 12
#    include <ETH.h>
void WiFiEvent(WiFiEvent_t event);
static bool esp32EthConnected = false;
#  endif
#  ifdef SECURE_CONNECTION
#    include <WiFiClientSecure.h>
WiFiClientSecure eClient;
#  else
#    include <WiFi.h>
WiFiClient eClient;
#  endif
#  include <Preferences.h>
#  include <WiFiManager.h>
Preferences preferences;
#  ifdef MDNS_SD
#    include <ESPmDNS.h>
#  endif
#elif defined(ESP8266)
#  include <ArduinoOTA.h>
#  include <DNSServer.h>
#  include <ESP8266WebServer.h>
#  include <ESP8266WiFi.h>
#  include <FS.h>
#  include <WiFiManager.h>
#  ifdef SECURE_CONNECTION
WiFiClientSecure eClient;
X509List caCert(certificate);
#  else
WiFiClient eClient;
#  endif
#  ifdef MDNS_SD
#    include <ESP8266mDNS.h>
#  endif
#else
#  include <Ethernet.h>
EthernetClient eClient;
#endif

#define convertTemp_CtoF(c) ((c * 1.8) + 32)
#define convertTemp_FtoC(f) ((f - 32) * 5 / 9)

// client link to pubsub mqtt
PubSubClient client(eClient);

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

char* ip2CharArray(IPAddress ip) { //from Nick Lee https://stackoverflow.com/questions/28119653/arduino-display-ethernet-localip
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}

bool to_bool(String const& s) { // thanks Chris Jester-Young from stackoverflow
  return s != "0";
}

void pub(char* topicori, char* payload, bool retainFlag) {
  String topic = String(mqtt_topic) + String(topicori);
  pubMQTT((char*)topic.c_str(), payload, retainFlag);
}

void pub(char* topicori, JsonObject& data) {
  Log.notice(F("Subject: %s" CR), topicori);
  digitalWrite(LED_RECEIVE, LED_RECEIVE_ON);
  logJson(data);
  if (client.connected()) {
    String topic = String(mqtt_topic) + String(topicori);
#ifdef valueAsASubject
#  ifdef ZgatewayPilight
    String value = data["value"];
    String protocol = data["protocol"];
    if (value != 0) {
      topic = topic + "/" + protocol + "/" + value;
    }
#  else
    SIGNAL_SIZE_UL_ULL value = data["value"];
    if (value != 0) {
      topic = topic + "/" + String(value);
    }
#  endif
#endif

#ifdef jsonPublishing
    Log.trace(F("jsonPublishing" CR));
#  if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    char JSONmessageBuffer[data.measureLength() + 1];
#  else
    char JSONmessageBuffer[JSON_MSG_BUFFER];
#  endif
    data.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    pubMQTT(topic, JSONmessageBuffer);
#endif

#ifdef simplePublishing
    Log.trace(F("simplePublishing" CR));
    // Loop through all the key-value pairs in obj
    for (JsonPair& p : data) {
#  if defined(ESP8266)
      yield();
#  endif
      if (p.value.is<SIGNAL_SIZE_UL_ULL>() && strcmp(p.key, "rssi") != 0) { //test rssi , bypass solution due to the fact that a int is considered as an SIGNAL_SIZE_UL_ULL
        if (strcmp(p.key, "value") == 0) { // if data is a value we don't integrate the name into the topic
          pubMQTT(topic, p.value.as<SIGNAL_SIZE_UL_ULL>());
        } else { // if data is not a value we integrate the name into the topic
          pubMQTT(topic + "/" + String(p.key), p.value.as<SIGNAL_SIZE_UL_ULL>());
        }
      } else if (p.value.is<int>()) {
        pubMQTT(topic + "/" + String(p.key), p.value.as<int>());
      } else if (p.value.is<float>()) {
        pubMQTT(topic + "/" + String(p.key), p.value.as<float>());
      } else if (p.value.is<char*>()) {
        pubMQTT(topic + "/" + String(p.key), p.value.as<const char*>());
      }
    }
#endif
  } else {
    Log.warning(F("client not connected can't pub" CR));
  }
}

void pub(char* topicori, char* payload) {
  if (client.connected()) {
    String topic = String(mqtt_topic) + String(topicori);
    Log.trace(F("Pub ack %s into: %s" CR), payload, topic.c_str());
    pubMQTT(topic, payload);
  } else {
    Log.warning(F("client not connected can't pub" CR));
  }
}

void pub_custom_topic(char* topicori, JsonObject& data, boolean retain) {
  if (client.connected()) {
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    char JSONmessageBuffer[data.measureLength() + 1];
#else
    char JSONmessageBuffer[JSON_MSG_BUFFER];
#endif
    data.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Log.trace(F("Pub json :%s into custom topic: %s" CR), JSONmessageBuffer, topicori);
    pubMQTT(topicori, JSONmessageBuffer, retain);
  } else {
    Log.warning(F("client not connected can't pub" CR));
  }
}

// Low level MQTT functions
void pubMQTT(char* topic, char* payload) {
  client.publish(topic, payload);
}

void pubMQTT(char* topicori, char* payload, bool retainFlag) {
  client.publish(topicori, payload, retainFlag);
}

void pubMQTT(String topic, char* payload) {
  client.publish((char*)topic.c_str(), payload);
}

void pubMQTT(char* topic, unsigned long payload) {
  char val[11];
  sprintf(val, "%lu", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, unsigned long long payload) {
  char val[21];
  sprintf(val, "%llu", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, String payload) {
  client.publish(topic, (char*)payload.c_str());
}

void pubMQTT(String topic, String payload) {
  client.publish((char*)topic.c_str(), (char*)payload.c_str());
}

void pubMQTT(String topic, int payload) {
  char val[12];
  sprintf(val, "%d", payload);
  client.publish((char*)topic.c_str(), val);
}

void pubMQTT(String topic, unsigned long long payload) {
  char val[21];
  sprintf(val, "%llu", payload);
  client.publish((char*)topic.c_str(), val);
}

void pubMQTT(String topic, float payload) {
  char val[12];
  dtostrf(payload, 3, 1, val);
  client.publish((char*)topic.c_str(), val);
}

void pubMQTT(char* topic, float payload) {
  char val[12];
  dtostrf(payload, 3, 1, val);
  client.publish(topic, val);
}

void pubMQTT(char* topic, int payload) {
  char val[6];
  sprintf(val, "%d", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, unsigned int payload) {
  char val[6];
  sprintf(val, "%u", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, long payload) {
  char val[11];
  sprintf(val, "%l", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, double payload) {
  char val[16];
  sprintf(val, "%d", payload);
  client.publish(topic, val);
}

void pubMQTT(String topic, unsigned long payload) {
  char val[11];
  sprintf(val, "%lu", payload);
  client.publish((char*)topic.c_str(), val);
}

void logJson(JsonObject& jsondata) {
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  char JSONmessageBuffer[jsondata.measureLength() + 1];
#else
  char JSONmessageBuffer[JSON_MSG_BUFFER];
#endif
  jsondata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Log.notice(F("Received json : %s" CR), JSONmessageBuffer);
}

bool cmpToMainTopic(char* topicOri, char* toAdd) {
  char topic[mqtt_topic_max_size];
  strcpy(topic, mqtt_topic);
  strcat(topic, toAdd);
  if (strstr(topicOri, topic) != NULL) {
    return true;
  } else {
    return false;
  }
}

void connectMQTT() {
#ifndef ESPWifiManualSetup
#  if defined(ESP8266) || defined(ESP32)
  checkButton(); // check if a reset of wifi/mqtt settings is asked
#  endif
#endif

  Log.warning(F("MQTT connection..." CR));
  char topic[mqtt_topic_max_size];
  strcpy(topic, mqtt_topic);
  strcat(topic, will_Topic);
  client.setBufferSize(mqtt_max_packet_size);
  if (client.connect(gateway_name, mqtt_user, mqtt_pass, topic, will_QoS, will_Retain, will_Message)) {
#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
    if (lowpowermode < 2)
      M5Display("MQTT connected", "", "");
#endif
    Log.notice(F("Connected to broker" CR));
    failure_number_mqtt = 0;
    // Once connected, publish an announcement...
    pub(will_Topic, Gateway_AnnouncementMsg, will_Retain);
    // publish version
    pub(version_Topic, OMG_VERSION, will_Retain);
    //Subscribing to topic
    char topic2[mqtt_topic_max_size];
    strcpy(topic2, mqtt_topic);
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
#if defined(SECURE_CONNECTION) && defined(ESP32)
    Log.warning(F("failed, ssl error code=%d" CR), eClient.lastError(nullptr, 0));
#elif defined(SECURE_CONNECTION) && defined(ESP8266)
    Log.warning(F("failed, ssl error code=%d" CR), eClient.getLastSSLError());
#endif
    digitalWrite(LED_INFO, LED_INFO_ON);
    delay(1000);
    digitalWrite(LED_INFO, !LED_INFO_ON);
    delay(4000);
#if defined(ESP8266) || defined(ESP32) && !defined(ESP32_ETHERNET)
    disconnection_handling(failure_number_mqtt);
#endif
  }
}

// Callback function, when the gateway receive an MQTT value on the topics subscribed this function is called
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  Log.trace(F("Hey I got a callback " CR));
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);
  // Conversion to a printable string
  p[length] = '\0';
  //launch the function to treat received data if this data concern OpenMQTTGateway
  if ((strstr(topic, subjectMultiGTWKey) != NULL) || (strstr(topic, subjectGTWSendKey) != NULL))
    receivingMQTT(topic, (char*)p);
  // Free the memory
  free(p);
}

void setup_parameters() {
  strcat(mqtt_topic, gateway_name);
}

void setup() {
  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  Log.begin(LOG_LEVEL, &Serial);
  Log.notice(F(CR "************* WELCOME TO OpenMQTTGateway **************" CR));

#if defined(ESP8266) || defined(ESP32)
#  ifdef ESP8266
#    ifndef ZgatewaySRFB // if we are not in sonoff rf bridge case we apply the ESP8266 GPIO optimization
  Serial.end();
  Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY); // enable on ESP8266 to free some pin
#    endif
#  elif ESP32
  preferences.begin(Gateway_Short_Name, false);
  lowpowermode = preferences.getUInt("lowpowermode", DEFAULT_LOW_POWER_MODE);
  preferences.end();
#    if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
  setupM5();
#    endif
#  endif

#  ifdef ESP32_ETHERNET
  setup_ethernet_esp32();
#  else // WIFI ESP
#    if defined(ESPWifiManualSetup)
  setup_wifi();
#    else
  setup_wifimanager(false);
#    endif
  Log.trace(F("OpenMQTTGateway Wifi protocol used: %d" CR), wifiProtocol);
  Log.trace(F("OpenMQTTGateway mac: %s" CR), WiFi.macAddress().c_str());
  Log.trace(F("OpenMQTTGateway ip: %s" CR), WiFi.localIP().toString().c_str());
#  endif

  setOTA();
#  ifdef SECURE_CONNECTION
  setupTLS();
#  endif
#else // In case of arduino platform

  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  //Begining ethernet connection in case of Arduino + W5100
  setup_ethernet();
#endif

  //setup LED status
  pinMode(LED_RECEIVE, OUTPUT);
  pinMode(LED_SEND, OUTPUT);
  pinMode(LED_INFO, OUTPUT);
  digitalWrite(LED_RECEIVE, !LED_RECEIVE_ON);
  digitalWrite(LED_SEND, !LED_SEND_ON);
  digitalWrite(LED_INFO, !LED_INFO_ON);

#if defined(MDNS_SD) && defined(ESP8266)
  Log.trace(F("Connecting to MQTT by mDNS without mqtt hostname" CR));
  connectMQTTmdns();
#else
  long port;
  port = strtol(mqtt_port, NULL, 10);
  Log.trace(F("Port: %l" CR), port);
  Log.trace(F("Mqtt server: %s" CR), mqtt_server);
  client.setServer(mqtt_server, port);
#endif

  setup_parameters();

  client.setCallback(callback);

  delay(1500);

#ifdef ZsensorBME280
  setupZsensorBME280();
  modules.add(ZsensorBME280);
#endif
#ifdef ZsensorHTU21
  setupZsensorHTU21();
  modules.add(ZsensorHTU21);
#endif
#ifdef ZsensorAHTx0
  setupZsensorAHTx0();
  modules.add(ZsensorAHTx0);
#endif
#ifdef ZsensorBH1750
  setupZsensorBH1750();
  modules.add(ZsensorBH1750);
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
  setupRF();
  modules.add(ZgatewayRF);
#endif
#ifdef ZgatewayRF2
  setupRF2();
  modules.add(ZgatewayRF2);
#endif
#ifdef ZgatewayPilight
  setupPilight();
  modules.add(ZgatewayPilight);
#endif
#ifdef ZgatewayWeatherStation
  setupWeatherStation();
  modules.add(ZgatewayWeatherStation);
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
  Log.trace(F("mqtt_max_packet_size: %d" CR), mqtt_max_packet_size);
  Log.notice(F("Setup OpenMQTTGateway end" CR));
}

#if defined(ESP8266) || defined(ESP32)
// Bypass for ESP not reconnecting automaticaly the second time https://github.com/espressif/arduino-esp32/issues/2501
bool wifi_reconnect_bypass() {
  uint8_t wifi_autoreconnect_cnt = 0;
#  ifdef ESP32
  while (WiFi.status() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
#  else
  while (WiFi.waitForConnectResult() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
#  endif
    Log.notice(F("Attempting Wifi connection with saved AP: %d" CR), wifi_autoreconnect_cnt);
    WiFi.begin();
    delay(500);
    wifi_autoreconnect_cnt++;
  }
  if (wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
    return true;
  } else {
    return false;
  }
}

// the 2 methods below are used to recover wifi connection by changing the protocols
void forceWifiProtocol() {
#  ifdef ESP32
  Log.warning(F("ESP32: Forcing to wifi %d" CR), wifiProtocol);
  esp_wifi_set_protocol(WIFI_IF_STA, wifiProtocol);
#  elif ESP8266
  Log.warning(F("ESP8266: Forcing to wifi %d" CR), wifiProtocol);
  WiFi.setPhyMode((WiFiPhyMode_t)wifiProtocol);
#  endif
}

void reinit_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  if (wifiProtocol) forceWifiProtocol();
  WiFi.begin();
}

void disconnection_handling(int failure_number) {
  Log.warning(F("disconnection_handling, failed %d times" CR), failure_number);
  if ((failure_number > maxConnectionRetry) && !connectedOnce) {
#  ifndef ESPWifiManualSetup
    Log.error(F("Failed connecting 1st time to mqtt, you should put TRIGGER_GPIO to LOW or erase the flash" CR));
#  endif
  }
  if (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_BG)) {
    Log.warning(F("Attempt to reinit wifi: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if ((failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_BG)) && (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_B))) // After maxConnectionRetry + ATTEMPTS_BEFORE_BG try to connect with BG protocol
  {
#  ifdef ESP32
    wifiProtocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G;
#  elif ESP8266
    wifiProtocol = WIFI_PHY_MODE_11G;
#  endif
    Log.warning(F("Wifi Protocol changed to WIFI_11G: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if ((failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_B)) && (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_B + ATTEMPTS_BEFORE_BG))) // After maxConnectionRetry + ATTEMPTS_BEFORE_B try to connect with B protocol
  {
#  ifdef ESP32
    wifiProtocol = WIFI_PROTOCOL_11B;
#  elif ESP8266
    wifiProtocol = WIFI_PHY_MODE_11B;
#  endif
    Log.warning(F("Wifi Protocol changed to WIFI_11B: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if (failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_B + ATTEMPTS_BEFORE_BG)) // After maxConnectionRetry + ATTEMPTS_BEFORE_B try to connect with B protocol
  {
#  ifdef ESP32
    wifiProtocol = 0;
#  elif ESP8266
    wifiProtocol = 0;
#  endif
    Log.warning(F("Wifi Protocol reverted to normal mode: %d" CR), wifiProtocol);
    reinit_wifi();
  }
}

void setOTA() {
  // Port defaults to 8266
  ArduinoOTA.setPort(ota_port);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ota_hostname);

  // No authentication by default
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    Log.trace(F("Start OTA, lock other functions" CR));
#  if defined(ZgatewayBT) && defined(ESP32)
    stopProcessing();
#  endif
#  if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
    M5Display("OTA in progress", "", "");
#  endif
  });
  ArduinoOTA.onEnd([]() {
    Log.trace(F("\nOTA done" CR));
#  if defined(ZgatewayBT) && defined(ESP32)
    startProcessing();
#  endif
#  if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
    M5Display("OTA done", "", "");
#  endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.trace(F("Progress: %u%%\r" CR), (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
#  if defined(ZgatewayBT) && defined(ESP32)
    startProcessing();
#  endif
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
  });
  ArduinoOTA.begin();
}

#  ifdef SECURE_CONNECTION
void setupTLS() {
#    if defined(NTP_SERVER)
  configTime(0, 0, NTP_SERVER);
#    endif
#    if defined(ESP32)
  eClient.setCACert(certificate);
#    elif defined(ESP8266)
  eClient.setTrustAnchors(&caCert);
  eClient.setBufferSizes(512, 512);
#    endif
}
#  endif
#endif

#if defined(ESPWifiManualSetup)
void setup_wifi() {
  char manual_wifi_ssid[] = wifi_ssid;
  char manual_wifi_password[] = wifi_password;
  delay(10);
  WiFi.mode(WIFI_STA);
  if (wifiProtocol) forceWifiProtocol();

  // We start by connecting to a WiFi network
  Log.trace(F("Connecting to %s" CR), manual_wifi_ssid);
#  ifdef NetworkAdvancedSetup
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  if (!WiFi.config(ip_adress, gateway_adress, subnet_adress, dns_adress)) {
    Log.error(F("Wifi STA Failed to configure" CR));
  }
  WiFi.begin(manual_wifi_ssid, manual_wifi_password);
#  else
  WiFi.begin(manual_wifi_ssid, manual_wifi_password);
#  endif

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Log.trace(F("." CR));
    failure_number_ntwk++;
    disconnection_handling(failure_number_ntwk);
#  if defined(ESP32) && defined(ZgatewayBT)
    if (failure_number_ntwk > maxConnectionRetryWifi)
      lowPowerESP32();
#  endif
  }
  Log.notice(F("WiFi ok with manual config credentials" CR));
}

#elif defined(ESP8266) || defined(ESP32)

WiFiManager wifiManager;

//flag for saving data
bool shouldSaveConfig = true;
//do we have been connected once to mqtt

//callback notifying us of the need to save config
void saveConfigCallback() {
  Log.trace(F("Should save config" CR));
  shouldSaveConfig = true;
}

#  ifdef TRIGGER_GPIO
void checkButton() { // code from tzapu/wifimanager examples
  // check for button press
  if (digitalRead(TRIGGER_GPIO) == LOW) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if (digitalRead(TRIGGER_GPIO) == LOW) {
      Log.trace(F("Trigger button Pressed" CR));
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if (digitalRead(TRIGGER_GPIO) == LOW) {
        Log.trace(F("Button Held" CR));
        Log.notice(F("Erasing ESP Config, restarting" CR));
        setup_wifimanager(true);
      }
    }
  }
}
#  else
void checkButton() {}
#  endif

void eraseAndRestart() {
#  if defined(ESP8266)
  WiFi.disconnect(true);
#  else
  WiFi.disconnect(true, true);
#  endif

  Log.trace(F("Formatting requested, result: %d" CR), SPIFFS.format());

#  if defined(ESP8266)
  ESP.eraseConfig();
  delay(5000);
  ESP.reset();
#  else
  ESP.restart();
#  endif
}

void setup_wifimanager(bool reset_settings) {
#  ifdef TRIGGER_GPIO
  pinMode(TRIGGER_GPIO, INPUT_PULLUP);
#  endif
  delay(10);
  WiFi.mode(WIFI_STA);
  if (wifiProtocol) forceWifiProtocol();

  if (reset_settings)
    eraseAndRestart();

  //read configuration from FS json
  Log.trace(F("mounting FS..." CR));

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
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if (json.success()) {
        Log.trace(F("\nparsed json" CR));
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
        if (json.containsKey("gateway_name"))
          strcpy(gateway_name, json["gateway_name"]);
      } else {
        Log.warning(F("failed to load json config" CR));
      }
    }
  }

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, parameters_size);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, parameters_size);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, parameters_size * 2);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt base topic", mqtt_topic, mqtt_topic_max_size);
  WiFiManagerParameter custom_gateway_name("name", "gateway name", gateway_name, parameters_size * 2);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around

  wifiManager.setConnectTimeout(WifiManager_TimeOut);
  //Set timeout before going to portal
  wifiManager.setConfigPortalTimeout(WifiManager_ConfigPortalTimeOut);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

//set static ip
#  ifdef NetworkAdvancedSetup
  Log.trace(F("Adv wifi cfg" CR));
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress ip_adress(ip);
  wifiManager.setSTAStaticIPConfig(ip_adress, gateway_adress, subnet_adress);
#  endif

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_gateway_name);
  wifiManager.addParameter(&custom_mqtt_topic);

  //set minimum quality of signal so it ignores AP's under that quality
  wifiManager.setMinimumSignalQuality(MinimumWifiSignalQuality);

  if (!wifi_reconnect_bypass()) // if we didn't connect with saved credential we start Wifimanager web portal
  {
#  ifdef ESP32
    if (lowpowermode < 2) {
#    if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
      M5Display("Connect your phone to WIFI AP:", WifiManager_ssid, WifiManager_password);
#    endif
    } else { // in case of low power mode we put the ESP to sleep again if we didn't get connected (typical in case the wifi is down)
#    ifdef ZgatewayBT
      lowPowerESP32();
#    endif
    }
#  endif

    Log.notice(F("Connect your phone to WIFI AP: %s with PWD: %s" CR), WifiManager_ssid, WifiManager_password);
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect(WifiManager_ssid, WifiManager_password)) {
      Log.warning(F("failed to connect and hit timeout" CR));
      delay(3000);
//reset and try again
#  if defined(ESP8266)
      ESP.reset();
#  else
      ESP.restart();
#  endif
      delay(5000);
    }
  }

#  if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
  if (lowpowermode < 2)
    M5Display("Wifi connected", "", "");
#  endif

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
  strcpy(gateway_name, custom_gateway_name.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Log.trace(F("saving config" CR));
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;
    json["mqtt_topic"] = mqtt_topic;
    json["gateway_name"] = gateway_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Log.error(F("failed to open config file for writing" CR));
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
}
#  ifdef ESP32_ETHERNET
void setup_ethernet_esp32() {
  bool ethBeginSuccess = false;
  WiFi.onEvent(WiFiEvent);
#    ifdef NetworkAdvancedSetup
  Log.trace(F("Adv eth cfg" CR));
  ETH.config(ip, gateway, subnet, Dns);
  ethBeginSuccess = ETH.begin();
#    else
  Log.trace(F("Spl eth cfg" CR));
  ethBeginSuccess = ETH.begin();
#    endif
  Log.trace(F("Connecting to Ethernet" CR));
  while (!esp32EthConnected) {
    delay(500);
    Log.trace(F("." CR));
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Log.trace(F("Ethernet Started" CR));
      ETH.setHostname(gateway_name);
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Log.notice(F("Ethernet Connected" CR));
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Log.trace(F("OpenMQTTGateway mac: %s" CR), ETH.macAddress().c_str());
      Log.trace(F("OpenMQTTGateway ip: %s" CR), ETH.localIP().toString().c_str());
      Log.trace(F("OpenMQTTGateway link speed: %d Mbps" CR), ETH.linkSpeed());
      esp32EthConnected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Log.error(F("Ethernet Disconnected" CR));
      esp32EthConnected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Log.error(F("Ethernet Stopped" CR));
      esp32EthConnected = false;
      break;
    default:
      break;
  }
}
#  endif
#else // Arduino case
void setup_ethernet() {
#  ifdef NetworkAdvancedSetup
  Log.trace(F("Adv eth cfg" CR));
  Ethernet.begin(mac, ip, Dns, gateway, subnet);
#  else
  Log.trace(F("Spl eth cfg" CR));
  Ethernet.begin(mac, ip);
#  endif
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Log.error(F("Ethernet shield was not found." CR));
  } else {
    Log.trace(F("ip: %s " CR), Ethernet.localIP());
  }
}
#endif

#if defined(MDNS_SD) && defined(ESP8266)
void connectMQTTmdns() {
  if (!MDNS.begin("ESP_MQTT")) {
    Log.error(F("Error setting up MDNS responder!" CR));
    while (1) {
      delay(1000);
    }
  }
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
#  if defined(ESP8266) || defined(ESP32)
  checkButton(); // check if a reset of wifi/mqtt settings is asked
#  endif
#endif

  unsigned long now = millis();

  // Switch off of the LED after TimeLedON
  if (now > (timer_led_measures + (TimeLedON * 1000))) {
    timer_led_measures = millis();
    digitalWrite(LED_RECEIVE, !LED_RECEIVE_ON);
    digitalWrite(LED_INFO, !LED_INFO_ON);
    digitalWrite(LED_SEND, !LED_SEND_ON);
  }

#if defined(ESP8266) || defined(ESP32)
#  ifdef ESP32_ETHERNET
  if (esp32EthConnected) {
#  else
  if (WiFi.status() == WL_CONNECTED) {
#  endif
    ArduinoOTA.handle();
#else
  if ((Ethernet.hardwareStatus() != EthernetW5100 && Ethernet.linkStatus() == LinkON) || (Ethernet.hardwareStatus() == EthernetW5100)) { //we are able to detect disconnection only on w5200 and w5500
#endif
    failure_number_ntwk = 0;
    if (client.connected()) {
      digitalWrite(LED_INFO, LED_INFO_ON);
      failure_number_ntwk = 0;

      client.loop();

#ifdef ZmqttDiscovery
      if (disc) {
        if (!connectedOnce) {
          pubMqttDiscovery(); // at first connection we publish the discovery payloads
        }
      }
#endif
      connectedOnce = true;
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
      if (now > (timer_sys_measures + (TimeBetweenReadingSYS * 1000)) || !timer_sys_measures) {
        timer_sys_measures = millis();
        stateMeasures();
      }
#endif
#ifdef ZsensorBME280
      MeasureTempHumAndPressure(); //Addon to measure Temperature, Humidity, Pressure and Altitude with a Bosch BME280
#endif
#ifdef ZsensorHTU21
      MeasureTempHum(); //Addon to measure Temperature, Humidity, of a HTU21 sensor
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
#ifdef ZsensorTSL2561
      MeasureLightIntensityTSL2561();
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
#ifdef ZgatewayLORA
      LORAtoMQTT();
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
#ifdef ZgatewayPilight
      PilighttoMQTT();
#endif
#ifdef ZgatewayBT
#  ifdef ZmqttDiscovery
      if (disc)
        launchBTDiscovery();
#  endif
#  ifndef ESP32
      if (BTtoMQTT())
        Log.trace(F("BTtoMQTT OK" CR));
#  else
      emptyBTQueue();
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
    } else {
      connectMQTT();
    }
  } else { // disconnected from network
    Log.warning(F("Network disconnected:" CR));
    digitalWrite(LED_INFO, LED_INFO_ON);
    delay(5000); // add a delay to avoid ESP32 crash and reset
    digitalWrite(LED_INFO, !LED_INFO_ON);
    delay(5000);
#if defined(ESP8266) || defined(ESP32) && !defined(ESP32_ETHERNET)
    Log.warning(F("wifi" CR));
    failure_number_ntwk++;
    disconnection_handling(failure_number_ntwk);
#else
    Log.warning(F("ethernet" CR));
#endif
  }
// Function that doesn't need an active connection
#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
  loopM5();
#endif
}

#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
void stateMeasures() {
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& SYSdata = jsonBuffer.createObject();
  unsigned long uptime = millis() / 1000;
  SYSdata["uptime"] = uptime;
  SYSdata["version"] = OMG_VERSION;
  Log.trace(F("retrieving value of system characteristics Uptime (s):%u" CR), uptime);
#  if defined(ESP8266) || defined(ESP32)
  uint32_t freeMem;
  freeMem = ESP.getFreeHeap();
  SYSdata["freemem"] = freeMem;
#    ifdef ESP32_ETHERNET
  SYSdata["mac"] = (char*)ETH.macAddress().c_str();
  SYSdata["ip"] = ip2CharArray(ETH.localIP());
  ETH.fullDuplex() ? SYSdata["fd"] = (bool)"true" : SYSdata["fd"] = (bool)"false";
  SYSdata["linkspeed"] = (int)ETH.linkSpeed();
#    else
  long rssi = WiFi.RSSI();
  SYSdata["rssi"] = rssi;
  String SSID = WiFi.SSID();
  SYSdata["SSID"] = SSID;
  SYSdata["ip"] = ip2CharArray(WiFi.localIP());
  String mac = WiFi.macAddress();
  SYSdata["mac"] = (char*)mac.c_str();
  SYSdata["wifiprt"] = (int)wifiProtocol;
#    endif
#  endif
#  ifdef ZgatewayBT
#    ifdef ESP32
  SYSdata["lowpowermode"] = (int)lowpowermode;
  SYSdata["btqblck"] = btQueueBlocked;
  SYSdata["btqsum"] = btQueueLengthSum;
  SYSdata["btqsnd"] = btQueueLengthCount;
  SYSdata["btqavg"] = (btQueueLengthCount > 0 ? btQueueLengthSum / (float)btQueueLengthCount : 0);
#    endif
  SYSdata["interval"] = BLEinterval;
  SYSdata["scanbcnct"] = BLEscanBeforeConnect;
#  endif
#  ifdef ZboardM5STACK
  M5.Power.begin();
  SYSdata["m5battlevel"] = (int8_t)M5.Power.getBatteryLevel();
  SYSdata["m5ischarging"] = (bool)M5.Power.isCharging();
  SYSdata["m5ischargefull"] = (bool)M5.Power.isChargeFull();
#  endif
#  if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP)
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
#  endif
  SYSdata.set("modules", modules);
  pub(subjectSYStoMQTT, SYSdata);
}
#endif

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation)
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
        Log.notice(F("no pub. dupl" CR));
        return true;
      }
    }
  }
  return false;
}
#endif

void receivingMQTT(char* topicOri, char* datacallback) {
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& jsondata = jsonBuffer.parseObject(datacallback);

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation)
  if (strstr(topicOri, subjectMultiGTWKey) != NULL) { // storing received value so as to avoid publishing this value if it has been already sent by this or another OpenMQTTGateway
    SIGNAL_SIZE_UL_ULL data = jsondata.success() ? jsondata["value"] : STRTO_UL_ULL(datacallback, NULL, 10);
    if (data != 0 && !isAduplicateSignal(data)) {
      storeSignalValue(data);
    }
  }
#endif

  if (jsondata.success()) { // json object ok -> json decoding
    // log the received json
    logJson(jsondata);
#ifdef ZgatewayPilight // ZgatewayPilight is only defined with json publishing due to its numerous parameters
    MQTTtoPilight(topicOri, jsondata);
#endif
#ifdef jsonReceiving
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
#  if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK)
    MQTTtoM5(topicOri, jsondata);
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
#endif
    digitalWrite(LED_SEND, LED_SEND_ON);

    MQTTtoSYS(topicOri, jsondata);
  } else { // not a json object --> simple decoding
#ifdef simpleReceiving
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

void MQTTtoSYS(char* topicOri, JsonObject& SYSdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoSYSset)) {
    Log.trace(F("MQTTtoSYS json" CR));
#if defined(ESP8266) || defined(ESP32)
    if (SYSdata.containsKey("cmd")) {
      const char* cmd = SYSdata["cmd"];
      Log.notice(F("Command: %s" CR), cmd);
      if (strstr(cmd, restartCmd) != NULL) { //restart
#  if defined(ESP8266)
        ESP.reset();
#  else
        ESP.restart();
#  endif
      } else if (strstr(cmd, eraseCmd) != NULL) { //erase and restart
#  ifndef ESPWifiManualSetup
        setup_wifimanager(true);
#  endif
      }
    }
#endif
#ifdef ZmqttDiscovery
    if (SYSdata.containsKey("discovery")) {
      if (SYSdata.is<bool>("discovery")) {
        disc = SYSdata["discovery"];
        if (disc)
          pubMqttDiscovery();
      } else {
        Log.error(F("Discovery command not a boolean" CR));
      }
      Log.notice(F("Discovery state: %T" CR), disc);
    }
#endif
  }
}
