#include <HMD/core/HassValidators.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace omg::hass;

// Test fixture
class HassValidatorsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup test environment if needed
  }

  void TearDown() override {
    // Cleanup if needed
  }
};

// ============================================================================
// Device Class Validation Tests
// ============================================================================

TEST_F(HassValidatorsTest, ValidatesCorrectDeviceClasses) {
  // Test common device classes
  EXPECT_TRUE(HassValidators::isValidDeviceClass("temperature"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("humidity"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("battery"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("pressure"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("illuminance"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("signal_strength"));
}

TEST_F(HassValidatorsTest, RejectsInvalidDeviceClasses) {
  // Test invalid device classes
  EXPECT_FALSE(HassValidators::isValidDeviceClass("invalid_class"));
  EXPECT_FALSE(HassValidators::isValidDeviceClass("Temperature")); // Case sensitive
  EXPECT_FALSE(HassValidators::isValidDeviceClass("temp"));
  EXPECT_FALSE(HassValidators::isValidDeviceClass("HUMIDITY"));
}

TEST_F(HassValidatorsTest, HandlesNullDeviceClass) {
  // Test null pointer
  EXPECT_FALSE(HassValidators::isValidDeviceClass(nullptr));
}

TEST_F(HassValidatorsTest, HandlesEmptyDeviceClass) {
  // Test empty string
  EXPECT_FALSE(HassValidators::isValidDeviceClass(""));
}

TEST_F(HassValidatorsTest, ValidatesAllSensorDeviceClasses) {
  // Test sensor-specific device classes
  EXPECT_TRUE(HassValidators::isValidDeviceClass("carbon_dioxide"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("carbon_monoxide"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("pm25"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("pm10"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("pm1"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("voltage"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("current"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("power"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("energy"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("frequency"));
}

TEST_F(HassValidatorsTest, ValidatesBinarySensorDeviceClasses) {
  // Test binary sensor device classes
  EXPECT_TRUE(HassValidators::isValidDeviceClass("motion"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("occupancy"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("door"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("window"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("connectivity"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("lock"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("moving"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("problem"));
}

TEST_F(HassValidatorsTest, ValidatesSpecializedDeviceClasses) {
  // Test specialized device classes
  EXPECT_TRUE(HassValidators::isValidDeviceClass("battery_charging"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("power_factor"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("precipitation_intensity"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("precipitation"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("sound_pressure"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("timestamp"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("irradiance"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("data_size"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("distance"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("duration"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("wind_speed"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("weight"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("water"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("gas"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("enum"));
  EXPECT_TRUE(HassValidators::isValidDeviceClass("restart"));
}

// ============================================================================
// Measurement Unit Validation Tests
// ============================================================================

TEST_F(HassValidatorsTest, ValidatesCorrectUnits) {
  // Test common measurement units
  EXPECT_TRUE(HassValidators::isValidUnit("°C"));
  EXPECT_TRUE(HassValidators::isValidUnit("°F"));
  EXPECT_TRUE(HassValidators::isValidUnit("%"));
  EXPECT_TRUE(HassValidators::isValidUnit("hPa"));
  EXPECT_TRUE(HassValidators::isValidUnit("lx"));
  EXPECT_TRUE(HassValidators::isValidUnit("dBm"));
}

TEST_F(HassValidatorsTest, RejectsInvalidUnits) {
  // Test invalid units
  EXPECT_FALSE(HassValidators::isValidUnit("celsius"));
  EXPECT_FALSE(HassValidators::isValidUnit("degrees"));
  EXPECT_FALSE(HassValidators::isValidUnit("HPA")); // Case sensitive
  EXPECT_FALSE(HassValidators::isValidUnit("invalid_unit"));
}

TEST_F(HassValidatorsTest, HandlesNullUnit) {
  // Test null pointer
  EXPECT_FALSE(HassValidators::isValidUnit(nullptr));
}

TEST_F(HassValidatorsTest, HandlesEmptyUnit) {
  // Test empty string
  EXPECT_FALSE(HassValidators::isValidUnit(""));
}

TEST_F(HassValidatorsTest, ValidatesElectricalUnits) {
  // Test electrical measurement units
  EXPECT_TRUE(HassValidators::isValidUnit("V")); // Volt
  EXPECT_TRUE(HassValidators::isValidUnit("mV")); // Millivolt
  EXPECT_TRUE(HassValidators::isValidUnit("A")); // Ampere
  EXPECT_TRUE(HassValidators::isValidUnit("W")); // Watt
  EXPECT_TRUE(HassValidators::isValidUnit("kW")); // Kilowatt
  EXPECT_TRUE(HassValidators::isValidUnit("kWh")); // Kilowatt-hour
  EXPECT_TRUE(HassValidators::isValidUnit("Ω")); // Ohm
}

TEST_F(HassValidatorsTest, ValidatesTimeUnits) {
  // Test time units
  EXPECT_TRUE(HassValidators::isValidUnit("s")); // Second
  EXPECT_TRUE(HassValidators::isValidUnit("ms")); // Millisecond
  EXPECT_TRUE(HassValidators::isValidUnit("min")); // Minute
  EXPECT_TRUE(HassValidators::isValidUnit("h")); // Hour
}

TEST_F(HassValidatorsTest, ValidatesDistanceUnits) {
  // Test distance units
  EXPECT_TRUE(HassValidators::isValidUnit("mm")); // Millimeter
  EXPECT_TRUE(HassValidators::isValidUnit("cm")); // Centimeter
  EXPECT_TRUE(HassValidators::isValidUnit("ft")); // Feet
}

TEST_F(HassValidatorsTest, ValidatesPressureUnits) {
  // Test pressure units
  EXPECT_TRUE(HassValidators::isValidUnit("hPa")); // Hectopascal
  EXPECT_TRUE(HassValidators::isValidUnit("bar")); // Bar
}

TEST_F(HassValidatorsTest, ValidatesSpeedUnits) {
  // Test speed units
  EXPECT_TRUE(HassValidators::isValidUnit("m/s")); // Meters per second
  EXPECT_TRUE(HassValidators::isValidUnit("km/h")); // Kilometers per hour
  EXPECT_TRUE(HassValidators::isValidUnit("m/s²")); // Acceleration
}

TEST_F(HassValidatorsTest, ValidatesWeightUnits) {
  // Test weight units
  EXPECT_TRUE(HassValidators::isValidUnit("kg")); // Kilogram
  EXPECT_TRUE(HassValidators::isValidUnit("lb")); // Pound
}

TEST_F(HassValidatorsTest, ValidatesSpecializedUnits) {
  // Test specialized units
  EXPECT_TRUE(HassValidators::isValidUnit("B")); // Byte
  EXPECT_TRUE(HassValidators::isValidUnit("UV index")); // UV index
  EXPECT_TRUE(HassValidators::isValidUnit("dB")); // Decibel
  EXPECT_TRUE(HassValidators::isValidUnit("Hz")); // Hertz
  EXPECT_TRUE(HassValidators::isValidUnit("bpm")); // Beats per minute
  EXPECT_TRUE(HassValidators::isValidUnit("mm/h")); // Millimeter per hour
  EXPECT_TRUE(HassValidators::isValidUnit("m³")); // Cubic meter
  EXPECT_TRUE(HassValidators::isValidUnit("mg/m³")); // Milligram per cubic meter
  EXPECT_TRUE(HassValidators::isValidUnit("μg/m³")); // Microgram per cubic meter
  EXPECT_TRUE(HassValidators::isValidUnit("µS/cm")); // Microsiemens per centimeter
  EXPECT_TRUE(HassValidators::isValidUnit("°")); // Degree
  EXPECT_TRUE(HassValidators::isValidUnit("wb²")); // Weber squared
}

// ============================================================================
// Count and Statistics Tests
// ============================================================================

TEST_F(HassValidatorsTest, ReturnsCorrectDeviceClassCount) {
  // Verify the count matches expected number of device classes
  size_t count = HassValidators::getValidClassesCount();
  EXPECT_GT(count, 0);
  EXPECT_EQ(count, 40); // Based on the implementation
}

TEST_F(HassValidatorsTest, ReturnsCorrectUnitCount) {
  // Verify the count matches expected number of units
  size_t count = HassValidators::getValidUnitsCount();
  EXPECT_GT(count, 0);
  EXPECT_EQ(count, 38); // Based on the implementation
}

TEST_F(HassValidatorsTest, CountsAreConsistent) {
  // Verify counts are always the same (static initialization)
  size_t classCount1 = HassValidators::getValidClassesCount();
  size_t classCount2 = HassValidators::getValidClassesCount();
  EXPECT_EQ(classCount1, classCount2);

  size_t unitCount1 = HassValidators::getValidUnitsCount();
  size_t unitCount2 = HassValidators::getValidUnitsCount();
  EXPECT_EQ(unitCount1, unitCount2);
}

TEST_F(HassValidatorsTest, ValidatesQuickly) {
  // Test validation performance (should be O(1) with unordered_set)
  // This is a simple smoke test - actual performance testing requires benchmarking
  const int iterations = 10000;

  for (int i = 0; i < iterations; i++) {
    HassValidators::isValidDeviceClass("temperature");
    HassValidators::isValidDeviceClass("invalid_class");
    HassValidators::isValidUnit("°C");
    HassValidators::isValidUnit("invalid_unit");
  }

  // If we got here without timeout, performance is acceptable
  SUCCEED();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(HassValidatorsTest, HandlesUnicodeCharacters) {
  // Test units with special Unicode characters
  EXPECT_TRUE(HassValidators::isValidUnit("°C")); // Degree sign
  EXPECT_TRUE(HassValidators::isValidUnit("Ω")); // Omega
  EXPECT_TRUE(HassValidators::isValidUnit("μg/m³")); // Mu character
  EXPECT_TRUE(HassValidators::isValidUnit("µS/cm")); // Micro sign
}

TEST_F(HassValidatorsTest, HandlesCaseSensitivity) {
  // Validation should be case-sensitive
  EXPECT_TRUE(HassValidators::isValidDeviceClass("temperature"));
  EXPECT_FALSE(HassValidators::isValidDeviceClass("Temperature"));
  EXPECT_FALSE(HassValidators::isValidDeviceClass("TEMPERATURE"));

  EXPECT_TRUE(HassValidators::isValidUnit("V"));
  EXPECT_FALSE(HassValidators::isValidUnit("v"));
}

TEST_F(HassValidatorsTest, HandlesWhitespace) {
  // Test that whitespace matters
  EXPECT_TRUE(HassValidators::isValidUnit("UV index")); // Space is part of unit
  EXPECT_FALSE(HassValidators::isValidDeviceClass(" temperature")); // Leading space
  EXPECT_FALSE(HassValidators::isValidDeviceClass("temperature ")); // Trailing space
}

TEST_F(HassValidatorsTest, RejectsPartialMatches) {
  // Validation should require exact matches
  EXPECT_FALSE(HassValidators::isValidDeviceClass("temp")); // Partial
  EXPECT_FALSE(HassValidators::isValidDeviceClass("temperature_sensor")); // Extra text
  EXPECT_FALSE(HassValidators::isValidUnit("C")); // Must be °C
}
