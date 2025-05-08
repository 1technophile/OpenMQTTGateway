/*
  OpenMQTTGateway - Home Assistant Validators Implementation
*/

#include "HassValidators.h"

#include "HassConstants.h"

namespace omg {
namespace hass {

// Initialize static members with Home Assistant supported values
const std::unordered_set<std::string_view> HassValidators::validClasses_ = {
    HASS_CLASS_BATTERY_CHARGING,
    HASS_CLASS_BATTERY,
    HASS_CLASS_CARBON_DIOXIDE,
    HASS_CLASS_CARBON_MONOXIDE,
    HASS_CLASS_CONNECTIVITY,
    HASS_CLASS_CURRENT,
    HASS_CLASS_DATA_SIZE,
    HASS_CLASS_DISTANCE,
    HASS_CLASS_DOOR,
    HASS_CLASS_DURATION,
    HASS_CLASS_ENERGY,
    HASS_CLASS_ENUM,
    HASS_CLASS_FREQUENCY,
    HASS_CLASS_GAS,
    HASS_CLASS_HUMIDITY,
    HASS_CLASS_ILLUMINANCE,
    HASS_CLASS_IRRADIANCE,
    HASS_CLASS_LOCK,
    HASS_CLASS_MOTION,
    HASS_CLASS_MOVING,
    HASS_CLASS_OCCUPANCY,
    HASS_CLASS_PM1,
    HASS_CLASS_PM10,
    HASS_CLASS_PM25,
    HASS_CLASS_POWER_FACTOR,
    HASS_CLASS_POWER,
    HASS_CLASS_PRECIPITATION_INTENSITY,
    HASS_CLASS_PRECIPITATION,
    HASS_CLASS_PRESSURE,
    HASS_CLASS_PROBLEM,
    HASS_CLASS_RESTART,
    HASS_CLASS_SIGNAL_STRENGTH,
    HASS_CLASS_SOUND_PRESSURE,
    HASS_CLASS_TEMPERATURE,
    HASS_CLASS_TIMESTAMP,
    HASS_CLASS_VOLTAGE,
    HASS_CLASS_WATER,
    HASS_CLASS_WEIGHT,
    HASS_CLASS_WIND_SPEED,
    HASS_CLASS_WINDOW};

const std::unordered_set<std::string_view> HassValidators::validUnits_ = {
    HASS_UNIT_AMP,
    HASS_UNIT_BYTE,
    HASS_UNIT_UV_INDEX,
    HASS_UNIT_VOLT,
    HASS_UNIT_WATT,
    HASS_UNIT_BPM,
    HASS_UNIT_BAR,
    HASS_UNIT_CM,
    HASS_UNIT_DB,
    HASS_UNIT_DBM,
    HASS_UNIT_FT,
    HASS_UNIT_HOUR,
    HASS_UNIT_HPA,
    HASS_UNIT_HZ,
    HASS_UNIT_KG,
    HASS_UNIT_KW,
    HASS_UNIT_KWH,
    HASS_UNIT_KMH,
    HASS_UNIT_LB,
    HASS_UNIT_LX,
    HASS_UNIT_MS,
    HASS_UNIT_MS2,
    HASS_UNIT_M3,
    HASS_UNIT_MGM3,
    HASS_UNIT_MIN,
    HASS_UNIT_MM,
    HASS_UNIT_MMH,
    HASS_UNIT_MILLISECOND,
    HASS_UNIT_MV,
    HASS_UNIT_USCM,
    HASS_UNIT_UGM3,
    HASS_UNIT_OHM,
    HASS_UNIT_PERCENT,
    HASS_UNIT_DEGREE,
    HASS_UNIT_CELSIUS,
    HASS_UNIT_FAHRENHEIT,
    HASS_UNIT_SECOND,
    HASS_UNIT_WB2};

bool HassValidators::isValidDeviceClass(const char* deviceClass) {
  if (!deviceClass || !deviceClass[0]) {
    return false;
  }

  return validClasses_.find(std::string_view(deviceClass)) != validClasses_.end();
}

bool HassValidators::isValidUnit(const char* unit) {
  if (!unit || !unit[0]) {
    return false;
  }

  return validUnits_.find(std::string_view(unit)) != validUnits_.end();
}

size_t HassValidators::getValidClassesCount() {
  return validClasses_.size();
}

size_t HassValidators::getValidUnitsCount() {
  return validUnits_.size();
}

void HassValidators::initialize() {
  // Static initialization is automatic in C++
  // This method is kept for potential future initialization needs
}

} // namespace hass
} // namespace omg
