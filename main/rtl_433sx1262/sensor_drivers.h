// Declares the SensorDriver table for the RTL_433SX1262 "Group A" sensors
// (Fine Offset / Ecowitt / LaCrosse, 868.35 MHz GFSK). Each driver is
// defined in its own .cpp file, ported from WeatherStation2Meshtastic's
// src/sensors/{fineoffset,lacrosse}/*.cpp.
#ifndef rtl_433sx1262_sensor_drivers_h
#define rtl_433sx1262_sensor_drivers_h

#include "sensor_types.h"

extern const SensorDriver DRIVER_WH32;
extern const SensorDriver DRIVER_WH31E;
extern const SensorDriver DRIVER_WH40;
extern const SensorDriver DRIVER_WH57;
extern const SensorDriver DRIVER_WH51;
extern const SensorDriver DRIVER_WH45;
extern const SensorDriver DRIVER_WS80;
extern const SensorDriver DRIVER_WS90;
extern const SensorDriver DRIVER_TX35;

// WS90 is tried before TX35: both can start with a 0x9 high nibble, and
// TX35's match() validates its own CRC to avoid colliding with WS90 frames.
static const SensorDriver* const RTL433SX1262_DRIVERS[] = {
    &DRIVER_WH32,
    &DRIVER_WH31E,
    &DRIVER_WH40,
    &DRIVER_WH57,
    &DRIVER_WH51,
    &DRIVER_WH45,
    &DRIVER_WS80,
    &DRIVER_WS90,
    &DRIVER_TX35,
};
static const size_t RTL433SX1262_DRIVER_COUNT = sizeof(RTL433SX1262_DRIVERS) / sizeof(RTL433SX1262_DRIVERS[0]);

#endif
