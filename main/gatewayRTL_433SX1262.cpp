/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
 - publish MQTT data related to received Fine Offset/Ecowitt/LaCrosse weather
   sensor signals ("Group A" protocol family, 868.35 MHz GFSK), received
   directly through an SX1262 radio via RadioLib.
 - support boards such as the Heltec WiFi LoRa 32 V3/V4, whose onboard SX1262
   radio the rtl_433_ESP library (used by ZgatewayRTL_433) cannot drive.

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

#ifdef ZgatewayRTL_433SX1262
#  include <RadioLib.h>
#  include <SPI.h>

#  include "TheengsCommon.h"
#  include "config_RF.h"
#  include "config_RTL_433SX1262.h"
#  include "rtl_433sx1262/sensor_drivers.h"

#  ifdef ZmqttDiscovery
#    include <vector>

#    include "config_mqttDiscovery.h"

struct RTL433SX1262Device {
  uint32_t id;
  char model[16];
  uint16_t caps;
  bool hasBattV;
  bool isDisc;
};
static std::vector<RTL433SX1262Device> rtl433sx1262Devices;
#  endif

static String sx1262Topic(uint32_t id, const char* model) {
  String topic = subjectRTL_433toMQTT;
#  if valueAsATopic
  topic = topic + "/" + String(model) + "/" + String(id);
#  endif
  return topic;
}

#  ifndef IRAM_ATTR
#    define IRAM_ATTR
#  endif

static SX1262 rtl433sx1262_radio = new Module(RTL433_SX1262_NSS, RTL433_SX1262_DIO1, RTL433_SX1262_RST, RTL433_SX1262_BUSY);
static volatile bool rtl433sx1262_rxFlag = false;

static void IRAM_ATTR rtl433sx1262_onRxDone() {
  rtl433sx1262_rxFlag = true;
}

void setupRTL_433SX1262() {
#  if defined(ARDUINO_ARCH_ESP32)
  SPI.begin(RTL433_SX1262_SCK, RTL433_SX1262_MISO, RTL433_SX1262_MOSI, RTL433_SX1262_NSS);
#  else
  SPI.begin();
#  endif

  float bitrateKbps = RTL433_SX1262_BITRATE_BPS / 1000.0f;
  int state = rtl433sx1262_radio.beginFSK(RTL433_SX1262_FREQ_MHZ, bitrateKbps, RTL433_SX1262_FREQ_DEV_KHZ,
                                          RTL433_SX1262_BW_KHZ, RTL433_SX1262_TX_POWER, RTL433_SX1262_PREAMBLE_LEN,
                                          RTL433_SX1262_TCXO_VOLTAGE);
  if (state != RADIOLIB_ERR_NONE) {
    THEENGS_LOG_ERROR(F("[RTL_433SX1262] beginFSK failed, code %d" CR), state);
    return;
  }

  uint16_t sw = RTL433_SX1262_SYNCWORD;
  uint8_t syncWord[] = {(uint8_t)((sw >> 8) & 0xFF), (uint8_t)(sw & 0xFF)};
  rtl433sx1262_radio.setSyncWord(syncWord, sizeof(syncWord));
  rtl433sx1262_radio.fixedPacketLengthMode(RTL433_SX1262_PKT_LEN);
  rtl433sx1262_radio.setCRC(0); // CRC/checksum is verified in software by the per-sensor parsers
  rtl433sx1262_radio.setRxBoostedGainMode(true);
  rtl433sx1262_radio.setDio2AsRfSwitch(true);
  rtl433sx1262_radio.setDio1Action(rtl433sx1262_onRxDone);
  rtl433sx1262_radio.startReceive();

  THEENGS_LOG_NOTICE(F("[RTL_433SX1262] Freq: %sMHz Bitrate: %dbps Sync: 0x%04X PktLen: %d" CR),
                     String(RTL433_SX1262_FREQ_MHZ).c_str(), RTL433_SX1262_BITRATE_BPS, (unsigned)RTL433_SX1262_SYNCWORD, RTL433_SX1262_PKT_LEN);
  THEENGS_LOG_TRACE(F("gatewayRTL_433SX1262 setup done" CR));
}

