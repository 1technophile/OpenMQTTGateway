// ============================================================================
// wh31e.cpp — Ambient Weather WH31E / WH31B thermo-hygrometer
// 7-byte frame: YY II CT TT HH XX AA (ref. rtl_433 ambientweather_wh31e.c)
//   YY = 0x30 (WH31E) / 0x37 (WH31B); CRC-8/0x31 over 6 bytes = 0; SUM8(b,6)==b[6]
//   batt = (b[2]>>2)&1 ; temp = ((b[2]&0x03)<<8 | b[3]) scaled 0.1, offset 400
// ============================================================================
#include "sensor_types.h"
#include "sensor_util.h"

static bool wh31e_match(const uint8_t* b, size_t len) {
  return len >= 7 && (b[0] == 0x30 || b[0] == 0x37);
}

static bool wh31e_parse(const uint8_t* b, size_t len, float rssi, SensorReading& o) {
  if (len < 7) return false;
  if (crc8_0x31(b, 6) != 0) return false; // b[5] = CRC of b[0..4]
  if (sum8(b, 6) != b[6]) return false; // b[6] = SUM of b[0..5]

  int rawT = ((b[2] & 0x03) << 8) | b[3];
  float tempC = (rawT - 400) * 0.1f;
  int hum = b[4];
  if (hum > 100 || tempC < -45.0f || tempC > 70.0f) return false;

  o.model = "WH31E";
  o.id = b[1];
  o.tempC = tempC;
  o.humidity = hum;
  o.battLow = ((b[2] & 0x04) != 0);
  o.rssi = rssi;
  o.caps = CAP_TEMP | CAP_HUM;
  return true;
}

extern const SensorDriver DRIVER_WH31E = {"WH31E", CAP_TEMP | CAP_HUM,
                                          wh31e_match, wh31e_parse};
