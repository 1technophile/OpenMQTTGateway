# OpenMQTTGateway - Claude Code Context

This document provides essential context about the OpenMQTTGateway project for AI assistants working on the codebase.

## Project Overview

OpenMQTTGateway is a unified firmware that bridges various wireless technologies (RF 433MHz, IR, BLE, LoRa, etc.) to MQTT protocol, enabling home automation integration with platforms like Home Assistant, OpenHAB, and Node-RED.

**Key Features:**
- Multi-protocol gateway (BLE, RF 433MHz, IR, LoRa, RTL_433, Pilight, etc.)
- ESP8266/ESP32 and Arduino support
- MQTT-based communication
- Web UI for configuration
- Home Assistant MQTT Discovery support
- Over 100 supported BLE sensors via Theengs Decoder

**Main Branch:** `development` (not master/main)

## Architecture

### Modular Design

The firmware uses a modular architecture where each protocol/sensor/actuator is implemented as a separate "gateway" or "sensor" module:

- **Gateways**: Receive and transmit signals (e.g., `gatewayBT.cpp`, `gatewayRF.cpp`, `gatewayIR.cpp`)
- **Sensors**: Read physical sensor data (e.g., `sensorBME280.cpp`, `sensorDHT.cpp`)
- **Actuators**: Control outputs (e.g., `actuatorONOFF.cpp`, `actuatorPWM.cpp`)

Each module has:
- A config file (e.g., `config_BT.h`, `config_RF.h`)
- An implementation file (e.g., `gatewayBT.cpp`, `gatewayRF.cpp`)

### Configuration System

Configuration is managed through:
1. **User_config.h** - Main configuration file with defaults
2. **config_*.h** - Module-specific configurations
3. **platformio.ini & environments.ini** - Build-time configurations
4. **prod_env.ini** - User-specific environments (gitignored)

Build flags in `environments.ini` can override User_config.h parameters.

## Directory Structure

```
OpenMQTTGateway/
├── main/                    # Main source code (Arduino convention)
│   ├── main.cpp            # Main application entry point
│   ├── User_config.h       # Global configuration
│   ├── config_*.h          # Module configurations
│   ├── gateway*.cpp        # Gateway implementations
│   ├── sensor*.cpp         # Sensor implementations
│   ├── actuator*.cpp       # Actuator implementations
│   ├── webUI.cpp           # Web interface
│   ├── mqttDiscovery.cpp   # Home Assistant discovery
│   └── rf/                 # RF-specific modules
├── lib/                     # Custom libraries
├── scripts/                 # Build and automation scripts
├── docs/                    # VuePress documentation
├── tests/                   # Test configurations
├── environments.ini         # PlatformIO environment definitions
├── platformio.ini          # PlatformIO configuration
└── prod_env.ini.example    # Example production environment

Note: src_dir is set to "main" in platformio.ini
```

## Key Concepts

### Environment-Based Builds

The project supports 80+ hardware configurations defined in `environments.ini`. Examples:
- `esp32dev-ble` - ESP32 with BLE gateway
- `esp32dev-pilight` - ESP32 with Pilight RF
- `esp32dev-rtl_433` - ESP32 with RTL_433 decoder
- `nodemcuv2-ir` - NodeMCU with IR
- `rfbridge` - Sonoff RF Bridge

### MQTT Topics

Standard topic structure:
- Base: `home/` (configurable via Base_Topic)
- Gateway name: `OpenMQTTGateway` or `OMG` (configurable)
- Example: `home/OpenMQTTGateway/BTtoMQTT`

### Module Activation

Modules are activated via build flags in environments.ini:
```ini
build_flags =
    '-DZgatewayBT="BT"'     # Enable BLE gateway
    '-DZgatewayRF="RF"'     # Enable RF gateway
    '-DZsensorBME280="BME280"' # Enable BME280 sensor
```

### Web UI & WiFi Manager

- ESP32/ESP8266 boards include a WiFiManager for initial setup
- Access point: SSID = `WifiManager_ssid`, Password = `WifiManager_password`
- Web UI provides runtime configuration and control
- Can disable WiFiManager with `ESPWifiManualSetup` flag

## Important Files

### Core Files

- **main/main.cpp** - Main application loop, MQTT handling, module coordination
- **main/User_config.h** - Global configuration defaults
- **main/webUI.cpp** - Web interface implementation
- **main/mqttDiscovery.cpp** - Home Assistant MQTT discovery

### Gateway Files

- **main/gatewayBT.cpp** - BLE gateway (uses Theengs Decoder)
- **main/gatewayRF.cpp** - RF 433MHz gateway (RCSwitch library)
- **main/gatewayRF2.cpp** - Alternative RF implementation
- **main/gatewayPilight.cpp** - Pilight protocol gateway
- **main/gatewayRTL_433.cpp** - RTL_433 decoder integration
- **main/gatewayIR.cpp** - Infrared gateway
- **main/gatewayLORA.cpp** - LoRa gateway

### Configuration Files

All in `main/` directory:
- **config_RF.h** - RF gateway configuration
- **config_BT.h** - BLE gateway configuration
- **config_IR.h** - IR gateway configuration
- etc.

### Build System

