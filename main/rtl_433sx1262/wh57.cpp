// ============================================================================
// wh57.cpp — Ecowitt WH57 / WH31L lightning detector
// 9-byte frame: 57 SI II II FF KK CC XX AA (ref. rtl_433 fineoffset_wh31l.c)
//   state = b[1]>>4 (8 = strike), distance = b[5]&0x3F (63 = none),
//   raw count = b[6]. CRC-8/0x31 over 8 bytes = 0, then SUM8(b,8)==b[8].
// The parser is stateless: it reports the raw count and distance; handling
// the 8-bit wraparound and cumulative total is the consumer's job.
// ============================================================================
#include "sensor_types.h"
#include "sensor_util.h"

static bool wh57_match(const uint8_t* b, size_t len) {
  return len >= 9 && b[0] == 0x57;
}

static bool wh57_parse(const uint8_t* b, size_t len, float rssi, SensorReading& o) {
  if (len < 9) return false;
  if (crc8_0x31(b, 8) != 0) return false;
  if (sum8(b, 8) != b[8]) return false;

  uint8_t state = b[1] >> 4;
  uint8_t dist = b[5] & 0x3F;

  o.model = "WH57";
  o.id = ((uint32_t)(b[1] & 0x0F) << 16) | ((uint32_t)b[2] << 8) | b[3];
  o.battLow = (((b[4] >> 1) & 0x03) == 0);
  o.rssi = rssi;
  o.caps = CAP_LIGHTNING;
  o.lightningCount = b[6];
  o.lightningCountValid = (state != 0); // state 0 = startup: count not reliable
  o.lightningDistKm = (state == 8 && dist != 63) ? dist : 63;
  return true;
}

extern const SensorDriver DRIVER_WH57 = {"WH57", CAP_LIGHTNING, wh57_match, wh57_parse};
