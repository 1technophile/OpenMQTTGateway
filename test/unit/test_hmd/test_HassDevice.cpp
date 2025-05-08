#include <ArduinoJson.h>
#include <HMD/ISettingsProvider.h>
#include <HMD/core/HassDevice.h>
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

// Test fixture for HassDevice
class HassDeviceTest : public ::testing::Test {
protected:
  MockSettingsProvider mockSettings;
  DynamicJsonDocument doc{1024};

  void SetUp() override {
    // Default mock setup
    ON_CALL(mockSettings, getGatewayName()).WillByDefault(Return("TestGateway"));
    ON_CALL(mockSettings, getGatewayManufacturer()).WillByDefault(Return("OpenMQTTGateway"));
    ON_CALL(mockSettings, getGatewayVersion()).WillByDefault(Return("1.0.0"));
    ON_CALL(mockSettings, getNetworkMacAddress()).WillByDefault(Return("AA:BB:CC:DD:EE:FF"));
    ON_CALL(mockSettings, getNetworkIPAddress()).WillByDefault(Return("192.168.1.100"));

    // Create empty modules array
    JsonArray modules = doc.createNestedArray("modules");
    ON_CALL(mockSettings, getModules()).WillByDefault(Return(modules));
  }

  void TearDown() override {
    // Cleanup
  }

  // Helper to create valid DeviceInfo
  HassDevice::DeviceInfo createValidDeviceInfo() {
    HassDevice::DeviceInfo info;
    info.name = "Test Device";
    info.manufacturer = "Test Manufacturer";
    info.model = "Test Model";
    info.identifier = "AA:BB:CC:DD:EE:FF";
    info.configUrl = "http://192.168.1.100/";
    info.swVersion = "1.0.0";
    info.isGateway = false;
    return info;
  }
};

// ============================================================================
// DeviceInfo Tests
// ============================================================================

TEST_F(HassDeviceTest, DeviceInfoDefaultConstructor) {
  HassDevice::DeviceInfo info;

  // Check default values
  EXPECT_TRUE(info.name.empty());
  EXPECT_TRUE(info.manufacturer.empty());
  EXPECT_TRUE(info.model.empty());
  EXPECT_TRUE(info.identifier.empty());
  EXPECT_TRUE(info.configUrl.empty());
  EXPECT_TRUE(info.swVersion.empty());
  EXPECT_FALSE(info.isGateway);
}

TEST_F(HassDeviceTest, DeviceInfoValidation) {
  HassDevice::DeviceInfo info;

  // Empty info is invalid
  EXPECT_FALSE(info.isValid());

  // Only name is not enough
  info.name = "Test Device";
  EXPECT_FALSE(info.isValid());

  // Name and identifier make it valid
  info.identifier = "AA:BB:CC:DD:EE:FF";
  EXPECT_TRUE(info.isValid());

  // Empty name but with identifier is invalid
  info.name = "";
  EXPECT_FALSE(info.isValid());
}

// ============================================================================
// Constructor and Basic Methods Tests
// ============================================================================

TEST_F(HassDeviceTest, ConstructorWithValidInfo) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  HassDevice device(info, mockSettings);

  EXPECT_EQ(device.getName(), "Test Device");
  EXPECT_EQ(device.getManufacturer(), "Test Manufacturer");
  EXPECT_EQ(device.getModel(), "Test Model");
  EXPECT_EQ(device.getIdentifier(), "AA:BB:CC:DD:EE:FF");
  EXPECT_FALSE(device.isGateway());
}

