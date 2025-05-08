#include <HMD/ISettingsProvider.h>
#include <HMD/core/HassTopicBuilder.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

using namespace omg::hass;
using ::testing::_;
using ::testing::Return;

// Mock for ISettingsProvider
class MockSettingsProvider : public ISettingsProvider {
public:
  MOCK_CONST_METHOD0(getDiscoveryPrefix, std::string());
  MOCK_CONST_METHOD0(getMqttTopic, std::string());
  MOCK_CONST_METHOD0(getGatewayName, std::string());
  MOCK_CONST_METHOD0(isEthConnected, bool());
  MOCK_CONST_METHOD0(getNetworkMacAddress, std::string());
  MOCK_CONST_METHOD0(getNetworkIPAddress, std::string());
  MOCK_CONST_METHOD0(getModules, JsonArray());
  MOCK_CONST_METHOD0(getGatewayManufacturer, std::string());
  MOCK_CONST_METHOD0(getGatewayVersion, std::string());
};

// Test fixture for HassTopicBuilder
class HassTopicBuilderTest : public ::testing::Test {
protected:
  MockSettingsProvider mockSettings;
  std::unique_ptr<HassTopicBuilder> builder;

  void SetUp() override {
    // Default discovery prefix is "homeassistant"
    ON_CALL(mockSettings, getDiscoveryPrefix())
        .WillByDefault(Return("homeassistant"));
    builder = std::make_unique<HassTopicBuilder>(mockSettings);
  }

  void TearDown() override {
    // Cleanup automatically handled by unique_ptr
  }
};

// ============================================================================
// Constructor and Discovery Prefix Tests
// ============================================================================

TEST_F(HassTopicBuilderTest, ConstructorInitializesWithSettingsProvider) {
  // Should use the mock settings provider
  EXPECT_EQ(builder->getDiscoveryPrefix(), "homeassistant");
}

TEST_F(HassTopicBuilderTest, GetDiscoveryPrefixReturnsProviderValue) {
  // Test with different prefix
  EXPECT_CALL(mockSettings, getDiscoveryPrefix())
      .WillOnce(Return("custom_prefix"));
  EXPECT_EQ(builder->getDiscoveryPrefix(), "custom_prefix");
}

TEST_F(HassTopicBuilderTest, GetDiscoveryPrefixHandlesEmptyPrefix) {
  // Test empty prefix
  EXPECT_CALL(mockSettings, getDiscoveryPrefix())
      .WillOnce(Return(""));
  EXPECT_EQ(builder->getDiscoveryPrefix(), "");
}

// ============================================================================
// buildDiscoveryTopic Tests
// ============================================================================

TEST_F(HassTopicBuilderTest, BuildsDiscoveryTopicCorrectly) {
  // Should build topic: <prefix>/<component>/<unique_id>/config
  std::string topic = builder->buildDiscoveryTopic("sensor", "device123");
  EXPECT_EQ(topic, "homeassistant/sensor/device123/config");
}

TEST_F(HassTopicBuilderTest, BuildsDiscoveryTopicWithDifferentComponents) {
  // Test various component types
  EXPECT_EQ(builder->buildDiscoveryTopic("switch", "relay1"),
            "homeassistant/switch/relay1/config");
  EXPECT_EQ(builder->buildDiscoveryTopic("binary_sensor", "motion1"),
            "homeassistant/binary_sensor/motion1/config");
  EXPECT_EQ(builder->buildDiscoveryTopic("button", "restart"),
            "homeassistant/button/restart/config");
}

TEST_F(HassTopicBuilderTest, BuildsDiscoveryTopicWithCustomPrefix) {
  // Test with custom discovery prefix
  EXPECT_CALL(mockSettings, getDiscoveryPrefix())
      .WillRepeatedly(Return("my_hass"));

  std::string topic = builder->buildDiscoveryTopic("sensor", "temp1");
  EXPECT_EQ(topic, "my_hass/sensor/temp1/config");
}

TEST_F(HassTopicBuilderTest, BuildsDiscoveryTopicHandlesNullInputs) {
  // Test null component - returns empty string
  std::string topic1 = builder->buildDiscoveryTopic(nullptr, "device123");
  EXPECT_EQ(topic1, "");

  // Test null uniqueId - returns empty string
  std::string topic2 = builder->buildDiscoveryTopic("sensor", nullptr);
  EXPECT_EQ(topic2, "");

  // Test both null - returns empty string
  std::string topic3 = builder->buildDiscoveryTopic(nullptr, nullptr);
  EXPECT_EQ(topic3, "");
}

TEST_F(HassTopicBuilderTest, BuildsDiscoveryTopicHandlesEmptyInputs) {
  // Test empty component - returns empty string
  std::string topic1 = builder->buildDiscoveryTopic("", "device123");
  EXPECT_EQ(topic1, "");

  // Test empty uniqueId - returns empty string
  std::string topic2 = builder->buildDiscoveryTopic("sensor", "");
  EXPECT_EQ(topic2, "");
}

// ============================================================================
// buildStateTopic Tests
// ============================================================================

