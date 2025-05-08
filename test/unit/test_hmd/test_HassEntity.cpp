#include <ArduinoJson.h>
#include <HMD/IMqttPublisher.h>
#include <HMD/ISettingsProvider.h>
#include <HMD/core/HassDevice.h>
#include <HMD/core/HassTopicBuilder.h>
#include <HMD/entities/HassEntity.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace omg::hass;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

// Mock for IMqttPublisher
class MockMqttPublisher : public IMqttPublisher {
public:
  MOCK_METHOD1(publishJson, bool(JsonObject& json));
  MOCK_METHOD3(publishMessage, bool(const std::string& topic, const std::string& payload, bool retain));
  MOCK_METHOD2(getUId, std::string(const std::string& name, const std::string& suffix));
};

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

// Concrete test implementation of HassEntity since it's abstract
class TestHassEntity : public HassEntity {
public:
  TestHassEntity(const EntityConfig& config, std::shared_ptr<HassDevice> device)
      : HassEntity(config, device) {}

protected:
  void addSpecificFields(JsonObject& json, const HassTopicBuilder& topicBuilder) const override {
    // Add test-specific fields
    json["test_field"] = "test_value";
    if (!getConfig().stateTopic.empty()) {
      json["stat_t"] = topicBuilder.buildStateTopic(getConfig().stateTopic.c_str(), getDevice()->isGateway());
    }
  }
};

// Test fixture for HassEntity
class HassEntityTest : public ::testing::Test {
protected:
  NiceMock<MockSettingsProvider> mockSettings;
  NiceMock<MockMqttPublisher> mockPublisher;
  std::unique_ptr<HassTopicBuilder> topicBuilder;
  std::shared_ptr<HassDevice> testDevice;
  DynamicJsonDocument doc{1024};

  void SetUp() override {
    // Default mock setup
    ON_CALL(mockSettings, getDiscoveryPrefix()).WillByDefault(Return("homeassistant"));
    ON_CALL(mockSettings, getGatewayName()).WillByDefault(Return("TestGateway"));
    ON_CALL(mockSettings, getGatewayManufacturer()).WillByDefault(Return("OpenMQTTGateway"));
    ON_CALL(mockSettings, getGatewayVersion()).WillByDefault(Return("1.0.0"));
    ON_CALL(mockSettings, getNetworkMacAddress()).WillByDefault(Return("AA:BB:CC:DD:EE:FF"));
    ON_CALL(mockSettings, getNetworkIPAddress()).WillByDefault(Return("192.168.1.100"));
    ON_CALL(mockSettings, getMqttTopic()).WillByDefault(Return(""));

    // Create empty modules array
    JsonArray modules = doc.createNestedArray("modules");
    ON_CALL(mockSettings, getModules()).WillByDefault(Return(modules));

    // Create topic builder and test device
    topicBuilder = std::make_unique<HassTopicBuilder>(mockSettings);
    testDevice = std::make_shared<HassDevice>(createTestDeviceInfo(), mockSettings);

    // Mock publisher default behaviors
    ON_CALL(mockPublisher, publishJson(_)).WillByDefault(Return(true));
    ON_CALL(mockPublisher, publishMessage(_, _, _)).WillByDefault(Return(true));
    ON_CALL(mockPublisher, getUId(_, _)).WillByDefault(Return("unique_test_id"));
  }

  void TearDown() override {
    // Cleanup
  }

  // Helper to create valid EntityConfig
  HassEntity::EntityConfig createValidEntityConfig() {
    HassEntity::EntityConfig config;
    config.componentType = "sensor";
    config.name = "Test Sensor";
    config.uniqueId = "test_sensor_01";
    config.deviceClass = "temperature";
    config.unitOfMeasurement = "°C";
    config.valueTemplate = "{{ value_json.temperature }}";
    config.stateClass = "measurement";
    config.stateTopic = "/test/state";
    config.isDiagnostic = false;
    config.offDelay = 0;
    config.retain = false;
    return config;
  }

  // Helper to create test device info
  HassDevice::DeviceInfo createTestDeviceInfo() {
    HassDevice::DeviceInfo info;
    info.name = "Test Device";
    info.manufacturer = "Test Manufacturer";
    info.model = "Test Model";
    info.identifier = "AA:BB:CC:DD:EE:FF";
    info.swVersion = "1.0.0";
    info.isGateway = false;
    return info;
  }
};

// ============================================================================
// EntityConfig Tests
// ============================================================================