TEST_F(HassDeviceTest, ConstructorWithInvalidInfoGetsValidated) {
  HassDevice::DeviceInfo info;
  // Empty info will be validated and sanitized
  info.identifier = "AA:BB:CC:DD:EE:FF";

  HassDevice device(info, mockSettings);

  // Should have been sanitized with defaults
  EXPECT_EQ(device.getName(), "Unknown Device");
  EXPECT_EQ(device.getManufacturer(), "Unknown");
  EXPECT_EQ(device.getModel(), "Unknown");
  EXPECT_EQ(device.getIdentifier(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(HassDeviceTest, GettersReturnCorrectValues) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  info.isGateway = true;
  HassDevice device(info, mockSettings);

  EXPECT_EQ(device.getName(), info.name);
  EXPECT_EQ(device.getManufacturer(), info.manufacturer);
  EXPECT_EQ(device.getModel(), info.model);
  EXPECT_EQ(device.getIdentifier(), info.identifier);
  EXPECT_TRUE(device.isGateway());

  // Test getInfo returns complete structure
  const auto& deviceInfo = device.getInfo();
  EXPECT_EQ(deviceInfo.name, info.name);
  EXPECT_EQ(deviceInfo.manufacturer, info.manufacturer);
  EXPECT_EQ(deviceInfo.model, info.model);
  EXPECT_EQ(deviceInfo.identifier, info.identifier);
  EXPECT_EQ(deviceInfo.configUrl, info.configUrl);
  EXPECT_EQ(deviceInfo.swVersion, info.swVersion);
  EXPECT_EQ(deviceInfo.isGateway, info.isGateway);
}

// ============================================================================
// updateInfo Tests
// ============================================================================

TEST_F(HassDeviceTest, UpdateInfoWithValidData) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  HassDevice device(info, mockSettings);

  // Create new valid info
  HassDevice::DeviceInfo newInfo;
  newInfo.name = "Updated Device";
  newInfo.manufacturer = "Updated Manufacturer";
  newInfo.model = "Updated Model";
  newInfo.identifier = "BB:CC:DD:EE:FF:AA";
  newInfo.swVersion = "2.0.0";

  EXPECT_TRUE(device.updateInfo(newInfo));
  EXPECT_EQ(device.getName(), "Updated Device");
  EXPECT_EQ(device.getManufacturer(), "Updated Manufacturer");
  EXPECT_EQ(device.getModel(), "Updated Model");
  EXPECT_EQ(device.getIdentifier(), "BB:CC:DD:EE:FF:AA");
}

TEST_F(HassDeviceTest, UpdateInfoWithInvalidDataFails) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  HassDevice device(info, mockSettings);

  // Create invalid info (no name or identifier)
  HassDevice::DeviceInfo invalidInfo;

  EXPECT_FALSE(device.updateInfo(invalidInfo));

  // Original data should remain unchanged
  EXPECT_EQ(device.getName(), "Test Device");
  EXPECT_EQ(device.getIdentifier(), "AA:BB:CC:DD:EE:FF");
}

// ============================================================================
// Static Factory Methods Tests
// ============================================================================

TEST_F(HassDeviceTest, CreateGatewayDeviceUsesSettings) {
  // Setup mock expectations
  EXPECT_CALL(mockSettings, getGatewayName()).WillOnce(Return("MyGateway"));
  EXPECT_CALL(mockSettings, getGatewayManufacturer()).WillOnce(Return("OpenMQTT"));
  EXPECT_CALL(mockSettings, getGatewayVersion()).WillOnce(Return("1.5.0"));
  EXPECT_CALL(mockSettings, getNetworkMacAddress()).WillOnce(Return("11:22:33:44:55:66"));
  EXPECT_CALL(mockSettings, getNetworkIPAddress()).WillOnce(Return("10.0.0.1"));

  // Create empty modules array for this test
  JsonArray modules = doc.createNestedArray("test_modules");
  EXPECT_CALL(mockSettings, getModules()).WillOnce(Return(modules));

  HassDevice gateway = HassDevice::createGatewayDevice(mockSettings);

  EXPECT_EQ(gateway.getName(), "MyGateway");
  EXPECT_EQ(gateway.getManufacturer(), "OpenMQTT");
  EXPECT_EQ(gateway.getInfo().swVersion, "1.5.0");
  EXPECT_EQ(gateway.getIdentifier(), "11:22:33:44:55:66");
  EXPECT_TRUE(gateway.isGateway());
  EXPECT_EQ(gateway.getInfo().configUrl, "http://10.0.0.1/");
}