- **platformio.ini** - Main PlatformIO config
- **environments.ini** - All environment definitions (~3000+ lines)
- **scripts/** - Python scripts for build automation

## Common Patterns

### Adding a New Gateway Module

1. Create `gateway[NAME].cpp` in `main/`
2. Create `config_[NAME].h` in `main/`
3. Add build flag in `environments.ini`: `-DZgateway[NAME]="[NAME]"`
4. Include conditional compilation: `#ifdef Zgateway[NAME]`
5. Implement setup function: `setup[NAME]()`
6. Implement loop function: `[NAME]toMQTT()`
7. Optionally implement MQTT callback: `MQTTto[NAME]()`

### Adding a New Sensor Module

Similar to gateways, but prefix with `sensor` instead of `gateway` and `Z` becomes `Zsensor`.

### Configuration Override Pattern

```cpp
#ifndef PARAM_NAME
#  define PARAM_NAME default_value
#endif
```

This allows platformio.ini build flags to override defaults.

### MQTT Publishing Pattern

```cpp
pub(subjectToMQTT, data);  // Basic publish
pub_custom_topic(topic, data, retain);  // Custom topic with retain
```

## Development Workflow

### Building

```bash
# List all environments
pio run --list-targets

# Build specific environment
pio run -e esp32dev-ble

# Upload to device
pio run -e esp32dev-ble -t upload

# Monitor serial
pio device monitor
```

### Code Formatting

- Uses clang-format with config in `.clang-format`
- CI checks formatting automatically
- Format before committing

### Testing

- Unit tests in `tests/` directory
- CI runs builds for all environments
- Nightly builds test comprehensive environment matrix

### Documentation

- Documentation uses VuePress, located in `docs/`
- Hosted at https://docs.openmqttgateway.com

## CI/CD

### GitHub Actions Workflows

Located in `.github/workflows/`:
- **build.yml** - Build validation on PRs
- **nightly.yml** - Nightly builds of all environments
- **format-check.yml** - Code formatting validation

### Build Matrix

The nightly build tests configurations:
- ESP32 variants (ESP32, ESP32-S3, ESP32-C3)
- ESP8266 variants (NodeMCU, Sonoff)
- Different gateway combinations
- Board-specific builds (M5Stack, Heltec, TTGO, etc.)

## Common Issues & Solutions

### Memory Constraints

- ESP8266 has limited RAM (~40KB free)
- Use `PROGMEM` for constants
- Minimize module combinations on ESP8266
- ESP32 has more headroom (~200KB+ free)

### RF Module Conflicts

- Some RF implementations (RF, RF2, Pilight, RTL_433) may conflict
- Use separate environments for different RF approaches
- Check GPIO pin assignments in config files

### BLE on ESP32

- BLE stack uses significant RAM
- Limit simultaneous BLE connections

## Related Projects

- **Theengs Decoder** - BLE device decoder library
- **Theengs Gateway** - Python version for Raspberry Pi/PC
- **RTL_433_ESP** - ESP32 port of RTL_433

## Resources

- **Documentation**: https://docs.openmqttgateway.com
- **Community Forum**: https://community.openmqttgateway.com
- **GitHub**: https://github.com/1technophile/OpenMQTTGateway
- **Web Installer**: https://docs.openmqttgateway.com/upload/web-install.html

## Products

- **Theengs Bridge** - ESP32 BLE gateway with Ethernet
- **Theengs Plug** - Smart plug with BLE gateway

## Conventions

### Naming

- Gateways: `gateway[NAME].cpp` with `Zgateway[NAME]` flag
- Sensors: `sensor[NAME].cpp` with `Zsensor[NAME]` flag
- Actuators: `actuator[NAME].cpp` with `Zactuator[NAME]` flag
- Config files: `config_[NAME].h`

### Code Style

- 2-space indentation
- Follow existing patterns in similar modules
- Use preprocessor directives for module isolation
- Comment complex RF protocols or BLE parsing

### Git Workflow

- Main branch: `development`
- Feature branches from `development`
- PRs target `development`
- Squash commits when appropriate
- Descriptive commit messages

## Quick Start for Contributors

1. Fork the repository
2. Create `prod_env.ini` for your local environments
3. Choose an environment from `environments.ini`
4. Build: `pio run -e [environment-name]`
5. Make changes following existing patterns
6. Test build with your environment
7. Run format check
8. Submit PR to `development` branch

## Platform-Specific Notes

### ESP32
- Preferred platform for new development
- More RAM and Flash
- Supports BLE natively
- Can use Ethernet via add-on boards

### ESP8266
- Legacy support
- Memory constrained
- No native BLE
- Good for simple RF/IR gateways

## Tips for AI Assistants

1. **Check environments.ini** when modifying build configurations
2. **Follow existing module patterns** when adding new features
3. **Test on relevant environments** - changes may affect multiple boards
4. **Memory is precious** especially on ESP8266
5. **MQTT topic structure** is critical for Home Assistant integration
6. **Config headers** allow user customization without code changes
7. **Use conditional compilation** to keep builds modular
8. **WebUI changes** need both backend (webUI.cpp) and frontend (config_WebUI.h)
9. **RF protocols** often need precise timing - be careful with changes
10. **The main branch is `development`** - always target PRs there