#  ifdef ZmqttDiscovery
// Matches the {key, name, unit, device_class} rows used by gatewayRTL_433.cpp's
// discovery so both gateways surface the same entities for the same fields.
struct SX1262DiscoveryParam {
  const char* key;
  const char* name;
  const char* unit;
  const char* deviceClass;
};

static const SX1262DiscoveryParam SX1262_DISCOVERY_PARAMS[] = {
    {"temperature_C", "Temperature", HASS_UNIT_CELSIUS, HASS_CLASS_TEMPERATURE},
    {"humidity", "Humidity", HASS_UNIT_PERCENT, HASS_CLASS_HUMIDITY},
    {"pressure_hPa", "Pressure", HASS_UNIT_HPA, HASS_CLASS_PRESSURE},
    {"rain_mm", "Rain", HASS_UNIT_MM, HASS_CLASS_PRECIPITATION},
    {"wind_avg_m_s", "Wind average", HASS_UNIT_MS, HASS_CLASS_WIND_SPEED},
    {"wind_max_m_s", "Wind max", HASS_UNIT_MS, HASS_CLASS_WIND_SPEED},
    {"wind_dir_deg", "Wind direction", HASS_UNIT_DEGREE, ""},
    {"uv", "UV value", "", ""},
    {"light_lux", "Illuminance", HASS_UNIT_LX, HASS_CLASS_ILLUMINANCE},
    {"strike_count", "Strike count", "", ""},
    {"strike_distance_km", "Strike distance", "km", HASS_CLASS_DISTANCE},
    {"pm2_5_ug_m3", "PM2.5", HASS_UNIT_UGM3, HASS_CLASS_PM25},
    {"pm10_ug_m3", "PM10", HASS_UNIT_UGM3, HASS_CLASS_PM10},
    {"co2_ppm", "Carbon Dioxide", HASS_UNIT_PPM, HASS_CLASS_CARBON_DIOXIDE},
    {"moisture", "Moisture", HASS_UNIT_PERCENT, HASS_CLASS_HUMIDITY},
};

static void discoverParam(const char* key, const char* topic, const char* deviceId, const char* model, const char* stateClass) {
  for (const auto& p : SX1262_DISCOVERY_PARAMS) {
    if (strcmp(p.key, key) == 0) {
      String value_template = "{{ value_json." + String(key) + " | is_defined }}";
      String unique_id = String(deviceId) + "-" + key;
      createDiscovery("sensor",
                      topic, p.name, unique_id.c_str(),
                      "", p.deviceClass, value_template.c_str(),
                      "", "", p.unit,
                      0,
                      "", "", false, "",
                      deviceId, "", model, deviceId, false,
                      stateClass);
      return;
    }
  }
  THEENGS_LOG_WARNING(F("[RTL_433SX1262] discovery key %s not found" CR), key);
}