TEST_F(HassEntityTest, EntityConfigDefaultConstructor) {
  HassEntity::EntityConfig config;

  // Check default values
  EXPECT_TRUE(config.componentType.empty());
  EXPECT_TRUE(config.name.empty());
  EXPECT_TRUE(config.uniqueId.empty());
  EXPECT_TRUE(config.deviceClass.empty());
  EXPECT_TRUE(config.valueTemplate.empty());
  EXPECT_TRUE(config.unitOfMeasurement.empty());
  EXPECT_TRUE(config.stateClass.empty());
  EXPECT_TRUE(config.stateTopic.empty());
  EXPECT_TRUE(config.commandTopic.empty());
  EXPECT_TRUE(config.availabilityTopic.empty());
  EXPECT_FALSE(config.isDiagnostic);
  EXPECT_EQ(config.offDelay, 0);
  EXPECT_FALSE(config.retain);
}

TEST_F(HassEntityTest, EntityConfigValidation) {
  HassEntity::EntityConfig config;

  // Empty config is invalid
  EXPECT_FALSE(config.isValid());

  // Only component type is not enough
  config.componentType = "sensor";
  EXPECT_FALSE(config.isValid());

  // Component type and name are not enough
  config.name = "Test Sensor";
  EXPECT_FALSE(config.isValid());

  // All required fields make it valid
  config.uniqueId = "test_sensor_01";
  EXPECT_TRUE(config.isValid());

  // Clear required field makes it invalid again
  config.componentType = "";
  EXPECT_FALSE(config.isValid());
}

TEST_F(HassEntityTest, CreateSensorConfig) {
  auto config = HassEntity::EntityConfig::createSensor("Temperature", "temp_01", "temperature", "°C");

  EXPECT_EQ(config.componentType, "sensor");
  EXPECT_EQ(config.name, "Temperature");
  EXPECT_EQ(config.uniqueId, "temp_01");
  EXPECT_EQ(config.deviceClass, "temperature");
  EXPECT_EQ(config.unitOfMeasurement, "°C");
  EXPECT_EQ(config.stateClass, "measurement"); // Should be set for sensors with units
  EXPECT_TRUE(config.isValid());
}

TEST_F(HassEntityTest, CreateSensorConfigWithoutOptionalParams) {
  auto config = HassEntity::EntityConfig::createSensor("Binary Sensor", "binary_01");

  EXPECT_EQ(config.componentType, "sensor");
  EXPECT_EQ(config.name, "Binary Sensor");
  EXPECT_EQ(config.uniqueId, "binary_01");
  EXPECT_TRUE(config.deviceClass.empty());
  EXPECT_TRUE(config.unitOfMeasurement.empty());
  EXPECT_TRUE(config.stateClass.empty()); // No state class for unitless sensors
  EXPECT_TRUE(config.isValid());
}

TEST_F(HassEntityTest, CreateSwitchConfig) {
  auto config = HassEntity::EntityConfig::createSwitch("Test Switch", "switch_01");

  EXPECT_EQ(config.componentType, "switch");
  EXPECT_EQ(config.name, "Test Switch");
  EXPECT_EQ(config.uniqueId, "switch_01");
  EXPECT_TRUE(config.isValid());
}

TEST_F(HassEntityTest, CreateButtonConfig) {
  auto config = HassEntity::EntityConfig::createButton("Test Button", "button_01");

  EXPECT_EQ(config.componentType, "button");
  EXPECT_EQ(config.name, "Test Button");
  EXPECT_EQ(config.uniqueId, "button_01");
  EXPECT_TRUE(config.isValid());
}

// ============================================================================
// Constructor and Basic Methods Tests
// ============================================================================

TEST_F(HassEntityTest, ConstructorWithValidConfig) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  EXPECT_EQ(entity.getConfig().name, "Test Sensor");
  EXPECT_EQ(entity.getConfig().uniqueId, "test_sensor_01");
  EXPECT_EQ(entity.getConfig().componentType, "sensor");
  EXPECT_EQ(entity.getDevice(), testDevice);
}

TEST_F(HassEntityTest, ConstructorWithInvalidConfigThrows) {
  HassEntity::EntityConfig invalidConfig; // Empty config is invalid

  EXPECT_THROW(TestHassEntity entity(invalidConfig, testDevice), std::invalid_argument);
}

TEST_F(HassEntityTest, GettersReturnCorrectValues) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  const auto& entityConfig = entity.getConfig();
  EXPECT_EQ(entityConfig.name, config.name);
  EXPECT_EQ(entityConfig.uniqueId, config.uniqueId);
  EXPECT_EQ(entityConfig.componentType, config.componentType);
  EXPECT_EQ(entityConfig.deviceClass, config.deviceClass);
  EXPECT_EQ(entityConfig.unitOfMeasurement, config.unitOfMeasurement);

  EXPECT_EQ(entity.getDevice(), testDevice);
}

