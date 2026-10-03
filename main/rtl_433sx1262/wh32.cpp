// ============================================================================
// wh32.cpp — Fine Offset WH32 / WH25 / WH32B / WH65B (temperature/humidity/pressure)
// 8-byte frame: MI IT TT HH PP PP CC XX (ref. rtl_433 fineoffset.c)
//   high nibble of b[0] = message type (0xD0/0xE0), checksum = sum8(b,6)==b[6]
//   WH25/WH32B include pressure (PP field); base WH32 does not -> CAP_PRESSURE
//   is only set when the pressure value is plausible.
// ============================================================================
#include "sensor_types.h"
#include "sensor_util.h"

static bool wh32_match(const uint8_t* b, size_t len) {
  if (len < 7) return false;
  uint8_t t = b[0] & 0xF0;
  return (t == 0xD0 || t == 0xE0);
}

static bool wh32_parse(const uint8_t* b, size_t len, float rssi, SensorReading& o) {
  if (len < 7) return false;
  if (sum8(b, 6) != b[6]) return false;

  bool invalid = (b[1] & 0x04) != 0;
  uint16_t rawT = (uint16_t)(((b[1] & 0x03) << 8) | b[2]);
  float tempC = (rawT - 400) * 0.1f;
  uint8_t hum = b[3];
  if (invalid || rawT == 0x7FF) return false;
  if (hum > 100 || tempC < -45.0f || tempC > 70.0f) return false;

  o.model = "WH32";
  o.id = (uint8_t)(((b[0] & 0x0F) << 4) | (b[1] >> 4));
  o.tempC = tempC;
  o.humidity = hum;
  o.battLow = (b[1] & 0x08) != 0;
  o.rssi = rssi;
  o.caps = CAP_TEMP | CAP_HUM;

  // Pressure (WH25/WH32B): 16-bit field in hPa*10. 0 or out of range = absent.
  uint16_t rawP = (uint16_t)((b[4] << 8) | b[5]);
  float hpa = rawP * 0.1f;
  if (rawP != 0 && hpa >= 800.0f && hpa <= 1100.0f) {
    o.pressureHpa = hpa;
    o.caps |= CAP_PRESSURE;
  }
  return true;
}

extern const SensorDriver DRIVER_WH32 = {"WH32", CAP_TEMP | CAP_HUM | CAP_PRESSURE,
                                         wh32_match, wh32_parse};
