# Test Helpers

This directory contains utility functions and helper classes for testing OpenMQTTGateway modules.

## Purpose

- Common test utilities and fixtures
- Mock initialization helpers
- Test data generators
- Assertion helpers for ESP32/Arduino specific testing

## Usage

Include helper files in your test files:

```cpp
#include "../helpers/test_helpers.h"
#include "../helpers/mock_helpers.h"
```

## File Structure

- `test_helpers.h` - Generic test utilities
- `mock_helpers.h` - Mock object initialization helpers
- `esp32_test_helpers.h` - ESP32-specific test utilities
- `json_helpers.h` - JSON testing utilities for MQTT messages

## Best Practices

Follow PlatformIO test best practices and maintain helper functions that are:
- Reusable across multiple test suites
- Well-documented with clear interfaces
- Independent of specific test implementations