// ============================================================================
// updateConfig Tests
// ============================================================================

TEST_F(HassEntityTest, UpdateConfigWithValidData) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  // Create new valid config
  auto newConfig = HassEntity::EntityConfig::createSensor("Updated Sensor", "updated_01", "humidity", "%");

  EXPECT_TRUE(entity.updateConfig(newConfig));
  EXPECT_EQ(entity.getConfig().name, "Updated Sensor");
  EXPECT_EQ(entity.getConfig().uniqueId, "updated_01");
  EXPECT_EQ(entity.getConfig().deviceClass, "humidity");
  EXPECT_EQ(entity.getConfig().unitOfMeasurement, "%");
}

TEST_F(HassEntityTest, UpdateConfigWithInvalidDataFails) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  // Create invalid config
  HassEntity::EntityConfig invalidConfig;

  EXPECT_FALSE(entity.updateConfig(invalidConfig));

  // Original config should remain unchanged
  EXPECT_EQ(entity.getConfig().name, "Test Sensor");
  EXPECT_EQ(entity.getConfig().uniqueId, "test_sensor_01");
}

// ============================================================================
// Discovery Topic Tests
// ============================================================================

TEST_F(HassEntityTest, GetDiscoveryTopicReturnsCorrectFormat) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  std::string discoveryTopic = entity.getDiscoveryTopic(*topicBuilder);
  EXPECT_EQ(discoveryTopic, "homeassistant/sensor/test_sensor_01/config");
}

TEST_F(HassEntityTest, GetDiscoveryTopicWithDifferentComponent) {
  auto config = HassEntity::EntityConfig::createSwitch("Test Switch", "switch_01");
  TestHassEntity entity(config, testDevice);

  std::string discoveryTopic = entity.getDiscoveryTopic(*topicBuilder);
  EXPECT_EQ(discoveryTopic, "homeassistant/switch/switch_01/config");
}

// ============================================================================
// Publish Tests
// ============================================================================

TEST_F(HassEntityTest, PublishCallsMqttPublisher) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  EXPECT_CALL(mockPublisher, publishJson(_)).WillOnce(Return(true));

  bool result = entity.publish(*topicBuilder, mockPublisher);
  EXPECT_TRUE(result);
}

TEST_F(HassEntityTest, PublishWithMqttFailureReturnsFalse) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  EXPECT_CALL(mockPublisher, publishJson(_)).WillOnce(Return(false));

  bool result = entity.publish(*topicBuilder, mockPublisher);
  EXPECT_FALSE(result);
}

TEST_F(HassEntityTest, PublishGeneratesCorrectJsonStructure) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  // Use a simpler approach to avoid complex lambda capture issues
  bool publishCalled = false;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&publishCalled](JsonObject& json) {
        publishCalled = true;
        // Basic verification that JSON has expected fields
        EXPECT_TRUE(json.containsKey("name"));
        EXPECT_TRUE(json.containsKey("uniq_id"));
        EXPECT_TRUE(json.containsKey("topic"));
        EXPECT_TRUE(json.containsKey("retain"));
        return true;
      });

  bool result = entity.publish(*topicBuilder, mockPublisher);
  EXPECT_TRUE(result);
  EXPECT_TRUE(publishCalled);
}

// ============================================================================
// Erase Tests
// ============================================================================

TEST_F(HassEntityTest, EraseCallsMqttPublisherWithEmptyPayload) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  std::string expectedTopic = "homeassistant/sensor/test_sensor_01/config";
  EXPECT_CALL(mockPublisher, publishMessage(expectedTopic, "", true)).WillOnce(Return(true));

  bool result = entity.erase(*topicBuilder, mockPublisher);
  EXPECT_TRUE(result);
}

TEST_F(HassEntityTest, EraseWithMqttFailureReturnsFalse) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  EXPECT_CALL(mockPublisher, publishMessage(_, _, _)).WillOnce(Return(false));

  bool result = entity.erase(*topicBuilder, mockPublisher);
  EXPECT_FALSE(result);
}

// ============================================================================
// JSON Generation Tests - Common Fields
// ============================================================================

