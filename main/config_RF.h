/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
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

#ifdef ZgatewayRF
extern void setupRF();
extern void RFtoMQTT();
extern void MQTTtoRF(char* topicOri, char* datacallback);
extern void MQTTtoRF(char* topicOri, JsonObject& RFdata);
extern void disableRFReceive();
extern void enableRFReceive();
#endif
#ifdef ZgatewayRF2
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
#  include <rtl_433_ESP.h>
rtl_433_ESP rtl_433;

extern void RTL_433Loop();
extern void setupRTL_433();
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

const char parameters[50][4][24] = {
    // RTL_433 key, name, unit, device_class
    {"temperature_C", "temperature", "°C", "temperature"},
    {"temperature_1_C", "temperature", "°C", "temperature"},
    {"temperature_2_C", "temperature", "°C", "temperature"},
    {"temperature_F", "temperature", "°F", "temperature"},
    {"time", "timestamp", "", "timestamp"},
    {"battery_ok", "battery", "%", "battery"},
    {"battery_mV", "battery", "mV", "voltage"},
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
    {"rain_rate_mm_h", "rain", "mm/h", "precipitation_intensity"},
    {"rain_in", "rain", "in", "precipitation"},
    {"rain_rate_in_h", "rain", "in/h", "precipitation_intensity"},
    {"rssi", "rssi", "dB", "signal_strength"},
    {"snr", "snr", "dB", ""},
    {"noise", "noise", "dB", "sound_pressure"},
    {"depth_cm", "depth", "cm", "distance"},
    {"power_W", "power", "W", "power"},
    {"light_lux", "light", "lx", "illuminance"},
    {"lux", "lux", "lx", "illuminance"},
    {"uvi", "UVI", "UV index", ""},
    {"uv", "UV", "UV level", ""},
    {"storm_dist", "storm distance", "mi", "distance"},
    {"storm_dist_km", "storm distance", "km", "distance"},
    {"strike_count", "strike count", "", ""}, // from rtl_433_mqtt_hass.py
    {"strike_distance", "strike distance", "mi", "distance"},
    {"strike_distance_km", "strike distance", "km", "distance"},
    {"co2_ppm", "Carbon Dioxide", "ppm", "carbon_dioxide"},
    {"pm2_5_ug_m3", "PM2.5", "μg/m³", "pm25"},
    {"pm10_ug_m3", "PM10", "μg/m³", "pm10"},
    {"estimated_pm10_0_ug_m3", "estimated PM10", "μg/m³", "pm10"},
    {"pm1_ug_m3", "PM1", "μg/m³", "pm1"},
    {"tamper", "tamper", "", ""},
    {"alarm", "alarm", "", ""},
    {"motion", "motion", "", "motion"},
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
#define subjectMQTTtoRF       "/commands/MQTTto433"
#define subjectRFtoMQTT       "/433toMQTT"
#define subjectcommonRFtoMQTT "/RFtoMQTT"
#define subjectGTWRFtoMQTT    "/433toMQTT"
#define RFprotocolKey         "433_" // protocol will be defined if a subject contains RFprotocolKey followed by a value of 1 digit
#define RFbitsKey             "RFBITS_" // bits  will be defined if a subject contains RFbitsKey followed by a value of 2 digits
#define repeatRFwMQTT         false // do we repeat a received signal by using MQTT with RF gateway
#define RFpulselengthKey      "PLSL_" // pulselength will be defined if a subject contains RFprotocolKey followed by a value of 3 digits
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
#define subjectMQTTtoRFset   "/commands/MQTTtoRF/config"
#define subjectRTL_433toMQTT "/RTL_433toMQTT"

/*-------------------RF frequency----------------------*/
//Match frequency to the hardware version of the radio used.
#ifndef RF_FREQUENCY
#  define RF_FREQUENCY 433.92
#endif

/**
 * Active  Module
 * 1 = ZgatewayPilight
 * 2 = ZgatewayRF
 * 3 = ZgatewayRTL_433
 * 4 = ZgatewayRF2
 */

struct RFConfig_s {
  float frequency;
  int rssiThreshold;
  int newOokThreshold;
  int activeReceiver;
};

#define ACTIVE_NONE     -1
#define ACTIVE_RECERROR 0
#define ACTIVE_PILIGHT  1
#define ACTIVE_RF       2
#define ACTIVE_RTL      3
#define ACTIVE_RF2      4

RFConfig_s RFConfig;

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
#  endif
#endif

#ifndef RF_EMITTER_GPIO
#  ifdef ESP8266
#    define RF_EMITTER_GPIO 3 // RX on nodemcu if it doesn't work with 3, try with 4 (D2) // put 5 with rf bridge direct mod
#  elif ESP32
#    define RF_EMITTER_GPIO 12 // D12 on DOIT ESP32
#  else
//IMPORTANT NOTE: On arduino UNO connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/boarddefs.h so as to free pin D3 for RF RECEIVER PIN
//RF PIN definition
#    define RF_EMITTER_GPIO 4 //4 = D4 on arduino
#  endif
#endif

#endif
