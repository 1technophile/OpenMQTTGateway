/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the radiofrequency gateways (ZgatewayRF and ZgatewayRF2) with RCswitch and newremoteswitch library
  
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
#ifndef config_RF_h
#define config_RF_h

#include <Arduino.h>
#include <ArduinoJson.h>

#if defined(ESP8266) || defined(ESP32)
#  include <EEPROM.h>
#endif

#ifdef ZgatewayRF
extern void setupRF();
extern void RFtoMQTT();
extern void MQTTtoRF(char* topicOri, char* datacallback);
extern void MQTTtoRF(char* topicOri, JsonObject& RFdata);
extern void disableRFReceive();
extern void enableRFReceive();
#endif
#ifdef ZgatewayRF2
extern void setupRF2();
extern void RF2toMQTT();
extern void MQTTtoRF2(char* topicOri, char* datacallback);
extern void MQTTtoRF2(char* topicOri, JsonObject& RFdata);
extern void disableRF2Receive();
extern void enableRF2Receive();
#endif
#ifdef ZgatewayPilight
extern void setupPilight();
extern void PilighttoMQTT();
extern void MQTTtoPilight(char* topicOri, char* datacallback);
extern void MQTTtoPilight(char* topicOri, JsonObject& RFdata);
extern void disablePilightReceive();
extern void enablePilightReceive();
#endif
#ifdef ZgatewayRTL_433
extern void RTL_433Loop();
extern void setupRTL_433();
extern void MQTTtoRTL_433(char* topicOri, JsonObject& RTLdata);
extern void enableRTLreceive();
extern void disableRTLreceive();
extern int getRTLrssiThreshold();
extern int getRTLCurrentRSSI();
extern int getRTLMessageCount();
extern int getRTLAverageRSSI();
extern int getOOKThresh();

#  ifdef ZmqttDiscovery
extern void launchRTL_433Discovery(bool overrideDiscovery);
// This structure stores the entities of the RTL 433 devices and is they have been discovered or not
// The uniqueId is composed of the device id + the key

#    define uniqueIdSize  60 // longest model + longest key
#    define modelNameSize 31 // longest model

struct RTL_433device {
  char uniqueId[uniqueIdSize];
  char modelName[modelNameSize];
  bool isDisc;
};

const char parameters[40][4][24] = {
    // RTL_433 key, name, unit, device_class
    {"temperature_C", "temperature", "°C", "temperature"},
    {"temperature_1_C", "temperature", "°C", "temperature"},
    {"temperature_2_C", "temperature", "°C", "temperature"},
    {"temperature_F", "temperature", "°F", "temperature"},
    {"time", "timestamp", "", "timestamp"},
    {"battery_ok", "battery", "", "battery"},
    {"humidity", "humidity", "%", "humidity"},
    {"moisture", "moisture", "%", "humidity"},
    {"pressure_hPa", "pressure", "hPa", "pressure"},
    {"pressure_kPa", "pressure", "kPa", "pressure"},
    {"wind_speed_km_h", "wind speed", "km/h", "wind_speed"},
    {"wind_avg_km_h", "wind average", "km/h", "wind_speed"},
    {"wind_avg_mi_h", "wind average", "mi/h", "wind_speed"},
    {"wind_avg_m_s", "wind average", "m/s", "wind_speed"},
    {"wind_speed_m_s", "wind speed", "m/s", "wind_speed"},
    {"gust_speed_km_h", "gust speed", "km/h", "wind_speed"},
    {"wind_max_km_h", "wind max", "km/h", "wind_speed"},
    {"wind_max_m_s", "wind max", "m/s", "wind_speed"},
    {"gust_speed_m_s", "gust speed", "m/s", "wind_speed"},
    {"wind_dir_deg", "wind direction", "°", ""},
    {"rain_mm", "rain", "mm", "precipitation"},
    {"rain_mm_h", "rain", "mm/h", "precipitation"},
    {"rain_in", "rain", "in", "precipitation_intensity"},
    {"rain_rate_in_h", "rain", "in/h", "precipitation_intensity"},
    {"rssi", "rssi", "dB", "signal_strength"},
    {"snr", "snr", "dB", ""},
    {"noise", "noise", "dB", ""},
    {"depth_cm", "depth", "cm", ""},
    {"power_W", "power", "W", "power"},
    {"light_lux", "light", "lx", ""},
    {"lux", "lux", "lx", ""},
    {"uv", "uv", "UV index", ""},
    {"uvi", "uvi", "UV index", ""},
    {"storm_dist", "storm distance", "mi", ""},
    {"strike_distance", "strike distance", "mi", ""},
    {"tamper", "tamper", "", ""},
    {"alarm", "alarm", "", ""},
    {"motion", "motion", "", "motion"},
    {"strike_count", "strike count", "", ""}, // from rtl_433_mqtt_hass.py
    {"event", "Status", "", "moisture"}};
