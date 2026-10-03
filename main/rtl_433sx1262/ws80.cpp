// ============================================================================
// ws80.cpp — Fine Offset WS80 station (ultrasonic wind + T/H + UV + light)
// 18-byte frame: YY IIII LL LL BB FF TT HH WW DD GG VV UU UU AA XX
//   (ref. rtl_433 fineoffset_ws80.c)
//   YY=0x80; CRC-8/0x31 of b[0..15]==b[16] (crc8(b,17)==0); SUM8(b[0..16])==b[17]
//   b[7] (FF) carries the MSBs: 0x03 temp, 0x10 wind, 0x20 dir, 0x40 gust
// ============================================================================
#include "sensor_types.h"
#include "sensor_util.h"

static bool ws80_match(const uint8_t* b, size_t len) {
  return len >= 18 && b[0] == 0x80;
}

static bool ws80_parse(const uint8_t* b, size_t len, float rssi, SensorReading& o) {
  if (len < 18) return false;
  if (crc8_0x31(b, 17) != 0) return false; // b[16] = CRC of b[0..15]
  if (sum8(b, 17) != b[17]) return false; // b[17] = SUM of b[0..16]

  o.model = "WS80";
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

  int batt_mv = b[6] * 20;
  o.battV = batt_mv / 1000.0f;
  o.battLow = (batt_mv > 0 && batt_mv < 1400);
  return o.caps != 0;
}

extern const SensorDriver DRIVER_WS80 = {"WS80", CAP_TEMP | CAP_HUM | CAP_WIND | CAP_UV,
                                         ws80_match, ws80_parse};
