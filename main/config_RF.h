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
extern void RFtoX();
extern void XtoRF(const char* topicOri, const char* datacallback);
extern void XtoRF(const char* topicOri, JsonObject& RFdata);
extern void disableRFReceive();
extern void enableRFReceive();
#endif
#ifdef ZgatewayRF2
extern void RF2toX();
extern void XtoRF2(const char* topicOri, const char* datacallback);
extern void XtoRF2(const char* topicOri, JsonObject& RFdata);
extern void disableRF2Receive();
extern void enableRF2Receive();
#endif
#ifdef ZgatewayPilight
extern void setupPilight();
extern void PilighttoX();
extern void XtoPilight(const char* topicOri, const char* datacallback);
extern void XtoPilight(const char* topicOri, JsonObject& RFdata);
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
#    define typeSize      10 // longest type

struct RTL_433device {
  char uniqueId[uniqueIdSize];
  char modelName[modelNameSize];
  char type[typeSize];
  bool isDisc;
};

const char parameters[51][4][24] = {
    // RTL_433 key, name, unit, device_class
    {"alarm", "Alarm", "", ""},
    {"battery_mV", "Battery Voltage", "mV", "voltage"},
    {"battery_ok", "Battery", "", "battery"},
    {"co2_ppm", "Carbon Dioxide", "ppm", "carbon_dioxide"},
    {"depth_cm", "Depth", "cm", "distance"},
    {"estimated_pm10_0_ug_m3", "Estimated PM10", "μg/m³", "pm10"},
    {"event", "Status", "", ""},
    {"group", "Group", "", ""},
    {"gust_speed_km_h", "Gust speed", "km/h", "wind_speed"},
    {"gust_speed_m_s", "Gust speed", "m/s", "wind_speed"},
    {"humidity", "Humidity", "%", "humidity"},
    {"light_lux", "Illuminance", "lx", "illuminance"},
    {"lux", "Illuminance", "lx", "illuminance"},
    {"moisture", "Moisture", "%", "humidity"},
    {"motion", "Motion", "", "motion"},
    {"noise", "Noise", "dB", "sound_pressure"},
    {"pm1_ug_m3", "PM1", "μg/m³", "pm1"},
    {"pm10_ug_m3", "PM10", "μg/m³", "pm10"},
    {"pm2_5_ug_m3", "PM2.5", "μg/m³", "pm25"},
    {"power_W", "Power", "W", "power"},
    {"pressure_hPa", "Pressure", "hPa", "pressure"},
    {"pressure_kPa", "Pressure", "kPa", "pressure"},
    {"rain_in", "Rain", "in", "precipitation"},
    {"rain_mm", "Rain", "mm", "precipitation"},
    {"rain_rate_in_h", "Rain", "in/h", "precipitation_intensity"},
    {"rain_rate_mm_h", "Rain", "mm/h", "precipitation_intensity"},
    {"rssi", "RSSI", "dB", "signal_strength"},
    {"snr", "SNR", "dB", ""},
    {"state", "State", "", ""},
    {"storm_dist_km", "Storm distance", "km", "distance"},
    {"storm_dist", "Storm distance", "mi", "distance"},
    {"strike_count", "Strike count", "", ""}, // from rtl_433_mqtt_hass.py
    {"strike_distance_km", "Strike distance", "km", "distance"},
    {"strike_distance", "Strike distance", "mi", "distance"},
    {"tamper", "Tamper", "", ""},
    {"temperature_1_C", "Temperature", "°C", "temperature"},
    {"temperature_2_C", "Temperature", "°C", "temperature"},
    {"temperature_C", "Temperature", "°C", "temperature"},
    {"temperature_F", "Temperature", "°F", "temperature"},
    {"time", "Timestamp", "", "timestamp"},
    {"unit", "Unit", "", ""},
    {"uv", "UV", "UV level", ""},
    {"uvi", "UVI", "UV index", ""},
    {"wind_avg_km_h", "Wind average", "km/h", "wind_speed"},
    {"wind_avg_m_s", "Wind average", "m/s", "wind_speed"},
    {"wind_avg_mi_h", "Wind average", "mi/h", "wind_speed"},
    {"wind_dir_deg", "Wind direction", "°", ""},
    {"wind_max_km_h", "Wind max", "km/h", "wind_speed"},
    {"wind_max_m_s", "Wind max", "m/s", "wind_speed"},
    {"wind_speed_km_h", "Wind speed", "km/h", "wind_speed"},
    {"wind_speed_m_s", "Wind speed", "m/s", "wind_speed"}};
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
