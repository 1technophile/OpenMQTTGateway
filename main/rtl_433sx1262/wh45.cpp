// ============================================================================
// wh45.cpp — Fine Offset / Ecowitt WH45 air quality (PM2.5 / PM10 / CO2 + T/H)
// 15-byte frame: YY IIII 0T TT HH Bp pp BP PP CC CC XX AA
//   (ref. rtl_433 fineoffset_wh45.c)
//   YY=0x45; CRC-8/0x31 of b[0..12]==b[13]; SUM8(b[0..13])==b[14]
//   temp = ((b[4]&0x07)<<8|b[5]) offset 400 *0.1 ; hum=b[6]
//   PM2.5 = ((b[7]&0x3f)<<8|b[8])*0.1 ; PM10 = ((b[9]&0x3f)<<8|b[10])*0.1
//   CO2   = (b[11]<<8)|b[12]
// ============================================================================
#include "sensor_types.h"
#include "sensor_util.h"

static bool wh45_match(const uint8_t* b, size_t len) {
  return len >= 15 && b[0] == 0x45;
}

static bool wh45_parse(const uint8_t* b, size_t len, float rssi, SensorReading& o) {
  if (len < 15) return false;
  if (crc8_0x31(b, 13) != b[13]) return false;
  if (sum8(b, 14) != b[14]) return false;

  int rawT = ((b[4] & 0x07) << 8) | b[5];
  float tempC = (rawT - 400) * 0.1f;
  int hum = b[6];
  if (hum > 100 || tempC < -45.0f || tempC > 70.0f) return false;

  o.model = "WH45";
  o.id = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
  o.tempC = tempC;
  o.humidity = hum;
  o.pm25 = (((b[7] & 0x3f) << 8) | b[8]) * 0.1f;
  o.pm10 = (((b[9] & 0x3f) << 8) | b[10]) * 0.1f;
  o.co2 = (uint16_t)((b[11] << 8) | b[12]);
  // battery bars: 6 = external USB power
  int bars = ((b[7] & 0x40) >> 4) | ((b[9] & 0xC0) >> 6);
  o.battLow = (bars <= 1);
  o.rssi = rssi;
  o.caps = CAP_TEMP | CAP_HUM | CAP_PM | CAP_CO2;
  return true;
}

extern const SensorDriver DRIVER_WH45 = {"WH45", CAP_TEMP | CAP_HUM | CAP_PM | CAP_CO2,
                                         wh45_match, wh45_parse};
