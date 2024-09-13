# LEDManager Library

## Overview

The LEDManager library provides a flexible and easy-to-use interface for controlling LEDs in projects. It supports both addressable LED strips and individual non-addressable LEDs, offering various modes of operation such as static, blinking, and pulsing. The library is designed to work seamlessly with PlatformIO and can be easily configured using build flags.

## Features

- Support for both addressable LED strips and individual non-addressable LEDs
- Multiple operation modes: OFF, STATIC, BLINK, and PULSE
- Individual control of LEDs within each strip (for addressable LEDs)
- Global brightness control
- Queueing of LED states for complex sequences
- Automatic return to previous state after temporary modes
- Compatible with PlatformIO build flags for easy configuration
- Customizable color schemes

## Installation

1. Clone this repository or download the ZIP file.
2. Extract the contents to your PlatformIO project's `lib` directory.

## Configuration

Configure the LEDManager library using build flags in your `platformio.ini` file:

```ini
build_flags = 
    ; For addressable LEDs:
    -DLED_ADDRESSABLE=true
    ; For non-addressable LEDs, comment out the line above
    
    ; Optional timing configurations:
    -DBLINK_INTERVAL=500
    -DPULSE_INTERVAL=30
    -DFADE_AMOUNT=5
```

## Usage

Here's a basic example of how to use the LEDManager library:

```cpp
#include "LEDManager.h"

LEDManager ledManager;

void setup() {
    
    #ifdef LED_ADDRESSABLE
    ledManager.addLEDStrip(5, 30);  // Add addressable LED strip on pin 5 with 30 LEDs
    #else
    ledManager.addLEDStrip(5, 1);   // Add non-addressable LED on pin 5
    ledManager.addLEDStrip(6, 1);   // Add non-addressable LED on pin 6
    #endif
    
    ledManager.setBrightness(128); // Set global brightness to 50%
}

void loop() {
    #ifdef LED_ADDRESSABLE
    // Set all LEDs on the strip to static green
    ledManager.setMode(0, -1, LEDManager::STATIC, LED_COLOR_GREEN);
    
    // Blink first 5 LEDs red for 3 times
    for (int i = 0; i < 5; i++) {
        ledManager.setMode(0, i, LEDManager::BLINK, LED_COLOR_RED, 3);
    }
    #else
    // Set first LED to static green
    ledManager.setMode(0, 0, LEDManager::STATIC, LED_COLOR_GREEN);
    
    // Blink second LED red
    ledManager.setMode(1, 0, LEDManager::BLINK, LED_COLOR_RED, 3);
    #endif

    ledManager.update();
    delay(100);
}
```

## API Reference

### Initialization and Setup

```cpp
LEDManager ledManager;
ledManager.addLEDStrip(int pin, int numLeds);
```

### Setting LED Mode

```cpp
ledManager.setMode(int stripIndex, int ledIndex, Mode mode, uint32_t color, int durationOrBlinkCount = -1);
```

- `stripIndex`: Index of the LED strip (0-based)
- `ledIndex`: LED index within the strip (0-based) or -1 for all LEDs in the strip (addressable LEDs only)
- `mode`: `LEDManager::OFF`, `LEDManager::STATIC`, `LEDManager::BLINK`, or `LEDManager::PULSE`
- `color`: Color value (use predefined colors from LEDColorDefinitions.h or custom 24-bit RGB values)
- `durationOrBlinkCount`: Duration for STATIC mode (in milliseconds), blink/pulse count for BLINK/PULSE modes (-1 for infinite)

### Setting Brightness

```cpp
ledManager.setBrightness(brightness);
```

- `brightness`: Global brightness value (0-255)

### Updating LEDs

Call this method in your main loop to update LED states:

```cpp
ledManager.update();
```
## Advanced Configuration

You can customize the behavior of the LEDManager by defining the following build flags:

- `BLINK_INTERVAL`: Sets the interval (in milliseconds) between blink states (default: 500)
- `PULSE_INTERVAL`: Sets the interval (in milliseconds) between pulse updates (default: 30)
- `FADE_AMOUNT`: Sets the increment/decrement value for pulsing (default: 5)

## Contributing

Contributions to the LEDManager library are welcome! Please feel free to submit a Pull Request.

## License

This library is released under the LGPL V3 License.