static void emitSX1262Discovery(const RTL433SX1262Device& dev, const String& topicStr) {
  String deviceIdStr = String(dev.model) + "-" + String(dev.id);
  const char* deviceId = deviceIdStr.c_str();
  const char* topic = topicStr.c_str();

  if (dev.caps & CAP_TEMP)
    discoverParam("temperature_C", topic, deviceId, dev.model, stateClassMeasurement);
  if (dev.caps & CAP_HUM)
    discoverParam("humidity", topic, deviceId, dev.model, stateClassMeasurement);
  if (dev.caps & CAP_PRESSURE)
    discoverParam("pressure_hPa", topic, deviceId, dev.model, stateClassMeasurement);
  if (dev.caps & CAP_RAIN)
    discoverParam("rain_mm", topic, deviceId, dev.model, stateClassTotalIncreasing);
  if (dev.caps & CAP_WIND) {
    discoverParam("wind_avg_m_s", topic, deviceId, dev.model, stateClassMeasurement);
    discoverParam("wind_max_m_s", topic, deviceId, dev.model, stateClassMeasurement);
    discoverParam("wind_dir_deg", topic, deviceId, dev.model, stateClassMeasurement);
  }
  if (dev.caps & CAP_UV) {
    discoverParam("uv", topic, deviceId, dev.model, stateClassMeasurement);
    discoverParam("light_lux", topic, deviceId, dev.model, stateClassMeasurement);
  }
  if (dev.caps & CAP_LIGHTNING) {
    discoverParam("strike_count", topic, deviceId, dev.model, stateClassTotalIncreasing);
    discoverParam("strike_distance_km", topic, deviceId, dev.model, stateClassMeasurement);
  }
  if (dev.caps & CAP_PM) {
    discoverParam("pm2_5_ug_m3", topic, deviceId, dev.model, stateClassMeasurement);
    discoverParam("pm10_ug_m3", topic, deviceId, dev.model, stateClassMeasurement);
  }
  if (dev.caps & CAP_CO2)
    discoverParam("co2_ppm", topic, deviceId, dev.model, stateClassMeasurement);
  if (dev.caps & CAP_SOIL)
    discoverParam("moisture", topic, deviceId, dev.model, stateClassMeasurement);

  // battery_ok is 0/1, mirrored as a percent sensor for consistency with gatewayRTL_433.cpp
  String battOkTemplate = "{{ (float(value_json.battery_ok) * 100) | round(0) | is_defined }}";
  String battOkUniqueId = String(deviceId) + "-battery_ok";
  createDiscovery("sensor",
                  topic, "Battery", battOkUniqueId.c_str(),
                  "", HASS_CLASS_BATTERY, battOkTemplate.c_str(),
                  "", "", HASS_UNIT_PERCENT,
                  0,
                  "", "", false, "",
                  deviceId, "", dev.model, deviceId, false,
                  stateClassMeasurement);

  if (dev.hasBattV) {
    String battVTemplate = "{{ value_json.battery_V | is_defined }}";
    String battVUniqueId = String(deviceId) + "-battery_V";
    createDiscovery("sensor",
                    topic, "Battery Voltage", battVUniqueId.c_str(),
                    "", HASS_CLASS_VOLTAGE, battVTemplate.c_str(),
                    "", "", HASS_UNIT_VOLT,
                    0,
                    "", "", false, "",
                    deviceId, "", dev.model, deviceId, false,
                    stateClassMeasurement);
  }
}

void launchRTL_433SX1262Discovery(bool overrideDiscovery) {
  if (!SYSConfig.discovery)
    return;
  for (auto& dev : rtl433sx1262Devices) {
    if (overrideDiscovery || !dev.isDisc) {
      emitSX1262Discovery(dev, sx1262Topic(dev.id, dev.model));
      dev.isDisc = true;
    }
  }
}
#  endif

