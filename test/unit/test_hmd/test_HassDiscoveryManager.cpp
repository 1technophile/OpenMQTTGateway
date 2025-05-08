#include <ArduinoJson.h>
#include <HMD/IMqttPublisher.h>
#include <HMD/ISettingsProvider.h>
#include <HMD/core/HassDevice.h>
#include <HMD/core/HassTopicBuilder.h>
#include <HMD/entities/HassButton.h>
#include <HMD/entities/HassSensor.h>
#include <HMD/entities/HassSwitch.h>
#include <HMD/manager/HassDiscoveryManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace omg::hass;
using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

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

// Test fixture for HassDiscoveryManager
class HassDiscoveryManagerTest : public ::testing::Test {
protected:
  NiceMock<MockSettingsProvider> mockSettings;
  NiceMock<MockMqttPublisher> mockPublisher;
  std::unique_ptr<HassDiscoveryManager> manager;
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
    ON_CALL(mockSettings, isEthConnected()).WillByDefault(Return(true));

    // Create empty modules array
    JsonArray modules = doc.createNestedArray("modules");
    ON_CALL(mockSettings, getModules()).WillByDefault(Return(modules));

    // Default UID generation
    ON_CALL(mockPublisher, getUId(_, _)).WillByDefault(Invoke([](const std::string& name, const std::string& suffix) {
      return name + (suffix.empty() ? "" : "_" + suffix);
    }));

    // Create manager instance
    manager = std::make_unique<HassDiscoveryManager>(mockSettings, mockPublisher);
  }

  void TearDown() override {
    manager.reset();
  }

  // Helper to create test entity config
  HassEntity::EntityConfig createTestEntityConfig(const std::string& componentType = "sensor",
                                                  const std::string& name = "Test Entity",
                                                  const std::string& uniqueId = "test_entity") {
    HassEntity::EntityConfig config;
    config.componentType = componentType;
    config.name = name;
    config.uniqueId = uniqueId;
    config.deviceClass = "temperature";
    config.unitOfMeasurement = "°C";
    config.stateClass = "measurement";
    config.stateTopic = "test/state";
    return config;
  }

  // Helper to create test sensor entity
  std::unique_ptr<HassSensor> createTestSensor(const std::string& name = "Test Sensor",
                                               const std::string& uniqueId = "test_sensor") {
    auto config = createTestEntityConfig("sensor", name, uniqueId);
    auto device = manager->getGatewayDevice();
    return std::make_unique<HassSensor>(config, device);
  }
};

// ============================================================================
// Constructor and Initialization Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, ConstructorInitializesCorrectly) {
  EXPECT_NE(manager, nullptr);
  EXPECT_EQ(manager->getEntityCount(), 0);
  EXPECT_NE(&manager->getTopicBuilder(), nullptr);
}

TEST_F(HassDiscoveryManagerTest, ConstructorInitializesGatewayDevice) {
  auto gatewayDevice = manager->getGatewayDevice();
  EXPECT_NE(gatewayDevice, nullptr);
  EXPECT_TRUE(gatewayDevice->isGateway());
  EXPECT_EQ(gatewayDevice->getName(), "TestGateway");
}

TEST_F(HassDiscoveryManagerTest, GetGatewayDeviceReturnsConsistentInstance) {
  auto device1 = manager->getGatewayDevice();
  auto device2 = manager->getGatewayDevice();
  EXPECT_EQ(device1, device2); // Same shared_ptr instance
}