#  endif
#  ifdef RTL_433_DISCOVERY_LOGGING
#    define DISCOVERY_TRACE_LOG(...) Log.trace(__VA_ARGS__)
#  else
#    define DISCOVERY_TRACE_LOG(...)
#  endif
#endif
/*-------------------RF topics & parameters----------------------*/
//433Mhz MQTT Subjects and keys
#define subjectMQTTtoRF    "/commands/MQTTto433"
#define subjectRFtoMQTT    "/433toMQTT"
#define subjectGTWRFtoMQTT "/433toMQTT"
#define RFprotocolKey      "433_" // protocol will be defined if a subject contains RFprotocolKey followed by a value of 1 digit
#define RFbitsKey          "RFBITS_" // bits  will be defined if a subject contains RFbitsKey followed by a value of 2 digits
#define repeatRFwMQTT      false // do we repeat a received signal by using MQTT with RF gateway
#define RFpulselengthKey   "PLSL_" // pulselength will be defined if a subject contains RFprotocolKey followed by a value of 3 digits
// subject monitored to listen traffic processed by other gateways to store data and avoid ntuple
#define subjectMultiGTWRF "+/+/433toMQTT"
//RF number of signal repetition - Can be overridden by specifying "repeat" in a JSON message.
#define RF_EMITTER_REPEAT  20
#define RF2_EMITTER_REPEAT 2 // Actual repeats is 2^R, where R is the here configured amount
//#define RF_DISABLE_TRANSMIT //Uncomment this line to disable RF transmissions. (RF Receive will work as normal.)
#define RFmqttDiscovery true //uncomment this line so as to create a discovery switch for each RF signal received

/*-------------------RF2 topics & parameters----------------------*/
//433Mhz newremoteswitch MQTT Subjects and keys
#define subjectMQTTtoRF2    "/commands/MQTTtoRF2"
#define subjectRF2toMQTT    "/RF2toMQTT"
#define subjectGTWRF2toMQTT "/RF2toMQTT"
#define RF2codeKey          "ADDRESS_" // code will be defined if a subject contains RF2codeKey followed by a value of 7 digits
#define RF2periodKey        "PERIOD_" // period  will be defined if a subject contains RF2periodKey followed by a value of 3 digits
#define RF2unitKey          "UNIT_" // number of your unit value  will be defined if a subject contains RF2unitKey followed by a value of 1-2 digits
#define RF2groupKey         "GROUP_" // number of your group value  will be defined if a subject contains RF2groupKey followed by a value of 1 digit
#define RF2dimKey           "DIM" // number of your dim value will be defined if a subject contains RF2dimKey and the payload contains the dim value as digits

/*-------------------ESPPiLight topics & parameters----------------------*/
//433Mhz Pilight MQTT Subjects and keys
#define subjectMQTTtoPilight         "/commands/MQTTtoPilight"
#define subjectMQTTtoPilightProtocol "/commands/MQTTtoPilight/protocols"
#define subjectPilighttoMQTT         "/PilighttoMQTT"
#define subjectGTWPilighttoMQTT      "/PilighttoMQTT"
#define repeatPilightwMQTT           false // do we repeat a received signal by using MQTT with Pilight gateway
//#define Pilight_rawEnabled true   // enables Pilight RAW return - switchable via MQTT

/*-------------------RTL_433 topics & parameters----------------------*/
//433Mhz RTL_433 MQTT Subjects and keys
#define subjectMQTTtoRTL_433 "/commands/MQTTtoRTL_433"
#define subjectRTL_433toMQTT "/RTL_433toMQTT"

/*-------------------CC1101 frequency----------------------*/
//Match frequency to the hardware version of the radio if ZradioCC1101 is used.
#ifndef CC1101_FREQUENCY
#  define CC1101_FREQUENCY 433.92
#endif
// Allow ZGatewayRF Module to change receive frequency of CC1101 Transceiver module
#if defined(ZradioCC1101) || defined(ZradioSX127x)
float receiveMhz = CC1101_FREQUENCY;
#endif
/*-------------------CC1101 DefaultTXPower----------------------*/
//Adjust the default TX-Power for sending radio if ZradioCC1101 is used.
//The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
#ifndef RF_CC1101_TXPOWER
#  define RF_CC1101_TXPOWER 12
#endif

/*-------------------PIN DEFINITIONS----------------------*/
#ifndef RF_RECEIVER_GPIO
#  ifdef ESP8266
#    define RF_RECEIVER_GPIO 0 // D3 on nodemcu // put 4 with rf bridge direct mod
#  elif ESP32
#    define RF_RECEIVER_GPIO 27 // D27 on DOIT ESP32
#  elif __AVR_ATmega2560__
#    define RF_RECEIVER_GPIO 1 //1 = D3 on mega
#  else
#    define RF_RECEIVER_GPIO 1 //1 = D3 on arduino
#  endif
#endif