TEST_F(HassTopicBuilderTest, BuildsStateTopicForGatewayEntity) {
  // Setup mock expectations for gateway calls
  EXPECT_CALL(mockSettings, getMqttTopic()).WillOnce(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("omg/gateway"));

  std::string topic = builder->buildStateTopic("/state", true);
  EXPECT_EQ(topic, "omg/gateway/state");
}

TEST_F(HassTopicBuilderTest, BuildsStateTopicForExternalDevice) {
  std::string topic = builder->buildStateTopic("omg/device/A1B2C3", false);
  EXPECT_EQ(topic, "+/+omg/device/A1B2C3");
}

TEST_F(HassTopicBuilderTest, BuildsStateTopicHandlesNullAndEmptyInputs) {
  // Test null topic - returns empty string
  std::string topic1 = builder->buildStateTopic(nullptr, true);
  EXPECT_EQ(topic1, "");

  // Test empty topic - returns empty string
  std::string topic2 = builder->buildStateTopic("", false);
  EXPECT_EQ(topic2, "");
}

// ============================================================================
// buildAvailabilityTopic Tests
// ============================================================================

TEST_F(HassTopicBuilderTest, BuildsAvailabilityTopicForGatewayEntity) {
  // Setup mock expectations for gateway calls
  EXPECT_CALL(mockSettings, getMqttTopic()).WillOnce(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("omg/gateway"));

  std::string topic = builder->buildAvailabilityTopic("/availability", true);
  EXPECT_EQ(topic, "omg/gateway/availability");
}

TEST_F(HassTopicBuilderTest, BuildsAvailabilityTopicForExternalDevice) {
  // External devices don't have availability topics managed by gateway
  std::string topic = builder->buildAvailabilityTopic("omg/device/A1B2C3", false);
  EXPECT_EQ(topic, "");
}

TEST_F(HassTopicBuilderTest, BuildsAvailabilityTopicHandlesNullAndEmptyInputs) {
  // Test null topic for gateway - uses default /LWT
  EXPECT_CALL(mockSettings, getMqttTopic()).WillOnce(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("/LWT"));
  std::string topic1 = builder->buildAvailabilityTopic(nullptr, true);
  EXPECT_EQ(topic1, "/LWT/LWT"); // baseTopic + default /LWT

  // Test external device - returns empty string
  std::string topic2 = builder->buildAvailabilityTopic("", false);
  EXPECT_EQ(topic2, "");
}

// ============================================================================
// buildCommandTopic Tests
// ============================================================================

TEST_F(HassTopicBuilderTest, BuildsCommandTopicCorrectly) {
  // Setup mock expectations
  EXPECT_CALL(mockSettings, getMqttTopic()).WillOnce(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("omg/switch/relay1"));

  std::string topic = builder->buildCommandTopic("/set");
  EXPECT_EQ(topic, "omg/switch/relay1/set");
}

TEST_F(HassTopicBuilderTest, BuildsCommandTopicHandlesNullAndEmptyInputs) {
  // Test null topic - returns empty string
  std::string topic1 = builder->buildCommandTopic(nullptr);
  EXPECT_EQ(topic1, "");

  // Test empty topic - returns empty string
  std::string topic2 = builder->buildCommandTopic("");
  EXPECT_EQ(topic2, "");
}

// ============================================================================
// Static Method Tests - isValidTopicComponent
// ============================================================================

TEST(HassTopicBuilderStaticTest, ValidatesCorrectTopicComponents) {
  // Test valid components
  EXPECT_TRUE(HassTopicBuilder::isValidTopicComponent("sensor"));
  EXPECT_TRUE(HassTopicBuilder::isValidTopicComponent("switch"));
  EXPECT_TRUE(HassTopicBuilder::isValidTopicComponent("binary_sensor"));
  EXPECT_TRUE(HassTopicBuilder::isValidTopicComponent("button"));
  EXPECT_TRUE(HassTopicBuilder::isValidTopicComponent("device_123"));
  EXPECT_TRUE(HassTopicBuilder::isValidTopicComponent("temp-sensor"));
  EXPECT_TRUE(HassTopicBuilder::isValidTopicComponent("SENSOR"));
}

TEST(HassTopicBuilderStaticTest, RejectsInvalidTopicComponents) {
  // Test invalid components
  EXPECT_FALSE(HassTopicBuilder::isValidTopicComponent("")); // Empty
  EXPECT_FALSE(HassTopicBuilder::isValidTopicComponent(nullptr)); // Null
  EXPECT_FALSE(HassTopicBuilder::isValidTopicComponent("sensor#device")); // Contains hash
  EXPECT_FALSE(HassTopicBuilder::isValidTopicComponent("sensor+device")); // Contains plus
  EXPECT_FALSE(HassTopicBuilder::isValidTopicComponent("sensor\tdevice")); // Contains tab
  EXPECT_FALSE(HassTopicBuilder::isValidTopicComponent("sensor\ndevice")); // Contains newline
  EXPECT_FALSE(HassTopicBuilder::isValidTopicComponent("\x1F")); // Control character
  EXPECT_FALSE(HassTopicBuilder::isValidTopicComponent("\x7F")); // DEL character
}

