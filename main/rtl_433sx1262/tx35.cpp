// ============================================================================
// tx35.cpp — LaCrosse TX35DTH-IT / TX29-IT / TFA 30.3155 / 30.3159 (temp/hum.)
//   (ref. rtl_433 lacrosse_tx35.c)
//
// After the 0x2DD4 sync word, the first nibble is the model code (9), so:
//   b[0] = 9<<4 | id_hi ; 5 bytes total, b[4] = CRC-8/0x31 of b[0..3]
//   temp_c = 10*(b[1]&0x0f) + ((b[2]>>4)&0x0f) + 0.1*(b[2]&0x0f) - 40
//   humidity = b[3] & 0x7f (0x6a = no humidity sensor)
//
// NB: shares the high nibble 0x9 with WS90 (family 0x90). Disambiguation
//     is handled by the CRC check in parse() (WS90 is tried first).
// ============================================================================
#include "sensor_types.h"
#include "sensor_util.h"

#define TX35_NO_HUMIDITY 0x6a
#define TX35_PROBE_FLAG  0x7d

static bool tx35_match(const uint8_t* b, size_t len) {
  if (len < 5) return false;
  if ((b[0] & 0xF0) != 0x90) return false;
  return crc8_0x31(b, 4) == b[4]; // validate right away to avoid colliding with WS90
}

static bool tx35_parse(const uint8_t* b, size_t len, float rssi, SensorReading& o) {
  if (len < 5) return false;
  if (crc8_0x31(b, 4) != b[4]) return false;

  float tempC = 10.0f * (b[1] & 0x0f) + 1.0f * ((b[2] >> 4) & 0x0f) + 0.1f * (b[2] & 0x0f) - 40.0f;
  if (tempC < -45.0f || tempC > 70.0f) return false;

  o.model = "TX35";
  o.id = ((b[0] & 0x0f) << 2) | (b[1] >> 6);
  o.tempC = tempC;
  o.battLow = (b[3] >> 7) != 0;
  o.rssi = rssi;
  o.caps = CAP_TEMP;

  uint8_t hum = b[3] & 0x7f;
  if (hum != TX35_NO_HUMIDITY && hum != TX35_PROBE_FLAG && hum <= 100) {
    o.humidity = hum;
    o.caps |= CAP_HUM;
  }
  return true;
}

extern const SensorDriver DRIVER_TX35 = {"TX35", CAP_TEMP | CAP_HUM, tx35_match, tx35_parse};
