/*  
    Theengs LEDManager - We Unite Sensors in One Open-Source Interface
  
    Copyright: (c)Florian ROBERT

    This file is part of LEDManager library.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>

#include <vector>

#ifdef LED_ADDRESSABLE
#  include <Adafruit_NeoPixel.h>
#endif

#ifndef BLINK_INTERVAL
#  define BLINK_INTERVAL 500
#endif

#ifndef PULSE_INTERVAL
#  define PULSE_INTERVAL 30
#endif

#ifndef FADE_AMOUNT
#  define FADE_AMOUNT 5
#endif

// Define the pins for the RGB LED power if needed
#ifndef LED_ADDRESSABLE_POWER
#  define LED_ADDRESSABLE_POWER -1
#endif

class LEDManager {
public:
  enum Mode { OFF,
              STATIC,
              BLINK,
              PULSE };

  LEDManager();
  void init();
  void addLEDStrip(int pin, int numLeds);
  void setMode(int stripIndex, int ledIndex, Mode mode, uint32_t color, int durationOrBlinkCount = -1);
  void update();
  void setBrightness(uint8_t brightness);

private:
  struct QueuedState {
    Mode mode;
    uint32_t color;
    int durationOrBlinkCount;
    bool isQueued;
  };

  struct LEDState {
    Mode mode;
    Mode previousMode;
    uint32_t color;
    uint32_t previousColor;
    int durationOrBlinkCount;
    unsigned long lastUpdateTime;
    int blinkCounter;
    bool isBlinkOn;
    int brightness;
    int fadeAmount;
    QueuedState queuedState;
  };

  struct LEDStrip {
#ifdef LED_ADDRESSABLE
    Adafruit_NeoPixel* strip;
#endif
    int pin;
    std::vector<LEDState> ledStates;
  };

  std::vector<LEDStrip> ledStrips;
  int globalBrightness;

  void setModeForSingleLED(int stripIndex, int ledIndex, Mode mode, uint32_t color, int durationOrBlinkCount);
  void setLEDColor(int stripIndex, int ledIndex, uint32_t color);
  void handleBlink(int stripIndex, int ledIndex);
  void handlePulse(int stripIndex, int ledIndex);
  void handleStatic(int stripIndex, int ledIndex);
  void returnToPreviousState(int stripIndex, int ledIndex);
  void applyQueuedState(int stripIndex, int ledIndex);
  void extractRGB(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b);
};

#endif // LED_MANAGER_H