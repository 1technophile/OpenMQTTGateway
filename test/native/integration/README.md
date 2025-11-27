# Integration Tests

This directory contains integration tests that verify component interactions and end-to-end functionality.

## Purpose

Integration tests verify:
- Module-to-module communication
- MQTT message flow and topic routing
- Configuration loading and management
- Gateway protocol integration
- Home Assistant discovery integration

## Test Categories

### Protocol Integration
- RF gateway message processing
- Bluetooth device discovery and data parsing
- IR signal transmission and reception
- LoRa communication handling

### MQTT Integration
- Message publishing to correct topics
- Command processing from MQTT
- Home Assistant auto-discovery payloads
- Configuration updates via MQTT

### Configuration Integration
- Config loading from NVS/SPIFFS
- JSON configuration parsing
- Runtime configuration updates
- Environment-based configuration

## Running Integration Tests

Integration tests may require:
- Mock MQTT broker setup
- Simulated hardware responses
- Configuration file fixtures

```bash
# Run integration tests specifically
pio test -e test_native --filter "*integration*"
```

## Guidelines

- Test realistic scenarios and workflows
- Use minimal mocking for true integration testing
- Include error handling and recovery scenarios
- Document test prerequisites and setup requirements
- Consider test execution time and resource usage