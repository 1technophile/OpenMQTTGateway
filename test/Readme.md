# Testing OpenMQTTGateway code

OpenMQTTGateway uses a robust testing setup designed to ensure reliability and maintainability. The testing framework is built on PlatformIO and GoogleTest, providing a structured approach to validate both native and embedded code. This guide explains how to run tests, organize them effectively, and follow best practices for high-quality development.

## Test Directory Structure

```
test/
├── test_runner.cpp          # Main test runner with GoogleTest
├── native/                 # Native tests (run on development machine)
│   ├── unit/              # Isolated unit tests
│   └── integration/       # Integration tests
└── embedded/               # Embedded tests (run on ESP32 hardware)
    └── test_embedded.cpp  # Hardware-in-the-loop tests
```

## Running Tests

### Native Tests (Fast, No Hardware Required)
```bash
# Run all native tests
pio test -e test_native

# Run with verbose output
pio test -e test_native -v

# Run specific test by filter
pio test -e test_native --filter "native/unit/test_rf"
```

### Embedded Tests (Requires ESP32 Hardware)
```bash
# Run embedded tests on any existing ESP32 configuration
# Example: using esp32dev-all-test environment
pio test -e esp32dev-all-test

# Or use any other ESP32 environment (esp32dev-ble, esp32dev-rf, etc.)
pio test -e esp32dev-ble

# Run with verbose output
pio test -e esp32dev-all-test -v

# Upload to specific port
pio test -e esp32dev-all-test --upload-port COM3  # Windows
pio test -e esp32dev-all-test --upload-port /dev/ttyUSB0  # Linux/Mac
```

### Run All Tests
```bash
# Run both native and embedded tests
pio test
```

## Test Environments

### Native Tests: `[env:test_native]`
- **Platform**: Native (runs on development machine)
- **Framework**: GoogleTest
- **Test Filter**: `native`
- **Purpose**: Fast unit testing with mocked dependencies
- **Advantages**: 
  - No hardware required
  - Fast execution (< 1 second)
  - CI/CD friendly
  - Full code coverage support

### Embedded Tests: Reuse Existing ESP32 Configurations
- **Available Environments**: 22+ existing ESP32 configurations
  - `esp32dev-all-test` - Full feature set
  - `esp32dev-ble` - BLE focused
  - `esp32dev-rf` - RF gateway
  - `esp32dev-ir` - Infrared
  - And many more in `environments.ini`
- **Platform**: ESP32 (espressif32)
- **Framework**: Arduino + GoogleTest (when test framework enabled)
- **Purpose**: Hardware-in-the-loop testing with real peripherals
- **Advantages**:
  - Reuses existing hardware configurations
  - Tests real hardware behavior
  - Validates ESP32-specific features (NVS, WiFi, BLE, etc.)
  - No duplicate environment definitions
  - Tests match actual deployment configurations


## Visual Code Integration
Visual Studio Code integrates testing seamlessly with its PlatformIO extension. However, due to the embedded nature of the project, only native tests can be executed directly within the IDE. For a complete testing experience, including embedded tests, it is recommended to use the CLI for better control and flexibility.

![Test_Integration](../img/test_integration.gif)



## Continuous Integration

Automated testing runs on:
- **Push**: To `main`, `development`, and `feature/*` branches
- **Pull Requests**: To `main` and `development` branches

The GitHub Actions workflow:
1. Sets up Ubuntu environment with Python 3.11
2. Installs PlatformIO
3. Runs native tests with `pio test -e test_native`
4. Reports test results

::: tip
Embedded tests require physical ESP32 hardware and are not run in CI/CD. Execute them manually before releases or when validating hardware-specific functionality.
:::

## Writing Tests

### Naming Conventions

- Test files: `test_[ComponentName].cpp`
- Test suites: `[ComponentName]Test`
- Test cases: descriptive names using GoogleTest conventions

### Native Test Example (Unit Test)

Located in `test/native/unit/test_rf/test_RFConfiguration.cpp`:

```cpp
#include <gtest/gtest.h>
#include <rf/RFConfiguration.h>
#include "../../mocks/mock_IStorage.h"
#include "../../mocks/mock_RFReceiver.h"

class RFConfigurationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test fixtures
    }
};

TEST_F(RFConfigurationTest, ShouldInitializeWithDefaults) {
    // Arrange
    MockRFReceiver mockReceiver;
    RFConfiguration config(mockReceiver);
    
    // Assert
    ASSERT_NEAR(config.getFrequency(), RF_FREQUENCY, 0.01);
    ASSERT_EQ(config.getActiveReceiver(), ACTIVE_RECEIVER);
}
```

### Embedded Test Example (Hardware Test)

Located in `test/embedded/test_embedded.cpp`:

```cpp
#include <Arduino.h>
#include <Preferences.h>
#include <gtest/gtest.h>

TEST(EmbeddedESP32, NVSBasicOperations) {
    Preferences prefs;
    
    // Test with real ESP32 NVS
    ASSERT_TRUE(prefs.begin("test", false));
    ASSERT_GT(prefs.putString("key1", "value1"), 0);
    ASSERT_STREQ(prefs.getString("key1", "").c_str(), "value1");
    
    prefs.clear();
    prefs.end();
}
```

::: tip
Embedded tests run on **any ESP32 environment** defined in `environments.ini`. Simply use `pio test -e <esp32_env_name>` to execute them on your target hardware configuration.
:::

## Test Organization Best Practices

### Native Tests (`test/native/`)
- **Purpose**: Fast feedback, logic validation, cross-platform
- **Dependencies**: Mocked (no hardware required)
- **Execution**: Runs on development machine
- **Use Cases**:
  - Algorithm validation
  - Configuration parsing
  - Data structure operations
  - Business logic testing

### Embedded Tests (`test/embedded/`)
- **Purpose**: Hardware validation, ESP32-specific features
- **Dependencies**: Real hardware (ESP32 required)
- **Execution**: Runs on physical ESP32 board using existing environments
- **Configuration**: Leverages 22+ existing ESP32 environments from `environments.ini`
- **Use Cases**:
  - NVS storage operations
  - WiFi/Bluetooth connectivity
  - GPIO/peripheral interactions
  - Real-time performance validation
  - Validating specific hardware configurations (BLE, RF, IR, etc.)

## Best Practices

1. **Test Naming**: Use descriptive names that explain behavior
2. **Arrange-Act-Assert**: Structure tests clearly 
3. **Mock External Dependencies**: Isolate units under test
4. **Test Both Success and Failure**: Include edge cases
5. **Use Test Helpers**: Reduce duplication with common utilities
6. **Follow PlatformIO Guidelines**: Align with framework conventions

## References

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/)
- [GoogleTest Documentation](https://google.github.io/googletest/)
- [OpenMQTTGateway Development Guide](development.md)
- [Test Best Practices](https://docs.platformio.org/en/stable/advanced/unit-testing/structure/best-practices.html)