// ============================================================================
// Gateway Device Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, GatewayDeviceHasCorrectProperties) {
  auto device = manager->getGatewayDevice();

  EXPECT_TRUE(device->isGateway());
  EXPECT_EQ(device->getName(), "TestGateway");
  EXPECT_EQ(device->getManufacturer(), "OpenMQTTGateway");
  // Gateway device uses modules array as model, which should be "[]" for empty array
  EXPECT_EQ(device->getModel(), "[]");
  EXPECT_EQ(device->getIdentifier(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(HassDiscoveryManagerTest, GatewayDeviceUsesSettingsProvider) {
  // Change settings and create new manager
  ON_CALL(mockSettings, getGatewayName()).WillByDefault(Return("CustomGateway"));
  ON_CALL(mockSettings, getGatewayManufacturer()).WillByDefault(Return("CustomMfg"));

  auto newManager = std::make_unique<HassDiscoveryManager>(mockSettings, mockPublisher);
  auto device = newManager->getGatewayDevice();

  EXPECT_EQ(device->getName(), "CustomGateway");
  EXPECT_EQ(device->getManufacturer(), "CustomMfg");
}

// ============================================================================
// External Device Creation Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, CreateExternalDeviceWithAllParameters) {
  auto device = manager->createExternalDevice("Test Device", "Test Mfg", "Test Model", "test_id");

  EXPECT_NE(device, nullptr);
  EXPECT_FALSE(device->isGateway());
  EXPECT_EQ(device->getName(), "Test Device");
  EXPECT_EQ(device->getManufacturer(), "Test Mfg");
  EXPECT_EQ(device->getModel(), "Test Model");
  EXPECT_EQ(device->getIdentifier(), "test_id");
}

TEST_F(HassDiscoveryManagerTest, CreateExternalDeviceWithNullParameters) {
  auto device = manager->createExternalDevice(nullptr, nullptr, nullptr, nullptr);

  EXPECT_NE(device, nullptr);
  EXPECT_FALSE(device->isGateway());
  EXPECT_EQ(device->getName(), "Unknown Device");
  EXPECT_EQ(device->getManufacturer(), "Unknown");
  EXPECT_EQ(device->getModel(), "Unknown");
  EXPECT_EQ(device->getIdentifier(), "");
}

TEST_F(HassDiscoveryManagerTest, CreateExternalDeviceWithEmptyStrings) {
  auto device = manager->createExternalDevice("", "", "", "");

  EXPECT_NE(device, nullptr);
  EXPECT_FALSE(device->isGateway());
  EXPECT_EQ(device->getName(), "Unknown Device");
  EXPECT_EQ(device->getManufacturer(), "Unknown");
  EXPECT_EQ(device->getModel(), "Unknown");
  EXPECT_EQ(device->getIdentifier(), "");
}

TEST_F(HassDiscoveryManagerTest, CreateMultipleExternalDevices) {
  auto device1 = manager->createExternalDevice("Device1", "Mfg1", "Model1", "id1");
  auto device2 = manager->createExternalDevice("Device2", "Mfg2", "Model2", "id2");

  EXPECT_NE(device1, device2);
  EXPECT_EQ(device1->getName(), "Device1");
  EXPECT_EQ(device2->getName(), "Device2");
  EXPECT_EQ(device1->getIdentifier(), "id1");
  EXPECT_EQ(device2->getIdentifier(), "id2");
}

// ============================================================================
// Entity Publishing Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, PublishEntitySuccess) {
  auto entity = createTestSensor();

  // Expect MQTT publish to be called
  EXPECT_CALL(mockPublisher, publishJson(_)).WillOnce(Return(true));

  bool result = manager->publishEntity(std::move(entity));

  EXPECT_TRUE(result);
  EXPECT_EQ(manager->getEntityCount(), 1);
}

TEST_F(HassDiscoveryManagerTest, PublishEntityFailure) {
  auto entity = createTestSensor();

  // Expect MQTT publish to fail
  EXPECT_CALL(mockPublisher, publishJson(_)).WillOnce(Return(false));

  bool result = manager->publishEntity(std::move(entity));

  EXPECT_FALSE(result);
  EXPECT_EQ(manager->getEntityCount(), 0); // Entity not added on failure
}

TEST_F(HassDiscoveryManagerTest, PublishNullEntityFails) {
  bool result = manager->publishEntity(nullptr);

  EXPECT_FALSE(result);
  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, PublishInvalidEntityFails) {
  // Create entity with invalid config (empty required fields)
  auto config = createTestEntityConfig();
  config.componentType = ""; // Invalid - empty component type
  auto device = manager->getGatewayDevice();

  // Entity constructor throws exception for invalid config
  EXPECT_THROW({ auto entity = std::make_unique<HassSensor>(config, device); }, std::invalid_argument);

  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, PublishMultipleEntities) {
  auto entity1 = createTestSensor("Sensor1", "sensor1");
  auto entity2 = createTestSensor("Sensor2", "sensor2");

  EXPECT_CALL(mockPublisher, publishJson(_)).Times(2).WillRepeatedly(Return(true));

  bool result1 = manager->publishEntity(std::move(entity1));
  bool result2 = manager->publishEntity(std::move(entity2));

  EXPECT_TRUE(result1);
  EXPECT_TRUE(result2);
  EXPECT_EQ(manager->getEntityCount(), 2);
}

