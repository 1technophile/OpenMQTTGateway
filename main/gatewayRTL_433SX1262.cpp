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
      data["strike_distance"] = r.lightningDistKm;
  }
  if (r.caps & CAP_PM) {
    data["pm2_5_ug_m3"] = r.pm25;
    data["pm10_ug_m3"] = r.pm10;
  }
  if (r.caps & CAP_CO2)
    data["co2_ppm"] = r.co2;
  if (r.caps & CAP_SOIL)
    data["moisture"] = r.soilMoisture;

  String topic = subjectRTL_433toMQTT;
#  if valueAsATopic
  topic = topic + "/" + String(r.model) + "/" + String(r.id);
#  endif
  data["origin"] = (char*)topic.c_str();

  enqueueJsonObject(data);
  storeSignalValue(MQTTvalue);
}

void RTL_433SX1262Loop() {
  if (!rtl433sx1262_rxFlag)
    return;
  rtl433sx1262_rxFlag = false;

  uint8_t buf[RTL433_SX1262_PKT_LEN];
  int state = rtl433sx1262_radio.readData(buf, RTL433_SX1262_PKT_LEN);
  float rssi = rtl433sx1262_radio.getRSSI();
  rtl433sx1262_radio.startReceive();

  if (state != RADIOLIB_ERR_NONE)
    return;

  for (size_t i = 0; i < RTL433SX1262_DRIVER_COUNT; i++) {
    const SensorDriver* drv = RTL433SX1262_DRIVERS[i];
    if (!drv->match(buf, RTL433_SX1262_PKT_LEN))
      continue;
    SensorReading reading;
    if (drv->parse(buf, RTL433_SX1262_PKT_LEN, rssi, reading)) {
      rtl433sx1262_publish(reading);
      return;
    }
  }
}

#endif
