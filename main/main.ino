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

// array to store previous received RFs, IRs codes and their timestamps
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#define array_size 12
unsigned long ReceivedSignal[array_size][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
//Time used to wait for an interval before checking system measures
unsigned long timer_sys_measures = 0;
#else // boards with smaller memory
#define array_size 4
unsigned long ReceivedSignal[array_size][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
#endif

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>

// Modules config inclusion
#if defined(ZgatewayRF) || defined(ZgatewayRF2) || defined(ZgatewayPilight)
#include "config_RF.h"
#endif
#ifdef ZgatewayWeatherStation
#include "config_WeatherStation.h"
#endif
#ifdef ZgatewayLORA
#include "config_LORA.h"
#endif
#ifdef ZgatewaySRFB
#include "config_SRFB.h"
#endif
#ifdef ZgatewayBT
#include "config_BT.h"
#endif
#ifdef ZgatewayIR
#include "config_IR.h"
#endif
#ifdef Zgateway2G
#include "config_2G.h"
#endif
#ifdef ZactuatorONOFF
#include "config_ONOFF.h"
#endif
#ifdef ZsensorINA226
#include "config_INA226.h"
#endif
#ifdef ZsensorHCSR501
#include "config_HCSR501.h"
#endif
#ifdef ZsensorADC
#include "config_ADC.h"
#endif
#ifdef ZsensorBH1750
#include "config_BH1750.h"
#endif
#ifdef ZsensorTSL2561
#include "config_TSL2561.h"
#endif
#ifdef ZsensorBME280
#include "config_BME280.h"
#endif
#ifdef ZsensorHTU21
#include "config_HTU21.h"
#endif
#ifdef ZsensorHCSR04
#include "config_HCSR04.h"
#endif
#ifdef ZsensorDHT
#include "config_DHT.h"
#endif
#ifdef ZsensorDS1820
#include "config_DS1820.h"
#endif
#ifdef ZgatewayRFM69
#include "config_RFM69.h"
#endif
#ifdef ZsensorGPIOInput
#include "config_GPIOInput.h"
#endif
#ifdef ZsensorGPIOKeyCode
#include "config_GPIOKeyCode.h"
#endif
#ifdef ZmqttDiscovery
#include "config_mqttDiscovery.h"
#endif
#ifdef ZactuatorFASTLED
#include "config_FASTLED.h"
#endif
#if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
#include "config_M5.h"
#endif
/*------------------------------------------------------------------------*/

//adding this to bypass the problem of the arduino builder issue 50
void callback(char *topic, byte *payload, unsigned int length);

bool connectedOnce = false; //indicate if we have been connected once to MQTT
int failure_number_ntwk = 0; // number of failure connecting to network
int failure_number_mqtt = 0; // number of failure connecting to MQTT

#ifdef ESP32
  #include <FS.h>
  #include "SPIFFS.h"
  #include <WiFi.h>
  #include <ArduinoOTA.h>
  #include <WiFiUdp.h>
  #include "esp_wifi.h"
  WiFiClient eClient;
  #include <WiFiManager.h>
  #include <Preferences.h>
  Preferences preferences;
#ifdef MDNS_SD
  #include <ESPmDNS.h>
#endif
#elif defined(ESP8266)
  #include <FS.h>
  #include <ESP8266WiFi.h>
  #include <ArduinoOTA.h>
  #include <DNSServer.h>
  #include <ESP8266WebServer.h>
  #include <WiFiManager.h>
  WiFiClient eClient;
#ifdef MDNS_SD
  #include <ESP8266mDNS.h>
#endif
#else
  #include <Ethernet.h>
  EthernetClient eClient;
#endif

// client link to pubsub mqtt
PubSubClient client(eClient);

//MQTT last attemps reconnection date
unsigned long lastMQTTReconnectAttempt = 0;

void revert_hex_data(char *in, char *out, int l)
{
  //reverting array 2 by 2 to get the data in good order
  int i = l - 2, j = 0;
  while (i != -2)
  {
    if (i % 2 == 0)
      out[j] = in[i + 1];
    else
      out[j] = in[i - 1];
    j++;
    i--;
  }
  out[l - 1] = '\0';
}

void extract_char(char *token_char, char *subset, int start, int l, bool reverse, bool isNumber)
{
  char tmp_subset[l + 1];
  memcpy(tmp_subset, &token_char[start], l);
  tmp_subset[l] = '\0';
  if (isNumber)
  {
    char tmp_subset2[l + 1];
    if (reverse)
      revert_hex_data(tmp_subset, tmp_subset2, l + 1);
    else
      strncpy(tmp_subset2, tmp_subset, l + 1);
    long long_value = strtoul(tmp_subset2, NULL, 16);
    sprintf(tmp_subset2, "%ld", long_value);
    strncpy(subset, tmp_subset2, l + 1);
  }
  else
  {
    if (reverse)
      revert_hex_data(tmp_subset, subset, l + 1);
    else
      strncpy(subset, tmp_subset, l + 1);
  }
  subset[l] = '\0';
}

int strpos(char *haystack, char *needle) //from @miere https://stackoverflow.com/users/548685/miere
{
  char *p = strstr(haystack, needle);
  if (p)
    return p - haystack;
  return -1;
}

char *ip2CharArray(IPAddress ip)
{ //from Nick Lee https://stackoverflow.com/questions/28119653/arduino-display-ethernet-localip
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}

bool to_bool(String const &s)
{ // thanks Chris Jester-Young from stackoverflow
  return s != "0";
}

void pub(char *topicori, char *payload, bool retainFlag)
{
  String topic = String(mqtt_topic) + String(topicori);
  pubMQTT((char *)topic.c_str(), payload, retainFlag);
}

void pub(char *topicori, JsonObject &data)
{
  Log.notice(F("Subject: %s" CR),topicori);
  logJson(data);
  if (client.connected())
  {
    digitalWrite(led_receive, HIGH);
    String topic = String(mqtt_topic) + String(topicori);
#ifdef valueAsASubject
    unsigned long value = data["value"];
    if (value != 0)
    {
      topic = topic + "/" + String(value);
    }
#endif

#ifdef jsonPublishing
    Log.trace(F("jsonPublishing" CR));
    #if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    char JSONmessageBuffer[data.measureLength() + 1];
    #else
    char JSONmessageBuffer[JSON_MSG_BUFFER];
    #endif
    data.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    pubMQTT(topic, JSONmessageBuffer);
#endif

#ifdef simplePublishing
    Log.trace(F("simplePublishing" CR));
    // Loop through all the key-value pairs in obj
    for (JsonPair &p : data)
    {
      #if defined(ESP8266)
      yield();
      #endif
      if (p.value.is<unsigned long>() && strcmp(p.key, "rssi") != 0)
      { //test rssi , bypass solution due to the fact that a int is considered as an unsigned long
        if (strcmp(p.key, "value") == 0)
        { // if data is a value we don't integrate the name into the topic
          pubMQTT(topic, p.value.as<unsigned long>());
        }
        else
        { // if data is not a value we integrate the name into the topic
          pubMQTT(topic + "/" + String(p.key), p.value.as<unsigned long>());
        }
      }
      else if (p.value.is<int>())
      {
        pubMQTT(topic + "/" + String(p.key), p.value.as<int>());
      }
      else if (p.value.is<float>())
      {
        pubMQTT(topic + "/" + String(p.key), p.value.as<float>());
      }
      else if (p.value.is<char *>())
      {
        pubMQTT(topic + "/" + String(p.key), p.value.as<const char *>());
      }
    }
#endif
  }
  else
  {
    Log.warning(F("client not connected can't pub" CR));
  }
}

void pub(char *topicori, char *payload)
{
  if (client.connected())
  {
    String topic = String(mqtt_topic) + String(topicori);
    Log.trace(F("Pub ack %s into: %s" CR), payload, topic.c_str());
    pubMQTT(topic, payload);
  }
  else
  {
    Log.warning(F("client not connected can't pub" CR));
  }
}

void pub_custom_topic(char *topicori, JsonObject &data, boolean retain)
{
  if (client.connected())
  {
    #if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    char JSONmessageBuffer[data.measureLength() + 1];
    #else
    char JSONmessageBuffer[JSON_MSG_BUFFER];
    #endif
    data.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Log.trace(F("Pub json :%s into custom topic: %s" CR), JSONmessageBuffer, topicori);
    pubMQTT(topicori, JSONmessageBuffer, retain);
  }
  else
  {
    Log.warning(F("client not connected can't pub" CR));
  }
}

// Low level MQTT functions 
void pubMQTT(char * topic, char * payload)
{
    client.publish(topic, payload);
}

void pubMQTT(char * topicori, char * payload, bool retainFlag)
{
  client.publish(topicori, payload, retainFlag);
}

void pubMQTT(String topic, char *  payload)
{
    client.publish((char *)topic.c_str(),payload);
}

void pubMQTT(char *topic, unsigned long payload)
{
  char val[11];
  sprintf(val, "%lu", payload);
  client.publish(topic, val);
}

void pubMQTT(char *topic, unsigned long long payload)
{
  char val[21];
  sprintf(val, "%llu", payload);
  client.publish(topic, val);
}

void pubMQTT(char *topic, String payload)
{
  client.publish(topic, (char *)payload.c_str());
}

void pubMQTT(String topic, String payload)
{
  client.publish((char *)topic.c_str(), (char *)payload.c_str());
}

void pubMQTT(String topic, int payload)
{
  char val[12];
  sprintf(val, "%d", payload);
  client.publish((char *)topic.c_str(), val);
}

void pubMQTT(String topic, float payload)
{
  char val[12];
  dtostrf(payload, 3, 1, val);
  client.publish((char *)topic.c_str(), val);
}

void pubMQTT(char *topic, float payload)
{
  char val[12];
  dtostrf(payload, 3, 1, val);
  client.publish(topic, val);
}

void pubMQTT(char *topic, int payload)
{
  char val[6];
  sprintf(val, "%d", payload);
  client.publish(topic, val);
}

void pubMQTT(char *topic, unsigned int payload)
{
  char val[6];
  sprintf(val, "%u", payload);
  client.publish(topic, val);
}

void pubMQTT(char *topic, long payload)
{
  char val[11];
  sprintf(val, "%l", payload);
  client.publish(topic, val);
}

void pubMQTT(char *topic, double payload)
{
  char val[16];
  sprintf(val, "%d", payload);
  client.publish(topic, val);
}

void pubMQTT(String topic, unsigned long payload)
{
  char val[11];
  sprintf(val, "%lu", payload);
  client.publish((char *)topic.c_str(), val);
}

void logJson(JsonObject &jsondata)
{
  #if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  char JSONmessageBuffer[jsondata.measureLength() + 1];
  #else
  char JSONmessageBuffer[JSON_MSG_BUFFER];
  #endif
  jsondata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Log.notice(F("Received json : %s" CR),JSONmessageBuffer);
}

bool cmpToMainTopic(char * topicOri, char * toAdd){
  char topic[mqtt_topic_max_size];
  strcpy(topic,mqtt_topic);
  strcat(topic,toAdd);
  if (strstr(topicOri,topic) != NULL){
    return true;
  }else{
    return false;
  }
}

void connectMQTT()
{

#ifndef ESPWifiManualSetup
  #if defined(ESP8266) || defined(ESP32)
    checkButton(); // check if a reset of wifi/mqtt settings is asked
  #endif
#endif

  Log.warning(F("MQTT connection..." CR));
  char topic[mqtt_topic_max_size];
  strcpy(topic,mqtt_topic);
  strcat(topic,will_Topic);
  if (client.connect(gateway_name, mqtt_user, mqtt_pass, topic, will_QoS, will_Retain, will_Message))
  {
    #if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
    if (low_power_mode < 2) 
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
    strcpy(topic2,mqtt_topic);
    strcat(topic2,subjectMQTTtoX);
    if (client.subscribe(topic2))
    {
      #ifdef ZgatewayRF
      client.subscribe(subjectMultiGTWRF); // subject on which other OMG will publish, this OMG will store these msg and by the way don't republish them if they have been already published
      #endif
      #ifdef ZgatewayIR
      client.subscribe(subjectMultiGTWIR); // subject on which other OMG will publish, this OMG will store these msg and by the way don't republish them if they have been already published
      #endif
      Log.trace(F("Subscription OK to the subjects" CR));
    }
  }
  else
  {
    failure_number_mqtt++; // we count the failure
    Log.warning(F("failure_number_mqtt: %d" CR), failure_number_mqtt);
    Log.warning(F("failed, rc=%d" CR), client.state());
    delay(5000);
    #if defined(ESP8266) || defined(ESP32)
    disconnection_handling(failure_number_mqtt);
    #endif
  }
}

// Callback function, when the gateway receive an MQTT value on the topics subscribed this function is called
void callback(char *topic, byte *payload, unsigned int length)
{
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  Log.trace(F("Hey I got a callback " CR));
  // Allocate the correct amount of memory for the payload copy
  byte *p = (byte *)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);
  // Conversion to a printable string
  p[length] = '\0';
  //launch the function to treat received data if this data concern OpenMQTTGateway
  if ((strstr(topic, subjectMultiGTWKey) != NULL) || (strstr(topic, subjectGTWSendKey) != NULL))
    receivingMQTT(topic, (char *)p);
  // Free the memory
  free(p);
}

void setup_parameters()
{
  strcat(mqtt_topic, gateway_name);
}

void setup()
{
  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  Log.begin(LOG_LEVEL, &Serial);
  Log.notice(F(CR "************* WELCOME TO OpenMQTTGateway **************" CR));

#if defined(ESP8266) || defined(ESP32)
  #ifdef ESP8266
    #ifndef ZgatewaySRFB // if we are not in sonoff rf bridge case we apply the ESP8266 pin optimization
    Serial.end();
    Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY); // enable on ESP8266 to free some pin
    #endif
  #elif ESP32
    preferences.begin(Gateway_Short_Name, false);
    low_power_mode = preferences.getUInt("low_power_mode", DEFAULT_LOW_POWER_MODE);
    preferences.end();
    #if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
    setupM5();
    #endif
  #endif

  Log.trace(F("OpenMQTTGateway Wifi protocol used: %d" CR), wifiProtocol);

  #if defined(ESPWifiManualSetup)
  setup_wifi();
  #else
  setup_wifimanager(false);
  #endif

  Log.trace(F("OpenMQTTGateway mac: %s" CR),WiFi.macAddress().c_str());
  Log.trace(F("OpenMQTTGateway ip: %s" CR),WiFi.localIP().toString().c_str());

  setOTA();

  #else // In case of arduino platform

  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  //Begining ethernet connection in case of Arduino + W5100
  setup_ethernet();
  #endif

  //setup LED status
  pinMode(led_receive, OUTPUT);
  pinMode(led_send, OUTPUT);
  pinMode(led_info, OUTPUT);
  digitalWrite(led_receive, LOW);
  digitalWrite(led_send, LOW);
  digitalWrite(led_info, LOW);

  #if defined(MDNS_SD) && defined(ESP8266)
  Log.trace(F("Connecting to MQTT by mDNS without mqtt hostname" CR));
  connectMQTTmdns();
  #else
  long port;
  port = strtol(mqtt_port, NULL, 10);
  Log.trace(F("Port: %l" CR),port);
  #ifdef mqtt_server_name // if name is defined we define the mqtt server by its name
    IPAddress mqtt_server_ip;
    WiFi.hostByName(mqtt_server_name, mqtt_server_ip);
    client.setServer(mqtt_server_ip, port);
    Log.trace(F("Mqtt server connection by host name: %s" CR),mqtt_server_ip.toString());
  #else                   // if not by its IP adress
    client.setServer(mqtt_server, port);
    Log.trace(F("Mqtt server connection by IP: %s" CR),mqtt_server);
  #endif
  #endif

  setup_parameters();

  client.setCallback(callback);

  delay(1500);

  lastMQTTReconnectAttempt = 0;

  #ifdef ZsensorBME280
  setupZsensorBME280();
  #endif
  #ifdef ZsensorHTU21
  setupZsensorHTU21();
  #endif
  #ifdef ZsensorBH1750
  setupZsensorBH1750();
  #endif
  #ifdef ZsensorTSL2561
  setupZsensorTSL2561();
  #endif
  #ifdef Zgateway2G
  setup2G();
  #endif
  #ifdef ZgatewayIR
  setupIR();
  #endif
  #ifdef ZgatewayLORA
  setupLORA();
  #endif
  #ifdef ZgatewayRF
  setupRF();
  #endif
  #ifdef ZgatewayRF2
  setupRF2();
  #endif
  #ifdef ZgatewayPilight
  setupPilight();
  #endif
  #ifdef ZgatewayWeatherStation
  setupWeatherStation();
  #endif
  #ifdef ZgatewaySRFB
  setupSRFB();
  #endif
  #ifdef ZgatewayBT
  setupBT();
  #endif
  #ifdef ZgatewayRFM69
  setupRFM69();
  #endif
  #ifdef ZsensorINA226
  setupINA226();
  #endif
  #ifdef ZsensorHCSR501
  setupHCSR501();
  #endif
  #ifdef ZsensorHCSR04
  setupHCSR04();
  #endif
  #ifdef ZsensorGPIOInput
  setupGPIOInput();
  #endif
  #ifdef ZsensorGPIOKeyCode
  setupGPIOKeyCode();
  #endif
  #ifdef ZactuatorFASTLED
  setupFASTLED();
  #endif
  #ifdef ZsensorDS1820
  setupZsensorDS1820();
  #endif
  #ifdef ZsensorADC
  setupADC();
  #endif
  #ifdef ZsensorDHT
  setupDHT();
  #endif
  Log.trace(F("MQTT_MAX_PACKET_SIZE: %d" CR),MQTT_MAX_PACKET_SIZE);
  #if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  if (MQTT_MAX_PACKET_SIZE == 128)
    Log.error(F("WRONG PUBSUBCLIENT LIBRARY USED PLEASE INSTALL THE ONE FROM RELEASE PAGE" CR));
  #endif
  Log.notice(F("Setup OpenMQTTGateway end" CR));
}

