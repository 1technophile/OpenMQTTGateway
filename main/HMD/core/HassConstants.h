/*
  OpenMQTTGateway - Home Assistant Constants
  
  Contains all Home Assistant specific constants (device classes, units, types).
  These constants are used by the modular discovery system.
  
  Copyright: (c) OpenMQTTGateway Contributors
*/

#pragma once

//=============================================================================
// HOME ASSISTANT DEVICE CLASSES
// From: https://github.com/home-assistant/core/blob/dev/homeassistant/const.py
//=============================================================================

#define HASS_CLASS_BATTERY_CHARGING        "battery_charging"
#define HASS_CLASS_BATTERY                 "battery"
#define HASS_CLASS_CARBON_DIOXIDE          "carbon_dioxide"
#define HASS_CLASS_CARBON_MONOXIDE         "carbon_monoxide"
#define HASS_CLASS_CONNECTIVITY            "connectivity"
#define HASS_CLASS_CURRENT                 "current"
#define HASS_CLASS_DATA_SIZE               "data_size"
#define HASS_CLASS_DISTANCE                "distance"
#define HASS_CLASS_DOOR                    "door"
#define HASS_CLASS_DURATION                "duration"
#define HASS_CLASS_ENERGY                  "energy"
#define HASS_CLASS_ENUM                    "enum"
#define HASS_CLASS_FREQUENCY               "frequency"
#define HASS_CLASS_GAS                     "gas"
#define HASS_CLASS_HUMIDITY                "humidity"
#define HASS_CLASS_ILLUMINANCE             "illuminance"
#define HASS_CLASS_IRRADIANCE              "irradiance"
#define HASS_CLASS_LOCK                    "lock"
#define HASS_CLASS_MOTION                  "motion"
#define HASS_CLASS_MOVING                  "moving"
#define HASS_CLASS_OCCUPANCY               "occupancy"
#define HASS_CLASS_PM1                     "pm1"
#define HASS_CLASS_PM10                    "pm10"
#define HASS_CLASS_PM25                    "pm25"
#define HASS_CLASS_POWER_FACTOR            "power_factor"
#define HASS_CLASS_POWER                   "power"
#define HASS_CLASS_PRECIPITATION_INTENSITY "precipitation_intensity"
#define HASS_CLASS_PRECIPITATION           "precipitation"
#define HASS_CLASS_PRESSURE                "pressure"
#define HASS_CLASS_PROBLEM                 "problem"
#define HASS_CLASS_RESTART                 "restart"
#define HASS_CLASS_SIGNAL_STRENGTH         "signal_strength"
#define HASS_CLASS_SOUND_PRESSURE          "sound_pressure"
#define HASS_CLASS_TEMPERATURE             "temperature"
#define HASS_CLASS_TIMESTAMP               "timestamp"
#define HASS_CLASS_VOLTAGE                 "voltage"
#define HASS_CLASS_WATER                   "water"
#define HASS_CLASS_WEIGHT                  "weight"
#define HASS_CLASS_WIND_SPEED              "wind_speed"
#define HASS_CLASS_WINDOW                  "window"

//=============================================================================
// HOME ASSISTANT MEASUREMENT UNITS
// From: https://github.com/home-assistant/core/blob/dev/homeassistant/const.py
//=============================================================================

#define HASS_UNIT_AMP         "A"
#define HASS_UNIT_BYTE        "B"
#define HASS_UNIT_UV_INDEX    "UV index"
#define HASS_UNIT_VOLT        "V"
#define HASS_UNIT_WATT        "W"
#define HASS_UNIT_BPM         "bpm"
#define HASS_UNIT_BAR         "bar"
#define HASS_UNIT_CM          "cm"
#define HASS_UNIT_DB          "dB"
#define HASS_UNIT_DBM         "dBm"
#define HASS_UNIT_FT          "ft"
#define HASS_UNIT_HOUR        "h"
#define HASS_UNIT_HPA         "hPa"
#define HASS_UNIT_HZ          "Hz"
#define HASS_UNIT_KG          "kg"
#define HASS_UNIT_KW          "kW"
#define HASS_UNIT_KWH         "kWh"
#define HASS_UNIT_KMH         "km/h"
#define HASS_UNIT_LB          "lb"
#define HASS_UNIT_LITER       "L"
#define HASS_UNIT_LX          "lx"
#define HASS_UNIT_MS          "m/s"
#define HASS_UNIT_MS2         "m/s²"
#define HASS_UNIT_M3          "m³"
#define HASS_UNIT_MGM3        "mg/m³"
#define HASS_UNIT_MIN         "min"
#define HASS_UNIT_MM          "mm"
#define HASS_UNIT_MMH         "mm/h"
#define HASS_UNIT_MILLISECOND "ms"
#define HASS_UNIT_MV          "mV"
#define HASS_UNIT_USCM        "µS/cm"
#define HASS_UNIT_UGM3        "μg/m³"
#define HASS_UNIT_OHM         "Ω"
#define HASS_UNIT_PERCENT     "%"
#define HASS_UNIT_DEGREE      "°"
#define HASS_UNIT_CELSIUS     "°C"
#define HASS_UNIT_FAHRENHEIT  "°F"
#define HASS_UNIT_SECOND      "s"
#define HASS_UNIT_WB2         "wb²"

// Additional commonly used units not in the standard HA list
#define HASS_UNIT_METER "m"
#define HASS_UNIT_PPM   "ppm"
#define HASS_UNIT_WM2   "wm²"

//=============================================================================
// HOME ASSISTANT COMPONENT TYPES
//=============================================================================

#define HASS_TYPE_SENSOR         "sensor"
#define HASS_TYPE_BINARY_SENSOR  "binary_sensor"
#define HASS_TYPE_SWITCH         "switch"
#define HASS_TYPE_BUTTON         "button"
#define HASS_TYPE_NUMBER         "number"
#define HASS_TYPE_UPDATE         "update"
#define HASS_TYPE_COVER          "cover"
#define HASS_TYPE_DEVICE_TRACKER "device_tracker"

//=============================================================================
// HOME ASSISTANT STATE CLASSES
//=============================================================================

#define stateClassNone            ""
#define stateClassMeasurement     "measurement"
#define stateClassTotal           "total"
#define stateClassTotalIncreasing "total_increasing"