#ifndef RF_EMITTER_GPIO
#  ifdef ESP8266
#    define RF_EMITTER_GPIO 3 // RX on nodemcu if it doesn't work with 3, try with 4 (D2) // put 5 with rf bridge direct mod
#  elif ESP32
#    define RF_EMITTER_GPIO 12 // D12 on DOIT ESP32
#  elif __AVR_ATmega2560__
#    define RF_EMITTER_GPIO 4
#  else
//IMPORTANT NOTE: On arduino UNO connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/boarddefs.h so as to free pin D3 for RF RECEIVER PIN
//RF PIN definition
#    define RF_EMITTER_GPIO 4 //4 = D4 on arduino
#  endif
#endif

#if defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRTL_433) || defined(ZgatewayRF2)
/**
 * Active Receiver Module
 * 1 = ZgatewayPilight
 * 2 = ZgatewayRF
 * 3 = ZgatewayRTL_433
 * 4 = ZgatewayRF2
 */
int activeReceiver = 0;
#  define ACTIVE_RECERROR 0
#  define ACTIVE_PILIGHT  1
#  define ACTIVE_RF       2
#  define ACTIVE_RTL      3
#  define ACTIVE_RF2      4

#  if defined(ZradioCC1101) || defined(ZradioSX127x)
bool validFrequency(float mhz) {
  //  CC1101 valid frequencies 300-348 MHZ, 387-464MHZ and 779-928MHZ.
  if (mhz >= 300 && mhz <= 348)
    return true;
  if (mhz >= 387 && mhz <= 464)
    return true;
  if (mhz >= 779 && mhz <= 928)
    return true;
  return false;
}
#  endif

int currentReceiver = -1;

#  if !defined(ZgatewayRFM69) && !defined(ZactuatorSomfy)
#    if defined(ESP8266) || defined(ESP32)
// Check if a receiver is available
bool validReceiver(int receiver) {
  switch (receiver) {
#      ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      return true;
#      endif
#      ifdef ZgatewayRF
    case ACTIVE_RF:
      return true;
#      endif
#      ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      return true;
#      endif
#      ifdef ZgatewayRF2
    case ACTIVE_RF2:
      return true;
#      endif
    default:
      Log.error(F("ERROR: stored receiver %d not available" CR), receiver);
  }
  return false;
}
#    endif
#  endif

void enableActiveReceiver(bool isBoot) {
// Save currently active receiver and restore after reboot.
// Only works with ESP and if there is no conflict.
#  if !defined(ZgatewayRFM69) && !defined(ZactuatorSomfy)
#    if defined(ESP8266) || defined(ESP32)
#      define _ACTIVE_RECV_MAGIC 0xA1B2C3D4

  struct {
    uint64_t magic;
    int receiver;
  } data;

  EEPROM.begin(sizeof(data));
  EEPROM.get(0, data);
  if (isBoot && data.magic == _ACTIVE_RECV_MAGIC && validReceiver(data.receiver)) {
    activeReceiver = data.receiver;
  } else {
    data.magic = _ACTIVE_RECV_MAGIC;
    data.receiver = activeReceiver;
    EEPROM.put(0, data);
  }
  EEPROM.end();
#    endif
#  endif

  // if (currentReceiver != activeReceiver) {
  Log.trace(F("enableActiveReceiver: %d" CR), activeReceiver);
  switch (activeReceiver) {
#  ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      enablePilightReceive();
      break;
#  endif
#  ifdef ZgatewayRF
    case ACTIVE_RF:
      enableRFReceive();
      break;
#  endif
#  ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      enableRTLreceive();
      break;
#  endif
#  ifdef ZgatewayRF2
    case ACTIVE_RF2:
      enableRF2Receive();
      break;
#  endif
#  ifndef ARDUINO_AVR_UNO // Space issues with the UNO
    default:
      Log.error(F("ERROR: unsupported receiver %d" CR), activeReceiver);
#  endif
  }
  currentReceiver = activeReceiver;
}

void disableActiveReceiver() {
  Log.trace(F("disableActiveReceiver: %d" CR), activeReceiver);
  switch (activeReceiver) {
#  ifdef ZgatewayPilight
    case ACTIVE_PILIGHT:
      disablePilightReceive();
      break;
#  endif
#  ifdef ZgatewayRF
    case ACTIVE_RF:
      disableRFReceive();
      break;
#  endif
#  ifdef ZgatewayRTL_433
    case ACTIVE_RTL:
      disableRTLreceive();
      break;
#  endif
#  ifdef ZgatewayRF2
    case ACTIVE_RF2:
      disableRF2Receive();
      break;
#  endif
#  ifndef ARDUINO_AVR_UNO // Space issues with the UNO
    default:
      Log.error(F("ERROR: unsupported receiver %d" CR), activeReceiver);
#  endif
  }
}

#endif

#endif
