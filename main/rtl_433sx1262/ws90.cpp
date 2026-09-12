// ============================================================================
// ws90.cpp — Fine Offset WS90 "Wittboy" 7-in-1 station (WS80 + piezo rain)
// 32-byte frame (ref. rtl_433 fineoffset_ws90.c). Bytes 1-13 identical to WS80.
//   YY=0x90; CRC-8/0x31 of b[0..29]==b[30] (crc8(b,31)==0); SUM8(b[0..30])==b[31]
//   rain_total = ((b[19]<<8)|b[20]) * 0.1 mm
// ============================================================================
#include "sensor_types.h"
#include "sensor_util.h"

static bool ws90_match(const uint8_t* b, size_t len) {
  return len >= 32 && b[0] == 0x90;
}

static bool ws90_parse(const uint8_t* b, size_t len, float rssi, SensorReading& o) {
  if (len < 32) return false;
  if (crc8_0x31(b, 31) != 0) return false; // b[30] = CRC of b[0..29]
  if (sum8(b, 31) != b[31]) return false; // b[31] = SUM of b[0..30]

  o.model = "WS90";
  o.id = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
  o.rssi = rssi;
  o.caps = 0;

  int temp_raw = ((b[7] & 0x03) << 8) | b[8];
  if (temp_raw != 0x3ff) {
    o.tempC = (temp_raw - 400) * 0.1f;
    o.caps |= CAP_TEMP;
  }
  if (b[9] != 0xff) {
    o.humidity = b[9];
    o.caps |= CAP_HUM;
  }

  int wind_avg = ((b[7] & 0x10) << 4) | b[10];
  int wind_dir = ((b[7] & 0x20) << 3) | b[11];
  int wind_max = ((b[7] & 0x40) << 2) | b[12];
  if (wind_avg != 0x1ff || wind_max != 0x1ff || wind_dir != 0x1ff) {
    if (wind_avg != 0x1ff) o.windAvgMs = wind_avg * 0.1f;
    if (wind_max != 0x1ff) o.windGustMs = wind_max * 0.1f;
    if (wind_dir != 0x1ff) o.windDirDeg = wind_dir;
    o.caps |= CAP_WIND;
  }

  if (b[13] != 0xff) {
    o.uvIndex = b[13] * 0.1f;
    o.caps |= CAP_UV;
  }
  int light_raw = (b[4] << 8) | b[5];
  if (light_raw != 0xffff) {
    o.lightLux = light_raw * 10.0f;
    o.caps |= CAP_UV;
  }

  // Total rain (cumulative counter, 0.1 mm steps)
  int rain_raw = (b[19] << 8) | b[20];
  o.rainMm = rain_raw * 0.1f;
  o.caps |= CAP_RAIN;

  int batt_mv = b[6] * 20;
  o.battV = batt_mv / 1000.0f;
  o.battLow = (batt_mv > 0 && batt_mv < 1400);
  return o.caps != 0;
}

extern const SensorDriver DRIVER_WS90 = {"WS90",
                                         CAP_TEMP | CAP_HUM | CAP_WIND | CAP_UV | CAP_RAIN,
                                         ws90_match, ws90_parse};