// ============================================================================
// Legacy Array Publishing Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, PublishEntityFromArraySensorSuccess) {
  const char* sensorArray[][13] = {
      {"sensor", "Test Sensor", "test_sensor", "temperature", "{{ value_json.temp }}", "", "", "°C", "measurement", "", "", "test/state", ""}};

  auto device = manager->getGatewayDevice();

  EXPECT_CALL(mockPublisher, publishJson(_)).WillOnce(Return(true));

  manager->publishEntityFromArray(sensorArray, 1, device);

  EXPECT_EQ(manager->getEntityCount(), 1);
}

TEST_F(HassDiscoveryManagerTest, PublishEntityFromArraySwitchSuccess) {
  const char* switchArray[][13] = {
      {"switch", "Test Switch", "test_switch", "", "", "true", "false", "", "", "false", "true", "test/state", "test/cmd"}};

  auto device = manager->getGatewayDevice();

  EXPECT_CALL(mockPublisher, publishJson(_)).WillOnce(Return(true));

  manager->publishEntityFromArray(switchArray, 1, device);

  EXPECT_EQ(manager->getEntityCount(), 1);
}

TEST_F(HassDiscoveryManagerTest, PublishEntityFromArrayButtonSuccess) {
  const char* buttonArray[][13] = {
      {"button", "Test Button", "test_button", "", "", "{\"cmd\":\"press\"}", "", "", "", "", "", "", "test/cmd"}};

  auto device = manager->getGatewayDevice();

  EXPECT_CALL(mockPublisher, publishJson(_)).WillOnce(Return(true));

  manager->publishEntityFromArray(buttonArray, 1, device);

  EXPECT_EQ(manager->getEntityCount(), 1);
}

TEST_F(HassDiscoveryManagerTest, PublishEntityFromArrayMultipleEntities) {
  const char* entityArray[][13] = {
      {"sensor", "Sensor1", "sensor1", "temperature", "", "", "", "°C", "", "", "", "test/state1", ""},
      {"sensor", "Sensor2", "sensor2", "humidity", "", "", "", "%", "", "", "", "test/state2", ""},
      {"switch", "Switch1", "switch1", "", "", "ON", "OFF", "", "", "OFF", "ON", "test/state3", "test/cmd3"}};

  auto device = manager->getGatewayDevice();

  EXPECT_CALL(mockPublisher, publishJson(_)).Times(3).WillRepeatedly(Return(true));

  manager->publishEntityFromArray(entityArray, 3, device);

  EXPECT_EQ(manager->getEntityCount(), 3);
}

TEST_F(HassDiscoveryManagerTest, PublishEntityFromArrayWithNullParameters) {
  auto device = manager->getGatewayDevice();

  // Test null array
  manager->publishEntityFromArray(nullptr, 1, device);
  EXPECT_EQ(manager->getEntityCount(), 0);

  // Test zero count
  const char* entityArray[][13] = {{"sensor", "Test", "test", "", "", "", "", "", "", "", "", "", ""}};
  manager->publishEntityFromArray(entityArray, 0, device);
  EXPECT_EQ(manager->getEntityCount(), 0);

  // Test null device
  manager->publishEntityFromArray(entityArray, 1, nullptr);
  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, PublishEntityFromArrayUnsupportedType) {
  const char* unsupportedArray[][13] = {
      {"unsupported_type", "Test", "test", "", "", "", "", "", "", "", "", "", ""}};

  auto device = manager->getGatewayDevice();

  manager->publishEntityFromArray(unsupportedArray, 1, device);

  EXPECT_EQ(manager->getEntityCount(), 0); // Unsupported type not added
}

TEST_F(HassDiscoveryManagerTest, PublishEntityFromArrayInvalidRow) {
  const char* invalidArray[][13] = {
      {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}};

  auto device = manager->getGatewayDevice();

  manager->publishEntityFromArray(invalidArray, 1, device);

  EXPECT_EQ(manager->getEntityCount(), 0); // Invalid row not processed
}