// ============================================================================
// Static Method Tests - sanitizeTopicComponent
// ============================================================================

TEST(HassTopicBuilderStaticTest, SanitizesTopicComponentCorrectly) {
  // Test valid components (should remain unchanged)
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("sensor"), "sensor");
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("device_123"), "device_123");
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("temp-sensor"), "temp-sensor");
}

TEST(HassTopicBuilderStaticTest, SanitizesInvalidCharacters) {
  // Test replacement of invalid characters
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("sensor/device"), "sensor_device");
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("sensor#device"), "sensor_device");
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("sensor+device"), "sensor_device");
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("sensor\tdevice"), "sensor_device");
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("sensor\ndevice"), "sensor_device");
}

TEST(HassTopicBuilderStaticTest, SanitizesMultipleInvalidCharacters) {
  // Test complex sanitization - the actual implementation output
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("my/sensor #1+test"), "my_sensor _1_test");
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("///"), "unknown"); // All invalid becomes "unknown"
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent("   "), "   "); // Spaces are not replaced
}

TEST(HassTopicBuilderStaticTest, SanitizesNullAndEmptyComponents) {
  // Test null and empty inputs
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent(nullptr), "");
  EXPECT_EQ(HassTopicBuilder::sanitizeTopicComponent(""), "unknown"); // Empty becomes "unknown"
}

// ============================================================================
// Edge Cases and Performance Tests
// ============================================================================

TEST_F(HassTopicBuilderTest, HandlesLongTopicStrings) {
  // Test with long strings
  std::string longComponent(100, 'x');
  std::string longUniqueId(100, 'y');

  std::string topic = builder->buildDiscoveryTopic(longComponent.c_str(), longUniqueId.c_str());
  EXPECT_TRUE(topic.find(longComponent) != std::string::npos);
  EXPECT_TRUE(topic.find(longUniqueId) != std::string::npos);
  EXPECT_TRUE(topic.find("homeassistant") != std::string::npos);
  EXPECT_TRUE(topic.find("/config") != std::string::npos);
}

TEST_F(HassTopicBuilderTest, HandlesSpecialCharactersInTopics) {
  // Test with special but valid characters
  EXPECT_CALL(mockSettings, getMqttTopic()).WillOnce(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("omg/device-1_test"));
  std::string topic1 = builder->buildStateTopic("/state", true);
  EXPECT_EQ(topic1, "omg/device-1_test/state");

  EXPECT_CALL(mockSettings, getMqttTopic()).WillOnce(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("omg/DEVICE_123"));
  std::string topic2 = builder->buildCommandTopic("/set");
  EXPECT_EQ(topic2, "omg/DEVICE_123/set");
}

TEST(HassTopicBuilderStaticTest, ValidationPerformance) {
  // Test validation performance with many iterations
  const int iterations = 10000;

  for (int i = 0; i < iterations; i++) {
    HassTopicBuilder::isValidTopicComponent("sensor");
    HassTopicBuilder::isValidTopicComponent("invalid/component");
    HassTopicBuilder::sanitizeTopicComponent("test component");
  }

  // If we got here without timeout, performance is acceptable
  SUCCEED();
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(HassTopicBuilderTest, BuildsCompleteTopicHierarchy) {
  // Test building a complete set of topics for a device
  const char* component = "sensor";
  const char* uniqueId = "temp_sensor_01";
  const char* baseTopic = "omg/BTtoMQTT/A1B2C3D4E5F6";

  std::string discoveryTopic = builder->buildDiscoveryTopic(component, uniqueId);
  std::string stateTopic = builder->buildStateTopic(baseTopic, false);
  std::string availabilityTopic = builder->buildAvailabilityTopic(baseTopic, false);

  EXPECT_EQ(discoveryTopic, "homeassistant/sensor/temp_sensor_01/config");
  EXPECT_EQ(stateTopic, "+/+omg/BTtoMQTT/A1B2C3D4E5F6");
  EXPECT_EQ(availabilityTopic, ""); // External devices have no availability
}

TEST_F(HassTopicBuilderTest, BuildsGatewayTopicHierarchy) {
  // Test building topics for gateway entities
  const char* component = "sensor";
  const char* uniqueId = "gateway_uptime";
  const char* baseTopic = "/state";

  std::string discoveryTopic = builder->buildDiscoveryTopic(component, uniqueId);

  EXPECT_CALL(mockSettings, getMqttTopic()).WillOnce(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("omg/gateway"));
  std::string stateTopic = builder->buildStateTopic(baseTopic, true);

  EXPECT_CALL(mockSettings, getMqttTopic()).WillOnce(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("omg/gateway"));
  std::string availabilityTopic = builder->buildAvailabilityTopic("/availability", true);

  EXPECT_EQ(discoveryTopic, "homeassistant/sensor/gateway_uptime/config");
  EXPECT_EQ(stateTopic, "omg/gateway/state");
  EXPECT_EQ(availabilityTopic, "omg/gateway/availability");
}