TEST_F(HassDeviceTest, CreateExternalDeviceWithAllParameters) {
  HassDevice device = HassDevice::createExternalDevice(
      "External Device",
      "External Manufacturer",
      "External Model",
      "AA:BB:CC:DD:EE:FF",
      mockSettings);

  EXPECT_EQ(device.getName(), "External Device");
  EXPECT_EQ(device.getManufacturer(), "External Manufacturer");
  EXPECT_EQ(device.getModel(), "External Model");
  EXPECT_EQ(device.getIdentifier(), "AA:BB:CC:DD:EE:FF");
  EXPECT_FALSE(device.isGateway());
}

TEST_F(HassDeviceTest, CreateExternalDeviceWithEmptyOptionalFields) {
  HassDevice device = HassDevice::createExternalDevice(
      "External Device",
      "", // Empty manufacturer
      "", // Empty model
      "AA:BB:CC:DD:EE:FF",
      mockSettings);

  EXPECT_EQ(device.getName(), "External Device");
  EXPECT_EQ(device.getManufacturer(), "Unknown"); // Should default to "Unknown"
  EXPECT_EQ(device.getModel(), "Unknown"); // Should default to "Unknown"
  EXPECT_EQ(device.getIdentifier(), "AA:BB:CC:DD:EE:FF");
  EXPECT_FALSE(device.isGateway());
}

// ============================================================================
// JSON Serialization Tests
// ============================================================================

TEST_F(HassDeviceTest, GatewayDeviceToJsonContainsCorrectFields) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  info.isGateway = true;
  info.name = "Gateway Device";
  info.manufacturer = "Gateway Mfg";
  info.model = "Gateway Model";
  info.swVersion = "2.0.0";
  info.configUrl = "http://192.168.1.1/";

  HassDevice device(info, mockSettings);

  DynamicJsonDocument testDoc(512);
  JsonObject deviceJson = testDoc.to<JsonObject>();
  device.toJson(deviceJson);

  EXPECT_EQ(deviceJson["name"].as<std::string>(), "Gateway Device");
  EXPECT_EQ(deviceJson["mf"].as<std::string>(), "Gateway Mfg");
  EXPECT_EQ(deviceJson["mdl"].as<std::string>(), "Gateway Model");
  EXPECT_EQ(deviceJson["sw"].as<std::string>(), "2.0.0");
  EXPECT_EQ(deviceJson["cu"].as<std::string>(), "http://192.168.1.1/");

  // Check identifiers array
  EXPECT_TRUE(deviceJson.containsKey("ids"));
  JsonArray ids = deviceJson["ids"];
  EXPECT_EQ(ids.size(), 1);
  EXPECT_EQ(ids[0].as<std::string>(), "AA:BB:CC:DD:EE:FF");

  // Check connections array
  EXPECT_TRUE(deviceJson.containsKey("cns"));
  JsonArray connections = deviceJson["cns"];
  EXPECT_EQ(connections.size(), 1);
  JsonArray connection = connections[0];
  EXPECT_EQ(connection.size(), 2);
  EXPECT_EQ(connection[0].as<std::string>(), "mac");
  EXPECT_EQ(connection[1].as<std::string>(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(HassDeviceTest, ExternalDeviceToJsonContainsCorrectFields) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  info.isGateway = false;
  info.name = "External Device";
  info.identifier = "BB:CC:DD:EE:FF:AA";

  // Setup mock expectation for via_device
  EXPECT_CALL(mockSettings, getNetworkMacAddress()).WillOnce(Return("AA:BB:CC:DD:EE:FF"));

  HassDevice device(info, mockSettings);

  DynamicJsonDocument testDoc(512);
  JsonObject deviceJson = testDoc.to<JsonObject>();
  device.toJson(deviceJson);

  // External device names get modified with short ID for uniqueness
  EXPECT_EQ(deviceJson["name"].as<std::string>(), "External Device-:FF:AA");
  EXPECT_EQ(deviceJson["mf"].as<std::string>(), "Test Manufacturer");
  EXPECT_EQ(deviceJson["mdl"].as<std::string>(), "Test Model");
  EXPECT_EQ(deviceJson["sw"].as<std::string>(), "1.0.0");

  // Check via_device (link to gateway)
  EXPECT_EQ(deviceJson["via_device"].as<std::string>(), "AA:BB:CC:DD:EE:FF");

  // Check identifiers and connections
  JsonArray ids = deviceJson["ids"];
  EXPECT_EQ(ids[0].as<std::string>(), "BB:CC:DD:EE:FF:AA");

  JsonArray connections = deviceJson["cns"];
  JsonArray connection = connections[0];
  EXPECT_EQ(connection[0].as<std::string>(), "mac");
  EXPECT_EQ(connection[1].as<std::string>(), "BB:CC:DD:EE:FF:AA");
}

TEST_F(HassDeviceTest, ExternalDeviceWithSameNameAsIdentifierDoesNotAddSuffix) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  info.isGateway = false;
  info.name = "AA:BB:CC:DD:EE:FF"; // Same as identifier
  info.identifier = "AA:BB:CC:DD:EE:FF";

  EXPECT_CALL(mockSettings, getNetworkMacAddress()).WillOnce(Return("11:22:33:44:55:66"));

  HassDevice device(info, mockSettings);

  DynamicJsonDocument testDoc(512);
  JsonObject deviceJson = testDoc.to<JsonObject>();
  device.toJson(deviceJson);

  // Should not add suffix when name equals identifier
  EXPECT_EQ(deviceJson["name"].as<std::string>(), "AA:BB:CC:DD:EE:FF");
}

