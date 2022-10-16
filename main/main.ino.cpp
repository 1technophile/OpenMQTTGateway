# 1 "/var/folders/nk/2b522cbn5pdfsm1_n0zm50680000gp/T/tmp3fm37ml6"
#include <Arduino.h>
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/main.ino"
# 28 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/main.ino"
#include "User_config.h"


#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation)

struct ReceivedSignal {
  SIGNAL_SIZE_UL_ULL value;
  uint32_t time;
};
# if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
ReceivedSignal receivedSignal[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
# else
ReceivedSignal receivedSignal[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
# endif
#define struct_size (sizeof(receivedSignal) / sizeof(ReceivedSignal))
#endif

#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)

unsigned long timer_sys_measures = 0;
#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_ENABLE_STD_STRING 1
#endif

#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <PubSubClient.h>

#include <string>

StaticJsonDocument<JSON_MSG_BUFFER> modulesBuffer;
JsonArray modules = modulesBuffer.to<JsonArray>();

#ifndef ZgatewayGFSunInverter



struct GfSun2000Data {};
#endif


#if defined(ZgatewayRF) || defined(ZgatewayRF2) || defined(ZgatewayPilight) || defined(ZactuatorSomfy) || defined(ZgatewayRTL_433)
# include "config_RF.h"
#endif
#ifdef ZgatewayWeatherStation
# include "config_WeatherStation.h"
#endif
#ifdef ZgatewayGFSunInverter
# include "config_GFSunInverter.h"
#endif
#ifdef ZgatewayLORA
# include "config_LORA.h"
#endif
#ifdef ZgatewaySRFB
# include "config_SRFB.h"
#endif
#ifdef ZgatewayBT
# include "config_BT.h"
#endif
#ifdef ZgatewayIR
# include "config_IR.h"
#endif
#ifdef Zgateway2G
# include "config_2G.h"
#endif
#ifdef ZactuatorONOFF
# include "config_ONOFF.h"
#endif
#ifdef ZsensorINA226
# include "config_INA226.h"
#endif
#ifdef ZsensorHCSR501
# include "config_HCSR501.h"
#endif
#ifdef ZsensorADC
# include "config_ADC.h"
#endif
#ifdef ZsensorBH1750
# include "config_BH1750.h"
#endif
#ifdef ZsensorTSL2561
# include "config_TSL2561.h"
#endif
#ifdef ZsensorBME280
# include "config_BME280.h"
#endif
#ifdef ZsensorHTU21
# include "config_HTU21.h"
#endif
#ifdef ZsensorAHTx0
# include "config_AHTx0.h"
#endif
#ifdef ZsensorHCSR04
# include "config_HCSR04.h"
#endif
#ifdef ZsensorDHT
# include "config_DHT.h"
#endif
#ifdef ZsensorSHTC3
# include "config_SHTC3.h"
#endif
#ifdef ZsensorDS1820
# include "config_DS1820.h"
#endif
#ifdef ZgatewayRFM69
# include "config_RFM69.h"
#endif
#ifdef ZsensorGPIOInput
# include "config_GPIOInput.h"
#endif
#ifdef ZsensorGPIOKeyCode
# include "config_GPIOKeyCode.h"
#endif
#ifdef ZmqttDiscovery
# include "config_mqttDiscovery.h"
#endif
#ifdef ZactuatorFASTLED
# include "config_FASTLED.h"
#endif
#ifdef ZactuatorPWM
# include "config_PWM.h"
#endif
#ifdef ZactuatorSomfy
# include "config_Somfy.h"
#endif
#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
# include "config_M5.h"
#endif
#if defined(ZboardHELTEC)
# include "config_HELTEC.h"
#endif
#if defined(ZgatewayRS232)
# include "config_RS232.h"
#endif


void setupTLS(bool self_signed = false, uint8_t index = 0);


void callback(char* topic, byte* payload, unsigned int length);

char mqtt_user[parameters_size + 1] = MQTT_USER;
char mqtt_pass[parameters_size + 1] = MQTT_PASS;
char mqtt_server[parameters_size + 1] = MQTT_SERVER;
char mqtt_port[6] = MQTT_PORT;
char mqtt_topic[parameters_size + 1] = Base_Topic;
char gateway_name[parameters_size + 1] = Gateway_Name;
#ifdef USE_MAC_AS_GATEWAY_NAME
#undef WifiManager_ssid
#undef ota_hostname
#define MAC_NAME_MAX_LEN 30
char WifiManager_ssid[MAC_NAME_MAX_LEN];
char ota_hostname[MAC_NAME_MAX_LEN];
#endif
bool connectedOnce = false;
bool justReconnected = false;
int failure_number_ntwk = 0;
int failure_number_mqtt = 0;
#ifdef ZmqttDiscovery
bool disc = true;
#endif
unsigned long timer_led_measures = 0;
static void* eClient = nullptr;
#if defined(ESP8266) || defined(ESP32)
static bool mqtt_secure = MQTT_SECURE_DEFAULT;
static uint8_t mqtt_ss_index = MQTT_SECURE_SELF_SIGNED_INDEX_DEFAULT;
static String mqtt_cert = "";
static String ota_server_cert = "";
#endif

#ifdef ESP32
# include <ArduinoOTA.h>
# include <FS.h>
# include <SPIFFS.h>
# include <nvs.h>
# include <nvs_flash.h>

# ifdef ESP32_ETHERNET
# include <ETH.h>
void WiFiEvent(WiFiEvent_t event);
static bool esp32EthConnected = false;
# endif

# include <WiFiClientSecure.h>
# include <WiFiMulti.h>
WiFiMulti wifiMulti;
# include <Preferences.h>
# include <WiFiManager.h>
Preferences preferences;
# ifdef MDNS_SD
# include <ESPmDNS.h>
# endif

#elif defined(ESP8266)
# include <ArduinoOTA.h>
# include <DNSServer.h>
# include <ESP8266WebServer.h>
# include <ESP8266WiFi.h>
# include <ESP8266WiFiMulti.h>
# include <FS.h>
# include <WiFiManager.h>
X509List caCert;
# if MQTT_SECURE_SELF_SIGNED_CLIENT
X509List* pClCert = nullptr;
PrivateKey* pClKey = nullptr;
# endif
ESP8266WiFiMulti wifiMulti;
# ifdef MDNS_SD
# include <ESP8266mDNS.h>
# endif

#else
# include <Ethernet.h>
#endif

#define convertTemp_CtoF(c) ((c * 1.8) + 32)
#define convertTemp_FtoC(f) ((f - 32) * 5 / 9)


PubSubClient client;
void revert_hex_data(const char* in, char* out, int l);
char* ip2CharArray(IPAddress ip);
bool to_bool(String const& s);
void pub(const char* topicori, const char* payload, bool retainFlag);
void pub(const char* topicori, JsonObject& data);
void pub(const char* topicori, const char* payload);
void pub_custom_topic(const char* topic, JsonObject& data, boolean retain);
void pubMQTT(const char* topic, const char* payload);
void pubMQTT(const char* topic, const char* payload, bool retainFlag);
void pubMQTT(String topic, const char* payload);
void pubMQTT(const char* topic, unsigned long payload);
void pubMQTT(const char* topic, unsigned long long payload);
void pubMQTT(const char* topic, String payload);
void pubMQTT(String topic, String payload);
void pubMQTT(String topic, int payload);
void pubMQTT(String topic, unsigned long long payload);
void pubMQTT(String topic, float payload);
void pubMQTT(const char* topic, float payload);
void pubMQTT(const char* topic, int payload);
void pubMQTT(const char* topic, unsigned int payload);
void pubMQTT(const char* topic, long payload);
void pubMQTT(const char* topic, double payload);
void pubMQTT(String topic, unsigned long payload);
bool cmpToMainTopic(const char* topicOri, const char* toAdd);
void connectMQTT();
void setup();
bool wifi_reconnect_bypass();
void setOTA();
void setupTLS(bool self_signed, uint8_t index);
void setup_wifi();
void saveConfigCallback();
void checkButton();
void checkButton();
void eraseAndRestart();
void saveMqttConfig();
void setup_wifimanager(bool reset_settings);
void setup_ethernet_esp32();
void setup_ethernet();
void connectMQTTmdns();
void loop();
unsigned long uptime();
void stateMeasures();
void storeSignalValue(SIGNAL_SIZE_UL_ULL MQTTvalue);
int getMin();
bool isAduplicateSignal(SIGNAL_SIZE_UL_ULL value);
void receivingMQTT(char* topicOri, char* datacallback);
void MQTTHttpsFWUpdate(char* topicOri, JsonObject& HttpsFwUpdateData);
void MQTTtoSYS(char* topicOri, JsonObject& SYSdata);
String toString(uint64_t input);
String toString(uint32_t input);
void watchdogReboot(byte reason);
void setupFASTLED();
int animation_step(int duration, int steps);
int animation_step_count(int duration, int steps);
void FASTLEDLoop();
boolean FASTLEDtoMQTT();
void MQTTtoFASTLED(char* topicOri, JsonObject& jsonData);
void MQTTtoFASTLED(char* topicOri, char* datacallback);
void Fire2012WithPalette();
void MQTTtoONOFF(char* topicOri, JsonObject& ONOFFdata);
void MQTTtoONOFF(char* topicOri, char* datacallback);
void ActuatorButtonTrigger();
void setupPWM();
static float perceptualToLinear(float perceptual, int channelIdx);
void PWMLoop();
boolean PWMtoMQTT();
void MQTTtoPWM(char* topicOri, JsonObject& jsonData);
void setupSomfy();
void MQTTtoSomfy(char* topicOri, JsonObject& jsonData);
void logToLCD(bool display);
void setupHELTEC();
void loopHELTEC();
void MQTTtoHELTEC(char* topicOri, JsonObject& HELTECdata);
void heltecPrint(char* line1, char* line2, char* line3);
void heltecPrint(char* line1, char* line2);
void heltecPrint(char* line1);
void logToLCD(bool display);
void setBrightness(int brightness);
void setupM5();
void sleepScreen();
void wakeScreen(int brightness);
void loopM5();
void MQTTtoM5(char* topicOri, JsonObject& M5data);
void displayIntro(int i, int X, int Y);
void drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool line1, bool line2, bool name);
void M5Print(char* line1, char* line2, char* line3);
void setup2G();
void setupGSM(bool deleteSMS);
void signalStrengthAnalysis();
bool _2GtoMQTT();
void MQTTto2G(char* topicOri, char* datacallback);
void MQTTto2G(char* topicOri, JsonObject& SMSdata);
void BTConfig_init();
void BTConfig_load();
JsonObject& getBTJsonObject(const char* json, bool haPresenceEnabled);
void pubBT(JsonObject& data);
void emptyBTQueue();
bool updateWorB(JsonObject& BTdata, bool isWhite);
void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type);
void dumpDevices();
void strupp(char* beg);
void DT24Discovery(const char* mac, const char* sensorModel_id);
void LYWSD03MMCDiscovery(const char* mac, const char* sensorModel);
void MHO_C401Discovery(const char* mac, const char* sensorModel);
void HHCCJCY01HHCCDiscovery(const char* mac, const char* sensorModel);
void XMWSDJ04MMCDiscovery(const char* mac, const char* sensorModel);
void LYWSD03MMCDiscovery(const char* mac, const char* sensorModel);
void MHO_C401Discovery(const char* mac, const char* sensorModel);
void HHCCJCY01HHCCDiscovery(const char* mac, const char* sensorModel);
void DT24Discovery(const char* mac, const char* sensorModel_id);
void XMWSDJ04MMCDiscovery(const char* mac, const char* sensorModel_id);
void procBLETask(void* pvParameters);
void BLEscan();
void BLEconnect();
void stopProcessing();
void startProcessing();
void coreTask(void* pvParameters);
void lowPowerESP32();
void deepSleep(uint64_t time_in_us);
void changelowpowermode(int newLowPowerMode);
void setupBT();
bool BTtoMQTT();
void RemoveJsonPropertyIf(JsonObject& obj, const char* key, bool condition);
boolean valid_service_data(const char* data, int size);
void launchBTDiscovery();
void PublishDeviceData(JsonObject& BLEdata, bool processBLEData);
void process_bledata(JsonObject& BLEdata);
void hass_presence(JsonObject& HomePresence);
void BTforceScan();
void immediateBTAction(void* pvParameters);
void startBTActionTask();
void MQTTtoBTAction(JsonObject& BTdata);
void MQTTtoBT(char* topicOri, JsonObject& BTdata);
void GFSunInverterDataHandler(GfSun2000Data data);
void GFSunInverterErrorHandler(int errorId, char* errorMessage);
void setupGFSunInverter();
void ZgatewayGFSunInverterMQTT();
uint64_t getUInt64fromHex(char const* str);
void setupIR();
void IRtoMQTT();
void MQTTtoIR(char* topicOri, JsonObject& IRdata);
void setupLORA();
void LORAtoMQTT();
void MQTTtoLORA(char* topicOri, JsonObject& LORAdata);
void MQTTtoLORA(char* topicOri, char* LORAdata);
void pilightCallback(const String& protocol, const String& message, int status,
                     size_t repeats, const String& deviceID);
void setupPilight();
void savePilightConfig();
void loadPilightConfig();
void PilighttoMQTT();
void MQTTtoPilight(char* topicOri, JsonObject& Pilightdata);
extern void disablePilightReceive();
extern void enablePilightReceive();
static char* dec2binWzerofill(unsigned long Dec, unsigned int bitLength);
void RFtoMQTTdiscovery(SIGNAL_SIZE_UL_ULL MQTTvalue);
void setupRF();
void RFtoMQTT();
void MQTTtoRF(char* topicOri, char* datacallback);
void MQTTtoRF(char* topicOri, JsonObject& RFdata);
void disableRFReceive();
void enableRFReceive();
void setupRF2();
void RF2toMQTTdiscovery(JsonObject& data);
void RF2toMQTT();
void rf2Callback(unsigned int period, unsigned long address, unsigned long groupBit, unsigned long unit, unsigned long switchType);
void MQTTtoRF2(char* topicOri, char* datacallback);
void MQTTtoRF2(char* topicOri, JsonObject& RF2data);
void disableRF2Receive();
void enableRF2Receive();
uint32_t gc_checksum();
void eeprom_setup();
void setupRFM69(void);
bool RFM69toMQTT(void);
void MQTTtoRFM69(char* topicOri, char* datacallback);
void MQTTtoRFM69(char* topicOri, JsonObject& RFM69data);
void setupRS232();
void RS232toMQTT();
void MQTTtoRS232(char* topicOri, JsonObject& RS232data);
void rtl_433_Callback(char* message);
void setupRTL_433();
void RTL_433Loop();
extern void MQTTtoRTL_433(char* topicOri, JsonObject& RTLdata);
extern void enableRTLreceive();
extern void disableRTLreceive();
extern int getRTLMinimumRSSI();
extern int getRTLCurrentRSSI();
extern int getRTLMessageCount();
void setupSRFB();
void _rfbSend(byte* message);
void _rfbSend(byte* message, int times);
bool SRFBtoMQTT();
void _rfbDecode();
void _rfbAck();
bool _rfbToArray(const char* in, byte* out);
bool _rfbToChar(byte* in, char* out);
void MQTTtoSRFB(char* topicOri, char* datacallback);
void MQTTtoSRFB(char* topicOri, JsonObject& SRFBdata);
void PairedDeviceAdded(byte newID);
void setupWeatherStation();
void sendWindSpeedData(byte id, float wind_speed, byte battery_status);
void sendRainData(byte id, float rain_volume, byte battery_status);
void sendWindData(byte id, int wind_direction, float wind_gust, byte battery_status);
void sendTemperatureData(byte id, float temperature, int humidity, byte battery_status);
void ZgatewayWeatherStationtoMQTT();
String getMacAddress();
String getUniqueId(String name, String sufix);
void createDiscoveryFromList(const char* mac,
                             const char* sensorList[][9],
                             int sensorCount,
                             const char* device_name,
                             const char* device_manufacturer,
                             const char* device_model);
void announceDeviceTrigger(bool use_gateway_info, char* topic, char* type, char* subtype, char* unique_id, char* device_name, char* device_manufacturer, char* device_model, char* device_mac);
void createDiscovery(const char* sensor_type,
                     const char* st_topic, const char* s_name, const char* unique_id,
                     const char* availability_topic, const char* device_class, const char* value_template,
                     const char* payload_on, const char* payload_off, const char* unit_of_meas,
                     int off_delay,
                     const char* payload_available, const char* payload_not_avalaible, bool gateway_entity, const char* cmd_topic,
                     const char* device_name, const char* device_manufacturer, const char* device_model, const char* device_mac, bool retainCmd,
                     const char* state_class, const char* state_off, const char* state_on);
