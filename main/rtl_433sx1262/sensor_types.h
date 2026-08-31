// Shared sensor-parsing types for the RTL_433SX1262 gateway.
// Ported from WeatherStation2Meshtastic's src/sensors/sensor_types.h.
//
// Each sensor driver exposes a match()/parse() pair (function pointers,
// no vtable). match() does a cheap check that a frame belongs to this
// sensor; parse() decodes it (verifying CRC/checksum) and fills a
// SensorReading. The gateway then converts SensorReading into an
// rtl_433-style JsonObject.
#ifndef rtl_433sx1262_sensor_types_h
#define rtl_433sx1262_sensor_types_h

#include <Arduino.h>

enum SensorCap : uint16_t {
  CAP_TEMP = 1u << 0,
  CAP_HUM = 1u << 1,
  CAP_PRESSURE = 1u << 2,
  CAP_RAIN = 1u << 3,
  CAP_WIND = 1u << 4,
  CAP_UV = 1u << 5,
  CAP_LIGHTNING = 1u << 6,
  CAP_PM = 1u << 7,
  CAP_CO2 = 1u << 8,
  CAP_SOIL = 1u << 9,
  CAP_LEAK = 1u << 10,
};

struct SensorReading {
  uint16_t caps = 0; // OR of the SensorCap bits that were set
  uint32_t id = 0; // per-unit sensor id
  const char* model = ""; // static model string, e.g. "WH32"

  float tempC = 0; // CAP_TEMP
  float humidity = 0; // CAP_HUM (%)
  float pressureHpa = 0; // CAP_PRESSURE

  float rainMm = 0; // CAP_RAIN (cumulative counter, sensor steps)

  float windAvgMs = 0; // CAP_WIND (m/s)
  float windGustMs = 0; // CAP_WIND (m/s)
  float windDirDeg = 0; // CAP_WIND (degrees, 0 = N)

  float uvIndex = 0; // CAP_UV
  float lightLux = 0; // CAP_UV (lux, optional)

  uint8_t lightningDistKm = 63; // CAP_LIGHTNING (63 = none)
  uint32_t lightningCount = 0; // CAP_LIGHTNING (raw packet counter)
  bool lightningCountValid = false;

  float pm25 = 0; // CAP_PM (ug/m3)
  float pm10 = 0; // CAP_PM (ug/m3)
  uint16_t co2 = 0; // CAP_CO2 (ppm)

  uint8_t soilMoisture = 0; // CAP_SOIL (%)
  bool leak = false; // CAP_LEAK

  bool battLow = false;
  float battV = 0;
  float rssi = 0;
};

typedef bool (*SensorMatchFn)(const uint8_t* buf, size_t len);
typedef bool (*SensorParseFn)(const uint8_t* buf, size_t len, float rssi,
                              SensorReading& out);

struct SensorDriver {
  const char* model;
  uint16_t caps; // capabilities declared by the driver (documentation only)
  SensorMatchFn match;
  SensorParseFn parse;
};

#endif