// ============================================================================
// Entity Erase Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, EraseEntitySuccess) {
  EXPECT_CALL(mockPublisher, publishMessage("homeassistant/sensor/test_id/config", "", true))
      .WillOnce(Return(true));

  manager->eraseEntity("sensor", "test_id");

  // Note: eraseEntity doesn't remove from internal list, just publishes empty config
}

TEST_F(HassDiscoveryManagerTest, EraseEntityWithNullParameters) {
  // Should not call publishMessage with null parameters
  EXPECT_CALL(mockPublisher, publishMessage(_, _, _)).Times(0);

  manager->eraseEntity(nullptr, "test_id");
  manager->eraseEntity("sensor", nullptr);
  manager->eraseEntity(nullptr, nullptr);
}

TEST_F(HassDiscoveryManagerTest, EraseEntityWithCustomDiscoveryPrefix) {
  ON_CALL(mockSettings, getDiscoveryPrefix()).WillByDefault(Return("custom_discovery"));
  auto customManager = std::make_unique<HassDiscoveryManager>(mockSettings, mockPublisher);

  EXPECT_CALL(mockPublisher, publishMessage("custom_discovery/sensor/test_id/config", "", true))
      .WillOnce(Return(true));

  customManager->eraseEntity("sensor", "test_id");
}

// ============================================================================
// Entity Management Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, ClearEntitiesRemovesAll) {
  // Add some entities first
  auto entity1 = createTestSensor("Sensor1", "sensor1");
  auto entity2 = createTestSensor("Sensor2", "sensor2");

  EXPECT_CALL(mockPublisher, publishJson(_)).Times(2).WillRepeatedly(Return(true));

  manager->publishEntity(std::move(entity1));
  manager->publishEntity(std::move(entity2));

  EXPECT_EQ(manager->getEntityCount(), 2);

  manager->clearEntities();

  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, ClearEntitiesWhenEmpty) {
  EXPECT_EQ(manager->getEntityCount(), 0);

  manager->clearEntities();

  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, RepublishAllEntitiesSuccess) {
  // Add some entities first
  auto entity1 = createTestSensor("Sensor1", "sensor1");
  auto entity2 = createTestSensor("Sensor2", "sensor2");

  EXPECT_CALL(mockPublisher, publishJson(_)).Times(2).WillRepeatedly(Return(true));

  manager->publishEntity(std::move(entity1));
  manager->publishEntity(std::move(entity2));

  // Now expect republish calls
  EXPECT_CALL(mockPublisher, publishJson(_)).Times(2).WillRepeatedly(Return(true));

  manager->republishAllEntities();

  EXPECT_EQ(manager->getEntityCount(), 2); // Count unchanged
}

TEST_F(HassDiscoveryManagerTest, RepublishAllEntitiesWhenEmpty) {
  EXPECT_EQ(manager->getEntityCount(), 0);

  // Should not call publishJson when no entities
  EXPECT_CALL(mockPublisher, publishJson(_)).Times(0);

  manager->republishAllEntities();

  EXPECT_EQ(manager->getEntityCount(), 0);
}

// ============================================================================
// Topic Builder Integration Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, TopicBuilderUsesSettingsProvider) {
  const auto& topicBuilder = manager->getTopicBuilder();

  // TopicBuilder should use the same settings provider
  // We can't directly test this, but we can verify the prefix is used correctly
  std::string topic = topicBuilder.buildDiscoveryTopic("sensor", "test_id");
  EXPECT_EQ(topic, "homeassistant/sensor/test_id/config");
}

TEST_F(HassDiscoveryManagerTest, TopicBuilderReturnsConsistentReference) {
  const auto& topicBuilder1 = manager->getTopicBuilder();
  const auto& topicBuilder2 = manager->getTopicBuilder();

  EXPECT_EQ(&topicBuilder1, &topicBuilder2); // Same reference
}

// ============================================================================
// Entity Validation Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, ValidEntityPassesValidation) {
  auto entity = createTestSensor();
  auto* entityPtr = entity.get();

  EXPECT_CALL(mockPublisher, publishJson(_)).WillOnce(Return(true));

  bool result = manager->publishEntity(std::move(entity));

  EXPECT_TRUE(result);
  EXPECT_EQ(manager->getEntityCount(), 1);
}