void pubMqttDiscovery();
void setupADC();
void MeasureADC();
void setupZsensorAHTx0();
void MeasureAHTTempHum();
void setupZsensorBH1750();
void MeasureLightIntensity();
void setupZsensorBME280();
void MeasureTempHumAndPressure();
void setupDHT();
void MeasureTempAndHum();
void setupZsensorDS1820();
void pubOneWire_HADiscovery();
void MeasureDS1820Temp();
void setupGPIOInput();
void MeasureGPIOInput();
void setupGPIOKeyCode();
void MeasureGPIOKeyCode();
void setupHCSR04();
void MeasureDistance();
void setupHCSR501();
void MeasureHCSR501();
void setupZsensorHTU21();
void MeasureTempHum();
void setupINA226();
void MeasureINA226();
static void writeRegister(byte reg, word value);
static word readRegister(byte reg);
void setupSHTC3();
void MeasureTempAndHum();
void displaySensorDetails(void);
void setupZsensorTSL2561();
void MeasureLightIntensityTSL2561();
#line 249 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/main.ino"
void revert_hex_data(const char* in, char* out, int l) {

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





long value_from_hex_data(const char* service_data, int offset, int data_length, bool reverse, bool canBeNegative = true) {
  char data[data_length + 1];
  memcpy(data, &service_data[offset], data_length);
  data[data_length] = '\0';
  long value;
  if (reverse) {

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

char* ip2CharArray(IPAddress ip) {
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}

bool to_bool(String const& s) {
  return s != "0";
}
# 303 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/main.ino"
void pub(const char* topicori, const char* payload, bool retainFlag) {
  String topic = String(mqtt_topic) + String(gateway_name) + String(topicori);
  pubMQTT(topic.c_str(), payload, retainFlag);
}







void pub(const char* topicori, JsonObject& data) {
  String dataAsString = "";
  serializeJson(data, dataAsString);
  Log.notice(F("Send on %s msg %s" CR), topicori, dataAsString.c_str());
  digitalWrite(LED_SEND_RECEIVE, LED_SEND_RECEIVE_ON);
  String topic = String(mqtt_topic) + String(gateway_name) + String(topicori);
#if valueAsATopic
# ifdef ZgatewayPilight
  String value = data["value"];
  String protocol = data["protocol"];
  if (value != "null" && value != 0) {
    topic = topic + "/" + protocol + "/" + value;
  }
# else
  SIGNAL_SIZE_UL_ULL value = data["value"];
  if (value != 0) {
    topic = topic + "/" + toString(value);
  }
# endif
#endif

#if jsonPublishing
  Log.trace(F("jsonPubl - ON" CR));
  pubMQTT(topic, dataAsString.c_str());
#endif

#if simplePublishing
  Log.trace(F("simplePub - ON" CR));

  for (JsonPair p : data) {
# if defined(ESP8266)
    yield();
# endif
    if (p.value().is<SIGNAL_SIZE_UL_ULL>() && strcmp(p.key().c_str(), "rssi") != 0) {
      if (strcmp(p.key().c_str(), "value") == 0) {
        pubMQTT(topic, p.value().as<SIGNAL_SIZE_UL_ULL>());
      } else {
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







void pub(const char* topicori, const char* payload) {
  String topic = String(mqtt_topic) + String(gateway_name) + String(topicori);
  pubMQTT(topic, payload);
}
# 382 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/main.ino"
void pub_custom_topic(const char* topic, JsonObject& data, boolean retain) {
  String buffer = "";
  serializeJson(data, buffer);
  pubMQTT(topic, buffer.c_str(), retain);
}







void pubMQTT(const char* topic, const char* payload) {
  pubMQTT(topic, payload, false);
}
# 405 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/main.ino"
void pubMQTT(const char* topic, const char* payload, bool retainFlag) {
  if (client.connected()) {
    Log.trace(F("[ OMG->MQTT ] topic: %s msg: %s " CR), topic, payload);
#if AWS_IOT
    client.publish(topic, payload);
#else
    client.publish(topic, payload, retainFlag);
#endif
  } else {
    Log.warning(F("Client not connected, aborting thes publication" CR));
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


  if (strncmp(topicOri, mqtt_topic, strlen(mqtt_topic)) != 0)
    return false;

  topicOri += strlen(mqtt_topic);

  if (strncmp(topicOri, gateway_name, strlen(gateway_name)) != 0)
    return false;
  topicOri += strlen(gateway_name);
  if (strncmp(topicOri, toAdd, strlen(toAdd)) != 0)
    return false;
  return true;
}

void connectMQTT() {
#ifndef ESPWifiManualSetup
# if defined(ESP8266) || defined(ESP32)
  checkButton();
# endif
#endif

  Log.warning(F("MQTT connection..." CR));
  char topic[mqtt_topic_max_size];
  strcpy(topic, mqtt_topic);
  strcat(topic, gateway_name);
  strcat(topic, will_Topic);
  client.setBufferSize(mqtt_max_packet_size);
#if AWS_IOT
  if (client.connect(gateway_name, mqtt_user, mqtt_pass)) {
#else
  if (client.connect(gateway_name, mqtt_user, mqtt_pass, topic, will_QoS, will_Retain, will_Message)) {
#endif

    displayPrint("MQTT connected");
    Log.notice(F("Connected to broker" CR));
    failure_number_mqtt = 0;

    pub(will_Topic, Gateway_AnnouncementMsg, will_Retain);

    pub(version_Topic, OMG_VERSION, will_Retain);

    char topic2[mqtt_topic_max_size];
    strcpy(topic2, mqtt_topic);
    strcat(topic2, gateway_name);
    strcat(topic2, subjectMQTTtoX);
    if (client.subscribe(topic2)) {
#ifdef ZgatewayRF
      client.subscribe(subjectMultiGTWRF);
#endif
#ifdef ZgatewayIR
      client.subscribe(subjectMultiGTWIR);
#endif
#ifdef MQTT_HTTPS_FW_UPDATE
      client.subscribe(subjectFWUpdate);
#endif
      Log.trace(F("Subscription OK to the subjects %s" CR), topic2);
    }
  } else {
    failure_number_mqtt++;
    Log.warning(F("failure_number_mqtt: %d" CR), failure_number_mqtt);
    Log.warning(F("failed, rc=%d" CR), client.state());
#if defined(ESP32)
    if (mqtt_secure)
      Log.warning(F("failed, ssl error code=%d" CR), ((WiFiClientSecure*)eClient)->lastError(nullptr, 0));
#elif defined(ESP8266)
    if (mqtt_secure)
      Log.warning(F("failed, ssl error code=%d" CR), ((WiFiClientSecure*)eClient)->getLastSSLError());
#endif
    digitalWrite(LED_ERROR, LED_ERROR_ON);
    delay(2000);
    digitalWrite(LED_ERROR, !LED_ERROR_ON);
    delay(5000);
    if (failure_number_mqtt > maxRetryWatchDog) {
      watchdogReboot(1);
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {



  Log.trace(F("Hey I got a callback %s" CR), topic);

  byte* p = (byte*)malloc(length + 1);

  memcpy(p, payload, length);

  p[length] = '\0';

  if ((strstr(topic, subjectMultiGTWKey) != NULL) ||
      (strstr(topic, subjectGTWSendKey) != NULL) ||
      (strstr(topic, subjectFWUpdate) != NULL))
    receivingMQTT(topic, (char*)p);


  free(p);
}

void setup() {

  Serial.begin(SERIAL_BAUD);
  Log.begin(LOG_LEVEL, &Serial);
  Log.notice(F(CR "************* WELCOME TO OpenMQTTGateway **************" CR));


  pinMode(LED_SEND_RECEIVE, OUTPUT);
  pinMode(LED_INFO, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);
  digitalWrite(LED_SEND_RECEIVE, !LED_SEND_RECEIVE_ON);
  digitalWrite(LED_INFO, !LED_INFO_ON);
  digitalWrite(LED_ERROR, !LED_ERROR_ON);

#if defined(ESP8266) || defined(ESP32)
# ifdef ESP8266
# ifndef ZgatewaySRFB
  Serial.end();
  Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY);
# endif
# elif ESP32
  preferences.begin(Gateway_Short_Name, false);
  lowpowermode = preferences.getUInt("lowpowermode", DEFAULT_LOW_POWER_MODE);
  preferences.end();
# if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
  setupM5();
# endif
# if defined(ZboardHELTEC)
  setupHELTEC();
  modules.add(ZboardHELTEC);
# endif
  Log.notice(F("OpenMQTTGateway Version: " OMG_VERSION CR));
# endif

# ifdef USE_MAC_AS_GATEWAY_NAME
  String s = WiFi.macAddress();
  sprintf(gateway_name, "%.2s%.2s%.2s%.2s%.2s%.2s",
          s.c_str(), s.c_str() + 3, s.c_str() + 6, s.c_str() + 9, s.c_str() + 12, s.c_str() + 15);
  snprintf(WifiManager_ssid, MAC_NAME_MAX_LEN, "%s_%s", Gateway_Short_Name, gateway_name);
  strcpy(ota_hostname, WifiManager_ssid);
  Log.notice(F("OTA Hostname: %s.local" CR), ota_hostname);
# endif

# ifdef ESP32_ETHERNET
  setup_ethernet_esp32();
# else
# if defined(ESPWifiManualSetup)
  setup_wifi();
# else
  setup_wifimanager(false);
# endif
  Log.trace(F("OpenMQTTGateway mac: %s" CR), WiFi.macAddress().c_str());
  Log.trace(F("OpenMQTTGateway ip: %s" CR), WiFi.localIP().toString().c_str());
# endif

  setOTA();
#else


  Serial.begin(SERIAL_BAUD);

  setup_ethernet();
#endif

#if defined(ESP8266) || defined(ESP32)
  if (mqtt_secure) {
    eClient = new WiFiClientSecure;
    setupTLS(MQTT_SECURE_SELF_SIGNED, mqtt_ss_index);
  } else {
    eClient = new WiFiClient;
  }
#else
  eClient = new EthernetClient;
#endif
  client.setClient(*(Client*)eClient);

#if defined(MDNS_SD) && (defined(ESP8266) || defined(ESP32))
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

#ifdef ZactuatorONOFF
  pinMode(ACTUATOR_ONOFF_GPIO, OUTPUT);
  digitalWrite(ACTUATOR_ONOFF_GPIO, ACTUATOR_ONOFF_DEFAULT);
#endif
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
#define ACTIVE_RECEIVER ACTIVE_RF
#endif
#ifdef ZgatewayRF2
  setupRF2();
  modules.add(ZgatewayRF2);
# ifdef ACTIVE_RECEIVER
#undef ACTIVE_RECEIVER
# endif
#define ACTIVE_RECEIVER ACTIVE_RF2
#endif
#ifdef ZgatewayPilight
  setupPilight();
  modules.add(ZgatewayPilight);
  disablePilightReceive();
# ifdef ACTIVE_RECEIVER
#undef ACTIVE_RECEIVER
# endif
#define ACTIVE_RECEIVER ACTIVE_PILIGHT
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
#ifdef ZgatewayRTL_433
  setupRTL_433();
  modules.add(ZgatewayRTL_433);
# ifdef ACTIVE_RECEIVER
#undef ACTIVE_RECEIVER
# endif
#define ACTIVE_RECEIVER ACTIVE_RTL
#endif
#if defined(ZgatewayRTL_433) || defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRF2)
# ifdef DEFAULT_RECEIVER
  activeReceiver = DEFAULT_RECEIVER;
# else
  activeReceiver = ACTIVE_RECEIVER;
# endif
  enableActiveReceiver(true);
#endif
  Log.trace(F("mqtt_max_packet_size: %d" CR), mqtt_max_packet_size);

#ifndef ARDUINO_AVR_UNO
  char jsonChar[100];
  serializeJson(modules, jsonChar, measureJson(modules) + 1);
  Log.notice(F("OpenMQTTGateway modules: %s" CR), jsonChar);
#endif
  Log.notice(F("************** Setup OpenMQTTGateway end **************" CR));
}

#if defined(ESP8266) || defined(ESP32)

bool wifi_reconnect_bypass() {
  uint8_t wifi_autoreconnect_cnt = 0;
# ifdef ESP32
  while (WiFi.status() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
# else
  while (WiFi.waitForConnectResult() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
# endif
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

void setOTA() {

  ArduinoOTA.setPort(ota_port);


  ArduinoOTA.setHostname(ota_hostname);


  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    Log.trace(F("Start OTA, lock other functions" CR));
    digitalWrite(LED_SEND_RECEIVE, LED_SEND_RECEIVE_ON);
    digitalWrite(LED_ERROR, LED_ERROR_ON);
# if defined(ZgatewayBT) && defined(ESP32)
    stopProcessing();
# endif
    lpDisplayPrint("OTA in progress");
  });
  ArduinoOTA.onEnd([]() {
    Log.trace(F("\nOTA done" CR));
    digitalWrite(LED_SEND_RECEIVE, !LED_SEND_RECEIVE_ON);
    digitalWrite(LED_ERROR, !LED_ERROR_ON);
# if defined(ZgatewayBT) && defined(ESP32)
    startProcessing();
# endif
    lpDisplayPrint("OTA done");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.trace(F("Progress: %u%%\r" CR), (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    digitalWrite(LED_SEND_RECEIVE, !LED_SEND_RECEIVE_ON);
    digitalWrite(LED_ERROR, !LED_ERROR_ON);
# if defined(ZgatewayBT) && defined(ESP32)
    startProcessing();
# endif
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

void setupTLS(bool self_signed, uint8_t index) {
  configTime(0, 0, NTP_SERVER);
  WiFiClientSecure* sClient = (WiFiClientSecure*)eClient;
# if MQTT_SECURE_SELF_SIGNED
  if (self_signed) {
    Log.notice(F("Using self signed cert index %u" CR), index);
# if defined(ESP32)
    sClient->setCACert(certs_array[index].server_cert);
# if MQTT_SECURE_SELF_SIGNED_CLIENT
    sClient->setCertificate(certs_array[index].client_cert);
    sClient->setPrivateKey(certs_array[index].client_key);
# endif
# elif defined(ESP8266)
    caCert.append(certs_array[index].server_cert);
    sClient->setTrustAnchors(&caCert);
    sClient->setBufferSizes(512, 512);
# if MQTT_SECURE_SELF_SIGNED_CLIENT
    if (pClCert != nullptr) {
      delete pClCert;
    }
    if (pClKey != nullptr) {
      delete pClKey;
    }
    pClCert = new X509List(certs_array[index].client_cert);
    pClKey = new PrivateKey(certs_array[index].client_key);
    sClient->setClientRSACert(pClCert, pClKey);
# endif
# endif
  } else
# endif
  {
    if (mqtt_cert.length() > 0) {
# if defined(ESP32)
      sClient->setCACert(mqtt_cert.c_str());
    } else {
      sClient->setCACert(certificate);
    }
# elif defined(ESP8266)
      caCert.append(mqtt_cert.c_str());
    } else {
      caCert.append(certificate);
    }
    sClient->setTrustAnchors(&caCert);
    sClient->setBufferSizes(512, 512);
# endif
  }
}
#endif

#if defined(ESPWifiManualSetup)
void setup_wifi() {
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(wifi_ssid, wifi_password);
  Log.trace(F("Connecting to %s" CR), wifi_ssid);
# ifdef wifi_ssid1
  wifiMulti.addAP(wifi_ssid1, wifi_password1);
  Log.trace(F("Connecting to %s" CR), wifi_ssid1);
# endif
  delay(10);



# ifdef NetworkAdvancedSetup
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  if (!WiFi.config(ip_adress, gateway_adress, subnet_adress, dns_adress)) {
    Log.error(F("Wifi STA Failed to configure" CR));
  }

# endif

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Log.trace(F("." CR));
    failure_number_ntwk++;
# if defined(ESP32) && defined(ZgatewayBT)
    if (lowpowermode) {
      if (failure_number_ntwk > maxConnectionRetryWifi) {
        lowPowerESP32();
      }
    } else {
      if (failure_number_ntwk > maxRetryWatchDog) {
        watchdogReboot(2);
      }
    }
# else
    if (failure_number_ntwk > maxRetryWatchDog) {
      watchdogReboot(2);
    }
# endif
  }
  Log.notice(F("WiFi ok with manual config credentials" CR));
}

#elif defined(ESP8266) || defined(ESP32)

WiFiManager wifiManager;


bool shouldSaveConfig = false;



void saveConfigCallback() {
  Log.trace(F("Should save config" CR));
  shouldSaveConfig = true;
}

# ifdef TRIGGER_GPIO
void checkButton() {
# if defined(INPUT_GPIO) && defined(ZsensorGPIOInput) && INPUT_GPIO == TRIGGER_GPIO
  MeasureGPIOInput();
# else

  if (digitalRead(TRIGGER_GPIO) == LOW) {

    delay(50);
    if (digitalRead(TRIGGER_GPIO) == LOW) {
      Log.trace(F("Trigger button Pressed" CR));

      delay(3000);
      if (digitalRead(TRIGGER_GPIO) == LOW) {
        Log.trace(F("Button Held" CR));
        Log.notice(F("Erasing ESP Config, restarting" CR));
        setup_wifimanager(true);
      }
    }
  }
# endif
}
# else
void checkButton() {}
# endif

void eraseAndRestart() {
  Log.trace(F("Formatting requested, result: %d" CR), SPIFFS.format());

# if defined(ESP8266)
  WiFi.disconnect(true);
  wifiManager.resetSettings();
  delay(5000);
  ESP.reset();
# else
  nvs_flash_erase();
  ESP.restart();
# endif
}

void saveMqttConfig() {
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

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Log.error(F("failed to open config file for writing" CR));
  }

  serializeJsonPretty(json, Serial);
  serializeJson(json, configFile);
  configFile.close();
}

void setup_wifimanager(bool reset_settings) {
# ifdef TRIGGER_GPIO
  pinMode(TRIGGER_GPIO, INPUT_PULLUP);
# endif
  delay(10);
  WiFi.mode(WIFI_STA);

  if (reset_settings)
    eraseAndRestart();


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

    Log.trace(F("reading config file" CR));
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      Log.trace(F("opened config file" CR));
      DynamicJsonDocument json(configFile.size() * 2);
      auto error = deserializeJson(json, configFile);
      if (error) {
        Log.error(F("deserialize config failed: %s, buffer capacity: %u" CR), error.c_str(), json.capacity());
      }
      serializeJsonPretty(json, Serial);
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
        if (json.containsKey("ota_server_cert"))
          ota_server_cert = json["ota_server_cert"].as<const char*>();
      } else {
        Log.warning(F("failed to load json config" CR));
      }
      configFile.close();
    }
  }

  wifiManager.setDebugOutput(WM_DEBUG_LEVEL);




# ifndef WIFIMNG_HIDE_MQTT_CONFIG
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, parameters_size);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, parameters_size);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, parameters_size);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt base topic", mqtt_topic, mqtt_topic_max_size);
  WiFiManagerParameter custom_mqtt_secure("secure", "mqtt secure", "1", 1, mqtt_secure ? "type=\"checkbox\" checked" : "type=\"checkbox\"");
  WiFiManagerParameter custom_mqtt_cert("cert", "mqtt broker cert", mqtt_cert.c_str(), 2048);
  WiFiManagerParameter custom_gateway_name("name", "gateway name", gateway_name, parameters_size);
# endif



  wifiManager.setConnectTimeout(WifiManager_TimeOut);

  wifiManager.setConfigPortalTimeout(WifiManager_ConfigPortalTimeOut);


  wifiManager.setSaveConfigCallback(saveConfigCallback);


# ifdef NetworkAdvancedSetup
  Log.trace(F("Adv wifi cfg" CR));
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress ip_adress(ip);
  wifiManager.setSTAStaticIPConfig(ip_adress, gateway_adress, subnet_adress);
# endif

# ifndef WIFIMNG_HIDE_MQTT_CONFIG

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_secure);
  wifiManager.addParameter(&custom_mqtt_cert);
  wifiManager.addParameter(&custom_gateway_name);
  wifiManager.addParameter(&custom_mqtt_topic);
# endif

  wifiManager.setMinimumSignalQuality(MinimumWifiSignalQuality);

  if (!wifi_reconnect_bypass())
  {
# ifdef ESP32
    if (lowpowermode < 2) {
      displayPrint("Connect your phone to WIFI AP:", WifiManager_ssid, WifiManager_password);
    } else {
# ifdef ZgatewayBT
      lowPowerESP32();
# endif
    }
# endif

    digitalWrite(LED_ERROR, LED_ERROR_ON);
    digitalWrite(LED_INFO, LED_INFO_ON);
    Log.notice(F("Connect your phone to WIFI AP: %s with PWD: %s" CR), WifiManager_ssid, WifiManager_password);



    if (!wifiManager.autoConnect(WifiManager_ssid, WifiManager_password)) {
      Log.warning(F("failed to connect and hit timeout" CR));
      delay(3000);

      watchdogReboot(3);
      delay(5000);
    }
    digitalWrite(LED_ERROR, !LED_ERROR_ON);
    digitalWrite(LED_INFO, !LED_INFO_ON);
  }

  displayPrint("Wifi connected");

  if (shouldSaveConfig) {

# ifndef WIFIMNG_HIDE_MQTT_CONFIG
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_pass, custom_mqtt_pass.getValue());
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    strcpy(gateway_name, custom_gateway_name.getValue());
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
# endif

    saveMqttConfig();
  }
}
# ifdef ESP32_ETHERNET
void setup_ethernet_esp32() {
  bool ethBeginSuccess = false;
  WiFi.onEvent(WiFiEvent);
# ifdef NetworkAdvancedSetup
  Log.trace(F("Adv eth cfg" CR));
  ETH.config(ip, gateway, subnet, Dns);
  ethBeginSuccess = ETH.begin();
# else
  Log.trace(F("Spl eth cfg" CR));
  ethBeginSuccess = ETH.begin();
# endif
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
      Log.trace(F("OpenMQTTGateway MAC: %s" CR), ETH.macAddress().c_str());
      Log.trace(F("OpenMQTTGateway IP: %s" CR), ETH.localIP().toString().c_str());
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
# endif
#else
void setup_ethernet() {
# ifdef NetworkAdvancedSetup
  Log.trace(F("Adv eth cfg" CR));
  Ethernet.begin(mac, ip, Dns, gateway, subnet);
# else
  Log.trace(F("Spl eth cfg" CR));
  Ethernet.begin(mac, ip);
# endif
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Log.error(F("Ethernet shield was not found." CR));
  } else {
    Log.trace(F("ip: %s " CR), Ethernet.localIP());
  }
}
#endif

#if defined(MDNS_SD) && (defined(ESP8266) || defined(ESP32))
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
# if defined(ESP8266) || defined(ESP32)
  checkButton();
# endif
#endif

  unsigned long now = millis();


  if (now > (timer_led_measures + (TimeLedON * 1000))) {
    timer_led_measures = millis();
    digitalWrite(LED_INFO, !LED_INFO_ON);
    digitalWrite(LED_SEND_RECEIVE, !LED_SEND_RECEIVE_ON);
  }

#if defined(ESP8266) || defined(ESP32)
# ifdef ESP32_ETHERNET
  if (esp32EthConnected) {
# else
  if (WiFi.status() == WL_CONNECTED) {
# endif
    ArduinoOTA.handle();
#else
  if ((Ethernet.hardwareStatus() != EthernetW5100 && Ethernet.linkStatus() == LinkON) || (Ethernet.hardwareStatus() == EthernetW5100)) {
#endif
    failure_number_ntwk = 0;
    if (client.connected()) {
      digitalWrite(LED_INFO, LED_INFO_ON);
      failure_number_ntwk = 0;

      client.loop();

#ifdef ZmqttDiscovery
      if (disc) {
        if (!connectedOnce || (discovery_republish_on_reconnect && !justReconnected)) {


          pubMqttDiscovery();
        }
      }
#endif
      connectedOnce = true;
      justReconnected = true;
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
      if (now > (timer_sys_measures + (TimeBetweenReadingSYS * 1000)) || !timer_sys_measures) {
        timer_sys_measures = millis();
        stateMeasures();
      }
#endif
#ifdef ZsensorBME280
      MeasureTempHumAndPressure();
#endif
#ifdef ZsensorHTU21
      MeasureTempHum();
#endif
#ifdef ZsensorAHTx0
      MeasureAHTTempHum();
#endif
#ifdef ZsensorHCSR04
      MeasureDistance();
#endif
#ifdef ZsensorBH1750
      MeasureLightIntensity();
#endif
#ifdef ZsensorTSL2561
      MeasureLightIntensityTSL2561();
#endif
#ifdef ZsensorDHT
      MeasureTempAndHum();
#endif
#ifdef ZsensorSHTC3
      MeasureTempAndHum();
#endif
#ifdef ZsensorDS1820
      MeasureDS1820Temp();
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
      MeasureADC();
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
#ifdef ZgatewayGFSunInverter
      ZgatewayGFSunInverterMQTT();
#endif
#ifdef ZgatewayPilight
      PilighttoMQTT();
#endif
#ifdef ZgatewayBT
# ifdef ZmqttDiscovery
      if (disc)
        launchBTDiscovery();
# endif
# ifndef ESP32
      if (BTtoMQTT())
        Log.trace(F("BTtoMQTT OK" CR));
# else
      emptyBTQueue();
# endif
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
#endif
    } else {

      justReconnected = false;
      connectMQTT();
    }
  } else {

    justReconnected = false;
    Log.warning(F("Network disconnected:" CR));
    digitalWrite(LED_ERROR, LED_ERROR_ON);
    delay(2000);
    digitalWrite(LED_ERROR, !LED_ERROR_ON);
    delay(2000);
#if defined(ESP8266) || defined(ESP32) && !defined(ESP32_ETHERNET)
# ifdef ESP32
    WiFi.reconnect();
# endif
    Log.warning(F("wifi" CR));
#else
    Log.warning(F("ethernet" CR));
#endif
  }

#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
  loopM5();
#endif


#if defined(ZboardHELTEC)
  loopHELTEC();
#endif
}




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

#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
void stateMeasures() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject SYSdata = jsonBuffer.to<JsonObject>();
  SYSdata["uptime"] = uptime();
  SYSdata["version"] = OMG_VERSION;
  Log.trace(F("retrieving value of system characteristics Uptime (s):%u" CR), uptime());
# if defined(ESP8266) || defined(ESP32)
  uint32_t freeMem;
  freeMem = ESP.getFreeHeap();
  SYSdata["freemem"] = freeMem;
  SYSdata["mqttport"] = mqtt_port;
  SYSdata["mqttsecure"] = mqtt_secure;
# ifdef ESP32
  SYSdata["freestack"] = uxTaskGetStackHighWaterMark(NULL);
# endif
# ifdef ESP32_ETHERNET
  SYSdata["mac"] = (char*)ETH.macAddress().c_str();
  SYSdata["ip"] = ip2CharArray(ETH.localIP());
  ETH.fullDuplex() ? SYSdata["fd"] = (bool)"true" : SYSdata["fd"] = (bool)"false";
  SYSdata["linkspeed"] = (int)ETH.linkSpeed();
# else
  long rssi = WiFi.RSSI();
  SYSdata["rssi"] = rssi;
  String SSID = WiFi.SSID();
  SYSdata["SSID"] = SSID;
  String BSSID = WiFi.BSSIDstr();
  SYSdata["BSSID"] = BSSID;
  SYSdata["ip"] = ip2CharArray(WiFi.localIP());
  String mac = WiFi.macAddress();
  SYSdata["mac"] = (char*)mac.c_str();
# endif
# endif
# ifdef ZgatewayBT
# ifdef ESP32
  SYSdata["lowpowermode"] = (int)lowpowermode;
  SYSdata["btqblck"] = btQueueBlocked;
  SYSdata["btqsum"] = btQueueLengthSum;
  SYSdata["btqsnd"] = btQueueLengthCount;
  SYSdata["btqavg"] = (btQueueLengthCount > 0 ? btQueueLengthSum / (float)btQueueLengthCount : 0);
# endif
  SYSdata["interval"] = BTConfig.BLEinterval;
  SYSdata["scanbcnct"] = BTConfig.BLEscanBeforeConnect;
  SYSdata["scnct"] = scanCount;
# endif
# ifdef ZboardM5STACK
  M5.Power.begin();
  SYSdata["m5battlevel"] = (int8_t)M5.Power.getBatteryLevel();
  SYSdata["m5ischarging"] = (bool)M5.Power.isCharging();
  SYSdata["m5ischargefull"] = (bool)M5.Power.isChargeFull();
# endif
# if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
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
# endif
# if defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRTL_433) || defined(ZgatewayRF2)
  SYSdata["actRec"] = (int)activeReceiver;
# endif
# ifdef ZradioCC1101
  SYSdata["mhz"] = (float)receiveMhz;
# endif
# if defined(ZgatewayRTL_433)
  if (activeReceiver == ACTIVE_RTL) {
    SYSdata["RTLminRssi"] = (int)getRTLMinimumRSSI();
    SYSdata["RTLRssi"] = (int)getRTLCurrentRSSI();
    SYSdata["RTLCnt"] = (int)getRTLMessageCount();
  }
# endif
  SYSdata["modules"] = modules;
  pub(subjectSYStoMQTT, SYSdata);
}
#endif

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation)



void storeSignalValue(SIGNAL_SIZE_UL_ULL MQTTvalue) {
  unsigned long now = millis();

  int o = getMin();
  Log.trace(F("Min ind: %d" CR), o);

  receivedSignal[o].value = MQTTvalue;
  receivedSignal[o].time = now;


  Log.trace(F("store code : %u / %u" CR), (unsigned long)receivedSignal[o].value, receivedSignal[o].time);
  Log.trace(F("Col: val/timestamp" CR));
  for (int i = 0; i < struct_size; i++) {
    Log.trace(F("mem code : %u / %u" CR), (unsigned long)receivedSignal[i].value, receivedSignal[i].time);
  }
}




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




bool isAduplicateSignal(SIGNAL_SIZE_UL_ULL value) {
  Log.trace(F("isAdupl?" CR));
  for (int i = 0; i < struct_size; i++) {
    if (receivedSignal[i].value == value) {
      unsigned long now = millis();
      if (now - receivedSignal[i].time < time_avoid_duplicate) {
        Log.notice(F("no pub. dupl" CR));
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
  if (strstr(topicOri, subjectMultiGTWKey) != NULL) {
    SIGNAL_SIZE_UL_ULL data = jsondata.isNull() ? STRTO_UL_ULL(datacallback, NULL, 10) : jsondata["value"];
    if (data != 0 && !isAduplicateSignal(data)) {
      storeSignalValue(data);
    }
  }
#endif

  if (!jsondata.isNull()) {

    String buffer = "";
    serializeJson(jsondata, buffer);
    Log.notice(F("[ MQTT->OMG ]: %s" CR), buffer.c_str());

#ifdef ZgatewayPilight
    MQTTtoPilight(topicOri, jsondata);
#endif
#ifdef ZgatewayRTL_433
    MQTTtoRTL_433(topicOri, jsondata);
#endif
#if jsonReceiving
# ifdef ZgatewayLORA
    MQTTtoLORA(topicOri, jsondata);
# endif
# ifdef ZgatewayRF
    MQTTtoRF(topicOri, jsondata);
# endif
# ifdef ZgatewayRF2
    MQTTtoRF2(topicOri, jsondata);
# endif
# ifdef Zgateway2G
    MQTTto2G(topicOri, jsondata);
# endif
# ifdef ZgatewaySRFB
    MQTTtoSRFB(topicOri, jsondata);
# endif
# ifdef ZgatewayIR
    MQTTtoIR(topicOri, jsondata);
# endif
# ifdef ZgatewayRFM69
    MQTTtoRFM69(topicOri, jsondata);
# endif
# ifdef ZgatewayBT
    MQTTtoBT(topicOri, jsondata);
# endif
# ifdef ZactuatorFASTLED
    MQTTtoFASTLED(topicOri, jsondata);
# endif
# ifdef ZactuatorPWM
    MQTTtoPWM(topicOri, jsondata);
# endif
# if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
    MQTTtoM5(topicOri, jsondata);
# endif
# if defined(ZboardHELTEC)
    MQTTtoHELTEC(topicOri, jsondata);
# endif
# ifdef ZactuatorONOFF
    MQTTtoONOFF(topicOri, jsondata);
# endif
# ifdef ZactuatorSomfy
    MQTTtoSomfy(topicOri, jsondata);
# endif
# ifdef ZgatewayRS232
    MQTTtoRS232(topicOri, jsondata);
# endif
# ifdef MQTT_HTTPS_FW_UPDATE
    MQTTHttpsFWUpdate(topicOri, jsondata);
# endif
#endif
    digitalWrite(LED_SEND_RECEIVE, LED_SEND_RECEIVE_ON);

    MQTTtoSYS(topicOri, jsondata);
  } else {
#if simpleReceiving
# ifdef ZgatewayLORA
    MQTTtoLORA(topicOri, datacallback);
# endif
# ifdef ZgatewayRF
    MQTTtoRF(topicOri, datacallback);
# endif
# ifdef ZgatewayRF315
    MQTTtoRF315(topicOri, datacallback);
# endif
# ifdef ZgatewayRF2
    MQTTtoRF2(topicOri, datacallback);
# endif
# ifdef Zgateway2G
    MQTTto2G(topicOri, datacallback);
# endif
# ifdef ZgatewaySRFB
    MQTTtoSRFB(topicOri, datacallback);
# endif
# ifdef ZgatewayRFM69
    MQTTtoRFM69(topicOri, datacallback);
# endif
# ifdef ZactuatorFASTLED
    MQTTtoFASTLED(topicOri, datacallback);
# endif
#endif
#ifdef ZactuatorONOFF
    MQTTtoONOFF(topicOri, datacallback);
#endif
  }
}

#ifdef MQTT_HTTPS_FW_UPDATE
# ifdef ESP32
# include "zzHTTPUpdate.h"
# elif ESP8266
# include <ESP8266httpUpdate.h>
# endif
void MQTTHttpsFWUpdate(char* topicOri, JsonObject& HttpsFwUpdateData) {
  if (strstr(topicOri, subjectFWUpdate) != NULL) {
    const char* version = HttpsFwUpdateData["version"];
    if (version && ((strlen(version) != strlen(OMG_VERSION)) || strcmp(version, OMG_VERSION) != 0)) {
      const char* url = HttpsFwUpdateData["url"];
      if (url) {
        if (!strstr((url + (strlen(url) - 5)), ".bin")) {
          Log.error(F("Invalid firmware extension" CR));
          return;
        }
      } else {
        Log.error(F("Invalid URL" CR));
        return;
      }

# if MQTT_HTTPS_FW_UPDATE_USE_PASSWORD > 0
      const char* pwd = HttpsFwUpdateData["password"];
      if (pwd) {
        if (strcmp(pwd, ota_password) != 0) {
          Log.error(F("Invalid OTA password" CR));
          return;
        }
      } else {
        Log.error(F("No password sent" CR));
        return;
      }
# endif

      Log.warning(F("Starting firmware update" CR));
      digitalWrite(LED_SEND_RECEIVE, LED_SEND_RECEIVE_ON);
      digitalWrite(LED_ERROR, LED_ERROR_ON);

# if defined(ZgatewayBT) && defined(ESP32)
      stopProcessing();
# endif

      const char* ota_cert = HttpsFwUpdateData["server_cert"];
      if (!ota_cert) {
        if (ota_server_cert.length() > 0) {
          Log.error(F("using stored cert" CR));
          ota_cert = ota_server_cert.c_str();
        } else {
          Log.error(F("using config cert" CR));
          ota_cert = OTAserver_cert;
        }
      }

      t_httpUpdate_return result = HTTP_UPDATE_FAILED;
      if (strstr(url, "http:")) {
        WiFiClient update_client;
# ifdef ESP32
        httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        result = httpUpdate.update(update_client, url);
# elif ESP8266
        ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        result = ESPhttpUpdate.update(update_client, url);
# endif

      } else {
        WiFiClientSecure update_client;
        if (mqtt_secure) {
          client.disconnect();
          update_client = *(WiFiClientSecure*)eClient;
        } else {
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
# if defined(ZgatewayBT) && defined(ESP32)
            startProcessing();
# endif
            return;
          }
        }

# ifdef ESP32
        update_client.setCACert(ota_cert);
        update_client.setTimeout(12);
        httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        httpUpdate.rebootOnUpdate(false);
        result = httpUpdate.update(update_client, url);
# elif ESP8266
        caCert.append(ota_cert);
        update_client.setTrustAnchors(&caCert);
        update_client.setTimeout(12000);
        ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        ESPhttpUpdate.rebootOnUpdate(false);
        result = ESPhttpUpdate.update(update_client, url);
# endif
      }

      switch (result) {
        case HTTP_UPDATE_FAILED:
# ifdef ESP32
          Log.error(F("HTTP_UPDATE_FAILED Error (%d): %s\n" CR), httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
# elif ESP8266
          Log.error(F("HTTP_UPDATE_FAILED Error (%d): %s\n" CR), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
# endif
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Log.notice(F("HTTP_UPDATE_NO_UPDATES" CR));
          break;

        case HTTP_UPDATE_OK:
          Log.notice(F("HTTP_UPDATE_OK" CR));
          ota_server_cert = ota_cert;
# ifndef ESPWifiManualSetup
          saveMqttConfig();
# endif
# ifdef ESP32
          ESP.restart();
# elif ESP8266
          ESP.reset();
# endif
          break;
      }

      digitalWrite(LED_SEND_RECEIVE, !LED_SEND_RECEIVE_ON);
      digitalWrite(LED_ERROR, !LED_ERROR_ON);

# if defined(ZgatewayBT) && defined(ESP32)
      startProcessing();
# endif
    }
  }
}
#endif

void MQTTtoSYS(char* topicOri, JsonObject& SYSdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoSYSset)) {
    Log.trace(F("MQTTtoSYS json" CR));
#if defined(ESP8266) || defined(ESP32)
    if (SYSdata.containsKey("cmd")) {
      const char* cmd = SYSdata["cmd"];
      Log.notice(F("Command: %s" CR), cmd);
      if (strstr(cmd, restartCmd) != NULL) {
# if defined(ESP8266)
        ESP.reset();
# else
        ESP.restart();
# endif
      } else if (strstr(cmd, eraseCmd) != NULL) {
# ifndef ESPWifiManualSetup
        setup_wifimanager(true);
# endif
      } else if (strstr(cmd, statusCmd) != NULL) {
        stateMeasures();
      }
    }

    if (SYSdata.containsKey("wifi_ssid") && SYSdata.containsKey("wifi_pass")) {
# if defined(ZgatewayBT) && defined(ESP32)
      stopProcessing();
# endif
      String prev_ssid = WiFi.SSID();
      String prev_pass = WiFi.psk();
      client.disconnect();
      WiFi.disconnect(true);

      Log.warning(F("Attempting connection to new AP" CR));
      WiFi.begin((const char*)SYSdata["wifi_ssid"], (const char*)SYSdata["wifi_pass"]);
      WiFi.waitForConnectResult();

      if (WiFi.status() != WL_CONNECTED) {
        Log.error(F("Failed to connect to new AP; falling back" CR));
        WiFi.disconnect(true);
        WiFi.begin(prev_ssid.c_str(), prev_pass.c_str());
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
# if defined(ESP8266)
          ESP.reset();
# else
          ESP.restart();
# endif
        }
      }
# if defined(ZgatewayBT) && defined(ESP32)
      startProcessing();
# endif
    }

# ifdef MQTTsetMQTT
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
# if MQTT_SECURE_SELF_SIGNED
        if (use_ss_cert) {
          cert_index = SYSdata["mqtt_cert_index"].as<uint8_t>();
          if (cert_index >= sizeof(certs_array) / sizeof(ss_certs)) {
            Log.error(F("mqtt_cert_index invalid - ignoring command" CR));
            return;
          }
        }
# endif

# if defined(ZgatewayBT) && defined(ESP32)
        stopProcessing();
# endif
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
# if defined(ZgatewayBT) && defined(ESP32)
        stopProcessing();
# endif
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
# ifndef ESPWifiManualSetup
        saveMqttConfig();
# endif
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
# if defined(ZgatewayBT) && defined(ESP32)
      startProcessing();
# endif
    }
# endif
    if (SYSdata.containsKey("mqtt_topic") || SYSdata.containsKey("gateway_name")) {
      if (SYSdata.containsKey("mqtt_topic")) {
        strncpy(mqtt_topic, SYSdata["mqtt_topic"], parameters_size);
      }
      if (SYSdata.containsKey("gateway_name")) {
        strncpy(gateway_name, SYSdata["gateway_name"], parameters_size);
      }
# ifndef ESPWifiManualSetup
      saveMqttConfig();
# endif
      client.disconnect();
    }
#endif

#ifdef ZmqttDiscovery
    if (SYSdata.containsKey("discovery")) {
      if (SYSdata["discovery"].is<bool>()) {
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

#if valueAsATopic && !defined(ZgatewayPilight)
# if defined(ESP32) || defined(ESP8266) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
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

# else

String toString(uint32_t input) {
  String result = String(input);

  return result;
}
# endif
#endif
# 2151 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/main.ino"
void watchdogReboot(byte reason) {
  Log.warning(F("Rebooting for reason code %d" CR), reason);
#if defined(ESP32)
  ESP.restart();
#elif defined(ESP8266)
  ESP.reset();
#else
#endif
}
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorFASTLED.ino"
# 23 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorFASTLED.ino"
#include "User_config.h"

#ifdef ZactuatorFASTLED

# include <FastLED.h>

enum LEDState {
  OFF,
  FIRE,
  GENERAL
};
LEDState currentLEDState;
long lastUpdate = 0;
long currentUpdate = 0;
CRGB leds[FASTLED_NUM_LEDS];
CRGB ledColorBlink[FASTLED_NUM_LEDS];
bool blinkLED[FASTLED_NUM_LEDS];
const long blinkInterval = 300;
const long fireUpdate = 10;
CRGBPalette16 gPal;

void setupFASTLED() {
  Log.notice(F("FASTLED_DATA_GPIO: %d" CR), FASTLED_DATA_GPIO);
  Log.notice(F("FASTLED_NUM_LEDS: %d" CR), FASTLED_NUM_LEDS);
  Log.trace(F("ZactuatorFASTLED setup done " CR));
  FastLED.addLeds<FASTLED_TYPE, FASTLED_DATA_GPIO>(leds, FASTLED_NUM_LEDS);
}


int animation_step(int duration, int steps) {
  int currentStep = ((currentUpdate % duration) / ((float)duration)) * steps;
  return currentStep;
}


int animation_step_count(int duration, int steps) {
  long lastAnimationNumber = lastUpdate / duration;
  long currentAnimationNumber = currentUpdate / duration;
  int lastStep = ((lastUpdate % duration) / ((float)duration)) * steps;
  int currentStep = ((currentUpdate % duration) / ((float)duration)) * steps;

  return currentStep - lastStep + (currentAnimationNumber - lastAnimationNumber) * steps;
}

void FASTLEDLoop() {
  lastUpdate = currentUpdate;
  currentUpdate = millis();

  if (currentLEDState == GENERAL) {
    for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
      int count = animation_step_count(blinkInterval, 2);
      int step = animation_step(blinkInterval, 2);

      if (count > 0) {
        if (blinkLED[i]) {
          if (step == 0) {
            leds[i] = ledColorBlink[i];
          } else {
            ledColorBlink[i] = leds[i];
            leds[i] = CRGB::Black;
          }
        }
      }
    }
  } else if (currentLEDState == FIRE) {
    int count = animation_step_count(fireUpdate, 1);
    if (count > 0) {

      Fire2012WithPalette();
    }
  }
  FastLED.show();
}

boolean FASTLEDtoMQTT() {
  return false;
}
# if jsonReceiving
void MQTTtoFASTLED(char* topicOri, JsonObject& jsonData) {
  currentLEDState = GENERAL;



  if (cmpToMainTopic(topicOri, subjectMQTTtoFASTLEDsetled)) {
    Log.trace(F("MQTTtoFASTLED JSON analysis" CR));
    int ledNr = jsonData["led"];
    Log.notice(F("Led numero: %d" CR), ledNr);
    const char* color = jsonData["hex"];
    Log.notice(F("Color hex: %s" CR), color);

    long number = (long)strtol(color, NULL, 16);
    bool blink = jsonData["blink"];
    if (ledNr <= FASTLED_NUM_LEDS) {
      Log.notice(F("Blink: %d" CR), blink);
      blinkLED[ledNr] = blink;
      leds[ledNr] = number;
    }
  }
}
# endif

# if simpleReceiving
void MQTTtoFASTLED(char* topicOri, char* datacallback) {
  Log.trace(F("MQTTtoFASTLED: " CR));
  currentLEDState = GENERAL;
  long number = 0;
  if (cmpToMainTopic(topicOri, subjectMQTTtoFASTLED)) {
    number = (long)strtol(&datacallback[1], NULL, 16);
    Log.notice(F("Number: %l" CR), number);
    for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
      leds[i] = number;
    }
    FastLED.show();
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoFASTLEDsetbrightness)) {
    number = (long)strtol(&datacallback[1], NULL, 16);
    Log.notice(F("Number: %l" CR), number);
    FastLED.setBrightness(number);
    FastLED.show();
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoFASTLEDsetanimation)) {
    String payload = datacallback;
    Log.notice(F("Datacallback: %s" CR), datacallback);
    if (strstr(datacallback, "fire") != NULL) {
      currentLEDState = FIRE;
      gPal = HeatColors_p;
    } else {
      currentLEDState = OFF;
    }
  } else {
    currentLEDState = OFF;
  }
  if (currentLEDState == OFF) {
    for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
  }
}
# endif
# 191 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorFASTLED.ino"
#define COOLING 55




#define SPARKING 120
bool gReverseDirection = false;

void Fire2012WithPalette() {

  static byte heat[FASTLED_NUM_LEDS];


  for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / FASTLED_NUM_LEDS) + 2));
  }


  for (int k = FASTLED_NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }


  if (random8() < SPARKING) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }


  for (int j = 0; j < FASTLED_NUM_LEDS; j++) {


    byte colorindex = scale8(heat[j], 240);
    CRGB color = ColorFromPalette(gPal, colorindex);
    int pixelnumber;
    if (gReverseDirection) {
      pixelnumber = (FASTLED_NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorONOFF.ino"
# 29 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorONOFF.ino"
#include "User_config.h"

#ifdef ZactuatorONOFF

# if jsonReceiving
void MQTTtoONOFF(char* topicOri, JsonObject& ONOFFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoONOFF)) {
    Log.trace(F("MQTTtoONOFF json data analysis" CR));
    int boolSWITCHTYPE = ONOFFdata["cmd"] | 99;
    int gpio = ONOFFdata["gpio"] | ACTUATOR_ONOFF_GPIO;
    if (boolSWITCHTYPE != 99) {
      Log.notice(F("MQTTtoONOFF boolSWITCHTYPE ok: %d" CR), boolSWITCHTYPE);
      Log.notice(F("GPIO number: %d" CR), gpio);
      pinMode(gpio, OUTPUT);
      digitalWrite(gpio, boolSWITCHTYPE);

      pub(subjectGTWONOFFtoMQTT, ONOFFdata);
    } else {
      if (ONOFFdata["cmd"] == "high_pulse") {
        Log.notice(F("MQTTtoONOFF high_pulse ok" CR));
        Log.notice(F("GPIO number: %d" CR), gpio);
        int pulselength = ONOFFdata["pulse_length"] | 500;
        Log.notice(F("Pulse length: %d ms" CR), pulselength);
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, HIGH);
        delay(pulselength);
        digitalWrite(gpio, LOW);
      } else if (ONOFFdata["cmd"] == "low_pulse") {
        Log.notice(F("MQTTtoONOFF low_pulse ok" CR));
        Log.notice(F("GPIO number: %d" CR), gpio);
        int pulselength = ONOFFdata["pulse_length"] | 500;
        Log.notice(F("Pulse length: %d ms" CR), pulselength);
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, LOW);
        delay(pulselength);
        digitalWrite(gpio, HIGH);
      } else {
        Log.error(F("MQTTtoONOFF failed json read" CR));
      }
    }
  }
}
# endif

# if simpleReceiving
void MQTTtoONOFF(char* topicOri, char* datacallback) {
  if ((cmpToMainTopic(topicOri, subjectMQTTtoONOFF))) {
    Log.trace(F("MQTTtoONOFF" CR));
    char* endptr = NULL;
    long gpio = strtol(datacallback, &endptr, 10);
    if (datacallback == endptr)
      gpio = ACTUATOR_ONOFF_GPIO;

    Log.notice(F("GPIO number: %d" CR), gpio);
    pinMode(gpio, OUTPUT);

    bool ON = false;
    if (strstr(topicOri, ONKey) != NULL)
      ON = true;
    if (strstr(topicOri, OFFKey) != NULL)
      ON = false;

    digitalWrite(gpio, ON);

    char b = ON;
    pub(subjectGTWONOFFtoMQTT, &b);
  }
}
# endif

void ActuatorButtonTrigger() {
  uint8_t level = !digitalRead(ACTUATOR_ONOFF_GPIO);
  char* level_string = "ON";
  if (level != ACTUATOR_ON) {
    level_string = "OFF";
  }
  Log.trace(F("Actuator triggered %s by button" CR), level_string);
  digitalWrite(ACTUATOR_ONOFF_GPIO, level);
  pub(subjectGTWONOFFtoMQTT, level_string);
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorPWM.ino"
# 68 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorPWM.ino"
#include "User_config.h"

#ifdef ZactuatorPWM

# include "config_PWM.h"

static long previousUpdateTime = 0;
static long currentUpdateTime = 0;

static const char* channelJsonKeys[] = PWM_CHANNEL_NAMES;
static const int channelPins[] = PWM_CHANNEL_PINS;
static const int kNumChannels = sizeof(channelPins) / sizeof(int);


static float currentValues[kNumChannels] = {};
static float fadeStartValues[kNumChannels] = {};
static float targetValues[kNumChannels] = {};

static long fadeStartUpdateTime[kNumChannels] = {};
static long fadeEndUpdateTime[kNumChannels] = {};
static bool fadeIsComplete = false;


static float calibrationMinLinear[kNumChannels];
static float calibrationMaxLinear[kNumChannels];
static float calibrationGamma[kNumChannels];

# if defined(ESP32)


static const int kNumDutyCycleBits = 16;
# elif defined(ESP8266)


static const int kNumDutyCycleBits = 10;
# else



static const int kNumDutyCycleBits = 8;
# endif

static const float kUNormToDutyCycle = (float)((1 << kNumDutyCycleBits) - 1);

void setupPWM() {
# 126 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorPWM.ino"
  for (int i = 0; i < kNumChannels; ++i) {
# if defined(ESP32)



    ledcSetup(i, 625.0, kNumDutyCycleBits);
    ledcAttachPin(channelPins[i], i);
# endif
    calibrationMinLinear[i] = 0.f;
    calibrationMaxLinear[i] = 1.f;
    calibrationGamma[i] = PWM_DEFAULT_GAMMA;
  }

  Log.trace(F("ZactuatorPWM setup done " CR));
}



static float perceptualToLinear(float perceptual, int channelIdx) {
  return pow(perceptual, calibrationGamma[channelIdx]);
}


void PWMLoop() {
  previousUpdateTime = currentUpdateTime;
  currentUpdateTime = millis();

  if (fadeIsComplete) {
    return;
  }

  fadeIsComplete = true;
  for (int i = 0; i < kNumChannels; ++i) {

    long totalFadeDuration = fadeEndUpdateTime[i] - fadeStartUpdateTime[i];
    float fadeLerpValue = 1.f;
    if (totalFadeDuration > 0) {
      fadeLerpValue = (float)(currentUpdateTime - fadeStartUpdateTime[i]) / (float)totalFadeDuration;
    }
    if (fadeLerpValue >= 1.f) {
      currentValues[i] = targetValues[i];
    } else {
      currentValues[i] = ((targetValues[i] - fadeStartValues[i]) * fadeLerpValue) + fadeStartValues[i];
      fadeIsComplete = false;
    }
  }



  for (int i = 0; i < kNumChannels; ++i) {
    float linear = perceptualToLinear(currentValues[i], i);


    if (linear > 0.f) {

      linear = (linear * (calibrationMaxLinear[i] - calibrationMinLinear[i])) + calibrationMinLinear[i];
    }

    long dutyCycle = (long)(linear * kUNormToDutyCycle);
# if defined(ESP32)
    ledcWrite(i, dutyCycle);
# else
    analogWrite(channelPins[i], dutyCycle);
# endif

  }
}

boolean PWMtoMQTT() {
  return false;
}

# if jsonReceiving
void MQTTtoPWM(char* topicOri, JsonObject& jsonData) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoPWMset)) {
    Log.trace(F("MQTTtoPWM JSON analysis" CR));

    int modifiedChannelBits = 0;
    for (int i = 0; i < kNumChannels; ++i) {
      fadeStartValues[i] = currentValues[i];
      JsonVariant value = jsonData[channelJsonKeys[i]];
      if (!value.isNull()) {
        float targetValue = value.as<float>();
        targetValue = std::min(targetValue, 1.f);
        targetValue = std::max(targetValue, 0.f);
        targetValues[i] = targetValue;


        fadeStartUpdateTime[i] = currentUpdateTime;
        fadeEndUpdateTime[i] = currentUpdateTime;

        modifiedChannelBits |= (1 << i);
      }
    }

    JsonVariant fade = jsonData["fade"];
    if (!fade.isNull()) {

      long endUpdateTime = currentUpdateTime + (long)(fade.as<float>() * (1000.f));

      for (int i = 0; i < kNumChannels; ++i) {
        if (modifiedChannelBits & (1 << i)) {
          fadeEndUpdateTime[i] = endUpdateTime;
        }
      }
    }
    fadeIsComplete = false;
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoPWMcalibrate)) {

    for (int i = 0; i < kNumChannels; ++i) {
      char key[64];
      snprintf(key, sizeof(key), "gamma-%s", channelJsonKeys[i]);
      JsonVariant value = jsonData[key];
      if (!value.isNull()) {
        float gamma = value.as<float>();

        gamma = std::min(gamma, 4.f);
        gamma = std::max(gamma, 0.5f);
        calibrationGamma[i] = gamma;
      }
      snprintf(key, sizeof(key), "min-%s", channelJsonKeys[i]);
      value = jsonData[key];
      if (!value.isNull()) {
        calibrationMinLinear[i] = perceptualToLinear(value.as<float>(), i);
      }
      snprintf(key, sizeof(key), "max-%s", channelJsonKeys[i]);
      value = jsonData[key];
      if (!value.isNull()) {
        calibrationMaxLinear[i] = perceptualToLinear(value.as<float>(), i);
      }
    }
  }
}
# endif

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorSomfy.ino"
# 24 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZactuatorSomfy.ino"
#include "User_config.h"

#ifdef ZactuatorSomfy

# include <EEPROM.h>
# include <EEPROMRollingCodeStorage.h>
# include <SomfyRemote.h>

# ifdef ZradioCC1101
# include <ELECHOUSE_CC1101_SRC_DRV.h>
# endif

void setupSomfy() {
# ifdef ZradioCC1101
  ELECHOUSE_cc1101.Init();
# endif
  pinMode(RF_EMITTER_GPIO, OUTPUT);
  digitalWrite(RF_EMITTER_GPIO, LOW);

# if defined(ESP32)
  if (!EEPROM.begin(max(4, SOMFY_REMOTE_NUM * 2))) {
    Log.error(F("failed to initialise EEPROM" CR));
  }
# elif defined(ESP8266)
  EEPROM.begin(max(4, SOMFY_REMOTE_NUM * 2));
# endif

  Log.trace(F("ZactuatorSomfy setup done " CR));
}

# if jsonReceiving
void MQTTtoSomfy(char* topicOri, JsonObject& jsonData) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoSomfy)) {
# ifdef ZradioCC1101
    ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY_SOMFY);
# endif
    Log.trace(F("MQTTtoSomfy json data analysis" CR));

    const int remoteIndex = jsonData["remote"];
    if (remoteIndex >= SOMFY_REMOTE_NUM) {
      Log.warning(F("ZactuatorSomfy remote does not exist" CR));
      return;
    }
    const String commandData = jsonData["command"];
    const Command command = getSomfyCommand(commandData);

    const int repeat = jsonData["repeat"] | 4;

    EEPROMRollingCodeStorage rollingCodeStorage(EEPROM_ADDRESS_START + remoteIndex * 2);
    SomfyRemote somfyRemote(RF_EMITTER_GPIO, somfyRemotes[remoteIndex], &rollingCodeStorage);
    somfyRemote.sendCommand(command, repeat);
# ifdef ZradioCC1101
    ELECHOUSE_cc1101.SetRx(receiveMhz);
# endif
  }
}
# endif

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZboardHeltec.ino"
# 31 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZboardHeltec.ino"
#include "User_config.h"
#if defined(ZboardHELTEC)
# include "ArduinoLog.h"
# include "config_HELTEC.h"


void logToLCD(bool display) {
  display ? Log.begin(LOG_LEVEL_LCD, &Oled) : Log.begin(LOG_LEVEL, &Serial);
}

void setupHELTEC() {
  Log.trace(F("Setup HELTEC Display" CR));
  Oled.begin();
  Log.notice(F("Setup HELTEC Display end" CR));

# if LOG_TO_LCD
  Log.begin(LOG_LEVEL_LCD, &Oled);
# endif
}

void loopHELTEC() {
  static int previousLogLevel;
  int currentLogLevel = Log.getLastMsgLevel();
  if (previousLogLevel != currentLogLevel && lowpowermode != 2) {
    switch (currentLogLevel) {
      case 1:
      case 2:



        break;
      case 3:



        break;
      default:


        Oled.fillScreen(WHITE);
        Oled.drawLogo((int)OLED_WIDTH * 0.24, (int)(OLED_WIDTH / 2) - OLED_WIDTH * 0.2, (int)(OLED_HEIGHT / 2) + OLED_HEIGHT * 0.2, true, true, true, true, true, true);
        break;
    }
  }
  previousLogLevel = currentLogLevel;
}

void MQTTtoHELTEC(char* topicOri, JsonObject& HELTECdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoHELTECset)) {
    Log.trace(F("MQTTtoHELTEC json set" CR));

    if (HELTECdata.containsKey("log-lcd")) {
      bool displayOnLCD = HELTECdata["log-lcd"];
      Log.notice(F("Set lcd log: %T" CR), displayOnLCD);
      logToLCD(displayOnLCD);
    }
  }
}



void heltecPrint(char* line1, char* line2, char* line3) {
  Oled.println(line1);
  Oled.println(line2);
  Oled.println(line3);
  delay(2000);
}

void heltecPrint(char* line1, char* line2) {
  Oled.println(line1);
  Oled.println(line2);
  delay(2000);
}

void heltecPrint(char* line1) {
  Oled.println(line1);
  delay(2000);
}



OledSerial Oled(0);
OledSerial::OledSerial(int x) {
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, LOW);
  delay(50);
  digitalWrite(RST_OLED, HIGH);

# if defined(WIFI_Kit_32) || defined(WIFI_LoRa_32) || defined(WIFI_LoRa_32_V2)
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);
# elif defined(Wireless_Stick)
  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_64_32);
# endif
}

void OledSerial::begin() {

  display->init();
  display->flipScreenVertically();
  display->setFont(ArialMT_Plain_10);

  display->setColor(WHITE);
  display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
  display->display();
  displayIntro(OLED_WIDTH * 0.24, (OLED_WIDTH / 2) - OLED_WIDTH * 0.2, (OLED_HEIGHT / 2) + OLED_HEIGHT * 0.2);
  display->setLogBuffer(OLED_TEXT_ROWS, OLED_TEXT_BUFFER);
  delay(1000);
}



int OledSerial::available(void) {
}

int OledSerial::peek(void) {
}

int OledSerial::read(void) {
}

void OledSerial::flush(void) {
}