#if defined(ESP8266) || defined(ESP32)
// Bypass for ESP not reconnecting automaticaly the second time https://github.com/espressif/arduino-esp32/issues/2501
bool wifi_reconnect_bypass(){
  uint8_t  wifi_autoreconnect_cnt = 0;
  #ifdef ESP32
    while (WiFi.status() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
  #else
    while (WiFi.waitForConnectResult() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
  #endif
    Log.notice(F("Attempting Wifi connection with saved AP: %d" CR), wifi_autoreconnect_cnt);
    WiFi.begin();
    delay(500);
    wifi_autoreconnect_cnt++;
  } 
  if(wifi_autoreconnect_cnt < maxConnectionRetryWifi)
  {
    return true;
  } else {
    return false;
  }
}

// the 2 methods below are used to recover wifi connection by changing the protocols
void forceWifiProtocol(){
#ifdef ESP32
  Log.warning(F("ESP32: Forcing to wifi %d" CR), wifiProtocol);
  esp_wifi_set_protocol(WIFI_IF_STA, wifiProtocol);
#elif ESP8266
  Log.warning(F("ESP8266: Forcing to wifi %d" CR), wifiProtocol);
  WiFi.setPhyMode((WiFiPhyMode_t)wifiProtocol);
#endif
}

void reinit_wifi()
{
  delay(10);
  WiFi.mode(WIFI_STA);
  if (!wifiProtocol) forceWifiProtocol();
  WiFi.begin();
}

void disconnection_handling( int failure_number){
  Log.warning(F("disconnection_handling, failed %d times" CR), failure_number);
  if (failure_number > maxConnectionRetry && !connectedOnce)
  {
  #ifndef ESPWifiManualSetup
    Log.error(F("Failed connecting 1st time to mqtt, reset wifi manager & erase network credentials" CR));
    setup_wifimanager(true);
  #endif
  }
  else if (failure_number <= maxConnectionRetry + ATTEMPTS_BEFORE_BG && connectedOnce)
  {
    Log.warning(F("Attempt to reinit wifi: %d" CR), wifiProtocol);
    reinit_wifi();
  }
  else if (failure_number > maxConnectionRetry + ATTEMPTS_BEFORE_BG && failure_number <= maxConnectionRetry + ATTEMPTS_BEFORE_B ) // After maxConnectionRetry + ATTEMPTS_BEFORE_BG try to connect with BG protocol
  {
    #ifdef ESP32
    wifiProtocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G;
    #elif ESP8266
    wifiProtocol = WIFI_PHY_MODE_11G;
    #endif
    Log.warning(F("Wifi Protocol changed to WIFI_11G: %d" CR), wifiProtocol);
    reinit_wifi();
  }
  else if (failure_number > maxConnectionRetry + ATTEMPTS_BEFORE_B && failure_number <= maxConnectionRetry + ATTEMPTS_BEFORE_B + ATTEMPTS_BEFORE_BG ) // After maxConnectionRetry + ATTEMPTS_BEFORE_B try to connect with B protocol
  {
    #ifdef ESP32
    wifiProtocol = WIFI_PROTOCOL_11B;
    #elif ESP8266
    wifiProtocol = WIFI_PHY_MODE_11B;
    #endif
    Log.warning(F("Wifi Protocol changed to WIFI_11B: %d" CR), wifiProtocol);
    reinit_wifi();
  }
  else if (failure_number > maxConnectionRetry + ATTEMPTS_BEFORE_B + ATTEMPTS_BEFORE_BG ) // After maxConnectionRetry + ATTEMPTS_BEFORE_B try to connect with B protocol
  {
    #ifdef ESP32
    wifiProtocol = 0;
    #elif ESP8266
    wifiProtocol = 0;
    #endif
    Log.warning(F("Wifi Protocol reverted to normal mode: %d" CR), wifiProtocol);
    reinit_wifi();
  }
}

void setOTA(){
  // Port defaults to 8266
  ArduinoOTA.setPort(ota_port);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ota_hostname);

  // No authentication by default
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    Log.trace(F("Start OTA" CR));
  });
  ArduinoOTA.onEnd([]() {
    Log.trace(F("\nEnd OTA" CR));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.trace(F("Progress: %u%%\r" CR), (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
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
#endif

#if defined(ESPWifiManualSetup)
void setup_wifi()
{

  delay(10);
  WiFi.mode(WIFI_STA);
  if (!wifiProtocol) forceWifiProtocol();

  // We start by connecting to a WiFi network
  Log.trace(F("Connecting to %s" CR), wifi_ssid);
  #ifdef ESPWifiAdvancedSetup
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  if (!WiFi.config(ip_adress, gateway_adress, subnet_adress, dns_adress))
  {
    Log.error(F("Wifi STA Failed to configure" CR));
  }
  WiFi.begin(wifi_ssid, wifi_password);
  #else
  WiFi.begin(wifi_ssid, wifi_password);
  #endif

  if(wifi_reconnect_bypass());
    Log.notice(F("Connected with saved credentials" CR));

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Log.trace(F("." CR));
    failure_number_ntwk++;
    disconnection_handling(failure_number_ntwk);
  }
  Log.notice(F("WiFi ok with manual config credentials" CR));
}

#elif defined(ESP8266) || defined(ESP32)

WiFiManager wifiManager;

//flag for saving data
bool shouldSaveConfig = true;
//do we have been connected once to mqtt

//callback notifying us of the need to save config
void saveConfigCallback()
{
  Log.trace(F("Should save config" CR));
  shouldSaveConfig = true;
}

void checkButton()
{ // code from tzapu/wifimanager examples
  // check for button press
  if (digitalRead(TRIGGER_PIN) == LOW)
  {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if (digitalRead(TRIGGER_PIN) == LOW)
    {
      Log.trace(F("Trigger button Pressed" CR));
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if (digitalRead(TRIGGER_PIN) == LOW)
      {
        Log.trace(F("Button Held" CR));
        Log.notice(F("Erasing ESP Config, restarting" CR));
        setup_wifimanager(true);
      }
    }
  }
}

void eraseAndRestart()
{
  #if defined(ESP8266)
  WiFi.disconnect(true);
  #else
  WiFi.disconnect(true, true);
  #endif

  Log.trace(F("Formatting requested, result: %d" CR), SPIFFS.format());

  #if defined(ESP8266)
  ESP.eraseConfig();
  delay(5000);
  ESP.reset();
  #else
  ESP.restart();
  #endif
}

void setup_wifimanager(bool reset_settings)
{

  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  delay(10);
  WiFi.mode(WIFI_STA);
  if (!wifiProtocol) forceWifiProtocol();

  if (reset_settings)
    eraseAndRestart();

  //read configuration from FS json
  Log.trace(F("mounting FS..." CR));

  if (SPIFFS.begin())
  {
    Log.trace(F("mounted file system" CR));
  }
  else
  {
    Log.warning(F("failed to mount FS -> formating" CR));
    SPIFFS.format();
    if (SPIFFS.begin())
      Log.trace(F("mounted file system after formating" CR));
  }
  if (SPIFFS.exists("/config.json"))
  {
    //file exists, reading and loading
    Log.trace(F("reading config file" CR));
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile)
    {
      Log.trace(F("opened config file" CR));
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject &json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if (json.success())
      {
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
      }
      else
      {
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
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, parameters_size);
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
  #ifdef NetworkAdvancedSetup
  Log.trace(F("Adv wifi cfg" CR));
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress ip_adress(ip);
  wifiManager.setSTAStaticIPConfig(ip_adress, gateway_adress,subnet_adress);
  #endif

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_gateway_name);
  wifiManager.addParameter(&custom_mqtt_topic);

  //set minimum quality of signal so it ignores AP's under that quality
  wifiManager.setMinimumSignalQuality(MinimumWifiSignalQuality);

  if(!wifi_reconnect_bypass())// if we didn't connect with saved credential we start Wifimanager web portal
  {
    #ifdef ESP32
    if (low_power_mode < 2)
    {
      #if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
      M5Display("Connect your phone to WIFI AP:", WifiManager_ssid, WifiManager_password);
      #endif
    } else { // in case of low power mode we put the ESP to sleep again if we didn't get connected (typical in case the wifi is down)
      #ifdef ZgatewayBT
      lowPowerESP32();
      #endif
    }
    #endif

    Log.notice(F("Connect your phone to WIFI AP: %s with PWD: %s" CR), WifiManager_ssid, WifiManager_password );
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect(WifiManager_ssid, WifiManager_password))
    {
      Log.warning(F("failed to connect and hit timeout" CR));
      delay(3000);
      //reset and try again
      #if defined(ESP8266)
      ESP.reset();
      #else
      ESP.restart();
      #endif
      delay(5000);
    }
  }

  #if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
  if (low_power_mode < 2) 
    M5Display("Wifi connected", "", "");
  #endif

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
  strcpy(gateway_name, custom_gateway_name.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    Log.trace(F("saving config" CR));
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;
    json["mqtt_topic"] = mqtt_topic;
    json["gateway_name"] = gateway_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Log.error(F("failed to open config file for writing" CR));
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
}

#else // Arduino case
void setup_ethernet()
{
  #ifdef NetworkAdvancedSetup
  Log.trace(F("Adv eth cfg" CR));
  Ethernet.begin(mac, ip, Dns, gateway, subnet);
  #else
  Log.trace(F("Spl eth cfg" CR));
  Ethernet.begin(mac, ip);
  #endif
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Log.error(F("Ethernet shield was not found." CR));
  }
  else
  {
    Log.trace(F("ip: %s " CR),Ethernet.localIP());
  }
}
#endif

#if defined(MDNS_SD) && defined(ESP8266)
void connectMQTTmdns()
{
  if (!MDNS.begin("ESP_MQTT"))
  {
    Log.error(F("Error setting up MDNS responder!" CR));
    while (1)
    {
      delay(1000);
    }
  }
  Log.trace(F("Browsing for MQTT service" CR));
  int n = MDNS.queryService("mqtt", "tcp");
  if (n == 0)
  {
    Log.error(F("no services found" CR));
  }
  else
  {
    Log.trace(F("%d service(s) found" CR),n);
    for (int i = 0; i < n; ++i)
    {
      Log.trace(F("Service %d %s found" CR),i, MDNS.hostname(i).c_str());
      Log.trace(F("IP %s Port %d" CR),MDNS.IP(i).toString().c_str(), MDNS.port(i));
    }
    if (n == 1)
    {
      Log.trace(F("One MQTT server found setting parameters" CR));
      client.setServer(MDNS.IP(0), int(MDNS.port(0)));
    }
    else
    {
      Log.error(F("Several MQTT servers found, please deactivate mDNS and set your default server" CR));
    }
  }
}
#endif

void loop()
{
  #ifndef ESPWifiManualSetup
    #if defined(ESP8266) || defined(ESP32)
      checkButton(); // check if a reset of wifi/mqtt settings is asked
    #endif
  #endif

  digitalWrite(led_receive, LOW);
  digitalWrite(led_info, LOW);
  digitalWrite(led_send, LOW);

  unsigned long now = millis();

  #if defined(ESP8266) || defined(ESP32)
  if (WiFi.status() == WL_CONNECTED)
  {
  #else
  if ((Ethernet.hardwareStatus() != EthernetW5100 && Ethernet.linkStatus() == LinkON) || (Ethernet.hardwareStatus() == EthernetW5100))
  { //we are able to detect disconnection only on w5200 and w5500
  #endif
    failure_number_ntwk = 0;
    if (client.connected())
    {
      
      #ifdef ZmqttDiscovery
      if(!connectedOnce) pubMqttDiscovery(); // at first connection we publish the discovery payloads
      #endif

      connectedOnce = true;
      failure_number_ntwk = 0;

      client.loop();

      #if defined(ESP8266) || defined(ESP32)
      ArduinoOTA.handle();
      #endif

      #if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
      if (now > (timer_sys_measures + (TimeBetweenReadingSYS * 1000)) || !timer_sys_measures)
      {
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
      #ifdef ZsensorDS1820
      MeasureDS1820Temp();  //Addon to measure the temperature with DS1820 sensor(s)
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
        #ifndef ESP32
        if (BTtoMQTT())
          Log.trace(F("BTtoMQTT OK" CR));
        #endif
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
      #ifdef ZactuatorFASTLED
      FASTLEDLoop();
      #endif
    }
    else
    {
      connectMQTT();
    }
  }
  else  
  { // disconnected from network
    #if defined(ESP8266) || defined(ESP32)
    Log.warning(F("wifi disconnected" CR));
    delay(10000); // add a delay to avoid ESP32 crash and reset
    failure_number_ntwk++;
    disconnection_handling(failure_number_ntwk);
    #else
    Log.warning(F("eth disconnected" CR));
    #endif
  }
  // Function that doesn't need an active connection
  #if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
  loopM5();
  #endif
}

#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
void stateMeasures()
{
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &SYSdata = jsonBuffer.createObject();
  unsigned long uptime = millis() / 1000;
  SYSdata["uptime"] = uptime;
  SYSdata["version"] = OMG_VERSION;
  Log.trace(F("retriving value of system characteristics Uptime (s):%u" CR),uptime);
  #if defined(ESP8266) || defined(ESP32)
    uint32_t freeMem;
    freeMem = ESP.getFreeHeap();
    SYSdata["freemem"] = freeMem;
    long rssi = WiFi.RSSI();
    SYSdata["rssi"] = rssi;
    String SSID = WiFi.SSID();
    SYSdata["SSID"] = SSID;
    SYSdata["ip"] = ip2CharArray(WiFi.localIP());
    String mac = WiFi.macAddress();
    SYSdata["mac"] = (char *)mac.c_str();
    SYSdata["wifiprt"] = (int)wifiProtocol;
  #else
    SYSdata["ip"] = ip2CharArray(Ethernet.localIP());
  #endif
  String modules = "";
  #ifdef ZgatewayRF
  modules = modules + ZgatewayRF;
  #endif
  #ifdef ZsensorBME280
  modules = modules + ZsensorBME280;
  #endif
  #ifdef ZsensorHTU21
  modules = modules + ZsensorHTU21;
  #endif
  #ifdef ZsensorHCSR04
  modules = modules + ZsensorHCSR04;
  #endif
  #ifdef ZsensorBH1750
  modules = modules + ZsensorBH1750;
  #endif
  #ifdef ZsensorTSL2561
  modules = modules + ZsensorTSL2561;
  #endif
  #ifdef ZsensorDHT
  modules = modules + ZsensorDHT;
  #endif
  #ifdef ZsensorDS1820
  modules = modules + ZsensorDS1820;
  #endif
  #ifdef ZactuatorONOFF
  modules = modules + ZactuatorONOFF;
  #endif
  #ifdef Zgateway2G
  modules = modules + Zgateway2G;
  #endif
  #ifdef ZgatewayIR
  modules = modules + ZgatewayIR;
  #endif
  #ifdef ZgatewayLORA
  modules = modules + ZgatewayLORA;
  #endif
  #ifdef ZgatewayRF2
  modules = modules + ZgatewayRF2;
  #endif
  #ifdef ZgatewayWeatherStation
  modules = modules + ZgatewayWeatherStation;
  #endif
  #ifdef ZgatewayPilight
  modules = modules + ZgatewayPilight;
  #endif
  #ifdef ZgatewaySRFB
  modules = modules + ZgatewaySRFB;
  #endif
  #ifdef ZgatewayBT
  modules = modules + ZgatewayBT;
    #ifdef ESP32
    SYSdata["lowpowermode"] = (int)low_power_mode;
    #endif
  #endif
  #ifdef ZgatewayRFM69
  modules = modules + ZgatewayRFM69;
  #endif
  #ifdef ZsensorINA226
  modules = modules + ZsensorINA226;
  #endif
  #ifdef ZsensorHCSR501
  modules = modules + ZsensorHCSR501;
  #endif
  #ifdef ZsensorGPIOInput
  modules = modules + ZsensorGPIOInput;
  #endif
  #ifdef ZsensorGPIOKeyCode
  modules = modules + ZsensorGPIOKeyCode;
  #endif
  #ifdef ZsensorGPIOKeyCode
  modules = modules + ZsensorGPIOKeyCode;
  #endif
  #ifdef ZmqttDiscovery
  modules = modules + ZmqttDiscovery;
  #endif
  #ifdef ZactuatorFASTLED
  modules = modules + ZactuatorFASTLED;
  #endif
  #ifdef ZboardM5STACK
  M5.Power.begin();
  SYSdata["m5-batt-level"] = (int8_t)M5.Power.getBatteryLevel();
  SYSdata["m5-is-charging"] = (bool)M5.Power.isCharging();
  SYSdata["m5-is-chargefull"] = (bool)M5.Power.isChargeFull();
  #endif
  #ifdef ZboardM5STICKC
  M5.Axp.EnableCoulombcounter();
  SYSdata["m5-bat-voltage"] = (float)M5.Axp.GetBatVoltage();
  SYSdata["m5-bat-current"] = (float)M5.Axp.GetBatCurrent();
  SYSdata["m5-vin-voltage"] = (float)M5.Axp.GetVinVoltage();
  SYSdata["m5-vin-current"] = (float)M5.Axp.GetVinCurrent();
  SYSdata["m5-vbus-voltage"] = (float)M5.Axp.GetVBusVoltage();
  SYSdata["m5-vbus-current"] = (float)M5.Axp.GetVBusCurrent();
  SYSdata["m5-temp-axp"] = (float)M5.Axp.GetTempInAXP192();
  SYSdata["m5-bat-power"] = (float)M5.Axp.GetBatPower();
  SYSdata["m5-bat-chargecurrent"] = (float)M5.Axp.GetBatChargeCurrent();
  SYSdata["m5-aps-voltage"] = (float)M5.Axp.GetAPSVoltage();
  #endif
  SYSdata["modules"] = modules;
  pub(subjectSYStoMQTT, SYSdata);
}
#endif

void storeValue(unsigned long MQTTvalue)
{
  unsigned long now = millis();
  // find oldest value of the buffer
  int o = getMin();
  Log.trace(F("Min ind: %d" CR),o);
  // replace it by the new one
  ReceivedSignal[o][0] = MQTTvalue;
  ReceivedSignal[o][1] = now;
  Log.trace(F("store code : %u / %u" CR),ReceivedSignal[o][0],ReceivedSignal[o][1]);
  Log.trace(F("Col: val/timestamp" CR));
  for (int i = 0; i < array_size; i++)
  {
    Log.trace(F("mem code : %u / %u" CR),ReceivedSignal[i][0],ReceivedSignal[i][1]);
  }
}

int getMin()
{
  unsigned int minimum = ReceivedSignal[0][1];
  int minindex = 0;
  for (int i = 0; i < array_size; i++)
  {
    if (ReceivedSignal[i][1] < minimum)
    {
      minimum = ReceivedSignal[i][1];
      minindex = i;
    }
  }
  return minindex;
}

bool isAduplicate(unsigned long value)
{
  Log.trace(F("isAdupl?" CR));
  // check if the value has been already sent during the last time_avoid_duplicate
  for (int i = 0; i < array_size; i++)
  {
    if (ReceivedSignal[i][0] == value)
    {
      unsigned long now = millis();
      if (now - ReceivedSignal[i][1] < time_avoid_duplicate)
      { // change
        Log.notice(F("no pub. dupl" CR));
        return true;
      }
    }
  }
  return false;
}

void receivingMQTT(char *topicOri, char *datacallback)
{

  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &jsondata = jsonBuffer.parseObject(datacallback);

  if (strstr(topicOri, subjectMultiGTWKey) != NULL) // storing received value so as to avoid publishing this value if it has been already sent by this or another OpenMQTTGateway
  {
    unsigned long data = 0;
    #ifdef jsonPublishing
    if (jsondata.success())
      data = jsondata["value"];
    #endif

    #ifdef simplePublishing
    data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
    #endif

    if (data != 0)
    {
      storeValue(data);
    }
  }

  if (jsondata.success())
  {                    // json object ok -> json decoding
    // log the received json
    logJson(jsondata);

    #ifdef ZgatewayPilight // ZgatewayPilight is only defined with json publishing due to its numerous parameters
    MQTTtoPilight(topicOri, jsondata);
    #endif
    #ifdef jsonReceiving
      #ifdef ZgatewayLORA
      MQTTtoLORA(topicOri, jsondata);
      #endif
      #ifdef ZgatewayRF
      MQTTtoRF(topicOri, jsondata);
      #endif
      #ifdef ZgatewayRF2
      MQTTtoRF2(topicOri, jsondata);
      #endif
      #ifdef Zgateway2G
      MQTTto2G(topicOri, jsondata);
      #endif
      #ifdef ZgatewaySRFB
      MQTTtoSRFB(topicOri, jsondata);
      #endif
      #ifdef ZgatewayIR
      MQTTtoIR(topicOri, jsondata);
      #endif
      #ifdef ZgatewayRFM69
      MQTTtoRFM69(topicOri, jsondata);
      #endif
      #ifdef ZgatewayBT
      MQTTtoBT(topicOri, jsondata);
      #endif
      #ifdef ZactuatorFASTLED
      MQTTtoFASTLED(topicOri, jsondata);
      #endif
      #if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
      MQTTtoM5(topicOri, jsondata);
      #endif 
    #endif
    #ifdef ZactuatorONOFF // outside the jsonpublishing macro due to the fact that we need to use simplepublishing with HA discovery
    MQTTtoONOFF(topicOri, jsondata);
    #endif
    digitalWrite(led_send, HIGH);

    MQTTtoSYS(topicOri, jsondata);
  }
  else
  { // not a json object --> simple decoding
    #ifdef simpleReceiving
      #ifdef ZgatewayLORA
      MQTTtoLORA(topicOri, datacallback);
      #endif
      #ifdef ZgatewayRF
      MQTTtoRF(topicOri, datacallback);
      #endif
      #ifdef ZgatewayRF315
      MQTTtoRF315(topicOri, datacallback);
      #endif
      #ifdef ZgatewayRF2
      MQTTtoRF2(topicOri, datacallback);
      #endif
      #ifdef Zgateway2G
      MQTTto2G(topicOri, datacallback);
      #endif
      #ifdef ZgatewaySRFB
      MQTTtoSRFB(topicOri, datacallback);
      #endif
      #ifdef ZgatewayRFM69
      MQTTtoRFM69(topicOri, datacallback);
      #endif
      #ifdef ZactuatorFASTLED
      MQTTtoFASTLED(topicOri, datacallback);
      #endif
    #endif
    #ifdef ZactuatorONOFF
    MQTTtoONOFF(topicOri, datacallback);
    #endif

    digitalWrite(led_send, HIGH);
  }
  //YELLOW OFF
  digitalWrite(led_send, HIGH);
}

void MQTTtoSYS(char *topicOri, JsonObject &SYSdata)
{ // json object decoding
if (cmpToMainTopic(topicOri,subjectMQTTtoSYSset))
  {
    Log.trace(F("MQTTtoSYS json" CR));
    #if defined(ESP8266) || defined(ESP32)
    if (SYSdata.containsKey("cmd"))
    {
      const char *cmd = SYSdata["cmd"];
      Log.notice(F("Command: %s" CR),cmd);
      if (strstr(cmd, restartCmd) != NULL)
      { //restart
      #if defined(ESP8266)
        ESP.reset();
      #else
        ESP.restart();
      #endif
      }
      else if (strstr(cmd, eraseCmd) != NULL)
      { //erase and restart
        #ifndef ESPWifiManualSetup
        setup_wifimanager(true);
        #endif
      }
      else
      {
        Log.warning(F("wrong command" CR));
      }
    }
    #endif
  }
}