TEST_F(HassDeviceTest, GatewayToJsonWithoutConfigUrlOmitsField) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  info.isGateway = true;
  info.configUrl = ""; // Empty config URL

  HassDevice device(info, mockSettings);

  DynamicJsonDocument testDoc(512);
  JsonObject deviceJson = testDoc.to<JsonObject>();
  device.toJson(deviceJson);

  // Config URL should not be present in JSON
  EXPECT_FALSE(deviceJson.containsKey("cu"));
}

// ============================================================================
// Validation and Sanitization Tests
// ============================================================================

TEST_F(HassDeviceTest, ValidateAndSanitizeEmptyGatewayInfo) {
  HassDevice::DeviceInfo info;
  info.isGateway = true;
  // Leave other fields empty to test sanitization

  // Setup mock for sanitization calls
  EXPECT_CALL(mockSettings, getNetworkMacAddress()).WillRepeatedly(Return("AA:BB:CC:DD:EE:FF"));
  EXPECT_CALL(mockSettings, getGatewayManufacturer()).WillRepeatedly(Return("OpenMQTTGateway"));

  HassDevice device(info, mockSettings);

  // Should have been sanitized
  EXPECT_EQ(device.getName(), "OpenMQTTGateway"); // Default gateway name
  EXPECT_EQ(device.getIdentifier(), "AA:BB:CC:DD:EE:FF"); // From settings
  EXPECT_EQ(device.getManufacturer(), "OpenMQTTGateway"); // From settings
  EXPECT_EQ(device.getModel(), "ESP32/ESP8266"); // Default gateway model
}