void OledSerial::fillScreen(OLEDDISPLAY_COLOR color) {
  display->clear();
  display->setColor(color);
  display->fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT);
}

size_t OledSerial::write(uint8_t c) {
  display->clear();
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);

  display->write((char)c);
  display->drawLogBuffer(0, 0);
  display->display();
  return 1;
}

size_t OledSerial::write(const uint8_t* buffer, size_t size) {
  display->clear();
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  while (size) {
    display->write((char)*buffer++);
    size--;
  }
  display->drawLogBuffer(0, 0);
  display->display();
  return size;
}





void OledSerial::displayIntro(int scale, int displayWidth, int displayHeight) {
  drawLogo(scale, displayWidth, displayHeight, false, true, false, false, false, false);
  drawLogo(scale, displayWidth, displayHeight, false, false, true, false, false, false);
  drawLogo(scale, displayWidth, displayHeight, false, true, true, true, false, false);
  drawLogo(scale, displayWidth, displayHeight, false, true, true, false, true, false);
  drawLogo(scale, displayWidth, displayHeight, true, true, true, true, true, false);
  drawLogo(scale, displayWidth, displayHeight, true, true, true, true, true, true);
}

void OledSerial::drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool line1, bool line2, bool name) {
  int circle1T = logoSize / 15;
  int circle2T = logoSize / 25;
  int circle3T = logoSize / 30;

  int circle3Y = circle1Y - (logoSize * 1.2);
  int circle3X = circle1X - (logoSize * 0.13);
  int circle2X = circle1X - (logoSize * 1.05);
  int circle2Y = circle1Y - (logoSize * 0.8);

  if (line1) {
    display->setColor(BLACK);
    display->drawLine(circle1X - 2, circle1Y, circle2X - 2, circle2Y);
    display->drawLine(circle1X - 1, circle1Y, circle2X - 1, circle2Y);
    display->drawLine(circle1X, circle1Y, circle2X, circle2Y);
    display->drawLine(circle1X + 1, circle1Y, circle2X + 1, circle2Y);
    display->drawLine(circle1X + 2, circle1Y, circle2X + 2, circle2Y);
    display->setColor(WHITE);
    display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2);
  }
  if (line2) {
    display->setColor(BLACK);
    display->drawLine(circle1X - 2, circle1Y, circle3X - 2, circle3Y);
    display->drawLine(circle1X - 1, circle1Y, circle3X - 1, circle3Y);
    display->drawLine(circle1X, circle1Y, circle3X, circle3Y);
    display->drawLine(circle1X + 1, circle1Y, circle3X + 1, circle3Y);
    display->setColor(WHITE);
    display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2);
  }
  if (circle1) {
    display->setColor(WHITE);
    display->fillCircle(circle1X, circle1Y, logoSize / 2);
    display->setColor(BLACK);
    display->fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T);
    display->setColor(WHITE);
    display->fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T * 2);
  }
  if (circle2) {
    display->setColor(WHITE);
    display->fillCircle(circle2X, circle2Y, logoSize / 3);
    display->setColor(BLACK);
    display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T);
    display->setColor(WHITE);
    display->fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2);
  }
  if (circle3) {
    display->setColor(WHITE);
    display->fillCircle(circle3X, circle3Y, logoSize / 4);
    display->setColor(BLACK);
    display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T);
    display->setColor(WHITE);
    display->fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2);
  }
  if (name) {
    display->setColor(BLACK);
    display->drawString(circle1X + (circle1X * 0.27), circle1Y, "penMQTTGateway");
  }
  display->display();
  delay(50);
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZboardM5.ino"
# 29 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZboardM5.ino"
#include "User_config.h"

#if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5STACK) || defined(ZboardM5TOUGH)
# ifdef ZboardM5STICKC
# include <M5StickC.h>
# endif
# ifdef ZboardM5STICKCP
# include <M5StickCPlus.h>
# endif
# ifdef ZboardM5STACK
# include <M5Stack.h>
# endif
# ifdef ZboardM5TOUGH
# include <M5Tough.h>
# endif
void logToLCD(bool display) {
  display ? Log.begin(LOG_LEVEL_LCD, &M5.Lcd) : Log.begin(LOG_LEVEL, &Serial);
}

void setBrightness(int brightness) {
# if defined(ZboardM5STACK)
  M5.Lcd.setBrightness(brightness * 2);
# endif
# if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  (!brightness) ? M5.Axp.ScreenBreath(0) : M5.Axp.ScreenBreath(7 + (int)brightness * 0.08);
# endif
}

void setupM5() {
  Log.notice(F("Setup M5" CR));
  pinMode(SLEEP_BUTTON, INPUT);



  Log.notice(F("Low power set to: %d" CR), lowpowermode);
  switch (lowpowermode)
  {
    case 0:
      wakeScreen(NORMAL_LCD_BRIGHTNESS);
      M5.Lcd.fillScreen(WHITE);
      displayIntro(M5.Lcd.width() * 0.25, (M5.Lcd.width() / 2) + M5.Lcd.width() * 0.12, (M5.Lcd.height() / 2) + M5.Lcd.height() * 0.2);
# if LOG_TO_LCD
      Log.begin(LOG_LEVEL_LCD, &M5.Lcd);
# endif
      break;
    case 1:
      wakeScreen(SLEEP_LCD_BRIGHTNESS);
      M5.Lcd.fillScreen(WHITE);
# if LOG_TO_LCD
      Log.begin(LOG_LEVEL_LCD, &M5.Lcd);
# endif
      break;
    case 2:
      M5.begin(false, false, false);
      break;
  }
  Log.notice(F("Setup M5 end" CR));
}

void sleepScreen() {
  Log.trace(F("Screen going to sleep" CR));
# if defined(ZboardM5STACK)
  M5.begin(false, false, false);
# endif
# if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  M5.Axp.ScreenBreath(0);
  M5.Axp.SetLDO2(false);
# endif
}

void wakeScreen(int brightness) {
  Log.trace(F("Screen wake up" CR));
  M5.begin();
  M5.Lcd.setCursor(0, 0, (M5.Lcd.height() > 200) ? 4 : 2);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setRotation(1);
  setBrightness(brightness);
}

void loopM5() {
  static int previousBtnState;
  int currentBtnState = digitalRead(SLEEP_BUTTON);
  if (currentBtnState != previousBtnState && currentBtnState == 0) {
    int newlowpowermode = lowpowermode;
    (lowpowermode == 2) ? newlowpowermode = 0 : newlowpowermode = newlowpowermode + 1;
    changelowpowermode(newlowpowermode);
  }
  previousBtnState = currentBtnState;
  static int previousLogLevel;
  int currentLogLevel = Log.getLastMsgLevel();
  if (previousLogLevel != currentLogLevel && lowpowermode != 2) {
    switch (currentLogLevel) {
      case 1:
      case 2:
        wakeScreen(NORMAL_LCD_BRIGHTNESS);
        M5.Lcd.fillScreen(TFT_RED);
        M5.Lcd.setTextColor(TFT_BLACK, TFT_RED);
        break;
      case 3:
        wakeScreen(NORMAL_LCD_BRIGHTNESS);
        M5.Lcd.fillScreen(TFT_ORANGE);
        M5.Lcd.setTextColor(TFT_BLACK, TFT_ORANGE);
        break;
      default:
        wakeScreen(SLEEP_LCD_BRIGHTNESS);
        M5.Lcd.fillScreen(TFT_WHITE);
        drawLogo(M5.Lcd.width() * 0.1875, (M5.Lcd.width() / 2) - M5.Lcd.width() * 0.24, M5.Lcd.height() * 0.5, true, true, true, true, true, true);
        break;
    }
  }
  previousLogLevel = currentLogLevel;
}

void MQTTtoM5(char* topicOri, JsonObject& M5data) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoM5set)) {
    Log.trace(F("MQTTtoM5 json set" CR));

    if (M5data.containsKey("log-lcd")) {
      bool displayOnLCD = M5data["log-lcd"];
      Log.notice(F("Set lcd log: %T" CR), displayOnLCD);
      logToLCD(displayOnLCD);
    }
  }
}

void displayIntro(int i, int X, int Y) {
  Log.trace(F("Intro display on screen" CR));
  drawLogo(i, X, Y, false, true, false, false, false, false);
  delay(50);
  drawLogo(i, X, Y, false, false, true, false, false, false);
  delay(50);
  drawLogo(i, X, Y, false, true, false, false, false, false);
  delay(50);
  drawLogo(i, X, Y, false, true, true, false, false, false);
  delay(50);
  drawLogo(i, X, Y, false, true, true, true, false, false);
  delay(50);
  drawLogo(i, X, Y, false, true, true, false, true, false);
  delay(50);
  drawLogo(i, X, Y, false, true, true, true, true, false);
  delay(50);
  drawLogo(i, X, Y, true, true, true, true, true, false);
}

void drawLogo(int logoSize, int circle1X, int circle1Y, bool circle1, bool circle2, bool circle3, bool line1, bool line2, bool name) {
  int circle1T = logoSize / 15;
  int circle2T = logoSize / 25;
  int circle3T = logoSize / 30;

  int circle3Y = circle1Y - (logoSize * 1.2);
  int circle3X = circle1X - (logoSize * 0.13);
  int circle2X = circle1X - (logoSize * 1.05);
  int circle2Y = circle1Y - (logoSize * 0.8);

  if (line1) {
    M5.Lcd.drawLine(circle1X - 2, circle1Y, circle2X - 2, circle2Y, BLUE);
    M5.Lcd.drawLine(circle1X - 1, circle1Y, circle2X - 1, circle2Y, BLUE);
    M5.Lcd.drawLine(circle1X, circle1Y, circle2X, circle2Y, BLUE);
    M5.Lcd.drawLine(circle1X + 1, circle1Y, circle2X + 1, circle2Y, BLUE);
    M5.Lcd.drawLine(circle1X + 2, circle1Y, circle2X + 2, circle2Y, BLUE);
    M5.Lcd.fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2, WHITE);
  }
  if (line2) {
    M5.Lcd.drawLine(circle1X - 2, circle1Y, circle3X - 2, circle3Y, BLUE);
    M5.Lcd.drawLine(circle1X - 1, circle1Y, circle3X - 1, circle3Y, BLUE);
    M5.Lcd.drawLine(circle1X, circle1Y, circle3X, circle3Y, BLUE);
    M5.Lcd.drawLine(circle1X + 1, circle1Y, circle3X + 1, circle3Y, BLUE);
    M5.Lcd.fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2, WHITE);
  }
  M5.Lcd.fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T * 2, WHITE);
  if (circle1) {
    M5.Lcd.fillCircle(circle1X, circle1Y, logoSize / 2, WHITE);
    M5.Lcd.fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T, TFT_GREEN);
    M5.Lcd.fillCircle(circle1X, circle1Y, logoSize / 2 - circle1T * 2, WHITE);
  }
  if (circle2) {
    M5.Lcd.fillCircle(circle2X, circle2Y, logoSize / 3, WHITE);
    M5.Lcd.fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T, TFT_ORANGE);
    M5.Lcd.fillCircle(circle2X, circle2Y, logoSize / 3 - circle2T * 2, WHITE);
  }
  if (circle3) {
    M5.Lcd.fillCircle(circle3X, circle3Y, logoSize / 4, WHITE);
    M5.Lcd.fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T, TFT_PINK);
    M5.Lcd.fillCircle(circle3X, circle3Y, logoSize / 4 - circle3T * 2, WHITE);
  }
  if (name) {
    M5.Lcd.setTextColor(BLUE);
    M5.Lcd.drawString("penMQTTGateway", circle1X + (circle1X * 0.27), circle1Y, (M5.Lcd.height() > 200) ? 4 : 2);
  }
}

void M5Print(char* line1, char* line2, char* line3) {
  if (lowpowermode == 2) digitalWrite(LED_INFO, LED_INFO_ON);
  wakeScreen(NORMAL_LCD_BRIGHTNESS);
  M5.Lcd.fillScreen(TFT_WHITE);
  drawLogo(M5.Lcd.width() * 0.1875, (M5.Lcd.width() / 2) - M5.Lcd.width() * 0.24, M5.Lcd.height() * 0.5, true, true, true, true, true, true);
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.drawString(line1, 5, M5.Lcd.height() * 0.7, 1);
  M5.Lcd.drawString(line2, 5, M5.Lcd.height() * 0.8, 1);
  M5.Lcd.drawString(line3, 5, M5.Lcd.height() * 0.9, 1);
  delay(2000);
  digitalWrite(LED_INFO, !LED_INFO_ON);
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/Zgateway2G.ino"
# 28 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/Zgateway2G.ino"
#include "User_config.h"

#ifdef Zgateway2G

# include <A6lib.h>
# include <ArduinoJson.h>


A6lib A6l(_2G_TX_GPIO, _2G_RX_GPIO);

int unreadSMSLocs[50] = {0};
int unreadSMSNum = 0;
SMSmessage sms;

void setup2G() {
  Log.notice(F("_2G_TX_GPIO: %d " CR), _2G_TX_GPIO);
  Log.notice(F("_2G_RX_GPIO: %d " CR), _2G_RX_GPIO);
  setupGSM(false);
  Log.trace(F("Zgateway2G setup done " CR));
}

void setupGSM(bool deleteSMS) {
  Log.trace(F("Init 2G module: %d" CR), _2G_PWR_GPIO);
  delay(1000);

  A6l.powerCycle(_2G_PWR_GPIO);
  Log.notice(F("waiting for network connection at bd: %d" CR), _2G_MODULE_BAUDRATE);
  A6l.blockUntilReady(_2G_MODULE_BAUDRATE);
  Log.notice(F("A6/A7 gsm ready" CR));
  signalStrengthAnalysis();
  delay(1000);

  if (deleteSMS) {
    if (A6l.deleteSMS(1, 4) == A6_OK) {
      Log.notice(F("delete SMS OK" CR));
    } else {
      Log.error(F("delete SMS KO" CR));
    }
  }
}

void signalStrengthAnalysis() {
  int signalStrength = 0;
  signalStrength = A6l.getSignalStrength();
  Log.trace(F("Signal strength: %d" CR), signalStrength);
  if (signalStrength < _2G_MIN_SIGNAL || signalStrength > _2G_MAX_SIGNAL) {
    Log.trace(F("Signal too low restart the module" CR));
    setupGSM(false);
  }
}

bool _2GtoMQTT() {

  unreadSMSNum = A6l.getUnreadSMSLocs(unreadSMSLocs, 512);
  Log.trace(F("Creating SMS  buffer" CR));
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject SMSdata = jsonBuffer.to<JsonObject>();
  for (int i = 0; i < unreadSMSNum; i++) {
    Log.notice(F("New  message at index: %d" CR), unreadSMSNum);
    sms = A6l.readSMS(unreadSMSLocs[i]);
    SMSdata["message"] = (const char*)sms.message.c_str();
    SMSdata["date"] = (const char*)sms.date.c_str();
    SMSdata["phone"] = (const char*)sms.number.c_str();
    A6l.deleteSMS(unreadSMSLocs[i]);
    Log.trace(F("Adv data 2GtoMQTT" CR));
    pub(subject2GtoMQTT, SMSdata);
    return true;
  }
  return false;
}
# if simpleReceiving
void MQTTto2G(char* topicOri, char* datacallback) {
  String data = datacallback;
  String topic = topicOri;

  if (cmpToMainTopic(topicOri, subjectMQTTto2G)) {
    Log.trace(F("MQTTto2G data analysis" CR));

    String phone_number = "";
    int pos0 = topic.lastIndexOf(_2GPhoneKey);
    if (pos0 != -1) {
      pos0 = pos0 + strlen(_2GPhoneKey);
      phone_number = topic.substring(pos0);
      Log.notice(F("MQTTto2G phone: %s" CR), (char*)phone_number.c_str());
      Log.notice(F("MQTTto2G sms: %s" CR), (char*)data.c_str());
      if (A6l.sendSMS(phone_number, data) == A6_OK) {
        Log.notice(F("SMS OK" CR));

        pub(subjectGTW2GtoMQTT, "SMS OK");
      } else {
        Log.error(F("SMS KO" CR));

        pub(subjectGTW2GtoMQTT, "SMS KO");
      }
    } else {
      Log.error(F("MQTTto2G Fail reading phone number" CR));
    }
  }
}
# endif

# if jsonReceiving
void MQTTto2G(char* topicOri, JsonObject& SMSdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTto2G)) {
    const char* sms = SMSdata["message"];
    const char* phone = SMSdata["phone"];
    Log.trace(F("MQTTto2G json data analysis" CR));
    if (sms && phone) {
      Log.notice(F("MQTTto2G phone: %s" CR), phone);
      Log.notice(F("MQTTto2G sms: %s" CR), sms);
      if (A6l.sendSMS(String(phone), String(sms)) == A6_OK) {
        Log.notice(F("SMS OK" CR));

        pub(subjectGTW2GtoMQTT, "SMS OK");
      } else {
        Log.error(F("SMS KO" CR));
        pub(subjectGTW2GtoMQTT, "SMS KO");
      }
    } else {
      Log.error(F("MQTTto2G failed json read" CR));
    }
  }
}
# endif
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayBLEConnect.ino"
#ifdef ESP32
# include "User_config.h"
# ifdef ZgatewayBT
# include "ArduinoJson.h"
# include "ArduinoLog.h"
# include "ZgatewayBLEConnect.h"

#define convertTemp_CtoF(c) ((c * 1.8) + 32)

extern bool ProcessLock;
extern std::vector<BLEdevice*> devices;

NimBLERemoteCharacteristic* zBLEConnect::getCharacteristic(const NimBLEUUID& service,
                                                           const NimBLEUUID& characteristic) {
  BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
  if (!m_pClient) {
    Log.error(F("No BLE client" CR));
  } else if (!m_pClient->isConnected() && !m_pClient->connect()) {
    Log.error(F("Connect to: %s failed" CR), m_pClient->getPeerAddress().toString().c_str());
  } else {
    BLERemoteService* pRemoteService = m_pClient->getService(service);
    if (!pRemoteService) {
      Log.notice(F("Failed to find service UUID: %s" CR), service.toString().c_str());
    } else {
      Log.trace(F("Found service: %s" CR), service.toString().c_str());
      Log.trace(F("Client isConnected, freeHeap: %d" CR), ESP.getFreeHeap());
      pRemoteCharacteristic = pRemoteService->getCharacteristic(characteristic);
      if (!pRemoteCharacteristic) {
        Log.notice(F("Failed to find characteristic UUID: %s" CR), characteristic.toString().c_str());
      }
    }
  }

  return pRemoteCharacteristic;
}

bool zBLEConnect::writeData(BLEAction* action) {
  NimBLERemoteCharacteristic* pChar = getCharacteristic(action->service, action->characteristic);
  if (pChar && (pChar->canWrite() || pChar->canWriteNoResponse())) {
    switch (action->value_type) {
      case BLE_VAL_HEX: {
        int len = action->value.length();
        if (len % 2) {
          Log.error(F("Invalid HEX value length" CR));
          return false;
        }

        std::vector<uint8_t> buf;
        for (auto i = 0; i < len; i += 2) {
          std::string temp = action->value.substr(i, 2);
          buf.push_back((uint8_t)strtoul(temp.c_str(), nullptr, 16));
        }
        return pChar->writeValue((const uint8_t*)&buf[0], buf.size(), !pChar->canWriteNoResponse());
      }
      case BLE_VAL_INT:
        return pChar->writeValue(strtol(action->value.c_str(), nullptr, 0), !pChar->canWriteNoResponse());
      case BLE_VAL_FLOAT:
        return pChar->writeValue(strtod(action->value.c_str(), nullptr), !pChar->canWriteNoResponse());
      default:
        return pChar->writeValue(action->value, !pChar->canWriteNoResponse());
    }
  }
  return false;
}

bool zBLEConnect::readData(BLEAction* action) {
  NimBLERemoteCharacteristic* pChar = getCharacteristic(action->service, action->characteristic);

  if (pChar && pChar->canRead()) {
    action->value = pChar->readValue();
    if (action->value != "") {
      return true;
    }
  }
  return false;
}

bool zBLEConnect::processActions(std::vector<BLEAction>& actions) {
  bool result = false;
  if (actions.size() > 0) {
    for (auto& it : actions) {
      if (NimBLEAddress(it.addr) == m_pClient->getPeerAddress()) {
        JsonObject BLEresult = getBTJsonObject();
        BLEresult["id"] = it.addr;
        BLEresult["service"] = it.service.toString();
        BLEresult["characteristic"] = it.characteristic.toString();

        if (it.write) {
          Log.trace(F("processing BLE write" CR));
          BLEresult["write"] = it.value;
          result = writeData(&it);
        } else {
          Log.trace(F("processing BLE read" CR));
          result = readData(&it);
          if (result) {
            switch (it.value_type) {
              case BLE_VAL_HEX: {
                char* pHex = NimBLEUtils::buildHexData(nullptr, (uint8_t*)it.value.c_str(), it.value.length());
                BLEresult["read"] = pHex;
                free(pHex);
                break;
              }
              case BLE_VAL_INT: {
                int ival = *(int*)it.value.data();
                BLEresult["read"] = ival;
                break;
              }
              case BLE_VAL_FLOAT: {
                float fval = *(double*)it.value.data();
                BLEresult["read"] = fval;
                break;
              }
              default:
                BLEresult["read"] = it.value.c_str();
                break;
            }
          }
        }

        it.complete = result;
        BLEresult["success"] = result;
        if (result || it.ttl <= 1) {
          pubBT(BLEresult);
        }
      }
    }
  }

  return result;
}


void LYWSD03MMC_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return;
  }
  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());

    if (length == 5) {
      Log.trace(F("Device identified creating BLE buffer" CR));
      JsonObject BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      for (std::vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
        BLEdevice* p = *it;
        if ((strcmp(p->macAdr, (char*)mac_address.c_str()) == 0)) {
          if (p->sensorModel_id == BLEconectable::id::LYWSD03MMC)
            BLEdata["model"] = "LYWSD03MMC";
          else if (p->sensorModel_id == BLEconectable::id::MHO_C401)
            BLEdata["model"] = "MHO-C401";
        }
      }
      BLEdata["id"] = (char*)mac_address.c_str();
      Log.trace(F("Device identified in CB: %s" CR), (char*)mac_address.c_str());
      BLEdata["tempc"] = (float)((pData[0] | (pData[1] << 8)) * 0.01);
      BLEdata["tempf"] = (float)(convertTemp_CtoF((pData[0] | (pData[1] << 8)) * 0.01));
      BLEdata["hum"] = (float)(pData[2]);
      BLEdata["volt"] = (float)(((pData[4] * 256) + pData[3]) / 1000.0);
      BLEdata["batt"] = (float)(((((pData[4] * 256) + pData[3]) / 1000.0) - 2.1) * 100);

      pubBT(BLEdata);
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

void LYWSD03MMC_connect::publishData() {
  NimBLEUUID serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
  NimBLEUUID charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar && pChar->canNotify()) {
    Log.trace(F("Registering notification" CR));
    if (pChar->subscribe(true, std::bind(&LYWSD03MMC_connect::notifyCB, this,
                                         std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4))) {
      m_taskHandle = xTaskGetCurrentTaskHandle();
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
        m_taskHandle = nullptr;
      }
    } else {
      Log.notice(F("Failed registering notification" CR));
    }
  }
}


void DT24_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return;
  }

  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());
    if (length == 20) {
      m_data.assign(pData, pData + length);
      return;
    } else if (m_data.size() == 20 && length == 16) {
      m_data.insert(m_data.end(), pData, pData + length);




      Log.trace(F("Device identified creating BLE buffer" CR));
      JsonObject& BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      BLEdata["model"] = "DT24";
      BLEdata["id"] = (char*)mac_address.c_str();
      Log.trace(F("Device identified in CB: %s" CR), (char*)mac_address.c_str());
      BLEdata["volt"] = (float)(((m_data[4] * 256 * 256) + (m_data[5] * 256) + m_data[6]) / 10.0);
      BLEdata["current"] = (float)(((m_data[7] * 256 * 256) + (m_data[8] * 256) + m_data[9]) / 1000.0);
      BLEdata["power"] = (float)(((m_data[10] * 256 * 256) + (m_data[11] * 256) + m_data[12]) / 10.0);
      BLEdata["energy"] = (float)(((m_data[13] * 256 * 256 * 256) + (m_data[14] * 256 * 256) + (m_data[15] * 256) + m_data[16]) / 100.0);
      BLEdata["price"] = (float)(((m_data[17] * 256 * 256) + (m_data[18] * 256) + m_data[19]) / 100.0);
      BLEdata["tempc"] = (float)(m_data[24] * 256) + m_data[25];
      BLEdata["tempf"] = (float)(convertTemp_CtoF((m_data[24] * 256) + m_data[25]));

      pubBT(BLEdata);
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

void DT24_connect::publishData() {
  NimBLEUUID serviceUUID("ffe0");
  NimBLEUUID charUUID("ffe1");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar && pChar->canNotify()) {
    Log.trace(F("Registering notification" CR));
    if (pChar->subscribe(true, std::bind(&DT24_connect::notifyCB, this,
                                         std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4))) {
      m_taskHandle = xTaskGetCurrentTaskHandle();
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
        m_taskHandle = nullptr;
      }
    } else {
      Log.notice(F("Failed registering notification" CR));
    }
  }
}


void HHCCJCY01HHCC_connect::publishData() {
  NimBLEUUID serviceUUID("00001204-0000-1000-8000-00805f9b34fb");
  NimBLEUUID charUUID("00001a00-0000-1000-8000-00805f9b34fb");
  NimBLEUUID charUUID2("00001a02-0000-1000-8000-00805f9b34fb");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar) {
    Log.trace(F("Read mode" CR));
    uint8_t buf[2] = {0xA0, 0x1F};
    pChar->writeValue(buf, 2, true);
    int batteryValue = -1;
    NimBLERemoteCharacteristic* pChar2 = getCharacteristic(serviceUUID, charUUID2);
    if (pChar2) {
      std::string value;
      value = pChar2->readValue();
      const char* val2 = value.c_str();
      batteryValue = val2[0];
      JsonObject& BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      BLEdata["model"] = "HHCCJCY01HHCC";
      BLEdata["id"] = (char*)mac_address.c_str();
      BLEdata["batt"] = (int)batteryValue;
      pubBT(BLEdata);
    } else {
      Log.notice(F("Failed getting characteristic" CR));
    }
  } else {
    Log.notice(F("Failed getting characteristic" CR));
  }
}


void XMWSDJ04MMC_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return;
  }
  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());

    if (length == 6) {
      Log.trace(F("Device identified creating BLE buffer" CR));
      JsonObject BLEdata = getBTJsonObject();
      String mac_address = m_pClient->getPeerAddress().toString().c_str();
      mac_address.toUpperCase();
      BLEdata["model"] = "XMWSDJ04MMC";
      BLEdata["id"] = (char*)mac_address.c_str();
      Log.trace(F("Device identified in CB: %s" CR), (char*)mac_address.c_str());
      BLEdata["tempc"] = (float)((pData[0] | (pData[1] << 8)) * 0.1);
      BLEdata["tempf"] = (float)(convertTemp_CtoF((pData[0] | (pData[1] << 8)) * 0.1));
      BLEdata["hum"] = (float)((pData[2] | (pData[3] << 8)) * 0.1);
      BLEdata["volt"] = (float)((pData[4] | (pData[5] << 8)) / 1000.0);
      BLEdata["batt"] = (float)((((pData[4] | (pData[5] << 8)) / 1000.0) - 2.1) * 100);

      pubBT(BLEdata);
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

void XMWSDJ04MMC_connect::publishData() {
  NimBLEUUID serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
  NimBLEUUID charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");
  NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);

  if (pChar && pChar->canNotify()) {
    Log.trace(F("Registering notification" CR));
    if (pChar->subscribe(true, std::bind(&XMWSDJ04MMC_connect::notifyCB, this,
                                         std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4))) {
      m_taskHandle = xTaskGetCurrentTaskHandle();
      if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
        m_taskHandle = nullptr;
      }
    } else {
      Log.notice(F("Failed registering notification" CR));
    }
  }
}


void SBS1_connect::notifyCB(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  if (m_taskHandle == nullptr) {
    return;
  }
  if (!ProcessLock) {
    Log.trace(F("Callback from %s characteristic" CR), pChar->getUUID().toString().c_str());

    if (length) {
      m_notifyVal = *pData;
    } else {
      Log.notice(F("Invalid notification data" CR));
      return;
    }
  } else {
    Log.trace(F("Callback process canceled by processLock" CR));
  }

  xTaskNotifyGive(m_taskHandle);
}

bool SBS1_connect::processActions(std::vector<BLEAction>& actions) {
  NimBLEUUID serviceUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
  NimBLEUUID charUUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");
  NimBLEUUID notifyCharUUID("cba20003-224d-11e6-9fb8-0002a5d5c51b");
  static byte ON[] = {0x57, 0x01, 0x01};
  static byte OFF[] = {0x57, 0x01, 0x02};

  bool result = false;
  if (actions.size() > 0) {
    for (auto& it : actions) {
      if (NimBLEAddress(it.addr) == m_pClient->getPeerAddress()) {
        NimBLERemoteCharacteristic* pChar = getCharacteristic(serviceUUID, charUUID);
        NimBLERemoteCharacteristic* pNotifyChar = getCharacteristic(serviceUUID, notifyCharUUID);

        if (it.write && pChar && pNotifyChar) {
          Log.trace(F("processing Switchbot %s" CR), it.value.c_str());
          if (pNotifyChar->subscribe(true,
                                     std::bind(&SBS1_connect::notifyCB,
                                               this, std::placeholders::_1, std::placeholders::_2,
                                               std::placeholders::_3, std::placeholders::_4),
                                     true)) {
            if (it.value == "on") {
              result = pChar->writeValue(ON, 3, false);
            } else {
              result = pChar->writeValue(OFF, 3, false);
            }

            if (result) {
              m_taskHandle = xTaskGetCurrentTaskHandle();
              if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(BLE_CNCT_TIMEOUT)) == pdFALSE) {
                m_taskHandle = nullptr;
              }
              result = m_notifyVal == 0x01;
            }
          }
        }

        it.complete = result;
        if (result || it.ttl <= 1) {
          JsonObject BLEresult = getBTJsonObject();
          BLEresult["id"] = it.addr;
          BLEresult["state"] = result ? it.value : it.value == "on" ? "off" : "on";
          pubBT(BLEresult);
        }
      }
    }
  }

  return result;
}

# endif
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayBT.ino"
# 31 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayBT.ino"
#include "User_config.h"

#ifdef ZgatewayBT

# include "FreeRTOS.h"
SemaphoreHandle_t semaphoreCreateOrUpdateDevice;
SemaphoreHandle_t semaphoreBLEOperation;
QueueHandle_t BLEQueue;

# include <NimBLEAdvertisedDevice.h>
# include <NimBLEDevice.h>
# include <NimBLEScan.h>
# include <NimBLEUtils.h>
# include <decoder.h>
# include <driver/adc.h>
# include <esp_bt.h>
# include <esp_bt_main.h>
# include <esp_wifi.h>
# include <stdatomic.h>

# include <vector>

# include "ZgatewayBLEConnect.h"
# include "soc/timer_group_reg.h"
# include "soc/timer_group_struct.h"

using namespace std;


BTConfig_s BTConfig;

#define device_flags_init 0 << 0
#define device_flags_isDisc 1 << 0
#define device_flags_isWhiteL 1 << 1
#define device_flags_isBlackL 1 << 2
#define device_flags_connect 1 << 3

TheengsDecoder decoder;

struct decompose {
  int start;
  int len;
  bool reverse;
};

vector<BLEAction> BLEactions;

vector<BLEdevice*> devices;
int newDevices = 0;

static BLEdevice NO_DEVICE_FOUND = {{0},
                                    0,
                                    false,
                                    false,
                                    false,
                                    false,
                                    TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL};
static bool oneWhite = false;

void BTConfig_init() {
  BTConfig.bleConnect = AttemptBLEConnect;
  BTConfig.BLEinterval = TimeBtwRead;
  BTConfig.BLEscanBeforeConnect = ScanBeforeConnect;
  BTConfig.pubOnlySensors = PublishOnlySensors;
  BTConfig.presenceEnable = HassPresence;
  BTConfig.presenceTopic = subjectHomePresence;
  BTConfig.presenceUseBeaconUuid = useBeaconUuidForPresence;
  BTConfig.minRssi = MinimumRSSI;
  BTConfig.extDecoderEnable = UseExtDecoder;
  BTConfig.extDecoderTopic = MQTTDecodeTopic;
  BTConfig.filterConnectable = BLE_FILTER_CONNECTABLE;
  BTConfig.pubKnownServiceData = pubKnownBLEServiceData;
  BTConfig.pubUnknownServiceData = pubUnknownBLEServiceData;
  BTConfig.pubKnownManufData = pubBLEManufacturerData;
  BTConfig.pubUnknownManufData = pubUnknownBLEManufacturerData;
  BTConfig.pubServiceDataUUID = pubBLEServiceUUID;
  BTConfig.pubBeaconUuidForTopic = useBeaconUuidForTopic;
  BTConfig.ignoreWBlist = false;
}

template <typename T>
void BTConfig_update(JsonObject& data, const char* key, T& var);
template <typename T>
void BTConfig_update(JsonObject& data, const char* key, T& var) {
  if (data.containsKey(key)) {
    if (var != data[key].as<T>()) {
      var = data[key].as<T>();
      Log.notice(F("BT config %s changed: %s" CR), key, data[key].as<String>());
    } else {
      Log.notice(F("BT config %s unchanged: %s" CR), key, data[key].as<String>());
    }
  }
}

void BTConfig_fromJson(JsonObject& BTdata, bool startup = false) {

  BTConfig_update(BTdata, "bleconnect", BTConfig.bleConnect);

  if (BTdata.containsKey("interval") && BTdata["interval"] != 0)
    BTConfig_update(BTdata, "interval", BTConfig.BLEinterval);

  BTConfig_update(BTdata, "scanbcnct", BTConfig.BLEscanBeforeConnect);

  BTConfig_update(BTdata, "onlysensors", BTConfig.pubOnlySensors);

  BTConfig_update(BTdata, "hasspresence", BTConfig.presenceEnable);

  BTConfig_update(BTdata, "presenceTopic", BTConfig.presenceTopic);

  BTConfig_update(BTdata, "presenceUseBeaconUuid", BTConfig.presenceUseBeaconUuid);

  BTConfig_update(BTdata, "minrssi", BTConfig.minRssi);

  BTConfig_update(BTdata, "extDecoderEnable", BTConfig.extDecoderEnable);

  BTConfig_update(BTdata, "extDecoderTopic", BTConfig.extDecoderTopic);

  BTConfig_update(BTdata, "filterConnectable", BTConfig.filterConnectable);

  BTConfig_update(BTdata, "pubKnownServiceData", BTConfig.pubKnownServiceData);

  BTConfig_update(BTdata, "pubUnknownServiceData", BTConfig.pubUnknownServiceData);

  BTConfig_update(BTdata, "pubKnownManufData", BTConfig.pubKnownManufData);

  BTConfig_update(BTdata, "pubUnknownManufData", BTConfig.pubUnknownManufData);

  BTConfig_update(BTdata, "pubServiceDataUUID", BTConfig.pubServiceDataUUID);

  BTConfig_update(BTdata, "pubBeaconUuidForTopic", BTConfig.pubBeaconUuidForTopic);

  BTConfig_update(BTdata, "ignoreWBlist", (BTConfig.ignoreWBlist));

  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jo = jsonBuffer.to<JsonObject>();
  jo["bleconnect"] = BTConfig.bleConnect;
  jo["interval"] = BTConfig.BLEinterval;
  jo["scanbcnct"] = BTConfig.BLEscanBeforeConnect;
  jo["onlysensors"] = BTConfig.pubOnlySensors;
  jo["hasspresence"] = BTConfig.presenceEnable;
  jo["presenceTopic"] = BTConfig.presenceTopic;
  jo["presenceUseBeaconUuid"] = BTConfig.presenceUseBeaconUuid;
  jo["minrssi"] = -abs(BTConfig.minRssi);
  jo["extDecoderEnable"] = BTConfig.extDecoderEnable;
  jo["extDecoderTopic"] = BTConfig.extDecoderTopic;
  jo["filterConnectable"] = BTConfig.filterConnectable;
  jo["pubKnownServiceData"] = BTConfig.pubKnownServiceData;
  jo["pubUnknownServiceData"] = BTConfig.pubUnknownServiceData;
  jo["pubKnownManufData"] = BTConfig.pubKnownManufData;
  jo["pubUnknownManufData"] = BTConfig.pubUnknownManufData;
  jo["pubServiceDataUUID"] = BTConfig.pubServiceDataUUID;
  jo["pubBeaconUuidForTopic"] = BTConfig.pubBeaconUuidForTopic;
  jo["ignoreWBlist"] = BTConfig.ignoreWBlist;

  if (startup) {
    Log.notice(F("BT config: "));
    serializeJsonPretty(jsonBuffer, Serial);
    Serial.println();
    return;
  }
  pub("/commands/BTtoMQTT/config", jo);

  if (BTdata.containsKey("erase") && BTdata["erase"].as<bool>()) {

    preferences.begin(Gateway_Short_Name, false);
    preferences.remove("BTConfig");
    preferences.end();
    Log.notice(F("BT config erased" CR));
    return;
  }

  if (BTdata.containsKey("save") && BTdata["save"].as<bool>()) {

    String conf = "";
    serializeJson(jsonBuffer, conf);
    preferences.begin(Gateway_Short_Name, false);
    preferences.putString("BTConfig", conf);
    preferences.end();
    Log.notice(F("BT config saved" CR));
  }
}

void BTConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  auto error = deserializeJson(jsonBuffer, preferences.getString("BTConfig", "{}"));
  preferences.end();
  Log.notice(F("BT config loaded" CR));
  if (error) {
    Log.error(F("BT config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
    return;
  }
  if (jsonBuffer.isNull()) {
    Log.warning(F("BT config is null" CR));
    return;
  }
  JsonObject jo = jsonBuffer.as<JsonObject>();
  BTConfig_fromJson(jo, true);
  Log.notice(F("BT config loaded" CR));
}

void pubBTMainCore(JsonObject& data, bool haPresenceEnabled = true) {
  if (abs((int)data["rssi"] | 0) < abs(BTConfig.minRssi) && data.containsKey("id")) {
    String topic = data["id"].as<const char*>();
    topic.replace(":", "");
    if (BTConfig.pubBeaconUuidForTopic && !BTConfig.extDecoderEnable && data.containsKey("model_id") && data["model_id"].as<String>() == "IBEACON")
      topic = data["uuid"].as<const char*>();
    if (BTConfig.extDecoderEnable && !data.containsKey("model"))
      topic = BTConfig.extDecoderTopic;
    topic = subjectBTtoMQTT + String("/") + topic;
    pub((char*)topic.c_str(), data);
  }
  if (haPresenceEnabled && data.containsKey("distance")) {
    if (data.containsKey("servicedatauuid"))
      data.remove("servicedatauuid");
    if (data.containsKey("servicedata"))
      data.remove("servicedata");
    if (BTConfig.presenceUseBeaconUuid && data.containsKey("model_id") && data["model_id"].as<String>() == "IBEACON") {
      data["mac"] = data["id"];
      data["id"] = data["uuid"];
    }
    String topic = String(mqtt_topic) + BTConfig.presenceTopic + String(gateway_name);
    Log.trace(F("Pub HA Presence %s" CR), topic.c_str());
    pub_custom_topic((char*)topic.c_str(), data, false);
  }
}

class JsonBundle {
public:
  StaticJsonDocument<JSON_MSG_BUFFER> buffer;
  JsonObject object;
  bool haPresence;

  JsonObject& createObject(const char* json = NULL, bool haPresenceEnabled = true) {
    buffer.clear();
    haPresence = haPresenceEnabled;
    object = buffer.to<JsonObject>();

    if (json != nullptr) {
      auto error = deserializeJson(buffer, json);
      if (error) {
        Log.error(F("deserialize object failed: %s" CR), error.c_str());
      }
    }
    return object;
  }
};

void PublishDeviceData(JsonObject& BLEdata, bool processBLEData = true);

static TaskHandle_t xCoreTaskHandle;
static TaskHandle_t xProcBLETaskHandle;

atomic_int forceBTScan;

JsonBundle jsonBTBufferQueue[BTQueueSize];
atomic_int jsonBTBufferQueueNext, jsonBTBufferQueueLast;
int btQueueBlocked = 0;
int btQueueLengthSum = 0;
int btQueueLengthCount = 0;

JsonObject& getBTJsonObject(const char* json, bool haPresenceEnabled) {
  int next, last;
  for (bool blocked = false;;) {
    next = atomic_load_explicit(&jsonBTBufferQueueNext, ::memory_order_seq_cst);
    last = atomic_load_explicit(&jsonBTBufferQueueLast, ::memory_order_seq_cst);
    if ((2 * BTQueueSize + last - next) % (2 * BTQueueSize) != BTQueueSize) break;
    if (!blocked) {
      blocked = true;
      btQueueBlocked++;
    }
    delay(1);
  }
  return jsonBTBufferQueue[last % BTQueueSize].createObject(json, haPresenceEnabled);
}


void pubBT(JsonObject& data) {
  int last = atomic_load_explicit(&jsonBTBufferQueueLast, ::memory_order_seq_cst);
  atomic_store_explicit(&jsonBTBufferQueueLast, (last + 1) % (2 * BTQueueSize), ::memory_order_seq_cst);
}


void emptyBTQueue() {
  for (bool first = true;;) {
    int next = atomic_load_explicit(&jsonBTBufferQueueNext, ::memory_order_seq_cst);
    int last = atomic_load_explicit(&jsonBTBufferQueueLast, ::memory_order_seq_cst);
    if (last == next) break;
    if (first) {
      int diff = (2 * BTQueueSize + last - next) % (2 * BTQueueSize);
      btQueueLengthSum += diff;
      btQueueLengthCount++;
      first = false;
    }
    JsonBundle& bundle = jsonBTBufferQueue[next % BTQueueSize];
    pubBTMainCore(bundle.object, bundle.haPresence);
    atomic_store_explicit(&jsonBTBufferQueueNext, (next + 1) % (2 * BTQueueSize), ::memory_order_seq_cst);
    vTaskDelay(1);
  }
}

bool ProcessLock = false;

void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type = 0);

BLEdevice* getDeviceByMac(const char* mac);
BLEdevice* getDeviceByMac(const char* mac) {
  Log.trace(F("getDeviceByMac %s" CR), mac);

  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    if ((strcmp((*it)->macAdr, mac) == 0)) {
      return *it;
    }
  }
  return &NO_DEVICE_FOUND;
}

bool updateWorB(JsonObject& BTdata, bool isWhite) {
  Log.trace(F("update WorB" CR));
  const char* jsonKey = isWhite ? "white-list" : "black-list";

  int size = BTdata[jsonKey].size();
  if (size == 0)
    return false;

  for (int i = 0; i < size; i++) {
    const char* mac = BTdata[jsonKey][i];
    createOrUpdateDevice(mac, (isWhite ? device_flags_isWhiteL : device_flags_isBlackL),
                         TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL);
  }

  return true;
}

void createOrUpdateDevice(const char* mac, uint8_t flags, int model, int mac_type) {
  if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(30000)) == pdFALSE) {
    Log.error(F("Semaphore NOT taken" CR));
    return;
  }

  BLEdevice* device = getDeviceByMac(mac);
  if (device == &NO_DEVICE_FOUND) {
    Log.trace(F("add %s" CR), mac);

    device = new BLEdevice();
    strcpy(device->macAdr, mac);
    device->isDisc = flags & device_flags_isDisc;
    device->isWhtL = flags & device_flags_isWhiteL;
    device->isBlkL = flags & device_flags_isBlackL;
    device->connect = flags & device_flags_connect;
    device->macType = mac_type;
    device->sensorModel_id = model;
    devices.push_back(device);
    newDevices++;
  } else {
    Log.trace(F("update %s" CR), mac);

    device->macType = mac_type;

    if (flags & device_flags_isDisc) {
      device->isDisc = true;
    }

    if (flags & device_flags_connect) {
      device->connect = true;
    }

    if (model != TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL) {
      device->sensorModel_id = model;
    }

    if (flags & device_flags_isWhiteL || flags & device_flags_isBlackL) {
      device->isWhtL = flags & device_flags_isWhiteL;
      device->isBlkL = flags & device_flags_isBlackL;
    }
  }


  oneWhite = oneWhite || device->isWhtL;

  xSemaphoreGive(semaphoreCreateOrUpdateDevice);
}

#define isWhite(device) device->isWhtL
#define isBlack(device) device->isBlkL
#define isDiscovered(device) device->isDisc

void dumpDevices() {
  for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
    BLEdevice* p = *it;
    Log.trace(F("macAdr %s" CR), p->macAdr);
    Log.trace(F("macType %d" CR), p->macType);
    Log.trace(F("isDisc %d" CR), p->isDisc);
    Log.trace(F("isWhtL %d" CR), p->isWhtL);
    Log.trace(F("isBlkL %d" CR), p->isBlkL);
    Log.trace(F("connect %d" CR), p->connect);
    Log.trace(F("sensorModel_id %d" CR), p->sensorModel_id);
  }
}

void strupp(char* beg) {
  while ((*beg = toupper(*beg)))
    ++beg;
}

# ifdef ZmqttDiscovery
void DT24Discovery(const char* mac, const char* sensorModel_id) {
#define DT24parametersCount 6
  Log.trace(F("DT24Discovery" CR));
  const char* DT24sensor[DT24parametersCount][9] = {
      {"sensor", "volt", mac, "power", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "amp", mac, "power", jsonCurrent, "", "", "A", stateClassMeasurement},
      {"sensor", "watt", mac, "power", jsonPower, "", "", "W", stateClassMeasurement},
      {"sensor", "watt-hour", mac, "power", jsonEnergy, "", "", "kWh", stateClassMeasurement},
      {"sensor", "price", mac, "", jsonMsg, "", "", "", stateClassNone},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement}

  };

  createDiscoveryFromList(mac, DT24sensor, DT24parametersCount, "DT24", "ATorch", sensorModel_id);
}

void LYWSD03MMCDiscovery(const char* mac, const char* sensorModel) {
#define LYWSD03MMCparametersCount 4
  Log.trace(F("LYWSD03MMCDiscovery" CR));
  const char* LYWSD03MMCsensor[LYWSD03MMCparametersCount][9] = {
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "volt", mac, "", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "hum", mac, "humidity", jsonHum, "", "", "%", stateClassMeasurement}

  };

  createDiscoveryFromList(mac, LYWSD03MMCsensor, LYWSD03MMCparametersCount, "LYWSD03MMC", "Xiaomi", sensorModel);
}

void MHO_C401Discovery(const char* mac, const char* sensorModel) {
#define MHO_C401parametersCount 4
  Log.trace(F("MHO_C401Discovery" CR));
  const char* MHO_C401sensor[MHO_C401parametersCount][9] = {
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "volt", mac, "", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "hum", mac, "humidity", jsonHum, "", "", "%", stateClassMeasurement}

  };

  createDiscoveryFromList(mac, MHO_C401sensor, MHO_C401parametersCount, "MHO_C401", "Xiaomi", sensorModel);
}

void HHCCJCY01HHCCDiscovery(const char* mac, const char* sensorModel) {
#define HHCCJCY01HHCCparametersCount 5
  Log.trace(F("HHCCJCY01HHCCDiscovery" CR));
  const char* HHCCJCY01HHCCsensor[HHCCJCY01HHCCparametersCount][9] = {
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "lux", mac, "illuminance", jsonLux, "", "", "lx", stateClassMeasurement},
      {"sensor", "fer", mac, "", jsonFer, "", "", "µS/cm", stateClassMeasurement},
      {"sensor", "moi", mac, "", jsonMoi, "", "", "%", stateClassMeasurement}

  };

  createDiscoveryFromList(mac, HHCCJCY01HHCCsensor, HHCCJCY01HHCCparametersCount, "HHCCJCY01HHCC", "Xiaomi", sensorModel);
}

void XMWSDJ04MMCDiscovery(const char* mac, const char* sensorModel) {
#define XMWSDJ04MMCparametersCount 4
  Log.trace(F("XMWSDJ04MMCDiscovery" CR));
  const char* XMWSDJ04MMCsensor[XMWSDJ04MMCparametersCount][9] = {
      {"sensor", "batt", mac, "battery", jsonBatt, "", "", "%", stateClassMeasurement},
      {"sensor", "volt", mac, "", jsonVolt, "", "", "V", stateClassMeasurement},
      {"sensor", "temp", mac, "temperature", jsonTempc, "", "", "°C", stateClassMeasurement},
      {"sensor", "hum", mac, "humidity", jsonHum, "", "", "%", stateClassMeasurement}

  };

  createDiscoveryFromList(mac, XMWSDJ04MMCsensor, XMWSDJ04MMCparametersCount, "XMWSDJ04MMC", "Xiaomi", sensorModel);
}

# else
void LYWSD03MMCDiscovery(const char* mac, const char* sensorModel) {}
void MHO_C401Discovery(const char* mac, const char* sensorModel) {}
void HHCCJCY01HHCCDiscovery(const char* mac, const char* sensorModel) {}
void DT24Discovery(const char* mac, const char* sensorModel_id) {}
void XMWSDJ04MMCDiscovery(const char* mac, const char* sensorModel_id) {}
# endif
# 525 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayBT.ino"
static int taskCore = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice* advertisedDevice) {
    if (xQueueSend(BLEQueue, &advertisedDevice, 0) != pdTRUE) {
      Log.error(F("BLEQueue full" CR));
    }
  }
};

std::string convertServiceData(std::string deviceServiceData) {
  int serviceDataLength = (int)deviceServiceData.length();
  char spr[2 * serviceDataLength + 1];
  for (int i = 0; i < serviceDataLength; i++) sprintf(spr + 2 * i, "%.2x", (unsigned char)deviceServiceData[i]);
  spr[2 * serviceDataLength] = 0;
  Log.trace("Converted service data (%d) to %s" CR, serviceDataLength, spr);
  return spr;
}

void procBLETask(void* pvParameters) {
  BLEAdvertisedDevice* advertisedDevice = nullptr;

  for (;;) {
    xQueueReceive(BLEQueue, &advertisedDevice, portMAX_DELAY);

    if (!ProcessLock) {
      Log.trace(F("Creating BLE buffer" CR));
      JsonObject& BLEdata = getBTJsonObject();
      String mac_adress = advertisedDevice->getAddress().toString().c_str();
      mac_adress.toUpperCase();
      BLEdata["id"] = (char*)mac_adress.c_str();
      BLEdata["mac_type"] = advertisedDevice->getAddress().getType();
      Log.notice(F("Device detected: %s" CR), (char*)mac_adress.c_str());
      BLEdevice* device = getDeviceByMac(BLEdata["id"].as<const char*>());

      if (BTConfig.filterConnectable && device->connect) {
        Log.notice(F("Filtered connectable device" CR));
        continue;
      }

      if (BTConfig.ignoreWBlist || ((!oneWhite || isWhite(device)) && !isBlack(device))) {
        if (advertisedDevice->haveName())
          BLEdata["name"] = (char*)advertisedDevice->getName().c_str();
        if (advertisedDevice->haveManufacturerData()) {
          char* manufacturerdata = BLEUtils::buildHexData(NULL, (uint8_t*)advertisedDevice->getManufacturerData().data(), advertisedDevice->getManufacturerData().length());
          BLEdata["manufacturerdata"] = manufacturerdata;
          free(manufacturerdata);
        }
        if (advertisedDevice->haveRSSI())
          BLEdata["rssi"] = (int)advertisedDevice->getRSSI();
        if (advertisedDevice->haveTXPower())
          BLEdata["txpower"] = (int8_t)advertisedDevice->getTXPower();
        if (advertisedDevice->haveRSSI() && !BTConfig.pubOnlySensors && BTConfig.presenceEnable) {
          hass_presence(BLEdata);
        }
        if (advertisedDevice->haveServiceData()) {
          int serviceDataCount = advertisedDevice->getServiceDataCount();
          Log.trace(F("Get services data number: %d" CR), serviceDataCount);
          for (int j = 0; j < serviceDataCount; j++) {
            std::string service_data = convertServiceData(advertisedDevice->getServiceData(j));
            Log.trace(F("Service data: %s" CR), service_data.c_str());
            BLEdata["servicedata"] = (char*)service_data.c_str();
            std::string serviceDatauuid = advertisedDevice->getServiceDataUUID(j).toString();
            Log.trace(F("Service data UUID: %s" CR), (char*)serviceDatauuid.c_str());
            BLEdata["servicedatauuid"] = (char*)serviceDatauuid.c_str();
            process_bledata(BLEdata);
          }

          PublishDeviceData(BLEdata, false);
        } else {
          PublishDeviceData(BLEdata);
        }
      } else {
        Log.trace(F("Filtered MAC device" CR));
      }
    }
  }
}




void BLEscan() {

  while (uxQueueMessagesWaiting(BLEQueue)) {
    yield();
  }
  disableCore0WDT();
  Log.notice(F("Scan begin" CR));
  BLEScan* pBLEScan = BLEDevice::getScan();
  MyAdvertisedDeviceCallbacks myCallbacks;
  pBLEScan->setAdvertisedDeviceCallbacks(&myCallbacks);
  pBLEScan->setActiveScan(ActiveBLEScan);
  pBLEScan->setInterval(BLEScanInterval);
  pBLEScan->setWindow(BLEScanWindow);
  BLEScanResults foundDevices = pBLEScan->start(Scan_duration / 1000, false);
  scanCount++;
  Log.notice(F("Found %d devices, scan number %d end" CR), foundDevices.getCount(), scanCount);
  enableCore0WDT();
  Log.trace(F("Process BLE stack free: %u" CR), uxTaskGetStackHighWaterMark(xProcBLETaskHandle));
}




void BLEconnect() {
  if (!ProcessLock) {
    Log.notice(F("BLE Connect begin" CR));
    do {
      for (vector<BLEdevice*>::iterator it = devices.begin(); it != devices.end(); ++it) {
        BLEdevice* p = *it;
        if (p->connect) {
          Log.trace(F("Model to connect found: %s" CR), p->macAdr);
          NimBLEAddress addr((const char*)p->macAdr, p->macType);
          if (p->sensorModel_id == BLEconectable::id::LYWSD03MMC ||
              p->sensorModel_id == BLEconectable::id::MHO_C401) {
            LYWSD03MMC_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == BLEconectable::id::DT24_BLE) {
            DT24_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {
            HHCCJCY01HHCC_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == BLEconectable::id::XMWSDJ04MMC) {
            XMWSDJ04MMC_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
            BLEclient.publishData();
          } else if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBS1) {
            SBS1_connect BLEclient(addr);
            BLEclient.processActions(BLEactions);
          } else {
            GENERIC_connect BLEclient(addr);
            if (BLEclient.processActions(BLEactions)) {


              p->connect = false;
            }
          }
          if (BLEactions.size() > 0) {
            std::vector<BLEAction> swap;
            for (auto& it : BLEactions) {
              if (!it.complete && --it.ttl) {
                swap.push_back(it);
              } else if (memcmp(it.addr, p->macAdr, sizeof(it.addr)) == 0) {
                if (p->sensorModel_id != BLEconectable::id::DT24_BLE &&
                    p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC &&
                    p->sensorModel_id != BLEconectable::id::LYWSD03MMC &&
                    p->sensorModel_id != BLEconectable::id::MHO_C401 &&
                    p->sensorModel_id != BLEconectable::id::XMWSDJ04MMC) {

                  p->connect = false;
                }
              }
            }
            std::swap(BLEactions, swap);
          }
        }
      }
    } while (BLEactions.size() > 0);
    Log.notice(F("BLE Connect end" CR));
  }
}

void stopProcessing() {
  Log.notice(F("Stop BLE processing" CR));
  ProcessLock = true;
  delay(Scan_duration < 2000 ? Scan_duration : 2000);
}

void startProcessing() {
  Log.notice(F("Start BLE processing" CR));
  ProcessLock = false;
  vTaskResume(xCoreTaskHandle);
}

void coreTask(void* pvParameters) {
  while (true) {
    if (!ProcessLock) {
      int n = 0;
      while (client.state() != 0 && n <= InitialMQTTConnectionTimeout && !ProcessLock) {
        n++;
        delay(1000);
      }
      if (client.state() != 0) {
        Log.warning(F("MQTT client disconnected no BLE scan" CR));
      } else if (!ProcessLock) {
        if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(30000)) == pdTRUE) {
          BLEscan();

          if ((!(scanCount % BTConfig.BLEscanBeforeConnect) || scanCount == 1) && BTConfig.bleConnect)
            BLEconnect();
          dumpDevices();
          xSemaphoreGive(semaphoreBLEOperation);
        } else {
          Log.error(F("Failed to start scan - BLE busy" CR));
        }
      }
      if (lowpowermode) {
        lowPowerESP32();
        int scan = atomic_exchange_explicit(&forceBTScan, 0, ::memory_order_seq_cst);
        if (scan == 1) BTforceScan();
      } else {
        for (int interval = BTConfig.BLEinterval, waitms; interval > 0; interval -= waitms) {
          int scan = atomic_exchange_explicit(&forceBTScan, 0, ::memory_order_seq_cst);
          if (scan == 1) BTforceScan();
          delay(waitms = interval > 100 ? 100 : interval);
        }
      }
    } else {
      Log.trace(F("BLE core task canceled by processLock" CR));
      vTaskSuspend(xCoreTaskHandle);
    }
  }
}

void lowPowerESP32() {
  Log.trace(F("Going to deep sleep for: %l s" CR), (BTConfig.BLEinterval / 1000));
  deepSleep(BTConfig.BLEinterval * 1000);
}

void deepSleep(uint64_t time_in_us) {
# if defined(ZboardM5STACK) || defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  sleepScreen();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)SLEEP_BUTTON, LOW);
# endif

  Log.trace(F("Deactivating ESP32 components" CR));
  BLEDevice::deinit(true);
  esp_bt_mem_release(ESP_BT_MODE_BTDM);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  adc_power_off();
#pragma GCC diagnostic pop
  esp_wifi_stop();
  esp_deep_sleep(time_in_us);
}

void changelowpowermode(int newLowPowerMode) {
  Log.notice(F("Changing LOW POWER mode to: %d" CR), newLowPowerMode);
# if defined(ZboardM5STACK) || defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  if (lowpowermode == 2) {
# ifdef ZboardM5STACK
    M5.Lcd.wakeup();
# endif
# if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
    M5.Axp.SetLDO2(true);
    M5.Lcd.begin();
# endif
  }
  char lpm[2];
  sprintf(lpm, "%d", newLowPowerMode);
  M5Print("Changing LOW POWER mode to:", lpm, "");
# endif
  lowpowermode = newLowPowerMode;
  preferences.begin(Gateway_Short_Name, false);
  preferences.putUInt("lowpowermode", lowpowermode);
  preferences.end();
}

void setupBT() {
  BTConfig_init();
  BTConfig_load();
  Log.notice(F("BLE scans interval: %d" CR), BTConfig.BLEinterval);
  Log.notice(F("BLE scans number before connect: %d" CR), BTConfig.BLEscanBeforeConnect);
  Log.notice(F("Publishing only BLE sensors: %T" CR), BTConfig.pubOnlySensors);
  Log.notice(F("minrssi: %d" CR), -abs(BTConfig.minRssi));
  Log.notice(F("Low Power Mode: %d" CR), lowpowermode);

  atomic_init(&forceBTScan, 0);
  atomic_init(&jsonBTBufferQueueNext, 0);
  atomic_init(&jsonBTBufferQueueLast, 0);

  semaphoreCreateOrUpdateDevice = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreCreateOrUpdateDevice);

  semaphoreBLEOperation = xSemaphoreCreateBinary();
  xSemaphoreGive(semaphoreBLEOperation);

  BLEQueue = xQueueCreate(32, sizeof(NimBLEAdvertisedDevice*));

  BLEDevice::setScanDuplicateCacheSize(BLEScanDuplicateCacheSize);
  BLEDevice::init("");

  xTaskCreatePinnedToCore(
      procBLETask,
      "procBLETask",
      5120,
      NULL,
      2,
      &xProcBLETaskHandle,
      1);


  xTaskCreatePinnedToCore(
      coreTask,
      "coreTask",
      10000,
      NULL,
      1,
      &xCoreTaskHandle,
      taskCore);
  Log.trace(F("ZgatewayBT multicore ESP32 setup done " CR));
}

bool BTtoMQTT() {
  BLEscan();
  return true;
}

void RemoveJsonPropertyIf(JsonObject& obj, const char* key, bool condition) {
  if (condition) {
    Log.trace(F("Removing %s" CR), key);
    obj.remove(key);
  }
}

boolean valid_service_data(const char* data, int size) {
  for (int i = 0; i < size; ++i) {
    if (data[i] != 48)
      return true;
  }
  return false;
}

# ifdef ZmqttDiscovery

void launchBTDiscovery() {
  if (newDevices == 0)
    return;
  if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(1000)) == pdFALSE) {
    Log.error(F("Semaphore NOT taken" CR));
    return;
  }
  newDevices = 0;
  vector<BLEdevice*> localDevices = devices;
  xSemaphoreGive(semaphoreCreateOrUpdateDevice);
  for (vector<BLEdevice*>::iterator it = localDevices.begin(); it != localDevices.end(); ++it) {
    BLEdevice* p = *it;
    Log.trace(F("Device mac %s" CR), p->macAdr);

    if (!isDiscovered(p) &&
        p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::IBEACON &&
        p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::MS_CDP &&
        p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::GAEN) {
      String macWOdots = String(p->macAdr);
      macWOdots.replace(":", "");
      if (!BTConfig.extDecoderEnable &&
          p->sensorModel_id > TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL &&
          p->sensorModel_id < TheengsDecoder::BLE_ID_NUM::BLE_ID_MAX &&
          p->sensorModel_id != TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {
        Log.trace(F("Looking for Model_id: %d" CR), p->sensorModel_id);
        std::string properties = decoder.getTheengProperties(p->sensorModel_id);
        Log.trace(F("properties: %s" CR), properties.c_str());
        std::string brand = decoder.getTheengAttribute(p->sensorModel_id, "brand");
        std::string model = decoder.getTheengAttribute(p->sensorModel_id, "model");
        std::string model_id = decoder.getTheengAttribute(p->sensorModel_id, "model_id");
        if (!properties.empty()) {
          StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
          deserializeJson(jsonBuffer, properties);
          for (JsonPair prop : jsonBuffer["properties"].as<JsonObject>()) {
            Log.trace("Key: %s", prop.key().c_str());
            Log.trace("Unit: %s", prop.value()["unit"].as<const char*>());
            Log.trace("Name: %s", prop.value()["name"].as<const char*>());
            String discovery_topic = String(subjectBTtoMQTT) + "/" + macWOdots;
            String entity_name = String(model_id.c_str()) + "-" + String(prop.key().c_str());
            String unique_id = macWOdots + "-" + String(prop.key().c_str());
# if OpenHABDiscovery
            String value_template = "{{ value_json." + String(prop.key().c_str()) + "}}";
# else
            String value_template = "{{ value_json." + String(prop.key().c_str()) + " | is_defined }}";
# endif
            if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::SBS1 && !strcmp(prop.key().c_str(), "state")) {
              String payload_on = "{\"SBS1\":\"on\",\"mac\":\"" + String(p->macAdr) + "\"}";
              String payload_off = "{\"SBS1\":\"off\",\"mac\":\"" + String(p->macAdr) + "\"}";
              createDiscovery("switch",
                              discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                              will_Topic, "switch", value_template.c_str(),
                              payload_on.c_str(), payload_off.c_str(), "", 0,
                              Gateway_AnnouncementMsg, will_Message, false, subjectMQTTtoBTset,
                              model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                              stateClassNone, "off", "on");
            } else {
              createDiscovery("sensor",
                              discovery_topic.c_str(), entity_name.c_str(), unique_id.c_str(),
                              will_Topic, prop.value()["name"], value_template.c_str(),
                              "", "", prop.value()["unit"],
                              0, "", "", false, "",
                              model.c_str(), brand.c_str(), model_id.c_str(), macWOdots.c_str(), false,
                              stateClassMeasurement);
            }
          }
        }
      } else {
        if (p->sensorModel_id > BLEconectable::id::MIN &&
                p->sensorModel_id < BLEconectable::id::MAX ||
            p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {

          if (p->sensorModel_id == BLEconectable::id::DT24_BLE) {
            DT24Discovery(macWOdots.c_str(), "DT24-BLE");
          }
          if (p->sensorModel_id == BLEconectable::id::LYWSD03MMC) {
            LYWSD03MMCDiscovery(macWOdots.c_str(), "LYWSD03MMC");
          }
          if (p->sensorModel_id == BLEconectable::id::MHO_C401) {
            MHO_C401Discovery(macWOdots.c_str(), "MHO-C401");
          }
          if (p->sensorModel_id == BLEconectable::id::XMWSDJ04MMC) {
            XMWSDJ04MMCDiscovery(macWOdots.c_str(), "XMWSDJ04MMC");
          }
          if (p->sensorModel_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {
            HHCCJCY01HHCCDiscovery(macWOdots.c_str(), "HHCCJCY01HHCC");
          }
        } else {
          Log.trace(F("Device UNKNOWN_MODEL %s" CR), p->macAdr);
        }
        p->isDisc = true;
      }
    } else {
      Log.trace(F("Device already discovered or that doesn't require discovery %s" CR), p->macAdr);
    }
  }
}
# endif

void PublishDeviceData(JsonObject& BLEdata, bool processBLEData) {
  if (abs((int)BLEdata["rssi"] | 0) < abs(BTConfig.minRssi)) {
    if (processBLEData) process_bledata(BLEdata);
    if (!BTConfig.pubOnlySensors || BLEdata.containsKey("model") || BLEdata.containsKey("distance")) {
      RemoveJsonPropertyIf(BLEdata, "servicedatauuid", !BTConfig.pubServiceDataUUID && BLEdata.containsKey("model"));
      RemoveJsonPropertyIf(BLEdata, "servicedata", !BTConfig.pubKnownServiceData && BLEdata.containsKey("model"));
      RemoveJsonPropertyIf(BLEdata, "manufacturerdata", !BTConfig.pubKnownManufData && BLEdata.containsKey("model"));
      pubBT(BLEdata);
    } else {
      RemoveJsonPropertyIf(BLEdata, "servicedata", !BTConfig.pubUnknownServiceData);
      RemoveJsonPropertyIf(BLEdata, "manufacturerdata", !BTConfig.pubUnknownManufData && BLEdata.containsKey("model"));
    }
  } else if (BLEdata.containsKey("distance")) {
    pubBT(BLEdata);
  } else {
    Log.trace(F("Low rssi, device filtered" CR));
  }
}

void process_bledata(JsonObject& BLEdata) {
  const char* mac = BLEdata["id"].as<const char*>();
  int model_id = BTConfig.extDecoderEnable ? -1 : decoder.decodeBLEJson(BLEdata);
  int mac_type = BLEdata["mac_type"].as<int>();
  if (model_id >= 0) {
    Log.trace(F("Decoder found device: %s" CR), BLEdata["model_id"].as<const char*>());
    if (model_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC) {
      createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type);
    } else {
      createOrUpdateDevice(mac, device_flags_init, model_id, mac_type);
    }
  } else {
    if (BLEdata.containsKey("name")) {
      std::string name = BLEdata["name"];
      if (name.compare("LYWSD03MMC") == 0)
        model_id = BLEconectable::id::LYWSD03MMC;
      else if (name.compare("DT24-BLE") == 0)
        model_id = BLEconectable::id::DT24_BLE;
      else if (name.compare("MHO-C401") == 0)
        model_id = BLEconectable::id::MHO_C401;
      else if (name.compare("XMWSDJ04MMC") == 0)
        model_id = BLEconectable::id::XMWSDJ04MMC;

      if (model_id > 0) {
        Log.trace(F("Connectable device found: %s" CR), name.c_str());
        createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type);
      }
    } else if (BTConfig.extDecoderEnable && model_id < 0 && BLEdata.containsKey("servicedata")) {
      const char* service_data = (const char*)(BLEdata["servicedata"] | "");
      if (strstr(service_data, "209800") != NULL) {
        model_id == TheengsDecoder::BLE_ID_NUM::HHCCJCY01HHCC;
        Log.trace(F("Connectable device found: HHCCJCY01HHCC" CR));
        createOrUpdateDevice(mac, device_flags_connect, model_id, mac_type);
      }
    }
  }
  if (!BTConfig.extDecoderEnable && model_id < 0) {
    Log.trace(F("No device found " CR));
  }
}

void hass_presence(JsonObject& HomePresence) {
  int BLErssi = HomePresence["rssi"];
  Log.trace(F("BLErssi %d" CR), BLErssi);
  int txPower = HomePresence["txpower"] | 0;
  if (txPower >= 0)
    txPower = -59;
  Log.trace(F("TxPower: %d" CR), txPower);
  double ratio = BLErssi * 1.0 / txPower;
  double distance;
  if (ratio < 1.0) {
    distance = pow(ratio, 10);
  } else {
    distance = (0.89976) * pow(ratio, 7.7095) + 0.111;
  }
  HomePresence["distance"] = distance;
  Log.trace(F("Ble distance %D" CR), distance);
}

void BTforceScan() {
  if (!ProcessLock) {
    BTtoMQTT();
    Log.trace(F("Scan done" CR));
    if (BTConfig.bleConnect)
      BLEconnect();
  } else {
    Log.trace(F("Cannot launch scan due to other process running" CR));
  }
}

void immediateBTAction(void* pvParameters) {
  if (BLEactions.size()) {

    ProcessLock = true;
    NimBLEScan* pScan = NimBLEDevice::getScan();
    if (pScan->isScanning()) {
      pScan->stop();
    }

    if (xSemaphoreTake(semaphoreBLEOperation, pdMS_TO_TICKS(5000)) == pdTRUE) {

      std::vector<BLEdevice*> dev_swap;
      dev_swap.push_back(getDeviceByMac(BLEactions.back().addr));
      std::swap(devices, dev_swap);

      std::vector<BLEAction> act_swap;
      act_swap.push_back(BLEactions.back());
      BLEactions.pop_back();
      std::swap(BLEactions, act_swap);


      ProcessLock = false;
      BLEconnect();

      std::swap(devices, dev_swap);
      std::swap(BLEactions, act_swap);


      if ((!(scanCount % BTConfig.BLEscanBeforeConnect) || scanCount == 1) && BTConfig.bleConnect)
        BLEconnect();
      xSemaphoreGive(semaphoreBLEOperation);
    } else {
      Log.error(F("BLE busy - command not sent" CR));
      JsonObject result = getBTJsonObject();
      result["id"] = BLEactions.back().addr;
      result["success"] = false;
      pubBT(result);
      BLEactions.pop_back();
      ProcessLock = false;
    }
  }
  vTaskDelete(NULL);
}

void startBTActionTask() {
  TaskHandle_t th;
  xTaskCreatePinnedToCore(
      immediateBTAction,
      "imActTask",
      5120,
      NULL,
      3,
      &th,
      1);
}

void MQTTtoBTAction(JsonObject& BTdata) {
  BLEAction action;
  memset(&action, 0, sizeof(BLEAction));
  if (BTdata.containsKey("SBS1")) {
    strcpy(action.addr, (const char*)BTdata["mac"]);
    action.write = true;
    action.value = BTdata["SBS1"].as<std::string>();
    action.ttl = 1;
    createOrUpdateDevice(action.addr, device_flags_connect,
                         TheengsDecoder::BLE_ID_NUM::SBS1, 1);
    BLEactions.push_back(action);
    startBTActionTask();
    return;
  }

  action.ttl = BTdata.containsKey("ttl") ? (uint8_t)BTdata["ttl"] : 1;
  action.addr_type = BTdata.containsKey("mac_type") ? BTdata["mac_type"].as<int>() : 0;
  action.value_type = BLE_VAL_STRING;
  if (BTdata.containsKey("value_type")) {
    String vt = BTdata["value_type"];
    vt.toUpperCase();
    if (vt == "HEX")
      action.value_type = BLE_VAL_HEX;
    else if (vt == "INT")
      action.value_type = BLE_VAL_INT;
    else if (vt == "FLOAT")
      action.value_type = BLE_VAL_FLOAT;
    else if (vt != "STRING") {
      Log.error(F("BLE value type invalid %s" CR), vt.c_str());
      return;
    }
  }

  Log.trace(F("BLE ACTION TTL = %u" CR), action.ttl);
  action.complete = false;
  if (BTdata.containsKey("ble_write_address") &&
      BTdata.containsKey("ble_write_service") &&
      BTdata.containsKey("ble_write_char") &&
      BTdata.containsKey("ble_write_value")) {
    strcpy(action.addr, (const char*)BTdata["ble_write_address"]);
    action.service = NimBLEUUID((const char*)BTdata["ble_write_service"]);
    action.characteristic = NimBLEUUID((const char*)BTdata["ble_write_char"]);
    action.value = std::string((const char*)BTdata["ble_write_value"]);
    action.write = true;
    Log.trace(F("BLE ACTION Write" CR));
  } else if (BTdata.containsKey("ble_read_address") &&
             BTdata.containsKey("ble_read_service") &&
             BTdata.containsKey("ble_read_char")) {
    strcpy(action.addr, (const char*)BTdata["ble_read_address"]);
    action.service = NimBLEUUID((const char*)BTdata["ble_read_service"]);
    action.characteristic = NimBLEUUID((const char*)BTdata["ble_read_char"]);
    action.write = false;
    Log.trace(F("BLE ACTION Read" CR));
  } else {
    return;
  }

  createOrUpdateDevice(action.addr, device_flags_connect,
                       TheengsDecoder::BLE_ID_NUM::UNKNOWN_MODEL,
                       action.addr_type);

  BLEactions.push_back(action);
  if (BTdata.containsKey("immediate") && BTdata["immediate"].as<bool>()) {
    startBTActionTask();
  }
}

