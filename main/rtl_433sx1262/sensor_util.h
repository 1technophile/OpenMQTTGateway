// Checksum helpers shared by the RTL_433SX1262 sensor parsers.
// Ported from WeatherStation2Meshtastic's src/sensors/sensor_util.h.
#ifndef rtl_433sx1262_sensor_util_h
#define rtl_433sx1262_sensor_util_h

#include <Arduino.h>

// CRC-8, polynomial 0x31, init 0x00, MSB-first (Fine Offset / Ecowitt standard)
static inline uint8_t crc8_0x31(const uint8_t* data, size_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
  }
  return crc;
}

// 8-bit sum checksum
static inline uint8_t sum8(const uint8_t* data, size_t len) {
  uint8_t s = 0;
  for (size_t i = 0; i < len; i++) s += data[i];
  return s;
}

#endif