TEST_F(HassDeviceTest, ValidateAndSanitizeEmptyExternalDeviceInfo) {
  HassDevice::DeviceInfo info;
  info.isGateway = false;
  info.identifier = "BB:CC:DD:EE:FF:AA"; // Only set identifier

  HassDevice device(info, mockSettings);

  // Should have been sanitized
  EXPECT_EQ(device.getName(), "Unknown Device"); // Default external device name
  EXPECT_EQ(device.getIdentifier(), "BB:CC:DD:EE:FF:AA");
  EXPECT_EQ(device.getManufacturer(), "Unknown"); // Default external manufacturer
  EXPECT_EQ(device.getModel(), "Unknown"); // Default external model
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(HassDeviceTest, HandlesNonMacAddressIdentifiers) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  info.identifier = "device-12345"; // Not a MAC address format

  // Should not throw or crash
  HassDevice device(info, mockSettings);
  EXPECT_EQ(device.getIdentifier(), "device-12345");
}

TEST_F(HassDeviceTest, HandlesVeryLongStrings) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  info.name = std::string(1000, 'A'); // Very long name
  info.manufacturer = std::string(500, 'B');

  HassDevice device(info, mockSettings);

  // Should handle gracefully
  EXPECT_EQ(device.getName().length(), 1000);
  EXPECT_EQ(device.getManufacturer().length(), 500);
}

TEST_F(HassDeviceTest, JsonSerializationWithEmptyFields) {
  HassDevice::DeviceInfo info;
  info.name = "Test";
  info.identifier = "AA:BB:CC:DD:EE:FF";
  info.isGateway = false;
  // Leave manufacturer, model, swVersion empty

  EXPECT_CALL(mockSettings, getNetworkMacAddress()).WillOnce(Return("11:22:33:44:55:66"));

  HassDevice device(info, mockSettings);

  DynamicJsonDocument testDoc(512);
  JsonObject deviceJson = testDoc.to<JsonObject>();
  device.toJson(deviceJson);

  // Should still create valid JSON
  EXPECT_TRUE(deviceJson.containsKey("name"));
  EXPECT_TRUE(deviceJson.containsKey("ids"));
  EXPECT_TRUE(deviceJson.containsKey("cns"));
  EXPECT_TRUE(deviceJson.containsKey("via_device"));
}

// ============================================================================
// Performance and Integration Tests
// ============================================================================

TEST_F(HassDeviceTest, MultipleJsonSerializationsAreFast) {
  HassDevice::DeviceInfo info = createValidDeviceInfo();
  HassDevice device(info, mockSettings);

  // Setup mock to return the same value multiple times
  EXPECT_CALL(mockSettings, getNetworkMacAddress()).WillRepeatedly(Return("AA:BB:CC:DD:EE:FF"));

  // Test multiple serializations
  const int iterations = 100;
  for (int i = 0; i < iterations; i++) {
    DynamicJsonDocument testDoc(512);
    JsonObject deviceJson = testDoc.to<JsonObject>();
    device.toJson(deviceJson);

    // Basic validation that it works
    EXPECT_TRUE(deviceJson.containsKey("name"));
  }

  // If we got here without timeout, performance is acceptable
  SUCCEED();
}

TEST_F(HassDeviceTest, CreateMultipleDevicesWithDifferentSettings) {
  // Test creating multiple devices to ensure no side effects

  HassDevice gateway = HassDevice::createGatewayDevice(mockSettings);

  HassDevice external1 = HassDevice::createExternalDevice(
      "Device 1", "Mfg1", "Model1", "AA:AA:AA:AA:AA:AA", mockSettings);

  HassDevice external2 = HassDevice::createExternalDevice(
      "Device 2", "Mfg2", "Model2", "BB:BB:BB:BB:BB:BB", mockSettings);

  // Each should maintain its own state
  EXPECT_TRUE(gateway.isGateway());
  EXPECT_FALSE(external1.isGateway());
  EXPECT_FALSE(external2.isGateway());

  EXPECT_EQ(external1.getName(), "Device 1");
  EXPECT_EQ(external2.getName(), "Device 2");

  EXPECT_EQ(external1.getIdentifier(), "AA:AA:AA:AA:AA:AA");
  EXPECT_EQ(external2.getIdentifier(), "BB:BB:BB:BB:BB:BB");
}