TEST_F(HassDiscoveryManagerTest, EntityWithEmptyComponentTypeFails) {
  auto config = createTestEntityConfig();
  config.componentType = ""; // Invalid
  auto device = manager->getGatewayDevice();

  EXPECT_THROW({ auto entity = std::make_unique<HassSensor>(config, device); }, std::invalid_argument);

  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, EntityWithEmptyNameFails) {
  auto config = createTestEntityConfig();
  config.name = ""; // Invalid
  auto device = manager->getGatewayDevice();

  EXPECT_THROW({ auto entity = std::make_unique<HassSensor>(config, device); }, std::invalid_argument);

  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, EntityWithEmptyUniqueIdFails) {
  auto config = createTestEntityConfig();
  config.uniqueId = ""; // Invalid
  auto device = manager->getGatewayDevice();

  EXPECT_THROW({ auto entity = std::make_unique<HassSensor>(config, device); }, std::invalid_argument);

  EXPECT_EQ(manager->getEntityCount(), 0);
}

// ============================================================================
// Integration and Performance Tests
// ============================================================================

TEST_F(HassDiscoveryManagerTest, FullWorkflowIntegrationTest) {
  // Create external device
  auto device = manager->createExternalDevice("Test Device", "Test Mfg", "Model1", "device123");

  // Create and publish multiple entity types
  auto sensorConfig = createTestEntityConfig("sensor", "Temperature", "temp_sensor");
  auto sensor = std::make_unique<HassSensor>(sensorConfig, device);

  auto switchConfig = createTestEntityConfig("switch", "Power Switch", "power_switch");
  auto switchEntity = std::make_unique<HassSwitch>(
      switchConfig,
      HassSwitch::SwitchConfig::createWithJsonPayloads("true", "false"),
      device);

  EXPECT_CALL(mockPublisher, publishJson(_)).Times(2).WillRepeatedly(Return(true));

  bool sensorResult = manager->publishEntity(std::move(sensor));
  bool switchResult = manager->publishEntity(std::move(switchEntity));

  EXPECT_TRUE(sensorResult);
  EXPECT_TRUE(switchResult);
  EXPECT_EQ(manager->getEntityCount(), 2);

  // Test republish
  EXPECT_CALL(mockPublisher, publishJson(_)).Times(2).WillRepeatedly(Return(true));
  manager->republishAllEntities();

  // Test erase
  EXPECT_CALL(mockPublisher, publishMessage("homeassistant/sensor/temp_sensor/config", "", true))
      .WillOnce(Return(true));
  manager->eraseEntity("sensor", "temp_sensor");

  // Test clear
  manager->clearEntities();
  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, HandlesLargeNumberOfEntities) {
  const int numEntities = 100;

  EXPECT_CALL(mockPublisher, publishJson(_)).Times(numEntities).WillRepeatedly(Return(true));

  for (int i = 0; i < numEntities; i++) {
    auto entity = createTestSensor("Sensor" + std::to_string(i), "sensor" + std::to_string(i));
    bool result = manager->publishEntity(std::move(entity));
    EXPECT_TRUE(result);
  }

  EXPECT_EQ(manager->getEntityCount(), numEntities);

  // Test republish performance
  EXPECT_CALL(mockPublisher, publishJson(_)).Times(numEntities).WillRepeatedly(Return(true));
  manager->republishAllEntities();

  manager->clearEntities();
  EXPECT_EQ(manager->getEntityCount(), 0);
}

TEST_F(HassDiscoveryManagerTest, HandlesEntityPublishFailuresGracefully) {
  auto entity1 = createTestSensor("Sensor1", "sensor1");
  auto entity2 = createTestSensor("Sensor2", "sensor2");
  auto entity3 = createTestSensor("Sensor3", "sensor3");

  // First succeeds, second fails, third succeeds
  EXPECT_CALL(mockPublisher, publishJson(_))
      .WillOnce(Return(true))
      .WillOnce(Return(false))
      .WillOnce(Return(true));

  bool result1 = manager->publishEntity(std::move(entity1));
  bool result2 = manager->publishEntity(std::move(entity2));
  bool result3 = manager->publishEntity(std::move(entity3));

  EXPECT_TRUE(result1);
  EXPECT_FALSE(result2);
  EXPECT_TRUE(result3);

  // Only successful entities should be in the list
  EXPECT_EQ(manager->getEntityCount(), 2);
}