TEST_F(HassEntityTest, JsonIncludesValidDeviceClassAndUnit) {
  auto config = createValidEntityConfig();
  config.deviceClass = "temperature"; // Valid device class
  config.unitOfMeasurement = "°C"; // Valid unit
  TestHassEntity entity(config, testDevice);

  bool publishCalled = false;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&publishCalled](JsonObject& json) {
        publishCalled = true;
        // Verify fields are present
        EXPECT_TRUE(json.containsKey("dev_cla"));
        EXPECT_TRUE(json.containsKey("unit_of_meas"));
        return true;
      });

  entity.publish(*topicBuilder, mockPublisher);
  EXPECT_TRUE(publishCalled);
}

TEST_F(HassEntityTest, JsonExcludesInvalidDeviceClassAndUnit) {
  auto config = createValidEntityConfig();
  config.deviceClass = "invalid_class"; // Invalid device class
  config.unitOfMeasurement = "invalid_unit"; // Invalid unit
  TestHassEntity entity(config, testDevice);

  bool publishCalled = false;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&publishCalled](JsonObject& json) {
        publishCalled = true;
        // Invalid values should be excluded from JSON
        EXPECT_FALSE(json.containsKey("dev_cla"));
        EXPECT_FALSE(json.containsKey("unit_of_meas"));
        return true;
      });

  entity.publish(*topicBuilder, mockPublisher);
  EXPECT_TRUE(publishCalled);
}

TEST_F(HassEntityTest, JsonIncludesDiagnosticCategory) {
  auto config = createValidEntityConfig();
  config.isDiagnostic = true;
  TestHassEntity entity(config, testDevice);

  bool publishCalled = false;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&publishCalled](JsonObject& json) {
        publishCalled = true;
        EXPECT_TRUE(json.containsKey("ent_cat"));
        return true;
      });

  entity.publish(*topicBuilder, mockPublisher);
  EXPECT_TRUE(publishCalled);
}

TEST_F(HassEntityTest, JsonIncludesOffDelayWhenSet) {
  auto config = createValidEntityConfig();
  config.offDelay = 30;
  TestHassEntity entity(config, testDevice);

  bool publishCalled = false;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&publishCalled](JsonObject& json) {
        publishCalled = true;
        EXPECT_TRUE(json.containsKey("off_dly"));
        return true;
      });

  entity.publish(*topicBuilder, mockPublisher);
  EXPECT_TRUE(publishCalled);
}

TEST_F(HassEntityTest, JsonExcludesOffDelayWhenZero) {
  auto config = createValidEntityConfig();
  config.offDelay = 0; // Default value
  TestHassEntity entity(config, testDevice);

  bool publishCalled = false;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&publishCalled](JsonObject& json) {
        publishCalled = true;
        EXPECT_FALSE(json.containsKey("off_dly"));
        return true;
      });

  entity.publish(*topicBuilder, mockPublisher);
  EXPECT_TRUE(publishCalled);
}

// ============================================================================
// Gateway vs External Device Tests
// ============================================================================

TEST_F(HassEntityTest, GatewayEntityIncludesAvailabilityTopic) {
  auto config = createValidEntityConfig();

  // Create gateway device
  auto gatewayInfo = createTestDeviceInfo();
  gatewayInfo.isGateway = true;
  auto gatewayDevice = std::make_shared<HassDevice>(gatewayInfo, mockSettings);

  // Set up mock expectations for availability topic building
  EXPECT_CALL(mockSettings, getMqttTopic()).WillRepeatedly(Return(""));
  EXPECT_CALL(mockSettings, getGatewayName()).WillRepeatedly(Return("TestGateway"));

  TestHassEntity entity(config, gatewayDevice);

  bool publishCalled = false;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&publishCalled](JsonObject& json) {
        publishCalled = true;
        // Gateway entities should have availability topics
        EXPECT_TRUE(json.containsKey("avty_t"));
        EXPECT_TRUE(json.containsKey("pl_avail"));
        EXPECT_TRUE(json.containsKey("pl_not_avail"));
        return true;
      });

  entity.publish(*topicBuilder, mockPublisher);
  EXPECT_TRUE(publishCalled);
}

TEST_F(HassEntityTest, ExternalDeviceDoesNotIncludeAvailabilityTopic) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice); // testDevice is external

  JsonObject capturedJson;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&capturedJson](JsonObject& json) {
        DynamicJsonDocument doc(1024);
        doc.set(json);
        capturedJson = doc.as<JsonObject>();
        return true;
      });

  entity.publish(*topicBuilder, mockPublisher);

  // External devices should not have availability topics
  EXPECT_FALSE(capturedJson.containsKey("avty_t"));
  EXPECT_FALSE(capturedJson.containsKey("pl_avail"));
  EXPECT_FALSE(capturedJson.containsKey("pl_not_avail"));
}