void MQTTtoBT(char* topicOri, JsonObject& BTdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoBTset)) {
    Log.trace(F("MQTTtoBT json set" CR));


    bool WorBupdated;
    WorBupdated = updateWorB(BTdata, true);
    WorBupdated |= updateWorB(BTdata, false);

    if (WorBupdated) {
      if (xSemaphoreTake(semaphoreCreateOrUpdateDevice, pdMS_TO_TICKS(1000)) == pdTRUE) {
        dumpDevices();
        xSemaphoreGive(semaphoreCreateOrUpdateDevice);
      }
    }


    if (BTdata.containsKey("interval") && BTdata["interval"] == 0) {
      Log.notice(F("BLE forced scan" CR));
      atomic_store_explicit(&forceBTScan, 1, ::memory_order_seq_cst);
    }







    if (BTdata.containsKey("init") && BTdata["init"].as<bool>()) {

      BTConfig_init();
    } else if (BTdata.containsKey("load") && BTdata["load"].as<bool>()) {

      BTConfig_load();
    }


    BTConfig_fromJson(BTdata);

    if (BTdata.containsKey("lowpowermode")) {
      changelowpowermode((int)BTdata["lowpowermode"]);
    }

    MQTTtoBTAction(BTdata);
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayGFSunInverter.ino"
# 26 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayGFSunInverter.ino"
#include "User_config.h"

#ifdef ZgatewayGFSunInverter

GfSun2000 GF = GfSun2000();

void GFSunInverterDataHandler(GfSun2000Data data) {
  StaticJsonDocument<2 * JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jdata = jsonBuffer.to<JsonObject>();

  jdata["device_id"] = (char*)data.deviceID;
  Log.trace(F("Device ID     : %s\n" CR), data.deviceID);
  jdata["ac_voltage"] = data.ACVoltage;
  Log.trace(F("AC Voltage    : %.1f\tV\n" CR), data.ACVoltage);
  jdata["dc_voltage"] = data.DCVoltage;
  Log.trace(F("DC Voltage    : %.1f\tV\n" CR), data.DCVoltage);
  jdata["power"] = data.averagePower;
  Log.trace(F("Output Power  : %.1f\tW (5min avg)\n" CR), data.averagePower);
  jdata["c_energy"] = data.customEnergyCounter;
  Log.trace(F("Custom Energy : %.1f\tkW/h (can be reseted)\n" CR), data.customEnergyCounter);
  jdata["t_energy"] = data.totalEnergyCounter;
  Log.trace(F("Total Energy  : %.1f\tkW/h\n" CR), data.totalEnergyCounter);

# ifdef GFSUNINVERTER_DEVEL
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer2;
  JsonObject jregister = jsonBuffer2.to<JsonObject>();
  char buffer[4];
  std::map<int16_t, int16_t>::iterator itr;
  for (itr = data.modbusRegistry.begin(); itr != data.modbusRegistry.end(); ++itr) {
    Log.notice("%d: %d\n", itr->first, itr->second);
    sprintf(buffer, "%d", itr->first);
    jregister[buffer] = itr->second;
  }
  jdata["register"] = jregister;
# endif
  pub(subjectRFtoMQTT, jdata);
}

void GFSunInverterErrorHandler(int errorId, char* errorMessage) {
  char buffer[50];
  sprintf(buffer, "Error response: %02X - %s\n", errorId, errorMessage);
  Log.error(buffer);
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jdata = jsonBuffer.to<JsonObject>();
  jdata["status"] = "error";
  jdata["msg"] = errorMessage;
  jdata["id"] = errorId;
  pub(subjectRFtoMQTT, jdata);
}

void setupGFSunInverter() {
  GF.setup(Serial2);
  GF.setDataHandler(GFSunInverterDataHandler);
  GF.setErrorHandler(GFSunInverterErrorHandler);
  Log.trace(F("ZgatewayGFSunInverter setup done " CR));
}

void ZgatewayGFSunInverterMQTT() {
  GF.readData();
  delay(GFSUNINVERTER_DELAY);
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayIR.ino"
# 28 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayIR.ino"
#include "User_config.h"

#ifdef ZgatewayIR

# if defined(ESP8266) || defined(ESP32)
# include <IRrecv.h>
# include <IRremoteESP8266.h>
# include <IRsend.h>
# include <IRutils.h>
# ifdef DumpMode
IRrecv irrecv(IR_RECEIVER_GPIO, 1024, 15U, true);
# else
IRrecv irrecv(IR_RECEIVER_GPIO);
# endif
IRsend irsend(IR_EMITTER_GPIO, IR_EMITTER_INVERTED);
# else
# include <IRremote.h>
IRrecv irrecv(IR_RECEIVER_GPIO);
IRsend irsend;
# endif


# ifndef NEC_BITS
#define NEC_BITS 32U
# endif
# ifndef SAMSUNG_BITS
#define SAMSUNG_BITS 32U
# endif
# ifndef SHARP_BITS
#define SHARP_ADDRESS_BITS 5U
#define SHARP_COMMAND_BITS 8U
#define SHARP_BITS (SHARP_ADDRESS_BITS + SHARP_COMMAND_BITS + 2)
# endif
# ifndef RC5_BITS
#define RC5_RAW_BITS 14U
#define RC5_BITS RC5_RAW_BITS - 2U
# endif
# ifndef DISH_BITS
#define DISH_BITS 16U
# endif
# ifndef SONY_20_BITS
#define SONY_20_BITS 20
# endif
# ifndef SONY_12_BITS
#define SONY_12_BITS 12U
# endif
# ifndef LG_BITS
#define LG_BITS 28U
# endif
# ifndef WHYNTER_BITS
#define WHYNTER_BITS 32U
# endif


uint64_t getUInt64fromHex(char const* str) {
  uint64_t result = 0;
  uint16_t offset = 0;

  if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) offset = 2;
  for (; isxdigit((unsigned char)str[offset]); offset++) {
    char c = str[offset];
    result *= 16;
    if (isdigit(c))
      result += c - '0';
    else if (isupper(c))
      result += c - 'A' + 10;
    else
      result += c - 'a' + 10;
  }
  return result;
}

void setupIR() {

# if defined(ESP8266) || defined(ESP32)
  irsend.begin();
# endif

  irrecv.enableIRIn();

  Log.notice(F("IR_EMITTER_GPIO: %d " CR), IR_EMITTER_GPIO);
  Log.notice(F("IR_RECEIVER_GPIO: %d " CR), IR_RECEIVER_GPIO);
  Log.trace(F("ZgatewayIR setup done " CR));
}

void IRtoMQTT() {
  decode_results results;

  if (irrecv.decode(&results)) {
    Log.trace(F("Creating IR buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject IRdata = jsonBuffer.to<JsonObject>();

    Log.trace(F("Rcv. IR" CR));
# ifdef ESP32
    Log.trace(F("IR Task running on core :%d" CR), xPortGetCoreID());
# endif
    IRdata["value"] = (SIGNAL_SIZE_UL_ULL)(results.value);
    IRdata["protocol"] = (int)(results.decode_type);
    IRdata["bits"] = (int)(results.bits);
# if defined(ESP8266) || defined(ESP32)
    String hex = resultToHexidecimal(&results);
    IRdata["hex"] = (const char*)hex.c_str();
    String protocol = typeToString(results.decode_type, false);
    IRdata["protocol_name"] = (const char*)protocol.c_str();
# endif
    String rawCode = "";

    for (uint16_t i = 1; i < results.rawlen; i++) {
# if defined(ESP8266) || defined(ESP32)
      if (i % 100 == 0)
        yield();
      rawCode = rawCode + (results.rawbuf[i] * RAWTICK);
# else
      rawCode = rawCode + (results.rawbuf[i] * USECPERTICK);
# endif
      if (i < results.rawlen - 1)
        rawCode = rawCode + ",";
    }
    IRdata["raw"] = rawCode;

# ifdef RawDirectForward
# if defined(ESP8266) || defined(ESP32)
    uint16_t rawsend[results.rawlen];
    for (uint16_t i = 1; i < results.rawlen; i++) {
      if (i % 100 == 0)
        yield();
# else
    unsigned int rawsend[results.rawlen];
    for (int i = 1; i < results.rawlen; i++) {
# endif
      rawsend[i] = results.rawbuf[i];
    }
    irsend.sendRaw(rawsend, results.rawlen, RawFrequency);
    Log.trace(F("raw redirected" CR));
# endif
    irrecv.resume();
    SIGNAL_SIZE_UL_ULL MQTTvalue = IRdata["value"].as<SIGNAL_SIZE_UL_ULL>();

    if ((pubIRunknownPrtcl == false && IRdata["protocol"].as<int>() == -1)) {
      Log.notice(F("--no pub unknwn prt--" CR));
    } else if (!isAduplicateSignal(MQTTvalue) && MQTTvalue != 0) {
      Log.trace(F("Adv data IRtoMQTT" CR));
      pub(subjectIRtoMQTT, IRdata);
      Log.trace(F("Store val: %D" CR), MQTTvalue);
      storeSignalValue(MQTTvalue);
      if (repeatIRwMQTT) {
        Log.trace(F("Pub. IR for rpt" CR));
        pubMQTT(subjectForwardMQTTtoIR, MQTTvalue);
      }
    }
  }
}

bool sendIdentifiedProtocol(const char* protocol_name, SIGNAL_SIZE_UL_ULL data, const char* hex, unsigned int valueBITS, uint16_t valueRPT);

# if jsonReceiving
void MQTTtoIR(char* topicOri, JsonObject& IRdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoIR)) {
    Log.trace(F("MQTTtoIR json" CR));
    uint64_t data = IRdata["value"];
    const char* raw = IRdata["raw"];
    const char* hex = IRdata["hex"];
    if (hex) {
      Log.trace(F("hex: %s" CR), hex);
      data = getUInt64fromHex(hex);
    }
    if (data != 0 || raw) {
      Log.trace(F("MQTTtoIR value || raw  detected" CR));
      bool signalSent = false;
      const char* protocol_name = IRdata["protocol_name"];
      unsigned int valueBITS = IRdata["bits"] | 0;
      uint16_t valueRPT = IRdata["repeat"] | repeatIRwNumber;

      if (raw) {
        Log.trace(F("Raw: %s" CR), raw);
        unsigned int s = strlen(raw);

        int count = 0;
        for (int i = 0; i < s; i++) {
          if (raw[i] == ',') {
            count++;
          }
        }
# ifdef IR_GC
        if (strcmp(protocol_name, "GC") == 0) {
          Log.trace(F("GC" CR));

          uint16_t GC[count + 1];
          String value = "";
          int j = 0;
          for (int i = 0; i < s; i++) {
            if (raw[i] != ',') {
              value = value + String(raw[i]);
            }
            if ((raw[i] == ',') || (i == s - 1)) {
              GC[j] = value.toInt();
              value = "";
              j++;
            }
          }
          irsend.sendGC(GC, j);
          signalSent = true;
        }
# endif
# ifdef IR_RAW
        if (strcmp(protocol_name, "Raw") == 0) {
          Log.trace(F("Raw" CR));

# if defined(ESP8266) || defined(ESP32)
          uint16_t Raw[count + 1];
# else
          unsigned int Raw[count + 1];
# endif
          String value = "";
          int j = 0;
          for (int i = 0; i < s; i++) {
            if (raw[i] != ',') {
              value = value + String(raw[i]);
            }
            if ((raw[i] == ',') || (i == s - 1)) {
              Raw[j] = value.toInt();
              value = "";
              j++;
            }
          }
          irsend.sendRaw(Raw, j, RawFrequency);
          signalSent = true;
        }
# endif
      } else if (protocol_name && (strcmp(protocol_name, "NEC") != 0)) {
        Log.trace(F("Using Identified Protocol: %s  bits: %d repeat: %d" CR), protocol_name, valueBITS, valueRPT);
        signalSent = sendIdentifiedProtocol(protocol_name, data, hex, valueBITS, valueRPT);
      } else {
        Log.trace(F("Using NEC protocol" CR));
        Log.notice(F("Sending IR signal with %s" CR), protocol_name);
        if (valueBITS == 0)
          valueBITS = NEC_BITS;
# if defined(ESP8266) || defined(ESP32)
        irsend.sendNEC(data, valueBITS, valueRPT);
# else
        for (int i = 0; i <= valueRPT; i++)
          irsend.sendNEC(data, valueBITS);
# endif
        signalSent = true;
      }
      if (signalSent) {
        Log.notice(F("MQTTtoIR OK" CR));
        pub(subjectGTWIRtoMQTT, IRdata);
      }
      irrecv.enableIRIn();
    } else {
      Log.error(F("MQTTtoIR failed json read" CR));
    }
  }
}
# endif

bool sendIdentifiedProtocol(const char* protocol_name, SIGNAL_SIZE_UL_ULL data, const char* hex, unsigned int valueBITS, uint16_t valueRPT) {
  uint8_t dataarray[valueBITS];
  if (hex) {
    const char* ptr = NULL;
    (strstr(hex, "0x") != NULL) ? ptr = hex += 2 : ptr = hex;
    for (int i = 0; i < sizeof dataarray / sizeof *dataarray; i++) {
      sscanf(ptr, "%2hhx", &dataarray[i]);
      ptr += 2;
    }
    for (int i = 0; i < valueBITS; i++) {
      Log.trace(F("%x"), dataarray[i]);
    }
  }
# ifdef IR_WHYNTER
  if (strcmp(protocol_name, "WHYNTER") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = WHYNTER_BITS;
# if defined(ESP8266) || defined(ESP32)
    irsend.sendWhynter(data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendWhynter(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_LG
  if (strcmp(protocol_name, "LG") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = LG_BITS;
# if defined(ESP8266) || defined(ESP32)
    irsend.sendLG(data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendLG(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_SONY
  if (strcmp(protocol_name, "SONY") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
# if defined(ESP8266) || defined(ESP32)
    if (valueBITS == 0)
      valueBITS = SONY_20_BITS;
    irsend.sendSony(data, valueBITS, valueRPT);
# else
    if (valueBITS == 0)
      valueBITS = SONY_12_BITS;
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendSony(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_DISH
  if (strcmp(protocol_name, "DISH") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = DISH_BITS;
# if defined(ESP8266) || defined(ESP32)
    irsend.sendDISH(data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendDISH(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_RC5
  if (strcmp(protocol_name, "RC5") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = RC5_BITS;
# if defined(ESP8266) || defined(ESP32)
    irsend.sendRC5(data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendRC5(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_RC6
  if (strcmp(protocol_name, "RC6") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = RC6_MODE0_BITS;
# if defined(ESP8266) || defined(ESP32)
    irsend.sendRC6(data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendRC6(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_SHARP
  if (strcmp(protocol_name, "SHARP") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = SHARP_BITS;
# if defined(ESP8266) || defined(ESP32)
    irsend.sendSharpRaw(data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendSharpRaw(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_SAMSUNG
  if (strcmp(protocol_name, "SAMSUNG") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = SAMSUNG_BITS;
# if defined(ESP8266) || defined(ESP32)
    irsend.sendSAMSUNG(data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendSAMSUNG(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_JVC
  if (strcmp(protocol_name, "JVC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = JVC_BITS;
# if defined(ESP8266) || defined(ESP32)
    irsend.sendJVC(data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendJVC(data, valueBITS);
# endif
    return true;
  }
# endif
# ifdef IR_PANASONIC
  if (strcmp(protocol_name, "PANASONIC") == 0) {
# if defined(ESP8266) || defined(ESP32)
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = PANASONIC_BITS;
    irsend.sendPanasonic(PanasonicAddress, data, valueBITS, valueRPT);
# else
    for (int i = 0; i <= valueRPT; i++)
      irsend.sendPanasonic(PanasonicAddress, data);
# endif
    return true;
  }
# endif

# if defined(ESP8266) || defined(ESP32)
# ifdef IR_COOLIX
  if (strcmp(protocol_name, "COOLIX") == 0) {
    Log.trace(F("Sending %s:" CR), protocol_name);
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kCoolixBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kCoolixDefaultRepeat);
    irsend.sendCOOLIX(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_RCMM
  if (strcmp(protocol_name, "RCMM") == 0) {
    Log.trace(F("Sending %s:" CR), protocol_name);
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kRCMMBits;
    irsend.sendRCMM(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DENON
  if (strcmp(protocol_name, "DENON") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = DENON_BITS;
    irsend.sendDenon(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_GICABLE
  if (strcmp(protocol_name, "GICABLE") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kGicableBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kGicableMinRepeat);
    irsend.sendGICable(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SHERWOOD
  if (strcmp(protocol_name, "SHERWOOD") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSherwoodBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kSherwoodMinRepeat);
    irsend.sendSherwood(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MITSUBISHI
  if (strcmp(protocol_name, "MITSUBISHI") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiMinRepeat);
    irsend.sendMitsubishi(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_NIKAI
  if (strcmp(protocol_name, "NIKAI") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kNikaiBits;
    irsend.sendNikai(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MIDEA
  if (strcmp(protocol_name, "MIDEA") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMideaBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMideaMinRepeat);
    irsend.sendMidea(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MAGIQUEST
  if (strcmp(protocol_name, "MAGIQUEST") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMagiquestBits;
    irsend.sendMagiQuest(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_LASERTAG
  if (strcmp(protocol_name, "LASERTAG") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kLasertagBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kLasertagMinRepeat);
    irsend.sendLasertag(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_CARRIER_AC
  if (strcmp(protocol_name, "CARRIER_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kCarrierAcBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kCarrierAcMinRepeat);
    irsend.sendCarrierAC(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MITSUBISHI2
  if (strcmp(protocol_name, "MITSUBISHI2") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiMinRepeat);
    irsend.sendMitsubishi2(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_AIWA_RC_T501
  if (strcmp(protocol_name, "AIWA_RC_T501") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kAiwaRcT501Bits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kAiwaRcT501MinRepeats);
    irsend.sendAiwaRCT501(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DAIKIN
  if (strcmp(protocol_name, "DAIKIN") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikinStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikinDefaultRepeat);
    irsend.sendDaikin(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_KELVINATOR
  if (strcmp(protocol_name, "KELVINATOR") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kKelvinatorStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kKelvinatorDefaultRepeat);
    irsend.sendKelvinator(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MITSUBISHI_AC
  if (strcmp(protocol_name, "MITSUBISHI_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiACStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiACMinRepeat);
    irsend.sendMitsubishiAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SANYO
  if (strcmp(protocol_name, "SANYOLC7461") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSanyoLC7461Bits;
    irsend.sendSanyoLC7461(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_GREE
  if (strcmp(protocol_name, "GREE") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kGreeStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kGreeDefaultRepeat);
    irsend.sendGree(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_ARGO
  if (strcmp(protocol_name, "ARGO") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kArgoStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kArgoDefaultRepeat);
    irsend.sendArgo(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_TROTEC
  if (strcmp(protocol_name, "TROTEC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kTrotecStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kTrotecDefaultRepeat);
    irsend.sendTrotec(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_TOSHIBA_AC
  if (strcmp(protocol_name, "TOSHIBA_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kToshibaACBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kToshibaACMinRepeat);
    irsend.sendToshibaAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_FUJITSU_AC
  if (strcmp(protocol_name, "FUJITSU_AC") == 0) {
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kFujitsuAcMinRepeat);
    irsend.sendFujitsuAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HAIER_AC
  if (strcmp(protocol_name, "HAIER_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHaierACStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHaierAcDefaultRepeat);
    irsend.sendHaierAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HITACHI_AC
  if (strcmp(protocol_name, "HITACHI_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    irsend.sendHitachiAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HITACHI_AC1
  if (strcmp(protocol_name, "HITACHI_AC1") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc1StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    irsend.sendHitachiAC1(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HITACHI_AC2
  if (strcmp(protocol_name, "HITACHI_AC2") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc2StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    irsend.sendHitachiAC2(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HAIER_AC_YRW02
  if (strcmp(protocol_name, "HAIER_AC_YRW02") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHaierACYRW02StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHaierAcYrw02DefaultRepeat);
    irsend.sendHaierACYRW02(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_WHIRLPOOL_AC
  if (strcmp(protocol_name, "WHIRLPOOL_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kWhirlpoolAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kWhirlpoolAcDefaultRepeat);
    irsend.sendWhirlpoolAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SAMSUNG_AC
  if (strcmp(protocol_name, "SAMSUNG_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSamsungAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kSamsungAcDefaultRepeat);
    irsend.sendSamsungAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_LUTRON
  if (strcmp(protocol_name, "LUTRON") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kLutronBits;
    irsend.sendLutron(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_ELECTRA_AC
  if (strcmp(protocol_name, "ELECTRA_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kElectraAcStateLength;
    irsend.sendElectraAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_PANASONIC_AC
  if (strcmp(protocol_name, "PANASONIC_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kPanasonicAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kPanasonicAcDefaultRepeat);
    irsend.sendPanasonicAC(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_PIONEER
  if (strcmp(protocol_name, "PIONEER") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kPioneerBits;
    irsend.sendPioneer(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_LG2
  if (strcmp(protocol_name, "LG2") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kLgBits;
    irsend.sendLG2(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MWM
  if (strcmp(protocol_name, "MWM") == 0) {
    irsend.sendMWM(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DAIKIN2
  if (strcmp(protocol_name, "DAIKIN2") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin2StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin2DefaultRepeat);
    irsend.sendDaikin2(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_VESTEL_AC
  if (strcmp(protocol_name, "VESTEL_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kVestelAcBits;
    irsend.sendVestelAc(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SAMSUNG36
  if (strcmp(protocol_name, "SAMSUNG36") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSamsung36Bits;
    irsend.sendSamsung36(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_TCL112AC
  if (strcmp(protocol_name, "TCL112AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kTcl112AcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kTcl112AcDefaultRepeat);
    irsend.sendTcl112Ac(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_TECO
  if (strcmp(protocol_name, "TECO") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kTecoBits;
    irsend.sendTeco(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_LEGOPF
  if (strcmp(protocol_name, "LEGOPF") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kLegoPfBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kLegoPfMinRepeat);
    irsend.sendLegoPf(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MITSUBISHIHEAVY88
  if (strcmp(protocol_name, "MITSUBISHIHEAVY88") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiHeavy88StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiHeavy88MinRepeat);
    irsend.sendMitsubishiHeavy88(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MITSUBISHIHEAVY152
  if (strcmp(protocol_name, "MITSUBISHIHEAVY152") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishiHeavy152StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishiHeavy152MinRepeat);
    irsend.sendMitsubishiHeavy152(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DAIKIN216
  if (strcmp(protocol_name, "DAIKIN216") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin216StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin216DefaultRepeat);
    irsend.sendDaikin216(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SHARP_AC
  if (strcmp(protocol_name, "SHARP_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSharpAcStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kSharpAcDefaultRepeat);
    irsend.sendSharpAc(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_GOODWEATHER
  if (strcmp(protocol_name, "GOODWEATHER_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kGoodweatherBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kGoodweatherMinRepeat);
    irsend.sendGoodweather(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_INAX
  if (strcmp(protocol_name, "INAX") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kInaxBits;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kInaxMinRepeat);
    irsend.sendInax(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DAIKIN160
  if (strcmp(protocol_name, "DAIKIN160") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin160StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin160DefaultRepeat);
    irsend.sendDaikin160(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_NEOCLIMA
  if (strcmp(protocol_name, "NEOCLIMA") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kNeoclimaStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kNeoclimaMinRepeat);
    irsend.sendNeoclima(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DAIKIN176
  if (strcmp(protocol_name, "DAIKIN176") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin176StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin176DefaultRepeat);
    irsend.sendDaikin176(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DAIKIN128
  if (strcmp(protocol_name, "DAIKIN128") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin128StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin128DefaultRepeat);
    irsend.sendDaikin128(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_AMCOR
  if (strcmp(protocol_name, "AMCOR") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kAmcorStateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kAmcorDefaultRepeat);
    irsend.sendAmcor(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DAIKIN152
  if (strcmp(protocol_name, "DAIKIN152") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDaikin152StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin152DefaultRepeat);
    irsend.sendDaikin152(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MITSUBISHI136
  if (strcmp(protocol_name, "MITSUBISHI136") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishi136StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishi136MinRepeat);
    irsend.sendMitsubishi136(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MITSUBISHI112
  if (strcmp(protocol_name, "MITSUBISHI112") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kMitsubishi112StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMitsubishi112MinRepeat);
    irsend.sendMitsubishi112(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HITACHI_AC424
  if (strcmp(protocol_name, "HITACHI_AC424") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc424StateLength;
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    irsend.sendHitachiAc424(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SONY_38K
  if (strcmp(protocol_name, "SONY_38K") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, (uint16_t)(kSonyMinRepeat + 1));
    if (valueBITS == 0)
      valueBITS = kSony20Bits;
    irsend.sendSony38(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_EPSON
  if (strcmp(protocol_name, "EPSON") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kEpsonMinRepeat);
    if (valueBITS == 0)
      valueBITS = kEpsonBits;
    irsend.sendEpson(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SYMPHONY
  if (strcmp(protocol_name, "SYMPHONY") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kSymphonyDefaultRepeat);
    if (valueBITS == 0)
      valueBITS = kSymphonyBits;
    irsend.sendSymphony(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HITACHI_AC3
  if (strcmp(protocol_name, "HITACHI_AC3") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kHitachiAcDefaultRepeat);
    if (valueBITS == 0)
      Log.error(F("For this protocol you should have a BIT number as there is no default one defined" CR));
    irsend.sendHitachiAc3(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DAIKIN64
  if (strcmp(protocol_name, "DAIKIN64") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDaikin64DefaultRepeat);
    if (valueBITS == 0)
      valueBITS = kDaikin64Bits;
    irsend.sendDaikin64(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_AIRWELL
  if (strcmp(protocol_name, "AIRWELL") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kAirwellMinRepeats);
    if (valueBITS == 0)
      valueBITS = kAirwellBits;
    irsend.sendAirwell(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DELONGHI_AC
  if (strcmp(protocol_name, "DELONGHI_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kDelonghiAcDefaultRepeat);
    if (valueBITS == 0)
      valueBITS = kDelonghiAcBits;
    irsend.sendDelonghiAc(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_DOSHISHA
  if (strcmp(protocol_name, "DOSHISHA") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kDoshishaBits;
    irsend.sendDoshisha(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_CARRIER_AC40
  if (strcmp(protocol_name, "CARRIER_AC40") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kCarrierAc40MinRepeat);
    if (valueBITS == 0)
      valueBITS = kCarrierAc40Bits;
    irsend.sendCarrierAC40(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_CARRIER_AC64
  if (strcmp(protocol_name, "CARRIER_AC64") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kCarrierAc64MinRepeat);
    if (valueBITS == 0)
      valueBITS = kCarrierAc64Bits;
    irsend.sendCarrierAC64(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HITACHI_AC344
  if (strcmp(protocol_name, "HITACHI_AC344") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc344StateLength;
    irsend.sendHitachiAc344(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_CORONA_AC
  if (strcmp(protocol_name, "CORONA_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kCoronaAcStateLength;
    irsend.sendCoronaAc(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MIDEA24
  if (strcmp(protocol_name, "MIDEA24") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMidea24MinRepeat);
    if (valueBITS == 0)
      valueBITS = kMidea24Bits;
    irsend.sendMidea24(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_ZEPEAL
  if (strcmp(protocol_name, "ZEPEAL") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kZepealMinRepeat);
    if (valueBITS == 0)
      valueBITS = kZepealBits;
    irsend.sendZepeal(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SANYO_AC
  if (strcmp(protocol_name, "SANYO_AC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSanyoAcStateLength;
    irsend.sendSanyoAc(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_VOLTAS
  if (strcmp(protocol_name, "VOLTAS") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kVoltasStateLength;
    irsend.sendVoltas(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_METZ
  if (strcmp(protocol_name, "METZ") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMetzMinRepeat);
    if (valueBITS == 0)
      valueBITS = kMetzBits;
    irsend.sendMetz(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_TRANSCOLD
  if (strcmp(protocol_name, "TRANSCOLD") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kTranscoldDefaultRepeat);
    if (valueBITS == 0)
      valueBITS = kTranscoldBits;
    irsend.sendTranscold(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_TECHNIBEL_AC
  if (strcmp(protocol_name, "TECHNIBELAC") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kTechnibelAcDefaultRepeat);
    if (valueBITS == 0)
      valueBITS = kTechnibelAcBits;
    irsend.sendTechnibelAc(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MIRAGE
  if (strcmp(protocol_name, "MIRAGE") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMirageMinRepeat);
    if (valueBITS == 0)
      valueBITS = kMirageStateLength;
    irsend.sendMirage(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_ELITESCREENS
  if (strcmp(protocol_name, "ELITESCREENS") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kEliteScreensDefaultRepeat);
    if (valueBITS == 0)
      valueBITS = kEliteScreensBits;
    irsend.sendElitescreens(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_PANASONIC_AC32
  if (strcmp(protocol_name, "PANASONIC_AC32") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kPioneerBits;
    irsend.sendPanasonicAC32(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_MILESTAG2
  if (strcmp(protocol_name, "MILESTAG2") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueRPT == repeatIRwNumber)
      valueRPT = std::max(valueRPT, kMilesMinRepeat);
    if (valueBITS == 0)
      valueBITS = kMilesTag2ShotBits;
    irsend.sendMilestag2(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_ECOCLIM
  if (strcmp(protocol_name, "ECOCLIM") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kEcoclimBits;
    irsend.sendEcoclim(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_XMP
  if (strcmp(protocol_name, "XMP") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kXmpBits;
    irsend.sendXmp(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_KELON168
  if (strcmp(protocol_name, "KELON168") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kKelon168Bits;
    irsend.sendKelon168(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_TEKNOPOINT
  if (strcmp(protocol_name, "TEKNOPOINT") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kTeknopointBits;
    irsend.sendTeknopoint(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HAIER_AC176
  if (strcmp(protocol_name, "HAIER_AC176") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHaierAC176Bits;
    irsend.sendHaierAC176(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_BOSE
  if (strcmp(protocol_name, "BOSE") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kBoseBits;
    irsend.sendBose(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_SANYO_AC88
  if (strcmp(protocol_name, "SANYO_AC88") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kSanyoAc88Bits;
    irsend.sendSanyoAc88(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_TROTEC_3550
  if (strcmp(protocol_name, "TROTEC_3550") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kTrotecBits;
    irsend.sendTrotec3550(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_ARRIS
  if (strcmp(protocol_name, "ARRIS") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kArrisBits;
    irsend.sendArris(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_RHOSS
  if (strcmp(protocol_name, "RHOSS") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kRhossBits;
    irsend.sendRhoss(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_AIRTON
  if (strcmp(protocol_name, "AIRTON") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kAirtonBits;
    irsend.sendAirton(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_COOLIX48
  if (strcmp(protocol_name, "COOLIX48") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kCoolix48Bits;
    irsend.sendCoolix48(data, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HITACHI_AC264
  if (strcmp(protocol_name, "HITACHI_AC264") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc264Bits;
    irsend.sendHitachiAc264(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
# ifdef IR_HITACHI_AC296
  if (strcmp(protocol_name, "HITACHI_AC296") == 0) {
    Log.notice(F("Sending IR signal with %s" CR), protocol_name);
    if (valueBITS == 0)
      valueBITS = kHitachiAc296Bits;
    irsend.sendHitachiAc296(dataarray, valueBITS, valueRPT);
    return true;
  }
# endif
  Log.warning(F("Unknown IR protocol" CR));
  return false;
# endif
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayLORA.ino"
# 28 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayLORA.ino"
#include "User_config.h"

#ifdef ZgatewayLORA

# include <LoRa.h>
# include <SPI.h>
# include <Wire.h>

void setupLORA() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);

  if (!LoRa.begin(LORA_BAND)) {
    Log.error(F("ZgatewayLORA setup failed!" CR));
    while (1)
      ;
  }
  LoRa.receive();
  Log.notice(F("LORA_SCK: %d" CR), LORA_SCK);
  Log.notice(F("LORA_MISO: %d" CR), LORA_MISO);
  Log.notice(F("LORA_MOSI: %d" CR), LORA_MOSI);
  Log.notice(F("LORA_SS: %d" CR), LORA_SS);
  Log.notice(F("LORA_RST: %d" CR), LORA_RST);
  Log.notice(F("LORA_DI0: %d" CR), LORA_DI0);
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
  LoRa.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
  LoRa.setCodingRate4(LORA_CODING_RATE);
  LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  Log.trace(F("ZgatewayLORA setup done" CR));
}

void LORAtoMQTT() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject LORAdata = jsonBuffer.to<JsonObject>();
    Log.trace(F("Rcv. LORA" CR));
# ifdef ESP32
    String taskMessage = "LORA Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();

# endif
    String packet;
    packet = "";
    for (int i = 0; i < packetSize; i++) {
      packet += (char)LoRa.read();
    }
    LORAdata["rssi"] = (int)LoRa.packetRssi();
    LORAdata["snr"] = (float)LoRa.packetSnr();
    LORAdata["pferror"] = (float)LoRa.packetFrequencyError();
    LORAdata["packetSize"] = (int)packetSize;
    LORAdata["message"] = (char*)packet.c_str();
    pub(subjectLORAtoMQTT, LORAdata);
    if (repeatLORAwMQTT) {
      Log.trace(F("Pub LORA for rpt" CR));
      pub(subjectMQTTtoLORA, LORAdata);
    }
  }
}

# if jsonReceiving
void MQTTtoLORA(char* topicOri, JsonObject& LORAdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORA)) {
    Log.trace(F("MQTTtoLORA json" CR));
    const char* message = LORAdata["message"];
    int txPower = LORAdata["txpower"] | LORA_TX_POWER;
    int spreadingFactor = LORAdata["spreadingfactor"] | LORA_SPREADING_FACTOR;
    long int frequency = LORAdata["frequency "] | LORA_BAND;
    long int signalBandwidth = LORAdata["signalbandwidth"] | LORA_SIGNAL_BANDWIDTH;
    int codingRateDenominator = LORAdata["codingrate"] | LORA_CODING_RATE;
    int preambleLength = LORAdata["preamblelength"] | LORA_PREAMBLE_LENGTH;
    byte syncWord = LORAdata["syncword"] | LORA_SYNC_WORD;
    bool Crc = LORAdata["enablecrc"] | DEFAULT_CRC;
    if (message) {
      LoRa.setTxPower(txPower);
      LoRa.setFrequency(frequency);
      LoRa.setSpreadingFactor(spreadingFactor);
      LoRa.setSignalBandwidth(signalBandwidth);
      LoRa.setCodingRate4(codingRateDenominator);
      LoRa.setPreambleLength(preambleLength);
      LoRa.setSyncWord(syncWord);
      if (Crc)
        LoRa.enableCrc();
      LoRa.beginPacket();
      LoRa.print(message);
      LoRa.endPacket();
      Log.trace(F("MQTTtoLORA OK" CR));
      pub(subjectGTWLORAtoMQTT, LORAdata);
    } else {
      Log.error(F("MQTTtoLORA Fail json" CR));
    }
  }
}
# endif
# if simpleReceiving
void MQTTtoLORA(char* topicOri, char* LORAdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORA)) {
    LoRa.beginPacket();
    LoRa.print(LORAdata);
    LoRa.endPacket();
    Log.notice(F("MQTTtoLORA OK" CR));
    pub(subjectGTWLORAtoMQTT, LORAdata);
  }
}
# endif
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayPilight.ino"
# 28 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayPilight.ino"
#include "User_config.h"

#ifdef ZgatewayPilight

# ifdef ZradioCC1101
# include <ELECHOUSE_CC1101_SRC_DRV.h>
# endif

# include <ESPiLight.h>
ESPiLight rf(RF_EMITTER_GPIO);

void pilightCallback(const String& protocol, const String& message, int status,
                     size_t repeats, const String& deviceID) {
  if (status == VALID) {
    Log.trace(F("Creating RF PiLight buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RFPiLightdata = jsonBuffer.to<JsonObject>();
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer2;
    JsonObject msg = jsonBuffer2.to<JsonObject>();
    auto error = deserializeJson(jsonBuffer2, message);
    if (error) {
      Log.error(F("deserializeJson() failed: %s" CR), error.c_str());
      return;
    }
    RFPiLightdata["message"] = msg;
    RFPiLightdata["protocol"] = (const char*)protocol.c_str();
    RFPiLightdata["length"] = (const char*)deviceID.c_str();

    const char* device_id = deviceID.c_str();
    if (!strlen(device_id)) {


      char* choices[] = {"key", "unit", "device_id", "systemcode", "unitcode", "programcode"};

      for (uint8_t i = 0; i < 6; i++) {
        if (msg[choices[i]]) {
          device_id = (const char*)msg[choices[i]];
          break;
        }
      }
    }

    RFPiLightdata["value"] = device_id;
    RFPiLightdata["repeats"] = (int)repeats;
    RFPiLightdata["status"] = (int)status;
    pub(subjectPilighttoMQTT, RFPiLightdata);
    if (repeatPilightwMQTT) {
      Log.trace(F("Pub Pilight for rpt" CR));
      pub(subjectMQTTtoPilight, RFPiLightdata);
    }
  }
}

void setupPilight() {
# ifdef ZradioCC1101
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);
  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY);
# endif
  rf.setCallback(pilightCallback);
  rf.initReceiver(RF_RECEIVER_GPIO);
  pinMode(RF_EMITTER_GPIO, OUTPUT);
  Log.notice(F("RF_EMITTER_GPIO: %d " CR), RF_EMITTER_GPIO);
  Log.notice(F("RF_RECEIVER_GPIO: %d " CR), RF_RECEIVER_GPIO);
  Log.trace(F("ZgatewayPilight command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoPilight);
  Log.trace(F("ZgatewayPilight setup done " CR));
}

void savePilightConfig() {
  Log.trace(F("saving Pilight config" CR));
  DynamicJsonDocument json(4096);
  deserializeJson(json, rf.enabledProtocols());

  File configFile = SPIFFS.open("/pilight.json", "w");
  if (!configFile) {
    Log.error(F("failed to open config file for writing" CR));
  }

  serializeJsonPretty(json, Serial);
  serializeJson(json, configFile);
  configFile.close();
}

void loadPilightConfig() {
  Log.trace(F("reading Pilight config file" CR));
  File configFile = SPIFFS.open("/pilight.json", "r");
  if (configFile) {
    Log.trace(F("opened Pilight config file" CR));
    DynamicJsonDocument json(configFile.size() * 4);
    auto error = deserializeJson(json, configFile);
    if (error) {
      Log.error(F("deserialize config failed: %s, buffer capacity: %u" CR), error.c_str(), json.capacity());
    }
    serializeJson(json, Serial);
    if (!json.isNull()) {
      String rflimit;
      serializeJson(json, rflimit);
      rf.limitProtocols(rflimit);
    } else {
      Log.warning(F("failed to load json config" CR));
    }
    configFile.close();
  }
}

void PilighttoMQTT() {
  rf.loop();
}

void MQTTtoPilight(char* topicOri, JsonObject& Pilightdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoPilightProtocol)) {
    bool success = false;
    if (Pilightdata.containsKey("reset")) {
      rf.limitProtocols(rf.availableProtocols());
      savePilightConfig();
      success = true;
    }
    if (Pilightdata.containsKey("limit")) {
      String output;
      serializeJson(Pilightdata["limit"], output);
      rf.limitProtocols(output);
      savePilightConfig();
      success = true;
    }
    if (Pilightdata.containsKey("enabled")) {
      Log.notice(F("PiLight protocols enabled: %s" CR), rf.enabledProtocols().c_str());
      success = true;
    }
    if (Pilightdata.containsKey("available")) {
      Log.notice(F("PiLight protocols available: %s" CR), rf.availableProtocols().c_str());
      success = true;
    }

    if (success) {
      pub(subjectGTWPilighttoMQTT, Pilightdata);
    } else {
      pub(subjectGTWPilighttoMQTT, "{\"Status\": \"Error\"}");
      Log.error(F("MQTTtoPilightProtocol Fail json" CR));
    }
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoPilight)) {
    const char* message = Pilightdata["message"];
    const char* protocol = Pilightdata["protocol"];
    const char* raw = Pilightdata["raw"];
    float tempMhz = Pilightdata["mhz"];
    bool success = false;
    if (raw) {
      uint16_t codes[MAXPULSESTREAMLENGTH];
      int repeats = rf.stringToRepeats(raw);
      if (repeats < 0) {
        switch (repeats) {
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_R:
            Log.trace(F("'r' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_END:
            Log.trace(F("';' or '@' not found in data string" CR));
            break;
        }
        repeats = 10;
      }
      int msgLength = rf.stringToPulseTrain(raw, codes, MAXPULSESTREAMLENGTH);
      if (msgLength > 0) {
# ifdef ZradioCC1101
        ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY);
        rf.disableReceiver();
# endif
        rf.sendPulseTrain(codes, msgLength, repeats);
        Log.notice(F("MQTTtoPilight raw ok" CR));
        success = true;
      } else {
        Log.trace(F("MQTTtoPilight raw KO" CR));
        switch (msgLength) {
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_C:
            Log.trace(F("'c' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_P:
            Log.trace(F("'p' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_END:
            Log.trace(F("';' or '@' not found in data string" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_TYPE:
            Log.trace(F("pulse type not defined" CR));
            break;
        }
        Log.error(F("Invalid JSON: raw data malformed" CR));
      }
    }
    if (message && protocol) {
      Log.trace(F("MQTTtoPilight msg & protocol ok" CR));
# ifdef ZradioCC1101
      ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY);
      rf.disableReceiver();
# endif
      int msgLength = rf.send(protocol, message);
      if (msgLength > 0) {
        Log.trace(F("Adv data MQTTtoPilight push state via PilighttoMQTT" CR));
        pub(subjectGTWPilighttoMQTT, Pilightdata);
        success = true;
      } else {
        switch (msgLength) {
          case ESPiLight::ERROR_UNAVAILABLE_PROTOCOL:
            Log.error(F("protocol is not available" CR));
            break;
          case ESPiLight::ERROR_INVALID_PILIGHT_MSG:
            Log.error(F("message is invalid" CR));
            break;
          case ESPiLight::ERROR_INVALID_JSON:
            Log.error(F("message is not a proper json object" CR));
            break;
          case ESPiLight::ERROR_NO_OUTPUT_PIN:
            Log.error(F("no transmitter pin" CR));
            break;
          default:
            Log.error(F("Invalid JSON: can't read message/protocol" CR));
        }
      }
    }
    if (Pilightdata.containsKey("active")) {
      Log.trace(F("PiLight active:" CR));
      activeReceiver = ACTIVE_PILIGHT;
      success = true;
    }
# ifdef ZradioCC1101
    if (Pilightdata.containsKey("mhz") && validFrequency(tempMhz)) {
      receiveMhz = tempMhz;
      Log.notice(F("PiLight Receive mhz: %F" CR), receiveMhz);
      success = true;
    }
# endif
    if (success) {
      pub(subjectGTWPilighttoMQTT, Pilightdata);
    } else {
      pub(subjectGTWPilighttoMQTT, "{\"Status\": \"Error\"}");
      Log.error(F("MQTTtoPilight Fail json" CR));
    }
    enableActiveReceiver(false);
  }
}

extern void disablePilightReceive() {
  Log.trace(F("disablePilightReceive" CR));
  rf.initReceiver(-1);
  rf.disableReceiver();
};

extern void enablePilightReceive() {
# ifdef ZradioCC1101
  Log.notice(F("Switching to Pilight Receiver: %F" CR), receiveMhz);
# else
  Log.notice(F("Switching to Pilight Receiver" CR));
# endif
# ifdef ZgatewayRF
  disableRFReceive();
# endif
# ifdef ZgatewayRF2
  disableRF2Receive();
# endif
# ifdef ZgatewayRTL_433
  disableRTLreceive();
# endif

# ifdef ZradioCC1101
  ELECHOUSE_cc1101.SetRx(receiveMhz);
# endif
  rf.setCallback(pilightCallback);
  rf.initReceiver(RF_RECEIVER_GPIO);
  pinMode(RF_EMITTER_GPIO, OUTPUT);
  rf.enableReceiver();
  loadPilightConfig();
};
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRF.ino"
# 28 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRF.ino"
#include "User_config.h"

#ifdef ZgatewayRF

# ifdef ZradioCC1101
# include <ELECHOUSE_CC1101_SRC_DRV.h>
# endif

# include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();


static const char* bin2tristate(const char* bin) {
  static char returnValue[50];
  int pos = 0;
  int pos2 = 0;
  while (bin[pos] != '\0' && bin[pos + 1] != '\0') {
    if (bin[pos] == '0' && bin[pos + 1] == '0') {
      returnValue[pos2] = '0';
    } else if (bin[pos] == '1' && bin[pos + 1] == '1') {
      returnValue[pos2] = '1';
    } else if (bin[pos] == '0' && bin[pos + 1] == '1') {
      returnValue[pos2] = 'F';
    } else {
      return "-";
    }
    pos = pos + 2;
    pos2++;
  }
  returnValue[pos2] = '\0';
  return returnValue;
}

static char* dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
  static char bin[64];
  unsigned int i = 0;

  while (Dec > 0) {
    bin[32 + i++] = ((Dec & 1) > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j < bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[31 + i - (j - (bitLength - i))];
    } else {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';

  return bin;
}

# if defined(ZmqttDiscovery) && !defined(RF_DISABLE_TRANSMIT) && defined(RFmqttDiscovery)

void RFtoMQTTdiscovery(SIGNAL_SIZE_UL_ULL MQTTvalue) {

  char val[11];
  sprintf(val, "%lu", MQTTvalue);
  Log.trace(F("RF Entity Discovered, create HA Discovery CFG" CR));
  char* switchRF[2] = {val, "RF"};
  Log.trace(F("CreateDiscoverySwitch: %s" CR), switchRF[1]);
# if valueAsATopic
  String discovery_topic = String(subjectRFtoMQTT) + "/" + String(switchRF[0]);
# else
  String discovery_topic = String(subjectRFtoMQTT);
# endif

  String theUniqueId = getUniqueId("-" + String(switchRF[0]), "-" + String(switchRF[1]));

  announceDeviceTrigger(
      false,
      (char*)discovery_topic.c_str(),
      "", "",
      (char*)theUniqueId.c_str(),
      "", "", "", "");
}
# endif

void setupRF() {

  Log.notice(F("RF_EMITTER_GPIO: %d " CR), RF_EMITTER_GPIO);
  Log.notice(F("RF_RECEIVER_GPIO: %d " CR), RF_RECEIVER_GPIO);
# ifdef ZradioCC1101
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.SetRx(receiveMhz);
# endif
# ifdef RF_DISABLE_TRANSMIT
  mySwitch.disableTransmit();
# else
  mySwitch.enableTransmit(RF_EMITTER_GPIO);
# endif
  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT);
  mySwitch.enableReceive(RF_RECEIVER_GPIO);
  Log.trace(F("ZgatewayRF command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoRF);
  Log.trace(F("ZgatewayRF setup done" CR));
}

void RFtoMQTT() {
  if (mySwitch.available()) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RFdata = jsonBuffer.to<JsonObject>();
    Log.trace(F("Rcv. RF" CR));
# ifdef ESP32
    Log.trace(F("RF Task running on core :%d" CR), xPortGetCoreID());
# endif
    SIGNAL_SIZE_UL_ULL MQTTvalue = mySwitch.getReceivedValue();
    int length = mySwitch.getReceivedBitlength();
    const char* binary = dec2binWzerofill(MQTTvalue, length);

    RFdata["value"] = (SIGNAL_SIZE_UL_ULL)MQTTvalue;
    RFdata["protocol"] = (int)mySwitch.getReceivedProtocol();
    RFdata["length"] = (int)mySwitch.getReceivedBitlength();
    RFdata["delay"] = (int)mySwitch.getReceivedDelay();
    RFdata["tre_state"] = bin2tristate(binary);
    RFdata["binary"] = binary;

    unsigned int* raw = mySwitch.getReceivedRawdata();
    String rawDump = "";
    for (unsigned int i = 0; i <= length * 2; i++) {
      rawDump = rawDump + String(raw[i]) + ",";
    }
    RFdata["raw"] = rawDump.c_str();

# ifdef ZradioCC1101
    RFdata["mhz"] = receiveMhz;
# endif
    mySwitch.resetAvailable();

    if (!isAduplicateSignal(MQTTvalue) && MQTTvalue != 0) {
# if defined(ZmqttDiscovery) && !defined(RF_DISABLE_TRANSMIT) && defined(RFmqttDiscovery)
      if (disc)
        RFtoMQTTdiscovery(MQTTvalue);
# endif
      pub(subjectRFtoMQTT, RFdata);

      Log.trace(F("Store val: %u" CR), (unsigned long)MQTTvalue);
      storeSignalValue(MQTTvalue);
      if (repeatRFwMQTT) {
        Log.trace(F("Pub RF for rpt" CR));
        pub(subjectMQTTtoRF, RFdata);
      }
    }
  }
}

# if simpleReceiving
void MQTTtoRF(char* topicOri, char* datacallback) {
# ifdef ZradioCC1101
  ELECHOUSE_cc1101.SetTx(receiveMhz);
# endif
  mySwitch.disableReceive();
  mySwitch.enableTransmit(RF_EMITTER_GPIO);
  SIGNAL_SIZE_UL_ULL data = STRTO_UL_ULL(datacallback, NULL, 10);



  String topic = topicOri;
  int valuePRT = 0;
  int valuePLSL = 0;
  int valueBITS = 0;
  int pos = topic.lastIndexOf(RFprotocolKey);
  if (pos != -1) {
    pos = pos + +strlen(RFprotocolKey);
    valuePRT = (topic.substring(pos, pos + 1)).toInt();
  }

  int pos2 = topic.lastIndexOf(RFpulselengthKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RFpulselengthKey);
    valuePLSL = (topic.substring(pos2, pos2 + 3)).toInt();
  }
  int pos3 = topic.lastIndexOf(RFbitsKey);
  if (pos3 != -1) {
    pos3 = pos3 + strlen(RFbitsKey);
    valueBITS = (topic.substring(pos3, pos3 + 2)).toInt();
  }

  if ((cmpToMainTopic(topicOri, subjectMQTTtoRF)) && (valuePRT == 0) && (valuePLSL == 0) && (valueBITS == 0)) {
    Log.trace(F("MQTTtoRF dflt" CR));
    mySwitch.setProtocol(1, 350);
    mySwitch.send(data, 24);

    pub(subjectGTWRFtoMQTT, datacallback);
  } else if ((valuePRT != 0) || (valuePLSL != 0) || (valueBITS != 0)) {
    Log.trace(F("MQTTtoRF usr par." CR));
    if (valuePRT == 0)
      valuePRT = 1;
    if (valuePLSL == 0)
      valuePLSL = 350;
    if (valueBITS == 0)
      valueBITS = 24;
    Log.notice(F("RF Protocol:%d" CR), valuePRT);
    Log.notice(F("RF Pulse Lgth: %d" CR), valuePLSL);
    Log.notice(F("Bits nb: %d" CR), valueBITS);
    mySwitch.setProtocol(valuePRT, valuePLSL);
    mySwitch.send(data, valueBITS);

    pub(subjectGTWRFtoMQTT, datacallback);
  }
# ifdef ZradioCC1101
  ELECHOUSE_cc1101.SetRx(receiveMhz);
  mySwitch.disableTransmit();
  mySwitch.enableReceive(RF_RECEIVER_GPIO);
# endif
}
# endif

# if jsonReceiving
void MQTTtoRF(char* topicOri, JsonObject& RFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRF)) {
    Log.trace(F("MQTTtoRF json" CR));
    SIGNAL_SIZE_UL_ULL data = RFdata["value"];
    if (data != 0) {
      int valuePRT = RFdata["protocol"] | 1;
      int valuePLSL = RFdata["delay"] | 350;
      int valueBITS = RFdata["length"] | 24;
      int valueRPT = RFdata["repeat"] | RF_EMITTER_REPEAT;
      Log.notice(F("RF Protocol:%d" CR), valuePRT);
      Log.notice(F("RF Pulse Lgth: %d" CR), valuePLSL);
      Log.notice(F("Bits nb: %d" CR), valueBITS);
# ifdef ZradioCC1101
      float trMhz = RFdata["mhz"] | CC1101_FREQUENCY;
      if (validFrequency((int)trMhz)) {
        ELECHOUSE_cc1101.SetTx(trMhz);
        Log.notice(F("Transmit mhz: %F" CR), trMhz);
      }
# endif
      mySwitch.disableReceive();
      mySwitch.enableTransmit(RF_EMITTER_GPIO);
      mySwitch.setRepeatTransmit(valueRPT);
      mySwitch.setProtocol(valuePRT, valuePLSL);
      mySwitch.send(data, valueBITS);
      Log.notice(F("MQTTtoRF OK" CR));
      pub(subjectGTWRFtoMQTT, RFdata);
      mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT);
    } else {
      bool success = false;
      if (RFdata.containsKey("active")) {
        Log.trace(F("RF active:" CR));
        activeReceiver = ACTIVE_RF;
        success = true;
      }
# ifdef ZradioCC1101
      float tempMhz = RFdata["mhz"];
      if (RFdata.containsKey("mhz") && validFrequency(tempMhz)) {
        receiveMhz = tempMhz;
        Log.notice(F("Receive mhz: %F" CR), receiveMhz);
        success = true;
      }
# endif
      if (success) {
        pub(subjectGTWRFtoMQTT, RFdata);
      } else {
# ifndef ARDUINO_AVR_UNO
        pub(subjectGTWRFtoMQTT, "{\"Status\": \"Error\"}");
# endif
        Log.error(F("MQTTtoRF Fail json" CR));
      }
    }
    enableActiveReceiver(false);
  }
}
# endif

int receiveInterupt = -1;

void disableRFReceive() {
  Log.trace(F("disableRFReceive %d" CR), receiveInterupt);
  if (receiveInterupt != -1) {
    receiveInterupt = -1;
    mySwitch.disableReceive();
  }
}

void enableRFReceive() {
# ifdef ZradioCC1101
  Log.notice(F("Switching to RF Receiver: %F" CR), receiveMhz);
# else
  Log.notice(F("Switching to RF Receiver" CR));
# endif
# ifndef ARDUINO_AVR_UNO
# ifdef ZgatewayPilight
  disablePilightReceive();
# endif
# ifdef ZgatewayRTL_433
  disableRTLreceive();
# endif
# endif
# ifdef ZgatewayRF2
  disableRF2Receive();
# endif

# ifdef ZradioCC1101
  ELECHOUSE_cc1101.SetRx(receiveMhz);
# endif
  mySwitch.disableTransmit();
  receiveInterupt = RF_RECEIVER_GPIO;
  mySwitch.enableReceive(RF_RECEIVER_GPIO);
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRF2.ino"
# 36 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRF2.ino"
#include "User_config.h"

#ifdef ZgatewayRF2

# ifdef ZradioCC1101
# include <ELECHOUSE_CC1101_SRC_DRV.h>
# endif

# include <NewRemoteReceiver.h>
# include <NewRemoteTransmitter.h>

struct RF2rxd {
  unsigned int period;
  unsigned long address;
  unsigned long groupBit;
  unsigned long unit;
  unsigned long switchType;
  bool hasNewData;
};

RF2rxd rf2rd;

void setupRF2() {
# ifdef ZradioCC1101
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(receiveMhz);
  ELECHOUSE_cc1101.SetRx(receiveMhz);
# endif
  NewRemoteReceiver::init(RF_RECEIVER_GPIO, 2, rf2Callback);
  Log.notice(F("RF_EMITTER_GPIO: %d " CR), RF_EMITTER_GPIO);
  Log.notice(F("RF_RECEIVER_GPIO: %d " CR), RF_RECEIVER_GPIO);
  Log.trace(F("ZgatewayRF2 command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoRF2);
  Log.trace(F("ZgatewayRF2 setup done " CR));
  pinMode(RF_EMITTER_GPIO, OUTPUT);
  digitalWrite(RF_EMITTER_GPIO, LOW);
}

# ifdef ZmqttDiscovery

void RF2toMQTTdiscovery(JsonObject& data) {
  Log.trace(F("switchRF2Discovery" CR));
  String payloadonstr;
  String payloadoffstr;

  int org_switchtype = data["switchType"];
  data["switchType"] = 1;
  serializeJson(data, payloadonstr);
  data["switchType"] = 0;
  serializeJson(data, payloadoffstr);
  data["switchType"] = org_switchtype;

  String switchname;
  switchname = "RF2_" + String((int)data["unit"]) + "_" +
               String((int)data["groupbit"]) + "_" +
               String((unsigned long)data["address"]);

  char* switchRF[8] = {"switch",
                       (char*)switchname.c_str(),
                       "",
                       "",
                       "",
                       (char*)payloadonstr.c_str(),
                       (char*)payloadoffstr.c_str(),
                       ""};



  Log.trace(F("CreateDiscoverySwitch: %s" CR), switchRF[1]);






  createDiscovery(switchRF[0], "", switchRF[1],
                  (char*)getUniqueId(switchRF[1], "").c_str(), will_Topic,
                  switchRF[3], switchRF[4], switchRF[5], switchRF[6],
                  switchRF[7], 0, "", "", true, subjectMQTTtoRF2,
                  "", "", "", "", false,
                  stateClassNone);
}
# endif

void RF2toMQTT() {
  if (rf2rd.hasNewData) {
    Log.trace(F("Creating RF2 buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RF2data = jsonBuffer.to<JsonObject>();

    rf2rd.hasNewData = false;

    Log.trace(F("Rcv. RF2" CR));
    RF2data["unit"] = (int)rf2rd.unit;
    RF2data["groupBit"] = (int)rf2rd.groupBit;
    RF2data["period"] = (int)rf2rd.period;
    RF2data["address"] = (unsigned long)rf2rd.address;
    RF2data["switchType"] = (int)rf2rd.switchType;
# ifdef ZmqttDiscovery
    if (disc)
      RF2toMQTTdiscovery(RF2data);
# endif

    pub(subjectRF2toMQTT, RF2data);
  }
}

void rf2Callback(unsigned int period, unsigned long address, unsigned long groupBit, unsigned long unit, unsigned long switchType) {
  rf2rd.period = period;
  rf2rd.address = address;
  rf2rd.groupBit = groupBit;
  rf2rd.unit = unit;
  rf2rd.switchType = switchType;
  rf2rd.hasNewData = true;
}

# if simpleReceiving
void MQTTtoRF2(char* topicOri, char* datacallback) {
# ifdef ZradioCC1101
  NewRemoteReceiver::disable();
  ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY);
# endif



  String topic = topicOri;
  bool boolSWITCHTYPE;
  boolSWITCHTYPE = to_bool(datacallback);
  bool isDimCommand = false;

  long valueCODE = 0;
  int valueUNIT = -1;
  int valuePERIOD = 0;
  int valueGROUP = 0;
  int valueDIM = -1;

  int pos = topic.lastIndexOf(RF2codeKey);
  if (pos != -1) {
    pos = pos + +strlen(RF2codeKey);
    valueCODE = (topic.substring(pos, pos + 8)).toInt();
    Log.notice(F("RF2 code: %l" CR), valueCODE);
  }
  int pos2 = topic.lastIndexOf(RF2periodKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RF2periodKey);
    valuePERIOD = (topic.substring(pos2, pos2 + 3)).toInt();
    Log.notice(F("RF2 Period: %d" CR), valuePERIOD);
  }
  int pos3 = topic.lastIndexOf(RF2unitKey);
  if (pos3 != -1) {
    pos3 = pos3 + strlen(RF2unitKey);
    valueUNIT = (topic.substring(pos3, topic.indexOf("/", pos3))).toInt();
    Log.notice(F("Unit: %d" CR), valueUNIT);
  }
  int pos4 = topic.lastIndexOf(RF2groupKey);
  if (pos4 != -1) {
    pos4 = pos4 + strlen(RF2groupKey);
    valueGROUP = (topic.substring(pos4, pos4 + 1)).toInt();
    Log.notice(F("RF2 Group: %d" CR), valueGROUP);
  }
  int pos5 = topic.lastIndexOf(RF2dimKey);
  if (pos5 != -1) {
    isDimCommand = true;
    valueDIM = atoi(datacallback);
    Log.notice(F("RF2 Dim: %d" CR), valueDIM);
  }

  if ((topic == subjectMQTTtoRF2) || (valueCODE != 0) || (valueUNIT != -1) || (valuePERIOD != 0)) {
    Log.trace(F("MQTTtoRF2" CR));
    if (valueCODE == 0)
      valueCODE = 8233378;
    if (valueUNIT == -1)
      valueUNIT = 0;
    if (valuePERIOD == 0)
      valuePERIOD = 272;
    NewRemoteReceiver::disable();
    Log.trace(F("Creating transmitter" CR));
    NewRemoteTransmitter transmitter(valueCODE, RF_EMITTER_GPIO, valuePERIOD, RF2_EMITTER_REPEAT);
    Log.trace(F("Sending data" CR));
    if (valueGROUP) {
      if (isDimCommand) {
        transmitter.sendGroupDim(valueDIM);
      } else {
        transmitter.sendGroup(boolSWITCHTYPE);
      }
    } else {
      if (isDimCommand) {
        transmitter.sendDim(valueUNIT, valueDIM);
      } else {
        transmitter.sendUnit(valueUNIT, boolSWITCHTYPE);
      }
    }
    Log.trace(F("Data sent" CR));
    NewRemoteReceiver::enable();


    String MQTTAddress;
    String MQTTperiod;
    String MQTTunit;
    String MQTTgroupBit;
    String MQTTswitchType;
    String MQTTdimLevel;

    MQTTAddress = String(valueCODE);
    MQTTperiod = String(valuePERIOD);
    MQTTunit = String(valueUNIT);
    MQTTgroupBit = String(rf2rd.groupBit);
    MQTTswitchType = String(boolSWITCHTYPE);
    MQTTdimLevel = String(valueDIM);
    String MQTTRF2string;
    Log.trace(F("Adv data MQTTtoRF2 push state via RF2toMQTT" CR));
    if (isDimCommand) {
      MQTTRF2string = subjectRF2toMQTT + String("/") + RF2codeKey + MQTTAddress + String("/") + RF2unitKey + MQTTunit + String("/") + RF2groupKey + MQTTgroupBit + String("/") + RF2dimKey + String("/") + RF2periodKey + MQTTperiod;
      pub((char*)MQTTRF2string.c_str(), (char*)MQTTdimLevel.c_str());
    } else {
      MQTTRF2string = subjectRF2toMQTT + String("/") + RF2codeKey + MQTTAddress + String("/") + RF2unitKey + MQTTunit + String("/") + RF2groupKey + MQTTgroupBit + String("/") + RF2periodKey + MQTTperiod;
      pub((char*)MQTTRF2string.c_str(), (char*)MQTTswitchType.c_str());
    }
  }
# ifdef ZradioCC1101
  ELECHOUSE_cc1101.SetRx(receiveMhz);
  NewRemoteReceiver::enable();
# endif
}
# endif

# if jsonReceiving
void MQTTtoRF2(char* topicOri, JsonObject& RF2data) {

  if (cmpToMainTopic(topicOri, subjectMQTTtoRF2)) {
    Log.trace(F("MQTTtoRF2 json" CR));
    int boolSWITCHTYPE = RF2data["switchType"] | 99;
    bool success = false;
    if (boolSWITCHTYPE != 99) {
# ifdef ZradioCC1101
      NewRemoteReceiver::disable();
      ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY);
# endif
      Log.trace(F("MQTTtoRF2 switch type ok" CR));
      bool isDimCommand = boolSWITCHTYPE == 2;
      unsigned long valueCODE = RF2data["address"];
      int valueUNIT = RF2data["unit"] | -1;
      int valuePERIOD = RF2data["period"];
      int valueGROUP = RF2data["group"];
      int valueDIM = RF2data["dim"] | -1;
      if ((valueCODE != 0) || (valueUNIT != -1) || (valuePERIOD != 0)) {
        Log.trace(F("MQTTtoRF2" CR));
        if (valueCODE == 0)
          valueCODE = 8233378;
        if (valueUNIT == -1)
          valueUNIT = 0;
        if (valuePERIOD == 0)
          valuePERIOD = 272;
        NewRemoteReceiver::disable();
        NewRemoteTransmitter transmitter(valueCODE, RF_EMITTER_GPIO, valuePERIOD, RF2_EMITTER_REPEAT);
        Log.trace(F("Sending" CR));
        if (valueGROUP) {
          if (isDimCommand) {
            transmitter.sendGroupDim(valueDIM);
          } else {
            transmitter.sendGroup(boolSWITCHTYPE);
          }
        } else {
          if (isDimCommand) {
            transmitter.sendDim(valueUNIT, valueDIM);
          } else {
            transmitter.sendUnit(valueUNIT, boolSWITCHTYPE);
          }
        }
        Log.notice(F("MQTTtoRF2 OK" CR));
        NewRemoteReceiver::enable();

        success = true;
      }
    }
    if (RF2data.containsKey("active")) {
      Log.trace(F("RF2 active:" CR));
      activeReceiver = ACTIVE_RF2;
      success = true;
    }
# ifdef ZradioCC1101
    float tempMhz = RF2data["mhz"];
    if (RF2data.containsKey("mhz") && validFrequency(tempMhz)) {
      receiveMhz = tempMhz;
      Log.notice(F("Receive mhz: %F" CR), receiveMhz);
      success = true;
    }
# endif
    if (success) {
      pub(subjectGTWRF2toMQTT, RF2data);
    } else {
# ifndef ARDUINO_AVR_UNO
      pub(subjectGTWRF2toMQTT, "{\"Status\": \"Error\"}");
# endif
      Log.error(F("MQTTtoRF2 failed json read" CR));
    }
    enableActiveReceiver(false);
  }
}
# endif

void disableRF2Receive() {
  Log.trace(F("disableRF2Receive" CR));
  NewRemoteReceiver::deinit();
  NewRemoteReceiver::init(-1, 2, rf2Callback);
  NewRemoteReceiver::deinit();
}

void enableRF2Receive() {
# ifdef ZradioCC1101
  Log.notice(F("Switching to RF2 Receiver: %F" CR), receiveMhz);
# else
  Log.notice(F("Switching to RF2 Receiver" CR));
# endif
# ifdef ZgatewayPilight
  disablePilightReceive();
# endif
# ifdef ZgatewayRTL_433
  disableRTLreceive();
# endif
# ifdef ZgatewayRF
  disableRFReceive();
# endif

  NewRemoteReceiver::init(RF_RECEIVER_GPIO, 2, rf2Callback);
# ifdef ZradioCC1101
  ELECHOUSE_cc1101.SetRx(receiveMhz);
# endif
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRFM69.ino"
# 31 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRFM69.ino"
#include "User_config.h"

#ifdef ZgatewayRFM69

# include <EEPROM.h>
# include <RFM69.h>

char RadioConfig[128];



struct _GLOBAL_CONFIG {
  uint32_t checksum;
  char rfmapname[32];
  char encryptkey[16 + 1];
  uint8_t networkid;
  uint8_t nodeid;
  uint8_t powerlevel;
  uint8_t rfmfrequency;
};

#define GC_POWER_LEVEL (pGC->powerlevel & 0x1F)
#define GC_IS_RFM69HCW ((pGC->powerlevel & 0x80) != 0)
#define SELECTED_FREQ(f) ((pGC->rfmfrequency == f) ? "selected" : "")

struct _GLOBAL_CONFIG* pGC;


uint32_t gc_checksum() {
  uint8_t* p = (uint8_t*)pGC;
  uint32_t checksum = 0;
  p += sizeof(pGC->checksum);
  for (size_t i = 0; i < (sizeof(*pGC) - 4); i++) {
    checksum += *p++;
  }
  return checksum;
}

# if defined(ESP8266) || defined(ESP32)
void eeprom_setup() {
  EEPROM.begin(4096);
  pGC = (struct _GLOBAL_CONFIG*)EEPROM.getDataPtr();

  if (gc_checksum() != pGC->checksum) {
    Log.trace(F("Factory reset" CR));
    memset(pGC, 0, sizeof(*pGC));
    strcpy_P(pGC->encryptkey, ENCRYPTKEY);
    strcpy_P(pGC->rfmapname, RFM69AP_NAME);
    pGC->networkid = NETWORKID;
    pGC->nodeid = NODEID;
    pGC->powerlevel = ((IS_RFM69HCW) ? 0x80 : 0x00) | POWER_LEVEL;
    pGC->rfmfrequency = FREQUENCY;
    pGC->checksum = gc_checksum();
    EEPROM.commit();
  }
}
# endif

RFM69 radio;

void setupRFM69(void) {
# if defined(ESP8266) || defined(ESP32)
  eeprom_setup();
# endif
  int freq;
  static const char PROGMEM JSONtemplate[] =
      R"({"msgType":"config","freq":%d,"rfm69hcw":%d,"netid":%d,"power":%d})";
  char payload[128];

  radio = RFM69(RFM69_CS, RFM69_IRQ, GC_IS_RFM69HCW, RFM69_IRQN);


  if (!radio.initialize(pGC->rfmfrequency, pGC->nodeid, pGC->networkid)) {
    Log.error(F("ZgatewayRFM69 initialization failed" CR));
  }

  if (GC_IS_RFM69HCW) {
    radio.setHighPower();
  }
  radio.setPowerLevel(GC_POWER_LEVEL);

  if (pGC->encryptkey[0] != '\0')
    radio.encrypt(pGC->encryptkey);

  switch (pGC->rfmfrequency) {
    case RF69_433MHZ:
      freq = 433;
      break;
    case RF69_868MHZ:
      freq = 868;
      break;
    case RF69_915MHZ:
      freq = 915;
      break;
    case RF69_315MHZ:
      freq = 315;
      break;
    default:
      freq = -1;
      break;
  }
  Log.notice(F("ZgatewayRFM69 Listening and transmitting at: %d" CR), freq);

  size_t len = snprintf_P(RadioConfig, sizeof(RadioConfig), JSONtemplate,
                          freq, GC_IS_RFM69HCW, pGC->networkid, GC_POWER_LEVEL);
  if (len >= sizeof(RadioConfig)) {
    Log.trace(F("\n\n*** RFM69 config truncated ***\n" CR));
  }
}

bool RFM69toMQTT(void) {

  if (radio.receiveDone()) {
    Log.trace(F("Creating RFM69 buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RFM69data = jsonBuffer.to<JsonObject>();
    uint8_t data[RF69_MAX_DATA_LEN + 1];
    uint8_t SENDERID = radio.SENDERID;
    uint8_t DATALEN = radio.DATALEN;
    uint16_t RSSI = radio.RSSI;


    memcpy(data, (void*)radio.DATA, DATALEN);
    data[DATALEN] = '\0';



    if (radio.ACKRequested()) {
      radio.sendACK();
    }


    Log.trace(F("Data received: %s" CR), (const char*)data);

    char buff[sizeof(subjectRFM69toMQTT) + 4];
    sprintf(buff, "%s/%d", subjectRFM69toMQTT, SENDERID);
    RFM69data["data"] = (char*)data;
    RFM69data["rssi"] = (int)radio.RSSI;
    RFM69data["senderid"] = (int)radio.SENDERID;
    pub(buff, RFM69data);

    return true;
  } else {
    return false;
  }
}

# if simpleReceiving
void MQTTtoRFM69(char* topicOri, char* datacallback) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRFM69)) {
    Log.trace(F("MQTTtoRFM69 data analysis" CR));
    char data[RF69_MAX_DATA_LEN + 1];
    memcpy(data, (void*)datacallback, RF69_MAX_DATA_LEN);
    data[RF69_MAX_DATA_LEN] = '\0';


    String topic = topicOri;
    int valueRCV = defaultRFM69ReceiverId;
    int pos = topic.lastIndexOf(RFM69receiverKey);
    if (pos != -1) {
      pos = pos + +strlen(RFM69receiverKey);
      valueRCV = (topic.substring(pos, pos + 3)).toInt();
      Log.notice(F("RFM69 receiver ID: %d" CR), valueRCV);
    }
    if (radio.sendWithRetry(valueRCV, data, strlen(data), 10)) {
      Log.notice(F(" OK " CR));

      char buff[sizeof(subjectGTWRFM69toMQTT) + 4];
      sprintf(buff, "%s/%d", subjectGTWRFM69toMQTT, radio.SENDERID);
      pub(buff, data);
    } else {
      Log.error(F("RFM69 sending failed" CR));
    }
  }
}
# endif
# if jsonReceiving
void MQTTtoRFM69(char* topicOri, JsonObject& RFM69data) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRFM69)) {
    const char* data = RFM69data["data"];
    Log.trace(F("MQTTtoRFM69 json data analysis" CR));
    if (data) {
      Log.trace(F("MQTTtoRFM69 data ok" CR));
      int valueRCV = RFM69data["receiverid"] | defaultRFM69ReceiverId;
      Log.notice(F("RFM69 receiver ID: %d" CR), valueRCV);
      if (radio.sendWithRetry(valueRCV, data, strlen(data), 10)) {
        Log.notice(F(" OK " CR));

        pub(subjectGTWRFM69toMQTT, RFM69data);
      } else {
        Log.error(F("MQTTtoRFM69 sending failed" CR));
      }
    } else {
      Log.error(F("MQTTtoRFM69 failed json read" CR));
    }
  }
}
# endif
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRS232.ino"
# 28 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRS232.ino"
#include "User_config.h"

#ifdef ZgatewayRS232

# include <SoftwareSerial.h>

SoftwareSerial RS232Serial(RS232_RX_GPIO, RS232_TX_GPIO);

void setupRS232() {

  pinMode(RS232_RX_GPIO, INPUT);
  pinMode(RS232_TX_GPIO, OUTPUT);

  RS232Serial.begin(RS232Baud);

  Log.notice(F("RS232_RX_GPIO: %d " CR), RS232_RX_GPIO);
  Log.notice(F("RS232_TX_GPIO: %d " CR), RS232_TX_GPIO);
  Log.notice(F("RS232Baud: %d " CR), RS232Baud);
  Log.trace(F("ZgatewayRS232 setup done " CR));
}

void RS232toMQTT() {

  if (RS232Serial.available()) {
    Log.trace(F("RS232toMQTT" CR));
    static char RS232data[MAX_INPUT];
    static unsigned int input_pos = 0;
    static char inChar;
    do {
      if (RS232Serial.available()) {
        inChar = RS232Serial.read();
        RS232data[input_pos] = inChar;
        input_pos++;
        Log.trace(F("Received %c" CR), inChar);
      }
    } while (inChar != RS232InPost && input_pos < MAX_INPUT);
    RS232data[input_pos] = 0;
    input_pos = 0;
    Log.trace(F("Publish %s" CR), RS232data);
    char* output = RS232data + sizeof(RS232Pre) - 1;
    pub(subjectRS232toMQTT, output);
  }
}

void MQTTtoRS232(char* topicOri, JsonObject& RS232data) {
  Log.trace(F("json" CR));
  if (cmpToMainTopic(topicOri, subjectMQTTtoRS232)) {
    Log.trace(F("MQTTtoRS232 json" CR));
    const char* data = RS232data["value"];
    const char* prefix = RS232data["prefix"] | RS232Pre;
    const char* postfix = RS232data["postfix"] | RS232Post;
    Log.trace(F("Value set: %s" CR), data);
    Log.trace(F("Prefix set: %s" CR), prefix);
    Log.trace(F("Postfix set: %s" CR), postfix);
    RS232Serial.print(prefix);
    RS232Serial.print(data);
    RS232Serial.print(postfix);
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRTL_433.ino"
# 29 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayRTL_433.ino"
#include "User_config.h"
#ifdef ZgatewayRTL_433
# ifndef ZradioCC1101
# error "CC1101 is the only supported receiver module for RTL_433 and needs to be enabled."
# endif

# include <rtl_433_ESP.h>

char messageBuffer[JSON_MSG_BUFFER];

rtl_433_ESP rtl_433(-1);

# include <ELECHOUSE_CC1101_SRC_DRV.h>

void rtl_433_Callback(char* message) {
  DynamicJsonDocument jsonBuffer2(JSON_MSG_BUFFER);
  JsonObject RFrtl_433_ESPdata = jsonBuffer2.to<JsonObject>();
  auto error = deserializeJson(jsonBuffer2, message);
  if (error) {
    Log.error(F("deserializeJson() failed: %s" CR), error.c_str());
    return;
  }

  String topic = String(subjectRTL_433toMQTT);
# if valueAsATopic
  String model = RFrtl_433_ESPdata["model"];
  String id = RFrtl_433_ESPdata["id"];
  if (model != 0) {
    topic = topic + "/" + model;
    if (id != 0) {
      topic = topic + "/" + id;
    }
  }
# endif

  pub((char*)topic.c_str(), RFrtl_433_ESPdata);
# ifdef MEMORY_DEBUG
  Log.trace(F("Post rtl_433_Callback: %d" CR), ESP.getFreeHeap());
# endif
}

void setupRTL_433() {
  rtl_433.initReceiver(RF_RECEIVER_GPIO, receiveMhz);
  rtl_433.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);
  Log.trace(F("ZgatewayRTL_433 command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoRTL_433);
  Log.notice(F("ZgatewayRTL_433 setup done " CR));
}

void RTL_433Loop() {
  rtl_433.loop();
}

extern void MQTTtoRTL_433(char* topicOri, JsonObject& RTLdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRTL_433)) {
    float tempMhz = RTLdata["mhz"];
    bool success = false;
    if (RTLdata.containsKey("mhz") && validFrequency(tempMhz)) {
      receiveMhz = tempMhz;
      Log.notice(F("RTL_433 Receive mhz: %F" CR), receiveMhz);
      success = true;
    }
    if (RTLdata.containsKey("active")) {
      Log.trace(F("RTL_433 active:" CR));
      activeReceiver = ACTIVE_RTL;
      success = true;
    }
    if (RTLdata.containsKey("rssi")) {
      int minimumRssi = RTLdata["rssi"] | 0;
      Log.notice(F("RTL_433 minimum RSSI: %d" CR), minimumRssi);
      rtl_433.setMinimumRSSI(minimumRssi);
      success = true;
    }
    if (RTLdata.containsKey("debug")) {
      int debug = RTLdata["debug"] | -1;
      Log.notice(F("RTL_433 set debug: %d" CR), debug);
      rtl_433.setDebug(debug);
      rtl_433.initReceiver(RF_RECEIVER_GPIO, receiveMhz);
      success = true;
    }
    if (RTLdata.containsKey("status")) {
      Log.notice(F("RTL_433 get status:" CR));
      rtl_433.getStatus(1);
      success = true;
    }
    if (success) {
      pub(subjectRTL_433toMQTT, RTLdata);
    } else {
      pub(subjectRTL_433toMQTT, "{\"Status\": \"Error\"}");
      Log.error(F("MQTTtoRTL_433 Fail json" CR));
    }
    enableActiveReceiver(false);
  }
}

extern void enableRTLreceive() {
  Log.notice(F("Switching to RTL_433 Receiver: %FMhz" CR), receiveMhz);
# ifdef ZgatewayRF
  disableRFReceive();
# endif
# ifdef ZgatewayRF2
  disableRF2Receive();
# endif
# ifdef ZgatewayPilight
  disablePilightReceive();
# endif
  ELECHOUSE_cc1101.SetRx(receiveMhz);
  rtl_433.enableReceiver(RF_RECEIVER_GPIO);
  pinMode(RF_EMITTER_GPIO, OUTPUT);
}

extern void disableRTLreceive() {
  Log.trace(F("disableRTLreceive" CR));
  rtl_433.enableReceiver(-1);
  rtl_433.disableReceiver();
}

extern int getRTLMinimumRSSI() {
  return rtl_433.minimumRssi;
}

extern int getRTLCurrentRSSI() {
  return rtl_433.currentRssi;
}

extern int getRTLMessageCount() {
  return rtl_433.messageCount;
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewaySRFB.ino"
# 31 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewaySRFB.ino"
#include "User_config.h"

#ifdef ZgatewaySRFB

unsigned char _uartbuf[RF_MESSAGE_SIZE + 3] = {0};
unsigned char _uartpos = 0;

void setupSRFB() {
  Log.trace(F("ZgatewaySRFB setup done " CR));
  Log.trace(F("Serial Baud: %l" CR), SERIAL_BAUD);
}

void _rfbSend(byte* message) {
  Serial.println();
  Serial.write(RF_CODE_START);
  Serial.write(RF_CODE_RFOUT);
  for (unsigned char j = 0; j < RF_MESSAGE_SIZE; j++) {
    Serial.write(message[j]);
  }
  Serial.write(RF_CODE_STOP);
  Serial.flush();
  Serial.println();
}

void _rfbSend(byte* message, int times) {
  char buffer[RF_MESSAGE_SIZE];
  _rfbToChar(message, buffer);
  Log.notice(F("[RFBRIDGE] Sending MESSAGE" CR));

  for (int i = 0; i < times; i++) {
    if (i > 0) {
      unsigned long start = millis();
      while (millis() - start < RF_SEND_DELAY)
        delay(1);
    }
    _rfbSend(message);
  }
}

bool SRFBtoMQTT() {
  static bool receiving = false;

  while (Serial.available()) {
    yield();
    byte c = Serial.read();

    if (receiving) {
      if (c == RF_CODE_STOP) {
        _rfbDecode();
        receiving = false;
      } else {
        _uartbuf[_uartpos++] = c;
      }
    } else if (c == RF_CODE_START) {
      _uartpos = 0;
      receiving = true;
    }
  }
  return receiving;
}

void _rfbDecode() {
  static unsigned long last = 0;
  if (millis() - last < RF_RECEIVE_DELAY)
    return;
  last = millis();

  byte action = _uartbuf[0];
  char buffer[RF_MESSAGE_SIZE * 2 + 1] = {0};

  if (action == RF_CODE_RFIN) {
    _rfbToChar(&_uartbuf[1], buffer);

    Log.trace(F("Creating SRFB buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject SRFBdata = jsonBuffer.to<JsonObject>();
    SRFBdata["raw"] = String(buffer).substring(0, 18);

    int val_Tsyn = (int)(int)value_from_hex_data(buffer, 0, 4, false, false);
    SRFBdata["delay"] = (int)val_Tsyn;

    int val_Tlow = (int)value_from_hex_data(buffer, 4, 4, false, false);
    SRFBdata["val_Tlow"] = (int)val_Tlow;

    int val_Thigh = (int)value_from_hex_data(buffer, 8, 4, false, false);
    SRFBdata["val_Thigh"] = (int)val_Thigh;

    unsigned long MQTTvalue = (unsigned long)value_from_hex_data(buffer, 12, 8, false, false);
    SRFBdata["value"] = (unsigned long)MQTTvalue;

    if (!isAduplicateSignal(MQTTvalue) && MQTTvalue != 0) {
      Log.trace(F("Adv data SRFBtoMQTT" CR));
      pub(subjectSRFBtoMQTT, SRFBdata);
      Log.trace(F("Store val: %lu" CR), MQTTvalue);
      storeSignalValue(MQTTvalue);
      if (repeatSRFBwMQTT) {
        Log.trace(F("Publish SRFB for rpt" CR));
        pub(subjectMQTTtoSRFB, SRFBdata);
      }
    }
    _rfbAck();
  }
}

void _rfbAck() {
  Log.trace(F("[RFBRIDGE] Sending ACK\n" CR));
  Serial.println();
  Serial.write(RF_CODE_START);
  Serial.write(RF_CODE_ACK);
  Serial.write(RF_CODE_STOP);
  Serial.flush();
  Serial.println();
}




bool _rfbToArray(const char* in, byte* out) {
  if (strlen(in) != RF_MESSAGE_SIZE * 2)
    return false;
  char tmp[3] = {0};
  for (unsigned char p = 0; p < RF_MESSAGE_SIZE; p++) {
    memcpy(tmp, &in[p * 2], 2);
    out[p] = strtol(tmp, NULL, 16);
  }
  return true;
}




bool _rfbToChar(byte* in, char* out) {
  for (unsigned char p = 0; p < RF_MESSAGE_SIZE; p++) {
    sprintf_P(&out[p * 2], PSTR("%02X" CR), in[p]);
  }
  return true;
}

# if simpleReceiving
void MQTTtoSRFB(char* topicOri, char* datacallback) {

  String topic = topicOri;
  int valueRPT = 0;

  if (topic == subjectMQTTtoSRFB) {
    int valueMiniPLSL = 0;
    int valueMaxiPLSL = 0;
    int valueSYNC = 0;

    int pos = topic.lastIndexOf(SRFBRptKey);
    if (pos != -1) {
      pos = pos + +strlen(SRFBRptKey);
      valueRPT = (topic.substring(pos, pos + 1)).toInt();
      Log.notice(F("SRFB Repeat: %d" CR), valueRPT);
    }

    int pos2 = topic.lastIndexOf(SRFBminipulselengthKey);
    if (pos2 != -1) {
      pos2 = pos2 + strlen(SRFBminipulselengthKey);
      valueMiniPLSL = (topic.substring(pos2, pos2 + 3)).toInt();
      Log.notice(F("RF Mini Pulse Lgth: %d" CR), valueMiniPLSL);
    }

    int pos3 = topic.lastIndexOf(SRFBmaxipulselengthKey);
    if (pos3 != -1) {
      pos3 = pos3 + strlen(SRFBmaxipulselengthKey);
      valueMaxiPLSL = (topic.substring(pos3, pos3 + 2)).toInt();
      Log.notice(F("RF Maxi Pulse Lgth: %d" CR), valueMaxiPLSL);
    }

    int pos4 = topic.lastIndexOf(SRFBsyncKey);
    if (pos4 != -1) {
      pos4 = pos4 + strlen(SRFBsyncKey);
      valueSYNC = (topic.substring(pos4, pos4 + 2)).toInt();
      Log.notice(F("RF sync: %d" CR), valueSYNC);
    }

    Log.trace(F("MQTTtoSRFB prts" CR));
    if (valueRPT == 0)
      valueRPT = 1;
    if (valueMiniPLSL == 0)
      valueMiniPLSL = 320;
    if (valueMaxiPLSL == 0)
      valueMaxiPLSL = 900;
    if (valueSYNC == 0)
      valueSYNC = 9500;

    byte hex_valueMiniPLSL[2];
    hex_valueMiniPLSL[0] = (int)((valueMiniPLSL >> 8) & 0xFF);
    hex_valueMiniPLSL[1] = (int)(valueMiniPLSL & 0xFF);

    byte hex_valueMaxiPLSL[2];
    hex_valueMaxiPLSL[0] = (int)((valueMaxiPLSL >> 8) & 0xFF);
    hex_valueMaxiPLSL[1] = (int)(valueMaxiPLSL & 0xFF);

    byte hex_valueSYNC[2];
    hex_valueSYNC[0] = (int)((valueSYNC >> 8) & 0xFF);
    hex_valueSYNC[1] = (int)(valueSYNC & 0xFF);

    unsigned long data = strtoul(datacallback, NULL, 10);
    byte hex_data[3];
    hex_data[0] = (unsigned long)((data >> 16) & 0xFF);
    hex_data[1] = (unsigned long)((data >> 8) & 0xFF);
    hex_data[2] = (unsigned long)(data & 0xFF);

    byte message_b[RF_MESSAGE_SIZE];

    memcpy(message_b, hex_valueSYNC, 2);
    memcpy(message_b + 2, hex_valueMiniPLSL, 2);
    memcpy(message_b + 4, hex_valueMaxiPLSL, 2);
    memcpy(message_b + 6, hex_data, 3);

    _rfbSend(message_b, valueRPT);

    pub(subjectGTWSRFBtoMQTT, datacallback);
  }
  if (topic == subjectMQTTtoSRFBRaw) {
    int pos = topic.lastIndexOf(SRFBRptKey);
    if (pos != -1) {
      pos = pos + +strlen(SRFBRptKey);
      valueRPT = (topic.substring(pos, pos + 1)).toInt();
      Log.notice(F("SRFB Repeat: %d" CR), valueRPT);
    }
    if (valueRPT == 0)
      valueRPT = 1;

    byte message_b[RF_MESSAGE_SIZE];
    _rfbToArray(datacallback, message_b);
    _rfbSend(message_b, valueRPT);

    pub(subjectGTWSRFBtoMQTT, datacallback);
  }
}
# endif
# if jsonReceiving
void MQTTtoSRFB(char* topicOri, JsonObject& SRFBdata) {

  const char* raw = SRFBdata["raw"];
  int valueRPT = SRFBdata["repeat"] | 1;
  if (cmpToMainTopic(topicOri, subjectMQTTtoSRFB)) {
    Log.trace(F("MQTTtoSRFB json" CR));
    if (raw) {
      Log.trace(F("MQTTtoSRFB raw ok" CR));
      byte message_b[RF_MESSAGE_SIZE];
      _rfbToArray(raw, message_b);
      _rfbSend(message_b, valueRPT);
    } else {
      unsigned long data = SRFBdata["value"];
      if (data != 0) {
        Log.notice(F("MQTTtoSRFB data ok" CR));
        int valueMiniPLSL = SRFBdata["val_Tlow"];
        int valueMaxiPLSL = SRFBdata["val_Thigh"];
        int valueSYNC = SRFBdata["delay"];

        if (valueRPT == 0)
          valueRPT = 1;
        if (valueMiniPLSL == 0)
          valueMiniPLSL = 320;
        if (valueMaxiPLSL == 0)
          valueMaxiPLSL = 900;
        if (valueSYNC == 0)
          valueSYNC = 9500;

        Log.notice(F("SRFB Repeat: %d" CR), valueRPT);
        Log.notice(F("RF Mini Pulse Lgth: %d" CR), valueMiniPLSL);
        Log.notice(F("RF Maxi Pulse Lgth: %d" CR), valueMaxiPLSL);
        Log.notice(F("RF sync: %d" CR), valueSYNC);

        byte hex_valueMiniPLSL[2];
        hex_valueMiniPLSL[0] = (int)((valueMiniPLSL >> 8) & 0xFF);
        hex_valueMiniPLSL[1] = (int)(valueMiniPLSL & 0xFF);

        byte hex_valueMaxiPLSL[2];
        hex_valueMaxiPLSL[0] = (int)((valueMaxiPLSL >> 8) & 0xFF);
        hex_valueMaxiPLSL[1] = (int)(valueMaxiPLSL & 0xFF);

        byte hex_valueSYNC[2];
        hex_valueSYNC[0] = (int)((valueSYNC >> 8) & 0xFF);
        hex_valueSYNC[1] = (int)(valueSYNC & 0xFF);

        byte hex_data[3];
        hex_data[0] = (unsigned long)((data >> 16) & 0xFF);
        hex_data[1] = (unsigned long)((data >> 8) & 0xFF);
        hex_data[2] = (unsigned long)(data & 0xFF);

        byte message_b[RF_MESSAGE_SIZE];

        memcpy(message_b, hex_valueSYNC, 2);
        memcpy(message_b + 2, hex_valueMiniPLSL, 2);
        memcpy(message_b + 4, hex_valueMaxiPLSL, 2);
        memcpy(message_b + 6, hex_data, 3);

        Log.notice(F("MQTTtoSRFB OK" CR));
        _rfbSend(message_b, valueRPT);

        pub(subjectGTWSRFBtoMQTT, SRFBdata);
      } else {
        Log.error(F("MQTTtoSRFB error decoding value" CR));
      }
    }
  }
}
# endif
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayWeatherStation.ino"
# 26 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZgatewayWeatherStation.ino"
#include "User_config.h"

#ifdef ZgatewayWeatherStation
# include <WeatherStationDataRx.h>
WeatherStationDataRx wsdr(RF_WS_RECEIVER_GPIO, true);

void PairedDeviceAdded(byte newID) {
# if defined(ESP8266) || defined(ESP32)
  Serial.printf("ZgatewayWeatherStation: New device paired %d\r\n", newID);
# else
  Serial.print("ZgatewayWeatherStation: New device paired ");
  Serial.println(newID, DEC);
# endif
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject RFdata = jsonBuffer.to<JsonObject>();
  RFdata["sensor"] = newID;
  RFdata["action"] = "paired";
  pub(subjectRFtoMQTT, RFdata);
  wsdr.pair(NULL, PairedDeviceAdded);
}

void setupWeatherStation() {
  Log.notice(F("RF_WS_RECEIVER_GPIO %d" CR), RF_WS_RECEIVER_GPIO);
  wsdr.begin();
  wsdr.pair(NULL, PairedDeviceAdded);
  Log.trace(F("ZgatewayWeatherStation setup done " CR));
}

void sendWindSpeedData(byte id, float wind_speed, byte battery_status) {
  unsigned long MQTTvalue = 10000 + round(wind_speed);
  if (!isAduplicateSignal(MQTTvalue)) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RFdata = jsonBuffer.to<JsonObject>();
    RFdata["sensor"] = id;
    RFdata["wind_speed"] = wind_speed;
    RFdata["battery"] = bitRead(battery_status, 0) == 0 ? "OK" : "Low";
    pub(subjectRFtoMQTT, RFdata);
    Log.trace(F("Store wind speed val: %lu" CR), MQTTvalue);
    storeSignalValue(MQTTvalue);
  }
}

void sendRainData(byte id, float rain_volume, byte battery_status) {
  unsigned long MQTTvalue = 11000 + round(rain_volume * 10.0);
  if (!isAduplicateSignal(MQTTvalue)) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RFdata = jsonBuffer.to<JsonObject>();
    RFdata["sensor"] = id;
    RFdata["rain_volume"] = rain_volume;
    RFdata["battery"] = bitRead(battery_status, 1) == 0 ? "OK" : "Low";
    pub(subjectRFtoMQTT, RFdata);
    Log.trace(F("Store rain_volume: %lu" CR), MQTTvalue);
    storeSignalValue(MQTTvalue);
  }
}

void sendWindData(byte id, int wind_direction, float wind_gust, byte battery_status) {
  unsigned long MQTTvalue = 20000 + round(wind_gust * 10.0) + wind_direction;
  if (!isAduplicateSignal(MQTTvalue)) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RFdata = jsonBuffer.to<JsonObject>();
    RFdata["sensor"] = id;
    RFdata["wind_direction"] = wind_direction;
    RFdata["wind_gust"] = wind_gust;
    RFdata["battery"] = bitRead(battery_status, 0) == 0 ? "OK" : "Low";
    pub(subjectRFtoMQTT, RFdata);
    Log.trace(F("Store wind data val: %lu" CR), MQTTvalue);
    storeSignalValue(MQTTvalue);
  }
}

void sendTemperatureData(byte id, float temperature, int humidity, byte battery_status) {
  unsigned long MQTTvalue = 40000 + abs(round(temperature * 100.0)) + humidity;
  if (!isAduplicateSignal(MQTTvalue)) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RFdata = jsonBuffer.to<JsonObject>();
    RFdata["sensor"] = id;
    RFdata["tempc"] = temperature;
    RFdata["tempf"] = wsdr.convertCtoF(temperature);
    RFdata["humidity"] = humidity;
    RFdata["battery"] = bitRead(battery_status, 0) == 0 ? "OK" : "Low";
    pub(subjectRFtoMQTT, RFdata);
    Log.trace(F("Store temp val: %lu" CR), MQTTvalue);
    storeSignalValue(MQTTvalue);
  }
}

void ZgatewayWeatherStationtoMQTT() {
  char newData = wsdr.readData();
  switch (newData) {
    case 'T':
      Log.trace(F("Temperature" CR));
      sendTemperatureData(wsdr.sensorID(), wsdr.readTemperature(), wsdr.readHumidity(), wsdr.batteryStatus());
      break;

    case 'S':
      Log.trace(F("Wind speed" CR));
      sendWindSpeedData(wsdr.sensorID(), wsdr.readWindSpeed(), wsdr.batteryStatus());
      break;

    case 'G':
      Log.trace(F("Wind direction" CR));
      sendWindData(wsdr.sensorID(), wsdr.readWindDirection(), wsdr.readWindGust(), wsdr.batteryStatus());
      break;

    case 'R':
      Log.trace(F("Rain volume" CR));
      sendRainData(wsdr.sensorID(), wsdr.readRainVolume(), wsdr.batteryStatus());
      break;

    default:
      break;
  }
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZmqttDiscovery.ino"
# 26 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZmqttDiscovery.ino"
#include "User_config.h"

#ifdef ZmqttDiscovery

String getMacAddress() {
  uint8_t baseMac[6];
  char baseMacChr[13] = {0};
# if defined(ESP8266)
  WiFi.macAddress(baseMac);
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
# elif defined(ESP32)
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
# else
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
# endif
  return String(baseMacChr);
}

String getUniqueId(String name, String sufix) {
  String uniqueId = (String)getMacAddress() + "-" + name + sufix;
  return String(uniqueId);
}

# ifdef ZgatewayBT
# 69 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZmqttDiscovery.ino"
void createDiscoveryFromList(const char* mac,
                             const char* sensorList[][9],
                             int sensorCount,
                             const char* device_name,
                             const char* device_manufacturer,
                             const char* device_model) {
  for (int i = 0; i < sensorCount; i++) {
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + sensorList[i][1];

    createDiscovery(sensorList[i][0],
                    discovery_topic.c_str(), sensorList[i][1], unique_id.c_str(),
                    will_Topic, sensorList[i][3], sensorList[i][4],
                    sensorList[i][5], sensorList[i][6], sensorList[i][7],
                    0, "", "", false, "",
                    device_name, device_manufacturer, device_model, mac, false,
                    sensorList[i][8]
    );
  }
}
# endif
# 103 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZmqttDiscovery.ino"
void announceDeviceTrigger(bool use_gateway_info, char* topic, char* type, char* subtype, char* unique_id, char* device_name, char* device_manufacturer, char* device_model, char* device_mac) {

  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject sensor = jsonBuffer.to<JsonObject>();


  sensor["automation_type"] = "trigger";


  if (type[0] != 0) {
    sensor["type"] = type;
  } else {
    sensor["type"] = "button_short_press";
  }


  if (subtype[0] != 0) {
    sensor["subtype"] = subtype;
  } else {
    sensor["subtype"] = "turn_on";
  }


  if (topic[0]) {
    char state_topic[mqtt_topic_max_size];

    strcpy(state_topic, mqtt_topic);
    strcat(state_topic, gateway_name);

    strcat(state_topic, topic);
    sensor["topic"] = state_topic;
  }


  StaticJsonDocument<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject device = jsonDeviceBuffer.to<JsonObject>();
  JsonArray identifiers = device.createNestedArray("identifiers");

  if (use_gateway_info) {
    char JSONmessageBuffer[JSON_MSG_BUFFER];
    serializeJson(modules, JSONmessageBuffer, sizeof(JSONmessageBuffer));

    device["name"] = gateway_name;
    device["model"] = JSONmessageBuffer;
    device["manufacturer"] = DEVICEMANUFACTURER;
    device["sw_version"] = OMG_VERSION;
    identifiers.add(getMacAddress());

  } else {
    char deviceid[13];
    memcpy(deviceid, &unique_id[0], 12);
    deviceid[12] = '\0';

    identifiers.add(deviceid);


    if (device_mac[0] != 0) {
      JsonArray connections = device.createNestedArray("connections");
      JsonArray connection_mac = connections.createNestedArray();
      connection_mac.add("mac");
      connection_mac.add(device_mac);
    }


    if (device_manufacturer[0]) {
      device["manufacturer"] = device_manufacturer;
    }


    if (device_name[0]) {
      device["name"] = device_name;
    }


    if (device_model[0]) {
      device["model"] = device_model;
    }

    device["via_device"] = gateway_name;
  }
  sensor["device"] = device;


  String topic_to_publish = String(discovery_Topic) + "/device_automation/" + String(unique_id) + "/config";
  Log.trace(F("Announce Device Trigger  %s" CR), topic_to_publish.c_str());
  pub_custom_topic((char*)topic_to_publish.c_str(), sensor, true);
}
# 217 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZmqttDiscovery.ino"
void createDiscovery(const char* sensor_type,
                     const char* st_topic, const char* s_name, const char* unique_id,
                     const char* availability_topic, const char* device_class, const char* value_template,
                     const char* payload_on, const char* payload_off, const char* unit_of_meas,
                     int off_delay,
                     const char* payload_available, const char* payload_not_avalaible, bool gateway_entity, const char* cmd_topic,
                     const char* device_name, const char* device_manufacturer, const char* device_model, const char* device_mac, bool retainCmd,
                     const char* state_class, const char* state_off, const char* state_on) {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject sensor = jsonBuffer.to<JsonObject>();







  if (st_topic[0]) {
    char state_topic[mqtt_topic_max_size];


    if (gateway_entity) {
      strcpy(state_topic, mqtt_topic);
      strcat(state_topic, gateway_name);
    } else {
      strcpy(state_topic, "+/+");
    }
    strcat(state_topic, st_topic);
    sensor["stat_t"] = state_topic;
  }

  if (device_class[0]) {

    int num_classes = sizeof(availableHASSClasses) / sizeof(availableHASSClasses[0]);
    for (int i = 0; i < num_classes; i++) {
      if (strcmp(availableHASSClasses[i], device_class) == 0) {
        sensor["dev_cla"] = device_class;
      }
    }
  }

  if (unit_of_meas[0]) {

    int num_units = sizeof(availableHASSUnits) / sizeof(availableHASSUnits[0]);
    for (int i = 0; i < num_units; i++) {
      if (strcmp(availableHASSUnits[i], unit_of_meas) == 0) {
        sensor["unit_of_meas"] = unit_of_meas;
      }
    }
  }
  sensor["name"] = s_name;
  sensor["uniq_id"] = unique_id;
  if (retainCmd)
    sensor["retain"] = retainCmd;
  if (value_template[0])
    sensor["val_tpl"] = value_template;
  if (payload_on[0])
    sensor["pl_on"] = payload_on;
  if (payload_off[0])
    sensor["pl_off"] = payload_off;
  if (off_delay != 0)
    sensor["off_delay"] = off_delay;
  if (payload_available[0])
    sensor["pl_avail"] = payload_available;
  if (payload_not_avalaible[0])
    sensor["pl_not_avail"] = payload_not_avalaible;
  if (state_class[0])
    sensor["state_class"] = state_class;
  if (state_on != nullptr)
    sensor["stat_on"] = state_on;
  if (state_off != nullptr)
    sensor["stat_off"] = state_off;

  if (cmd_topic[0]) {
    char command_topic[mqtt_topic_max_size];
    strcpy(command_topic, mqtt_topic);
    strcat(command_topic, gateway_name);
    strcat(command_topic, cmd_topic);
    sensor["cmd_t"] = command_topic;
  }

  StaticJsonDocument<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject device = jsonDeviceBuffer.to<JsonObject>();
  JsonArray identifiers = device.createNestedArray("identifiers");

  if (gateway_entity) {

    String model = "";
    serializeJson(modules, model);
    device["name"] = String(gateway_name);
    device["model"] = model;
    device["manufacturer"] = DEVICEMANUFACTURER;
    device["sw_version"] = OMG_VERSION;
    identifiers.add(String(getMacAddress()));
  } else {


    char deviceid[13];
    memcpy(deviceid, &unique_id[0], 12);
    deviceid[12] = '\0';
    identifiers.add(deviceid);


    if (device_mac[0] != 0) {
      JsonArray connections = device.createNestedArray("connections");
      JsonArray connection_mac = connections.createNestedArray();
      connection_mac.add("mac");
      connection_mac.add(device_mac);
    }

    if (device_manufacturer[0]) {
      device["manufacturer"] = device_manufacturer;
    }

    if (device_model[0]) {
      device["model"] = device_model;
    }

    if (device_name[0]) {

      device["name"] = device_name + String("-") + String(device_mac + 6);
    }

    device["via_device"] = String(gateway_name);
  }

  sensor["device"] = device;

  String topic = String(discovery_Topic) + "/" + String(sensor_type) + "/" + String(unique_id) + "/config";
  Log.trace(F("Announce Device %s on  %s" CR), String(sensor_type).c_str(), topic.c_str());
  pub_custom_topic((char*)topic.c_str(), sensor, true);
}

void pubMqttDiscovery() {
  Log.trace(F("omgStatusDiscovery" CR));
  createDiscovery("binary_sensor",
                  will_Topic, "SYS: Connectivity", (char*)getUniqueId("connectivity", "").c_str(),
                  will_Topic, "connectivity", "",
                  Gateway_AnnouncementMsg, will_Message, "",
                  0,
                  Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Uptime", (char*)getUniqueId("uptime", "").c_str(),
                  "", "", "{{ value_json.uptime }}",
                  "", "", "s",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );

# if defined(ESP8266) || defined(ESP32)
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Free memory", (char*)getUniqueId("freemem", "").c_str(),
                  "", "", "{{ value_json.freemem }}",
                  "", "", "B",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: IP", (char*)getUniqueId("ip", "").c_str(),
                  "", "", "{{ value_json.ip }}",
                  "", "", "",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# ifndef ESP32_ETHERNET
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Rssi", (char*)getUniqueId("rssi", "").c_str(),
                  "", "", "{{ value_json.rssi }}",
                  "", "", "dB",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif
# endif
# ifdef ESP32
# ifdef ZgatewayBT
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Low Power Mode", (char*)getUniqueId("lowpowermode", "").c_str(),
                  "", "", "{{ value_json.lowpowermode }}",
                  "", "", "",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif
# if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Bat voltage", (char*)getUniqueId("m5batvoltage", "").c_str(),
                  "", "", "{{ value_json.m5batvoltage }}",
                  "", "", "V",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Bat current", (char*)getUniqueId("m5batcurrent", "").c_str(),
                  "", "", "{{ value_json.m5batcurrent }}",
                  "", "", "A",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Vin voltage", (char*)getUniqueId("m5vinvoltage", "").c_str(),
                  "", "", "{{ value_json.m5vinvoltage }}",
                  "", "", "V",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Vin current", (char*)getUniqueId("m5vincurrent", "").c_str(),
                  "", "", "{{ value_json.m5vincurrent }}",
                  "", "", "A",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif
# ifdef ZboardM5STACK
  createDiscovery("sensor",
                  subjectSYStoMQTT, "SYS: Batt level", (char*)getUniqueId("m5battlevel", "").c_str(),
                  "", "", "{{ value_json.m5battlevel }}",
                  "", "", "%",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("binary_sensor",
                  subjectSYStoMQTT, "SYS: Is Charging", (char*)getUniqueId("m5ischarging", "").c_str(),
                  "", "{{ value_json.m5ischarging }}", "",
                  "", "", "%",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("binary_sensor",
                  subjectSYStoMQTT, "SYS: Is Charge Full", (char*)getUniqueId("m5ischargefull", "").c_str(),
                  "", "{{ value_json.m5ischargefull }}", "",
                  "", "", "%",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif
# endif
  createDiscovery("switch",
                  will_Topic, "SYS: Restart gateway", (char*)getUniqueId("restart", "").c_str(),
                  will_Topic, "", "",
                  "{\"cmd\":\"restart\"}", "", "",
                  0,
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset,
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("switch",
                  will_Topic, "SYS: Erase credentials", (char*)getUniqueId("erase", "").c_str(),
                  will_Topic, "", "",
                  "{\"cmd\":\"erase\"}", "", "",
                  0,
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset,
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("switch",
                  "", "SYS: Auto discovery", (char*)getUniqueId("discovery", "").c_str(),
                  will_Topic, "", "",
                  "{\"discovery\":true}", "{\"discovery\":false}", "",
                  0,
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset,
                  "", "", "", "", true,
                  stateClassNone
  );

# ifdef ZsensorBME280
#define BMEparametersCount 5
  Log.trace(F("bme280Discovery" CR));
  char* BMEsensor[BMEparametersCount][8] = {
      {"sensor", "temp", "bme", "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "pa", "bme", "", jsonPa, "", "", "hPa"},
      {"sensor", "hum", "bme", "humidity", jsonHum, "", "", "%"},
      {"sensor", "altim", "bme", "", jsonAltim, "", "", "m"},
      {"sensor", "altift", "bme", "", jsonAltif, "", "", "ft"}

  };

  for (int i = 0; i < BMEparametersCount; i++) {

    createDiscovery(BMEsensor[i][0],
                    BMETOPIC, BMEsensor[i][1], (char*)getUniqueId(BMEsensor[i][1], BMEsensor[i][2]).c_str(),
                    will_Topic, BMEsensor[i][3], BMEsensor[i][4],
                    BMEsensor[i][5], BMEsensor[i][6], BMEsensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false,
                    stateClassNone
    );
  }
# endif

# ifdef ZsensorHTU21
#define HTUparametersCount 2
  Log.trace(F("htu21Discovery" CR));
  char* HTUsensor[HTUparametersCount][8] = {
      {"sensor", "temp", "htu", "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "hum", "htu", "humidity", jsonHum, "", "", "%"}

  };

  for (int i = 0; i < HTUparametersCount; i++) {

    createDiscovery(HTUsensor[i][0],
                    HTUTOPIC, HTUsensor[i][1], (char*)getUniqueId(HTUsensor[i][1], HTUsensor[i][2]).c_str(),
                    will_Topic, HTUsensor[i][3], HTUsensor[i][4],
                    HTUsensor[i][5], HTUsensor[i][6], HTUsensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false,
                    stateClassNone
    );
  }
# endif

# ifdef ZsensorAHTx0
#define AHTparametersCount 2
  Log.trace(F("AHTx0Discovery" CR));
  char* AHTsensor[AHTparametersCount][8] = {
      {"sensor", "temp", "aht", "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "hum", "aht", "humidity", jsonHum, "", "", "%"}

  };

  for (int i = 0; i < AHTparametersCount; i++) {
    createDiscovery(AHTsensor[i][0],
                    AHTTOPIC, AHTsensor[i][1], (char*)getUniqueId(AHTsensor[i][1], AHTsensor[i][2]).c_str(),
                    will_Topic, AHTsensor[i][3], AHTsensor[i][4],
                    AHTsensor[i][5], AHTsensor[i][6], AHTsensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false,
                    stateClassNone
    );
  }
# endif

# ifdef ZsensorDHT
#define DHTparametersCount 2
  Log.trace(F("DHTDiscovery" CR));
  char* DHTsensor[DHTparametersCount][8] = {
      {"sensor", "temp", "dht", "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "hum", "dht", "humidity", jsonHum, "", "", "%"}

  };

  for (int i = 0; i < DHTparametersCount; i++) {

    createDiscovery(DHTsensor[i][0],
                    DHTTOPIC, DHTsensor[i][1], (char*)getUniqueId(DHTsensor[i][1], DHTsensor[i][2]).c_str(),
                    will_Topic, DHTsensor[i][3], DHTsensor[i][4],
                    DHTsensor[i][5], DHTsensor[i][6], DHTsensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false,
                    stateClassNone
    );
  }
# endif

# ifdef ZsensorADC
  Log.trace(F("ADCDiscovery" CR));
  char* ADCsensor[8] = {"sensor", "adc", "", "", jsonAdc, "", "", ""};



  createDiscovery(ADCsensor[0],
                  ADCTOPIC, ADCsensor[1], (char*)getUniqueId(ADCsensor[1], ADCsensor[2]).c_str(),
                  will_Topic, ADCsensor[3], ADCsensor[4],
                  ADCsensor[5], ADCsensor[6], ADCsensor[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZsensorBH1750
#define BH1750parametersCount 3
  Log.trace(F("BH1750Discovery" CR));
  char* BH1750sensor[BH1750parametersCount][8] = {
      {"sensor", "lux", "BH1750", "illuminance", jsonLux, "", "", "lx"},
      {"sensor", "ftCd", "BH1750", "", jsonFtcd, "", "", ""},
      {"sensor", "wattsm2", "BH1750", "", jsonWm2, "", "", "wm²"}

  };

  for (int i = 0; i < BH1750parametersCount; i++) {

    createDiscovery(BH1750sensor[i][0],
                    subjectBH1750toMQTT, BH1750sensor[i][1], (char*)getUniqueId(BH1750sensor[i][1], BH1750sensor[i][2]).c_str(),
                    will_Topic, BH1750sensor[i][3], BH1750sensor[i][4],
                    BH1750sensor[i][5], BH1750sensor[i][6], BH1750sensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false,
                    stateClassNone
    );
  }
# endif

# ifdef ZsensorTSL2561
#define TSL2561parametersCount 3
  Log.trace(F("TSL2561Discovery" CR));
  char* TSL2561sensor[TSL2561parametersCount][8] = {
      {"sensor", "lux", "TSL2561", "illuminance", jsonLux, "", "", "lx"},
      {"sensor", "ftcd", "TSL2561", "", jsonFtcd, "", "", ""},
      {"sensor", "wattsm2", "TSL2561", "", jsonWm2, "", "", "wm²"}

  };

  for (int i = 0; i < TSL2561parametersCount; i++) {

    createDiscovery(TSL2561sensor[i][0],
                    subjectTSL12561toMQTT, TSL2561sensor[i][1], (char*)getUniqueId(TSL2561sensor[i][1], TSL2561sensor[i][2]).c_str(),
                    will_Topic, TSL2561sensor[i][3], TSL2561sensor[i][4],
                    TSL2561sensor[i][5], TSL2561sensor[i][6], TSL2561sensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false,
                    stateClassNone
    );
  }
# endif

# ifdef ZsensorHCSR501
  Log.trace(F("HCSR501Discovery" CR));
  char* HCSR501sensor[8] = {"binary_sensor", "hcsr501", "", "", jsonPresence, "true", "false", ""};



  createDiscovery(HCSR501sensor[0],
                  subjectHCSR501toMQTT, HCSR501sensor[1], (char*)getUniqueId(HCSR501sensor[1], HCSR501sensor[2]).c_str(),
                  will_Topic, HCSR501sensor[3], HCSR501sensor[4],
                  HCSR501sensor[5], HCSR501sensor[6], HCSR501sensor[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZsensorGPIOInput
  Log.trace(F("GPIOInputDiscovery" CR));
  char* GPIOInputsensor[8] = {"binary_sensor", "GPIOInput", "", "", jsonGpio, INPUT_GPIO_ON_VALUE, INPUT_GPIO_OFF_VALUE, ""};



  createDiscovery(GPIOInputsensor[0],
                  subjectGPIOInputtoMQTT, GPIOInputsensor[1], (char*)getUniqueId(GPIOInputsensor[1], GPIOInputsensor[2]).c_str(),
                  will_Topic, GPIOInputsensor[3], GPIOInputsensor[4],
                  GPIOInputsensor[5], GPIOInputsensor[6], GPIOInputsensor[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZsensorINA226
#define INA226parametersCount 3
  Log.trace(F("INA226Discovery" CR));
  char* INA226sensor[INA226parametersCount][8] = {
      {"sensor", "volt", "INA226", "", jsonVolt, "", "", "V"},
      {"sensor", "current", "INA226", "", jsonCurrent, "", "", "A"},
      {"sensor", "power", "INA226", "", jsonPower, "", "", "W"}

  };

  for (int i = 0; i < INA226parametersCount; i++) {

    createDiscovery(INA226sensor[i][0],
                    subjectINA226toMQTT, INA226sensor[i][1], (char*)getUniqueId(INA226sensor[i][1], INA226sensor[i][2]).c_str(),
                    will_Topic, INA226sensor[i][3], INA226sensor[i][4],
                    INA226sensor[i][5], INA226sensor[i][6], INA226sensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false,
                    stateClassNone
    );
  }
# endif

# ifdef ZsensorDS1820

  pubOneWire_HADiscovery();
# endif

# ifdef ZactuatorONOFF
  Log.trace(F("actuatorONOFFDiscovery" CR));
  char* actuatorONOFF[8] = {"switch", "actuatorONOFF", "", "", "", "{\"cmd\":1}", "{\"cmd\":0}", ""};



  createDiscovery(actuatorONOFF[0],
                  subjectGTWONOFFtoMQTT, actuatorONOFF[1], (char*)getUniqueId(actuatorONOFF[1], actuatorONOFF[2]).c_str(),
                  will_Topic, actuatorONOFF[3], actuatorONOFF[4],
                  actuatorONOFF[5], actuatorONOFF[6], actuatorONOFF[7],
                  0, "", "", true, subjectMQTTtoONOFF,
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZgatewayRF

  Log.trace(F("gatewayRFDiscovery" CR));
  char* gatewayRF[8] = {"sensor", "gatewayRF", "", "", jsonVal, "", "", ""};



  createDiscovery(gatewayRF[0],
                  subjectRFtoMQTT, gatewayRF[1], (char*)getUniqueId(gatewayRF[1], gatewayRF[2]).c_str(),
                  will_Topic, gatewayRF[3], gatewayRF[4],
                  gatewayRF[5], gatewayRF[6], gatewayRF[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );

# endif

# ifdef ZgatewayRF2

  Log.trace(F("gatewayRF2Discovery" CR));
  char* gatewayRF2[8] = {"sensor", "gatewayRF2", "", "", jsonAddress, "", "", ""};



  createDiscovery(gatewayRF2[0],
                  subjectRF2toMQTT, gatewayRF2[1], (char*)getUniqueId(gatewayRF2[1], gatewayRF2[2]).c_str(),
                  will_Topic, gatewayRF2[3], gatewayRF2[4],
                  gatewayRF2[5], gatewayRF2[6], gatewayRF2[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZgatewayRFM69

  Log.trace(F("gatewayRFM69Discovery" CR));
  char* gatewayRFM69[8] = {"sensor", "gatewayRFM69", "", "", jsonVal, "", "", ""};



  createDiscovery(gatewayRFM69[0],
                  subjectRFM69toMQTT, gatewayRFM69[1], (char*)getUniqueId(gatewayRFM69[1], gatewayRFM69[2]).c_str(),
                  will_Topic, gatewayRFM69[3], gatewayRFM69[4],
                  gatewayRFM69[5], gatewayRFM69[6], gatewayRFM69[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZgatewayLORA

  Log.trace(F("gatewayLORADiscovery" CR));
  char* gatewayLORA[8] = {"sensor", "gatewayLORA", "", "", jsonMsg, "", "", ""};



  createDiscovery(gatewayLORA[0],
                  subjectLORAtoMQTT, gatewayLORA[1], (char*)getUniqueId(gatewayLORA[1], gatewayLORA[2]).c_str(),
                  will_Topic, gatewayLORA[3], gatewayLORA[4],
                  gatewayLORA[5], gatewayLORA[6], gatewayLORA[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZgatewaySRFB

  Log.trace(F("gatewaySRFBDiscovery" CR));
  char* gatewaySRFB[8] = {"sensor", "gatewaySRFB", "", "", jsonVal, "", "", ""};



  createDiscovery(gatewaySRFB[0],
                  subjectSRFBtoMQTT, gatewaySRFB[1], (char*)getUniqueId(gatewaySRFB[1], gatewaySRFB[2]).c_str(),
                  will_Topic, gatewaySRFB[3], gatewaySRFB[4],
                  gatewaySRFB[5], gatewaySRFB[6], gatewaySRFB[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZgatewayPilight

  Log.trace(F("gatewayPilightDiscovery" CR));
  char* gatewayPilight[8] = {"sensor", "gatewayPilight", "", "", jsonMsg, "", "", ""};



  createDiscovery(gatewayPilight[0],
                  subjectPilighttoMQTT, gatewayPilight[1], (char*)getUniqueId(gatewayPilight[1], gatewayPilight[2]).c_str(),
                  will_Topic, gatewayPilight[3], gatewayPilight[4],
                  gatewayPilight[5], gatewayPilight[6], gatewayPilight[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZgatewayIR

  Log.trace(F("gatewayIRDiscovery" CR));
  char* gatewayIR[8] = {"sensor", "gatewayIR", "", "", jsonVal, "", "", ""};



  createDiscovery(gatewayIR[0],
                  subjectIRtoMQTT, gatewayIR[1], (char*)getUniqueId(gatewayIR[1], gatewayIR[2]).c_str(),
                  will_Topic, gatewayIR[3], gatewayIR[4],
                  gatewayIR[5], gatewayIR[6], gatewayIR[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef Zgateway2G

  Log.trace(F("gateway2GDiscovery" CR));
  char* gateway2G[8] = {"sensor", "gateway2G", "", "", jsonMsg, "", "", ""};



  createDiscovery(gateway2G[0],
                  subject2GtoMQTT, gateway2G[1], (char*)getUniqueId(gateway2G[1], gateway2G[2]).c_str(),
                  will_Topic, gateway2G[3], gateway2G[4],
                  gateway2G[5], gateway2G[6], gateway2G[7],
                  0, "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
# endif

# ifdef ZgatewayBT
  createDiscovery("sensor",
                  subjectSYStoMQTT, "BT: Interval between scans", (char*)getUniqueId("interval", "").c_str(),
                  "", "", "{{ value_json.interval }}",
                  "", "", "ms",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("sensor",
                  subjectSYStoMQTT, "BT: Connnect every X scan(s)", (char*)getUniqueId("scanbcnct", "").c_str(),
                  "", "", "{{ value_json.scanbcnct }}",
                  "", "", "",
                  0,
                  "", "", true, "",
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("switch",
                  will_Topic, "BT: Force scan", (char*)getUniqueId("force_scan", "").c_str(),
                  will_Topic, "", "",
                  "{\"interval\":0}", "", "",
                  0,
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset,
                  "", "", "", "", false,
                  stateClassNone
  );
  createDiscovery("switch",
                  "", "BT: Publish only sensors", (char*)getUniqueId("only_sensors", "").c_str(),
                  "", "", "",
                  "{\"onlysensors\":true}", "{\"onlysensors\":false}", "",
                  0,
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset,
                  "", "", "", "", true,
                  stateClassNone
  );
  createDiscovery("switch",
                  "", "BT: Publish HASS presence", (char*)getUniqueId("hasspresence", "").c_str(),
                  "", "", "",
                  "{\"hasspresence\":true}", "{\"hasspresence\":false}", "",
                  0,
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset,
                  "", "", "", "", true,
                  stateClassNone
  );
# ifdef ESP32
  createDiscovery("switch",
                  "", "SYS: Low Power Mode command", (char*)getUniqueId("lowpowermode", "").c_str(),
                  "", "", "",
                  "{\"lowpowermode\":2}", "{\"lowpowermode\":0}", "",
                  0,
                  "", "", true, subjectMQTTtoBTset,
                  "", "", "", "", true,
                  stateClassNone
  );
  createDiscovery("switch",
                  "", "BT: Connect to devices", (char*)getUniqueId("bleconnect", "").c_str(),
                  "", "", "",
                  "{\"bleconnect\":true}", "{\"bleconnect\":false}", "",
                  0,
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset,
                  "", "", "", "", true,
                  stateClassNone
  );
# endif
# endif
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorADC.ino"
# 29 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorADC.ino"
#include "User_config.h"

#ifdef ZsensorADC

# if defined(ESP8266)
ADC_MODE(ADC_TOUT);
# endif


unsigned long timeadc = 0;

void setupADC() {
  Log.notice(F("ADC_GPIO: %d" CR), ADC_GPIO);
}

void MeasureADC() {
  if (millis() > (timeadc + TimeBetweenReadingADC)) {
# if defined(ESP8266)
    yield();
# endif
    timeadc = millis();
    static int persistedadc;
    int val = analogRead(ADC_GPIO);
    if (isnan(val)) {
      Log.error(F("Failed to read from ADC !" CR));
    } else {
      if (val >= persistedadc + ThresholdReadingADC || val <= persistedadc - ThresholdReadingADC) {
        Log.trace(F("Creating ADC buffer" CR));
        StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
        JsonObject ADCdata = jsonBuffer.to<JsonObject>();
        ADCdata["adc"] = (int)val;
# if defined(ADC_DIVIDER)
        float volt = 0;
# if defined(ESP32)

        volt = val * (3.3 / 4096.0);
# elif defined(ESP8266)

        volt = val * (3.3 / 1024.0);
# else

        volt = val * (5.0 / 1024.0);
# endif
        volt *= ADC_DIVIDER;

        val = (volt * 100);
        volt = (float)val / 100.0;
        ADCdata["volt"] = (float)volt;
# endif
        pub(ADCTOPIC, ADCdata);
        persistedadc = val;
      }
    }
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorAHTx0.ino"
# 38 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorAHTx0.ino"
#include "User_config.h"

#ifdef ZsensorAHTx0
# include <stdint.h>

# include "Adafruit_AHTX0.h"
# include "Wire.h"
# include "config_AHTx0.h"


unsigned long timeAHTx0 = 0;


Adafruit_AHTX0 ahtSensor;

void setupZsensorAHTx0() {
  delay(10);
  Log.notice(F("AHTx0 Initialized - begin()" CR));

# if defined(ESP32)
  Wire.begin(AHT_I2C_SDA, AHT_I2C_SCL);
  if (!ahtSensor.begin(&Wire)) {
    Log.error(F("Failed to initialize AHTx0 sensor!" CR));
  }
# else
  if (!ahtSensor.begin()) {
    Log.error(F("Failed to initialize AHTx0 sensor!" CR));
  }
# endif
}

void MeasureAHTTempHum() {
  if (millis() > (timeAHTx0 + TimeBetweenReadingAHTx0)) {
    Log.trace(F("Read AHTx0 Sensor" CR));

    timeAHTx0 = millis();
    static float persisted_aht_tempc;
    static float persisted_aht_hum;

    sensors_event_t ahtTempC, ahtHum;
    if (!ahtSensor.getEvent(&ahtHum, &ahtTempC))
    {
      Log.error(F("Failed to read from sensor AHTx0!" CR));
      return;
    }


    if (isnan(ahtTempC.temperature) || isnan(ahtHum.relative_humidity)) {
      Log.error(F("Failed to read from sensor AHTx0!" CR));
    } else {
      Log.notice(F("Creating AHTx0 buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject AHTx0data = jsonBuffer.to<JsonObject>();

      if (ahtTempC.temperature != persisted_aht_tempc || AHTx0_always) {
        float ahtTempF = convertTemp_CtoF(ahtTempC.temperature);
        AHTx0data["tempc"] = (float)ahtTempC.temperature;
        AHTx0data["tempf"] = (float)ahtTempF;
      } else {
        Log.notice(F("Same Temp. Don't send it" CR));
      }


      if (ahtHum.relative_humidity != persisted_aht_hum || AHTx0_always) {
        AHTx0data["hum"] = (float)ahtHum.relative_humidity;
      } else {
        Log.notice(F("Same Humidity. Don't send it" CR));
      }

      if (AHTx0data.size() > 0) {
        pub(AHTTOPIC, AHTx0data);
      }
    }
    persisted_aht_tempc = ahtTempC.temperature;
    persisted_aht_hum = ahtHum.relative_humidity;
  }
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorBH1750.ino"
# 39 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorBH1750.ino"
#include "User_config.h"

#ifdef ZsensorBH1750
# include "Wire.h"
# include "math.h"

void setupZsensorBH1750() {
  Log.notice(F("Setup BH1750 on adress: %H" CR), BH1750_i2c_addr);
  Wire.begin();
  Wire.beginTransmission(BH1750_i2c_addr);
  Wire.write(0x10);
  Wire.endTransmission();
  delay(300);
}

void MeasureLightIntensity() {
  if (millis() > (timebh1750 + TimeBetweenReadingBH1750)) {
    Log.trace(F("Creating BH1750 buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject BH1750data = jsonBuffer.to<JsonObject>();

    timebh1750 = millis();
    unsigned int i = 0;
    static float persistedll;
    static float persistedlf;
    static float persistedlw;
    unsigned int Lux;
    float ftcd;
    float Wattsm2;


    Wire.requestFrom(BH1750_i2c_addr, 2);
    if (Wire.available() != 2) {
      Log.error(F("Failed to read from LightSensor BH1750!" CR));
    } else {
      i = Wire.read();
      i <<= 8;
      i |= Wire.read();


      Lux = i / 1.2;
      ftcd = Lux / 10.764;
      Wattsm2 = Lux / 683.0;


      if (Lux != persistedll || bh1750_always) {
        BH1750data["lux"] = (unsigned int)Lux;
      } else {
        Log.trace(F("Same lux don't send it" CR));
      }


      if (ftcd != persistedlf || bh1750_always) {
        BH1750data["ftcd"] = (unsigned int)ftcd;
      } else {
        Log.trace(F("Same ftcd don't send it" CR));
      }


      if (Wattsm2 != persistedlw || bh1750_always) {
        BH1750data["wattsm2"] = (unsigned int)Wattsm2;
      } else {
        Log.trace(F("Same wattsm2 don't send it" CR));
      }
      if (BH1750data.size() > 0)
        pub(subjectBH1750toMQTT, BH1750data);
    }
    persistedll = Lux;
    persistedlf = ftcd;
    persistedlw = Wattsm2;
  }
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorBME280.ino"
# 40 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorBME280.ino"
#include "User_config.h"

#ifdef ZsensorBME280
# include <stdint.h>

# include "SparkFunBME280.h"
# include "Wire.h"


BME280 mySensor;

void setupZsensorBME280() {
# if defined(ESP8266) || defined(ESP32)

  Wire.begin(BME280_PIN_SDA, BME280_PIN_SCL);
# else
  Wire.begin();
# endif

  mySensor.settings.commInterface = I2C_MODE;
  mySensor.settings.I2CAddress = BME280_i2c_addr;
  Log.notice(F("Setup BME280 on adress: %X" CR), BME280_i2c_addr);







  mySensor.settings.runMode = 3;
# 81 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorBME280.ino"
  mySensor.settings.tStandby = 1;
# 90 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorBME280.ino"
  mySensor.settings.filter = 4;





  mySensor.settings.tempOverSample = 1;





  mySensor.settings.pressOverSample = 1;





  mySensor.settings.humidOverSample = 1;





  mySensor.settings.tempCorrection = BME280Correction;

  delay(10);

  int ret = mySensor.begin();
  if (ret == 0x60) {
    Log.notice(F("Bosch BME280 successfully initialized: %X" CR), ret);
  } else {
    Log.notice(F("Bosch BME280 failed: %X" CR), ret);
  }
}

void MeasureTempHumAndPressure() {
  if (millis() > (timebme280 + TimeBetweenReadingbme280)) {
    timebme280 = millis();
    static float persisted_bme_tempc;
    static float persisted_bme_tempf;
    static float persisted_bme_hum;
    static float persisted_bme_pa;
    static float persisted_bme_altim;
    static float persisted_bme_altift;

    float BmeTempC = mySensor.readTempC();
    float BmeTempF = mySensor.readTempF();
    float BmeHum = mySensor.readFloatHumidity();
    float BmePa = mySensor.readFloatPressure();
    float BmeAltiM = mySensor.readFloatAltitudeMeters();
    float BmeAltiFt = mySensor.readFloatAltitudeFeet();


    if (isnan(BmeTempC) || isnan(BmeTempF) || isnan(BmeHum) || isnan(BmePa) || isnan(BmeAltiM) || isnan(BmeAltiFt)) {
      Log.error(F("Failed to read from BME280!" CR));
    } else {
      Log.trace(F("Creating BME280 buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject BME280data = jsonBuffer.to<JsonObject>();

      if (BmeTempC != persisted_bme_tempc || bme280_always) {
        BME280data["tempc"] = (float)BmeTempC;
      } else {
        Log.trace(F("Same Degrees C don't send it" CR));
      }


      if (BmeTempF != persisted_bme_tempf || bme280_always) {
        BME280data["tempf"] = (float)BmeTempF;
      } else {
        Log.trace(F("Same Degrees F don't send it" CR));
      }


      if (BmeHum != persisted_bme_hum || bme280_always) {
        BME280data["hum"] = (float)BmeHum;
      } else {
        Log.trace(F("Same Humidity don't send it" CR));
      }


      if (BmePa != persisted_bme_pa || bme280_always) {
        BME280data["pa"] = (float)BmePa;
      } else {
        Log.trace(F("Same Pressure don't send it" CR));
      }


      if (BmeAltiM != persisted_bme_altim || bme280_always) {
        Log.trace(F("Sending Altitude Meter to MQTT" CR));
        BME280data["altim"] = (float)BmeAltiM;
      } else {
        Log.trace(F("Same Altitude Meter don't send it" CR));
      }


      if (BmeAltiFt != persisted_bme_altift || bme280_always) {
        BME280data["altift"] = (float)BmeAltiFt;
      } else {
        Log.trace(F("Same Altitude Feet don't send it" CR));
      }
      if (BME280data.size() > 0)
        pub(BMETOPIC, BME280data);
    }

    persisted_bme_tempc = BmeTempC;
    persisted_bme_tempf = BmeTempF;
    persisted_bme_hum = BmeHum;
    persisted_bme_pa = BmePa;
    persisted_bme_altim = BmeAltiM;
    persisted_bme_altift = BmeAltiFt;
  }
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorDHT.ino"
# 30 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorDHT.ino"
#include "User_config.h"

#ifdef ZsensorDHT
# include <DHT.h>
# include <DHT_U.h>

DHT dht(DHT_RECEIVER_GPIO, DHT_SENSOR_TYPE);


unsigned long timedht = 0;

void setupDHT() {
  Log.notice(F("Reading DHT on pin: %d" CR), DHT_RECEIVER_GPIO);
}

void MeasureTempAndHum() {
  if (millis() > (timedht + TimeBetweenReadingDHT)) {
    timedht = millis();
    static float persistedh;
    static float persistedt;
    float h = dht.readHumidity();

    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Log.error(F("Failed to read from DHT sensor!" CR));
    } else {
      Log.trace(F("Creating DHT buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject DHTdata = jsonBuffer.to<JsonObject>();
      if (h != persistedh || dht_always) {
        DHTdata["hum"] = (float)h;
      } else {
        Log.trace(F("Same hum don't send it" CR));
      }
      if (t != persistedt || dht_always) {
        DHTdata["tempc"] = (float)t;
        DHTdata["tempf"] = dht.convertCtoF(t);
      } else {
        Log.trace(F("Same temp don't send it" CR));
      }
      if (DHTdata.size() > 0)
        pub(DHTTOPIC, DHTdata);
    }
    persistedh = h;
    persistedt = t;
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorDS1820.ino"
# 26 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorDS1820.ino"
#include "User_config.h"

#ifdef ZsensorDS1820
# include <DallasTemperature.h>
# include <OneWire.h>

OneWire owbus(DS1820_OWBUS_GPIO);
DallasTemperature ds1820(&owbus);
DeviceAddress ds1820_address, ds1820_devices[OW_MAX_SENSORS];

static uint8_t ds1820_count = 0;
static uint8_t ds1820_resolution[OW_MAX_SENSORS];
static String ds1820_type[OW_MAX_SENSORS];
static String ds1820_addr[OW_MAX_SENSORS];

void setupZsensorDS1820() {
  Log.trace(F("DS1820: configured pin: %d for 1-wire bus" CR), DS1820_OWBUS_GPIO);
  ds1820.begin();


  uint8_t numDevicesOnBus = ds1820.getDeviceCount();


  Log.notice(F("DS1820: Found %d devices" CR), numDevicesOnBus);


  for (int deviceIndex = 0; deviceIndex < numDevicesOnBus && ds1820_count < OW_MAX_SENSORS; deviceIndex++) {

    if (ds1820.getAddress(ds1820_address, deviceIndex) && ds1820.validFamily(ds1820_address)) {
      ds1820_addr[ds1820_count] = String("0x");
      for (uint8_t i = 0; i < 8; i++) {
        if (ds1820_address[i] < 0x10) ds1820_addr[ds1820_count] += String("0");
        ds1820_devices[ds1820_count][i] = ds1820_address[i];
        ds1820_addr[ds1820_count] += String(ds1820_address[i], HEX);
      }


      if (ds1820_address[0] == 0x10) {





        ds1820_type[ds1820_count] = String("DS1820/DS18S20");
      } else if (ds1820_address[0] == 0x28) {

        ds1820_type[ds1820_count] = String("DS18B20");
        ds1820.setResolution(ds1820_address, DS1820_RESOLUTION);
      } else if (ds1820_address[0] == 0x22) {
        ds1820_type[ds1820_count] = String("DS1822");
        ds1820.setResolution(ds1820_address, DS1820_RESOLUTION);
      } else {
        ds1820_type[ds1820_count] = String("DS1825");
        ds1820.setResolution(ds1820_address, DS1820_RESOLUTION);
      }
      ds1820_resolution[ds1820_count] = ds1820.getResolution(ds1820_address);
      Log.trace(F("DS1820: Device %d, Type: %s, Address: %s, Resolution: %d" CR),
                ds1820_count,
                (char*)ds1820_type[ds1820_count].c_str(),
                (char*)ds1820_addr[ds1820_count].c_str(),
                ds1820_resolution[ds1820_count]);
      ds1820_count++;
    }
  }

  if (ds1820.getDS18Count() == 0) {
    Log.error(F("DS1820: Failed to enumerate sensors on 1-wire bus. Check your GPIO assignment!" CR));
  }



  ds1820.setWaitForConversion(false);
}

void pubOneWire_HADiscovery() {

# ifdef ZmqttDiscovery

  if (disc) {
    for (int index = 0; index < ds1820_count; index++) {
      createDiscovery("sensor",
                      (char*)(String(OW_TOPIC) + "/" + ds1820_addr[index]).c_str(),
                      (char*)("DS12B20_" + String(index + 1) + "_c").c_str(),
                      (char*)(ds1820_addr[index] + "_c").c_str(),
                      will_Topic,
                      "temperature",
                      jsonTempc,
                      "", "", "°C",
                      0, "", "", true, "",
                      "", "", "", "", false,
                      stateClassMeasurement
      );
    }
  }
# endif
}

void MeasureDS1820Temp() {
  static float persisted_temp[OW_MAX_SENSORS];
  static unsigned long timeDS1820 = 0;
  static boolean triggeredConversion = false;
  float current_temp[OW_MAX_SENSORS];



  if (!triggeredConversion && ((millis() - timeDS1820) > (DS1820_INTERVAL_SEC * 1000UL - DS1820_CONV_TIME))) {
    Log.trace(F("DS1820: Trigger temperature conversion..." CR));
    ds1820.requestTemperatures();
    triggeredConversion = true;
  } else if (triggeredConversion && ((millis() - timeDS1820) > DS1820_INTERVAL_SEC * 1000UL)) {
    timeDS1820 = millis();
    triggeredConversion = false;

    if (ds1820_count < 1) {
      Log.error(F("DS1820: Failed to identify any temperature sensors on 1-wire bus during setup!" CR));
    } else {
      Log.trace(F("DS1820: Reading temperature(s) from %d sensor(s)..." CR), ds1820_count);
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject DS1820data = jsonBuffer.to<JsonObject>();

      for (uint8_t i = 0; i < ds1820_count; i++) {
        current_temp[i] = round(ds1820.getTempC(ds1820_devices[i]) * 10) / 10.0;
        if (current_temp[i] == -127) {
          Log.error(F("DS1820: Device %s currently disconnected!" CR), (char*)ds1820_addr[i].c_str());
        } else if (DS1820_ALWAYS || current_temp[i] != persisted_temp[i]) {
          DS1820data["tempf"] = (float)DallasTemperature::toFahrenheit(current_temp[i]);
          DS1820data["tempc"] = (float)current_temp[i];

          if (DS1820_DETAILS) {
            DS1820data["type"] = ds1820_type[i];
            DS1820data["res"] = ds1820_resolution[i] + String("bit" CR);
            DS1820data["addr"] = ds1820_addr[i];
          }
          pub((char*)(String(OW_TOPIC) + "/" + ds1820_addr[i]).c_str(), DS1820data);
          delay(10);
        } else {
          Log.trace(F("DS1820: Temperature for device %s didn't change, don't publish it." CR), (char*)ds1820_addr[i].c_str());
        }
        persisted_temp[i] = current_temp[i];
      }
    }
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorGPIOInput.ino"
# 29 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorGPIOInput.ino"
#include "User_config.h"

#ifdef ZsensorGPIOInput
# if defined(TRIGGER_GPIO) && INPUT_GPIO == TRIGGER_GPIO
unsigned long resetTime = 0;
# endif
unsigned long lastDebounceTime = 0;
int InputState = 3;
int lastInputState = 3;

void setupGPIOInput() {
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO);
  pinMode(INPUT_GPIO, INPUT_PULLUP);
}

void MeasureGPIOInput() {
  int reading = digitalRead(INPUT_GPIO);






  if (reading != lastInputState) {

    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > GPIOInputDebounceDelay) {


# if defined(ESP8266) || defined(ESP32)
    yield();
# endif
# if defined(TRIGGER_GPIO) && INPUT_GPIO == TRIGGER_GPIO
    if (reading == LOW) {
      if (resetTime == 0) {
        resetTime = millis();
      } else if ((millis() - resetTime) > 10000) {
        Log.trace(F("Button Held" CR));
        Log.notice(F("Erasing ESP Config, restarting" CR));
        setup_wifimanager(true);
      }
    } else {
      resetTime = 0;
    }
# endif

    if (reading != InputState) {
      InputState = reading;
      Log.trace(F("Creating GPIOInput buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject GPIOdata = jsonBuffer.to<JsonObject>();
      if (InputState == HIGH) {
        GPIOdata["gpio"] = "HIGH";
      }
      if (InputState == LOW) {
        GPIOdata["gpio"] = "LOW";
      }
      if (GPIOdata.size() > 0)
        pub(subjectGPIOInputtoMQTT, GPIOdata);

# ifdef ZactuatorONOFF
      if (InputState == ACTUATOR_BUTTON_TRIGGER_LEVEL) {
        ActuatorButtonTrigger();
      }
# endif
    }
  }


  lastInputState = reading;
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorGPIOKeyCode.ino"
# 26 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorGPIOKeyCode.ino"
#include "User_config.h"

#ifdef ZsensorGPIOKeyCode

int InputStateGPIOKeyCode = 0x0f;
int lastInputStateGPIOKeyCode = 0x0f;
int lastLatchStateGPIOKeyCode = 0;

void setupGPIOKeyCode() {
  pinMode(GPIOKeyCode_LATCH_GPIO, INPUT_PULLUP);
  pinMode(GPIOKeyCode_D0_GPIO, INPUT_PULLUP);
  pinMode(GPIOKeyCode_D1_GPIO, INPUT_PULLUP);
  pinMode(GPIOKeyCode_D2_GPIO, INPUT_PULLUP);

}

void MeasureGPIOKeyCode() {
  int latch = digitalRead(GPIOKeyCode_LATCH_GPIO);





  {


# if defined(ESP8266) || defined(ESP32)
    yield();
# endif

    if (latch > 0 && lastLatchStateGPIOKeyCode != latch) {
      int reading = digitalRead(GPIOKeyCode_D0_GPIO) | (digitalRead(GPIOKeyCode_D1_GPIO) << 1) | (digitalRead(GPIOKeyCode_D2_GPIO) << 2);


      char hex[3];

      InputStateGPIOKeyCode = reading;
      sprintf(hex, "%02x", InputStateGPIOKeyCode);
      hex[2] = 0;
      Log.notice(F("GPIOKeyCode %H" CR), hex);
      pub(subjectGPIOKeyCodetoMQTT, hex);
      lastLatchStateGPIOKeyCode = latch;
    }

    if (latch != lastLatchStateGPIOKeyCode) {
      lastLatchStateGPIOKeyCode = latch;
      Log.notice(F("GPIOKeyCode latch %d" CR), latch);
      if (latch == 0)
        pub(subjectGPIOKeyCodeStatetoMQTT, "done");
    }


    lastInputStateGPIOKeyCode = InputStateGPIOKeyCode;
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorHCSR04.ino"
# 30 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorHCSR04.ino"
#include "User_config.h"

#ifdef ZsensorHCSR04

unsigned long timeHCSR04 = 0;

void setupHCSR04() {
  Log.notice(F("HCSR04 trigger pin: %d" CR), HCSR04_TRI_GPIO);
  Log.notice(F("HCSR04 echo pin: %d" CR), HCSR04_ECH_GPIO);
  pinMode(HCSR04_TRI_GPIO, OUTPUT);
  pinMode(HCSR04_ECH_GPIO, INPUT);
}

void MeasureDistance() {
  if (millis() > (timeHCSR04 + TimeBetweenReadingHCSR04)) {
    timeHCSR04 = millis();
    Log.trace(F("Creating HCSR04 buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject HCSR04data = jsonBuffer.to<JsonObject>();
    digitalWrite(HCSR04_TRI_GPIO, LOW);
    delayMicroseconds(2);
    digitalWrite(HCSR04_TRI_GPIO, HIGH);
    delayMicroseconds(10);
    digitalWrite(HCSR04_TRI_GPIO, LOW);
    unsigned long duration = pulseIn(HCSR04_ECH_GPIO, HIGH);
    if (isnan(duration)) {
      Log.error(F("Failed to read from HC SR04 sensor!" CR));
    } else {
      static unsigned int distance = 99999;
      unsigned int d = duration / 58.2;
      HCSR04data["distance"] = (int)d;
      if (d > distance) {
        HCSR04data["direction"] = "away";
        Log.trace(F("HC SR04 Distance changed" CR));
      } else if (d < distance) {
        HCSR04data["direction"] = "towards";
        Log.trace(F("HC SR04 Distance changed" CR));
      } else if (HCSR04_always) {
        HCSR04data["direction"] = "static";
        Log.trace(F("HC SR04 Distance hasn't changed" CR));
      }
      distance = d;
      if (HCSR04data.size() > 0)
        pub(subjectHCSR04, HCSR04data);
    }
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorHCSR501.ino"
# 29 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorHCSR501.ino"
#include "User_config.h"

#ifdef ZsensorHCSR501

void setupHCSR501() {
  Log.notice(F("HCSR501 pin: %d" CR), HCSR501_GPIO);
  pinMode(HCSR501_GPIO, INPUT);
# ifdef HCSR501_LED_NOTIFY_GPIO
  pinMode(HCSR501_LED_NOTIFY_GPIO, OUTPUT);
  digitalWrite(HCSR501_LED_NOTIFY_GPIO, LOW);
# endif
}

void MeasureHCSR501() {
  if (millis() > TimeBeforeStartHCSR501) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject HCSR501data = jsonBuffer.to<JsonObject>();
    static int pirState = LOW;
    int PresenceValue = digitalRead(HCSR501_GPIO);
# if defined(ESP8266) || defined(ESP32)
    yield();
# endif
    if (PresenceValue == HIGH) {
      if (pirState == LOW) {

        HCSR501data["presence"] = "true";
        pirState = HIGH;
      }
    } else {
      if (pirState == HIGH) {

        HCSR501data["presence"] = "false";
        pirState = LOW;
      }
    }
# ifdef HCSR501_LED_NOTIFY_GPIO
    digitalWrite(HCSR501_LED_NOTIFY_GPIO, pirState == HCSR501_LED_ON);
# endif
    if (HCSR501data.size() > 0)
      pub(subjectHCSR501toMQTT, HCSR501data);
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorHTU21.ino"
# 38 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorHTU21.ino"
#include "User_config.h"

#ifdef ZsensorHTU21
# include <stdint.h>

# include "SparkFunHTU21D.h"
# include "Wire.h"
# include "config_HTU21.h"


unsigned long timehtu21 = 0;


HTU21D htuSensor;

void setupZsensorHTU21() {
  delay(10);
  Log.notice(F("HTU21 Initialized - begin()" CR));

# if defined(ESP32)
  Wire.begin(I2C_SDA, I2C_SCL);
  htuSensor.begin(Wire);
# else
  htuSensor.begin();
# endif
}

void MeasureTempHum() {
  if (millis() > (timehtu21 + TimeBetweenReadinghtu21)) {
    Log.trace(F("Read HTU21 Sensor" CR));

    timehtu21 = millis();
    static float persisted_htu_tempc;
    static float persisted_htu_hum;

    float HtuTempC = htuSensor.readTemperature();
    float HtuHum = htuSensor.readHumidity();

    if (HtuTempC >= 998 || HtuHum >= 998) {
      Log.error(F("Failed to read from sensor HTU21!" CR));
      return;
    }


    if (isnan(HtuTempC) || isnan(HtuHum)) {
      Log.error(F("Failed to read from sensor HTU21!" CR));
    } else {
      Log.notice(F("Creating HTU21 buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject HTU21data = jsonBuffer.to<JsonObject>();

      if (HtuTempC != persisted_htu_tempc || htu21_always) {
        float HtuTempF = (HtuTempC * 1.8) + 32;
        HTU21data["tempc"] = (float)HtuTempC;
        HTU21data["tempf"] = (float)HtuTempF;
      } else {
        Log.notice(F("Same Temp. Don't send it" CR));
      }


      if (HtuHum != persisted_htu_hum || htu21_always) {
        HTU21data["hum"] = (float)HtuHum;
      } else {
        Log.notice(F("Same Humidity. Don't send it" CR));
      }

      if (HTU21data.size() > 0)
        pub(HTUTOPIC, HTU21data);
    }
    persisted_htu_tempc = HtuTempC;
    persisted_htu_hum = HtuHum;
  }
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorINA226.ino"
# 35 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorINA226.ino"
#include "User_config.h"

#ifdef ZsensorINA226
# include <Wire.h>

float rShunt = 0.1;
const int INA226_ADDR = 0x40;


unsigned long timeINA226 = 0;

void setupINA226() {
  Wire.begin();

  writeRegister(0x00, 0x4427);
}

void MeasureINA226() {
  if (millis() > (timeINA226 + TimeBetweenReadingINA226)) {
    timeINA226 = millis();
    Log.trace(F("Creating INA226 buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject INA226data = jsonBuffer.to<JsonObject>();

    Log.trace(F("Retrieving electrical data" CR));

    float volt = readRegister(0x02) * 0.00125;

    int shuntvolt = readRegister(0x01);
    if (shuntvolt && 0x8000) {
      shuntvolt = ~shuntvolt;
      shuntvolt += 1;
      shuntvolt *= -1;
    }
    float current = shuntvolt * 0.0000025 / rShunt;
    float power = abs(volt * current);

    char volt_c[7];
    char current_c[7];
    char power_c[7];
    dtostrf(volt, 6, 3, volt_c);
    dtostrf(current, 6, 3, current_c);
    dtostrf(power, 6, 3, power_c);
    INA226data["volt"] = (char*)volt_c;
    INA226data["current"] = (char*)current_c;
    INA226data["power"] = (char*)power_c;
    pub(subjectINA226toMQTT, INA226data);
  }
}

static void writeRegister(byte reg, word value) {
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}

static word readRegister(byte reg) {
  word res = 0x0000;
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() == 0) {
    if (Wire.requestFrom(INA226_ADDR, 2) >= 2) {
      res = Wire.read() * 256;
      res += Wire.read();
    }
  }
  return res;
}

#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorSHTC3.ino"
#include "User_config.h"

#ifdef ZsensorSHTC3
# include <SparkFun_SHTC3.h>

SHTC3 mySHTC3;


unsigned long timedht = 0;
void errorDecoder(SHTC3_Status_TypeDef message);

void errorDecoder(SHTC3_Status_TypeDef message)
{
  switch (message) {
    case SHTC3_Status_Nominal:
      Log.notice("Nominal");
      break;
    case SHTC3_Status_Error:
      Log.error("Error");
      break;
    case SHTC3_Status_CRC_Fail:
      Log.error("CRC Fail");
      break;
    default:
      Log.error("Unknown return code");
      break;
  }
}

void setupSHTC3() {
  Wire.begin();
  errorDecoder(mySHTC3.begin());
}

void MeasureTempAndHum() {
  if (millis() > (timedht + TimeBetweenReadingSHTC3)) {
    timedht = millis();
    static float persistedh;
    static float persistedt;
    SHTC3_Status_TypeDef result = mySHTC3.update();
    if (mySHTC3.lastStatus == SHTC3_Status_Nominal) {

      float t = mySHTC3.toDegC();
      float h = mySHTC3.toPercent();

      if (isnan(h) || isnan(t)) {
        Log.error(F("Failed to read from SHTC3 sensor!" CR));
      } else {
        Log.trace(F("Creating SHTC3 buffer" CR));
        StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
        JsonObject SHTC3data = jsonBuffer.to<JsonObject>();
        if (h != persistedh || shtc3_always) {
          SHTC3data["hum"] = (float)h;
        } else {
          Log.trace(F("Same hum don't send it" CR));
        }
        if (t != persistedt || shtc3_always) {
          SHTC3data["tempc"] = (float)t;
          SHTC3data["tempf"] = mySHTC3.toDegF();
        } else {
          Log.trace(F("Same temp don't send it" CR));
        }
        if (SHTC3data.size() > 0)
          pub(SHTC3TOPIC, SHTC3data);
      }
      persistedh = h;
      persistedt = t;
    } else {
      errorDecoder(mySHTC3.lastStatus);
      Log.error(F("Failed to read from SHTC3 sensor!" CR));
    }
  }
}
#endif
# 1 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorTSL2561.ino"
# 40 "/Users/sgracey/Code/OpenMQTTGatewayProd/main/ZsensorTSL2561.ino"
#include "User_config.h"

#ifdef ZsensorTSL2561
# include <Adafruit_Sensor.h>
# include <Adafruit_TSL2561_U.h>

# include "Wire.h"
# include "math.h"

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void displaySensorDetails(void) {
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Log.trace(F("------------------------------------" CR));
  Log.trace(("Sensor: %s" CR), sensor.name);
  Log.trace(("Driver Ver: %s" CR), sensor.version);
  Log.trace(("Unique ID: %s" CR), sensor.sensor_id);
  Log.trace(("Max Value: %s lux" CR), sensor.max_value);
  Log.trace(("Min Value: %s lux" CR), sensor.min_value);
  Log.trace(("Resolution: %s lux" CR), sensor.resolution);
  Log.trace(F("------------------------------------" CR));
  delay(500);
}

void setupZsensorTSL2561() {
  Log.notice(F("Setup TSL2561 on adress: %H" CR), TSL2561_ADDR_FLOAT);
  Wire.begin();
  Wire.beginTransmission(TSL2561_ADDR_FLOAT);

  if (!tsl.begin()) {
    Log.error(F("No TSL2561 detected" CR));
  }




  tsl.enableAutoRange(true);



  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);

  Log.trace(F("TSL2561 Initialized. Printing detials now." CR));
  displaySensorDetails();
}

void MeasureLightIntensityTSL2561() {
  if (millis() > (timetsl2561 + TimeBetweenReadingtsl2561)) {
    static uint32_t persisted_lux;
    timetsl2561 = millis();

    Log.trace(F("Creating TSL2561 buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject TSL2561data = jsonBuffer.to<JsonObject>();

    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light)

    {
      if (persisted_lux != event.light || tsl2561_always) {
        persisted_lux = event.light;

        TSL2561data["lux"] = (float)event.light;
        TSL2561data["ftcd"] = (float)(event.light) / 10.764;
        TSL2561data["wattsm2"] = (float)(event.light) / 683.0;

        pub(subjectTSL12561toMQTT, TSL2561data);
      } else {
        Log.trace(F("Same lux value, do not send" CR));
      }
    } else {
      Log.error(F("Failed to read from TSL2561" CR));
    }
  }
}
#endif