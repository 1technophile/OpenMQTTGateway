// ============================================================================
// wh51.cpp — Ecowitt WH51 / WN31 / SwitchDoc SM23 soil moisture
// 14-byte frame: FF IIIIII TB YY MM ZA AA XX XX XX CC SS
//   (ref. rtl_433 fineoffset.c, fineoffset_WH51_callback)
//   FF=0x51; CRC-8/0x31 of b[0..11] == b[12]; SUM8(b[0..12]) == b[13]
//   moisture = b[6] (%) ; battery_mV = (b[4]&0x1f)*100
// ============================================================================
#include "sensor_types.h"
#include "sensor_util.h"

static bool wh51_match(const uint8_t* b, size_t len) {
  return len >= 14 && b[0] == 0x51;
}

static bool wh51_parse(const uint8_t* b, size_t len, float rssi, SensorReading& o) {
  if (len < 14) return false;
  if (crc8_0x31(b, 12) != b[12]) return false;
  if (sum8(b, 13) != b[13]) return false;

  int moisture = b[6];
  if (moisture > 100) return false;
  int batt_mv = (b[4] & 0x1f) * 100;

  o.model = "WH51";
  o.id = ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
  o.soilMoisture = (uint8_t)moisture;
  o.battV = batt_mv / 1000.0f;
  o.battLow = (batt_mv > 0 && batt_mv < 900); // < 0.9 V
  o.rssi = rssi;
  o.caps = CAP_SOIL;
  return true;
}

extern const SensorDriver DRIVER_WH51 = {"WH51", CAP_SOIL, wh51_match, wh51_parse};