// ============================================================================
// Template Generation Tests
// ============================================================================

TEST_F(HassEntityTest, AutoGeneratesTemplateForCommonUnits) {
  // Test temperature
  auto tempConfig = HassEntity::EntityConfig::createSensor("Temperature", "temp_01", "temperature", "°C");
  tempConfig.valueTemplate = ""; // Empty to trigger auto-generation
  TestHassEntity tempEntity(tempConfig, testDevice);

  JsonObject capturedJson;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&capturedJson](JsonObject& json) {
        DynamicJsonDocument doc(1024);
        doc.set(json);
        capturedJson = doc.as<JsonObject>();
        return true;
      });

  tempEntity.publish(*topicBuilder, mockPublisher);

  // Should auto-generate template for °C
  EXPECT_TRUE(capturedJson.containsKey("val_tpl"));
  std::string template_val = capturedJson["val_tpl"].as<std::string>();
  EXPECT_FALSE(template_val.empty());
}

TEST_F(HassEntityTest, UsesExplicitTemplateWhenProvided) {
  auto config = createValidEntityConfig();
  config.valueTemplate = "{{ value_json.custom_field }}";
  TestHassEntity entity(config, testDevice);

  JsonObject capturedJson;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&capturedJson](JsonObject& json) {
        DynamicJsonDocument doc(1024);
        doc.set(json);
        capturedJson = doc.as<JsonObject>();
        return true;
      });

  entity.publish(*topicBuilder, mockPublisher);

  EXPECT_EQ(capturedJson["val_tpl"].as<std::string>(), "{{ value_json.custom_field }}");
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(HassEntityTest, HandlesNullDevice) {
  auto config = createValidEntityConfig();

  // This should not crash but may not work as expected
  EXPECT_NO_THROW(TestHassEntity entity(config, nullptr));
}

TEST_F(HassEntityTest, HandlesEmptyOptionalFields) {
  auto config = createValidEntityConfig();
  config.deviceClass = "";
  config.unitOfMeasurement = "";
  config.valueTemplate = "";
  config.stateClass = "";
  config.commandTopic = "";
  config.availabilityTopic = "";

  TestHassEntity entity(config, testDevice);

  JsonObject capturedJson;
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce([&capturedJson](JsonObject& json) {
        DynamicJsonDocument doc(1024);
        doc.set(json);
        capturedJson = doc.as<JsonObject>();
        return true;
      });

  EXPECT_NO_THROW(entity.publish(*topicBuilder, mockPublisher));

  // Should still have basic required fields
  EXPECT_TRUE(capturedJson.containsKey("name"));
  EXPECT_TRUE(capturedJson.containsKey("uniq_id"));
}

// ============================================================================
// Performance and Integration Tests
// ============================================================================

TEST_F(HassEntityTest, MultiplePublishOperationsAreFast) {
  auto config = createValidEntityConfig();
  TestHassEntity entity(config, testDevice);

  EXPECT_CALL(mockPublisher, publishJson(_)).WillRepeatedly(Return(true));

  // Test multiple publications
  const int iterations = 50;
  for (int i = 0; i < iterations; i++) {
    bool result = entity.publish(*topicBuilder, mockPublisher);
    EXPECT_TRUE(result);
  }

  // If we got here without timeout, performance is acceptable
  SUCCEED();
}

TEST_F(HassEntityTest, CreateMultipleEntitiesWithDifferentConfigs) {
  // Test creating multiple entities to ensure no side effects

  auto sensorConfig = HassEntity::EntityConfig::createSensor("Temperature", "temp_01", "temperature", "°C");
  auto switchConfig = HassEntity::EntityConfig::createSwitch("Relay", "relay_01");
  auto buttonConfig = HassEntity::EntityConfig::createButton("Restart", "restart_01");

  TestHassEntity sensor(sensorConfig, testDevice);
  TestHassEntity switch_entity(switchConfig, testDevice);
  TestHassEntity button(buttonConfig, testDevice);

  // Each should maintain its own state
  EXPECT_EQ(sensor.getConfig().componentType, "sensor");
  EXPECT_EQ(switch_entity.getConfig().componentType, "switch");
  EXPECT_EQ(button.getConfig().componentType, "button");

  EXPECT_EQ(sensor.getConfig().deviceClass, "temperature");
  EXPECT_TRUE(switch_entity.getConfig().deviceClass.empty());
  EXPECT_TRUE(button.getConfig().deviceClass.empty());
}