static void rtl433sx1262_publish(const SensorReading& r) {
  unsigned long MQTTvalue = r.id * 1000UL + (unsigned long)round(r.tempC * 10.0f + 500.0f) + (unsigned long)round(r.humidity) + (unsigned long)round(r.rainMm * 10.0f) + (unsigned long)round(r.windAvgMs * 10.0f) + (unsigned long)r.lightningCount + (unsigned long)r.soilMoisture + (unsigned long)round(r.pm25 * 10.0f) + (unsigned long)r.co2;
  if (isAduplicateSignal(MQTTvalue))
    return;

  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject data = jsonBuffer.to<JsonObject>();

  data["model"] = r.model;
  data["id"] = r.id;
  data["rssi"] = r.rssi;
  data["battery_ok"] = r.battLow ? 0 : 1;
  if (r.battV > 0)
    data["battery_V"] = r.battV;

  if (r.caps & CAP_TEMP)
    data["temperature_C"] = r.tempC;
  if (r.caps & CAP_HUM)
    data["humidity"] = r.humidity;
  if (r.caps & CAP_PRESSURE)
    data["pressure_hPa"] = r.pressureHpa;
  if (r.caps & CAP_RAIN)
    data["rain_mm"] = r.rainMm;
  if (r.caps & CAP_WIND) {
    data["wind_avg_m_s"] = r.windAvgMs;
    data["wind_max_m_s"] = r.windGustMs;
    data["wind_dir_deg"] = r.windDirDeg;
  }
  if (r.caps & CAP_UV) {
    data["uv"] = r.uvIndex;
    data["light_lux"] = r.lightLux;
  }
  if (r.caps & CAP_LIGHTNING) {
    data["strike_count"] = r.lightningCount;
    if (r.lightningDistKm != 63)
      data["strike_distance_km"] = r.lightningDistKm;
  }
  if (r.caps & CAP_PM) {
    data["pm2_5_ug_m3"] = r.pm25;
    data["pm10_ug_m3"] = r.pm10;
  }
  if (r.caps & CAP_CO2)
    data["co2_ppm"] = r.co2;
  if (r.caps & CAP_SOIL)
    data["moisture"] = r.soilMoisture;

  String topic = sx1262Topic(r.id, r.model);
  data["origin"] = (char*)topic.c_str();

#  ifdef ZmqttDiscovery
  if (SYSConfig.discovery) {
    RTL433SX1262Device* dev = nullptr;
    for (auto& d : rtl433sx1262Devices) {
      if (d.id == r.id && strcmp(d.model, r.model) == 0) {
        dev = &d;
        break;
      }
    }
    if (!dev) {
      RTL433SX1262Device newDev;
      newDev.id = r.id;
      strlcpy(newDev.model, r.model, sizeof(newDev.model));
      newDev.caps = 0;
      newDev.hasBattV = false;
      newDev.isDisc = false;
      rtl433sx1262Devices.push_back(newDev);
      dev = &rtl433sx1262Devices.back();
    }
    dev->caps = r.caps;
    dev->hasBattV = dev->hasBattV || (r.battV > 0);
    if (!dev->isDisc) {
      emitSX1262Discovery(*dev, topic);
      dev->isDisc = true;
    }
  }
#  endif

  enqueueJsonObject(data);
  storeSignalValue(MQTTvalue);
}

static void rtl433sx1262_logHex(const uint8_t* buf, size_t len) {
  char hex[RTL433_SX1262_PKT_LEN * 3 + 1];
  size_t pos = 0;
  for (size_t i = 0; i < len && pos + 3 < sizeof(hex); i++)
    pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X ", buf[i]);
  THEENGS_LOG_TRACE(F("[RTL_433SX1262] raw: %s" CR), hex);
}

void RTL_433SX1262Loop() {
  if (!rtl433sx1262_rxFlag)
    return;
  rtl433sx1262_rxFlag = false;

  uint8_t buf[RTL433_SX1262_PKT_LEN];
  int state = rtl433sx1262_radio.readData(buf, RTL433_SX1262_PKT_LEN);
  float rssi = rtl433sx1262_radio.getRSSI();
  rtl433sx1262_radio.startReceive();

  if (state != RADIOLIB_ERR_NONE) {
    THEENGS_LOG_TRACE(F("[RTL_433SX1262] readData failed, code %d, rssi %.1f" CR), state, rssi);
    return;
  }

  THEENGS_LOG_TRACE(F("[RTL_433SX1262] packet received, rssi %.1f" CR), rssi);
  rtl433sx1262_logHex(buf, RTL433_SX1262_PKT_LEN);

  for (size_t i = 0; i < RTL433SX1262_DRIVER_COUNT; i++) {
    const SensorDriver* drv = RTL433SX1262_DRIVERS[i];
    if (!drv->match(buf, RTL433_SX1262_PKT_LEN))
      continue;
    SensorReading reading;
    if (drv->parse(buf, RTL433_SX1262_PKT_LEN, rssi, reading)) {
      THEENGS_LOG_NOTICE(F("[RTL_433SX1262] Decoded %s id=%lu rssi=%.1f" CR), reading.model, (unsigned long)reading.id, rssi);
      rtl433sx1262_publish(reading);
      return;
    }
    THEENGS_LOG_TRACE(F("[RTL_433SX1262] %s sync matched but parse/CRC failed" CR), drv->model);
  }
}

#endif
