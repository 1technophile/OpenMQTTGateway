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

#include "LEDManager.h"

LEDManager::LEDManager() : globalBrightness(255) {}

void LEDManager::addLEDStrip(int pin, int numLeds) {
  LEDStrip ledStrip;
  ledStrip.pin = pin;
#ifdef LED_ADDRESSABLE
  ledStrip.strip = new Adafruit_NeoPixel(numLeds, pin, NEO_GRB + NEO_KHZ800);
  ledStrip.strip->begin();
  ledStrip.strip->show();
  ledStrip.strip->setBrightness(globalBrightness);
#else
  pinMode(pin, OUTPUT);
#endif

  ledStrip.ledStates.resize(numLeds, {OFF, OFF, 0, 0, 0, 0, 0, false, 255, FADE_AMOUNT, {OFF, 0, -1, false}});

  ledStrips.push_back(ledStrip);
}

void LEDManager::setMode(int stripIndex, int ledIndex, Mode mode, uint32_t color, int durationOrBlinkCount) {
  if (stripIndex >= 0 && stripIndex < ledStrips.size()) {
    if (ledIndex == -1) {
      for (int i = 0; i < ledStrips[stripIndex].ledStates.size(); i++) {
        setModeForSingleLED(stripIndex, i, mode, color, durationOrBlinkCount);
      }
    } else if (ledIndex >= 0 && ledIndex < ledStrips[stripIndex].ledStates.size()) {
      setModeForSingleLED(stripIndex, ledIndex, mode, color, durationOrBlinkCount);
    }
  }
}

void LEDManager::setModeForSingleLED(int stripIndex, int ledIndex, Mode mode, uint32_t color, int durationOrBlinkCount) {
  auto& state = ledStrips[stripIndex].ledStates[ledIndex];

  if ((state.mode == BLINK || state.mode == PULSE) && state.durationOrBlinkCount > 0) {
    state.queuedState = {mode, color, durationOrBlinkCount, true};
    return;
  }

  if (mode != BLINK && mode != PULSE) {
    state.previousMode = state.mode;
    state.previousColor = state.color;
  }
  state.mode = mode;
  state.color = color;
  state.durationOrBlinkCount = durationOrBlinkCount;
  state.lastUpdateTime = millis();
  state.blinkCounter = 0;
  state.isBlinkOn = false;
  state.brightness = globalBrightness;
  state.fadeAmount = FADE_AMOUNT;
  state.queuedState.isQueued = false;
}

void LEDManager::setBrightness(uint8_t brightness) {
#ifdef LED_ADDRESSABLE
  globalBrightness = brightness;
  for (auto& ledStrip : ledStrips) {
    ledStrip.strip->setBrightness(brightness);
    for (auto& state : ledStrip.ledStates) {
      state.brightness = brightness;
    }
  }
#endif
}

void LEDManager::setLEDColor(int stripIndex, int ledIndex, uint32_t color) {
  if (stripIndex < 0 || stripIndex >= ledStrips.size()) return;
  if (ledIndex < 0 || ledIndex >= ledStrips[stripIndex].ledStates.size()) return;

  uint8_t r, g, b;
  extractRGB(color, r, g, b);

  r = (r * globalBrightness) / 255;
  g = (g * globalBrightness) / 255;
  b = (b * globalBrightness) / 255;

#ifdef LED_ADDRESSABLE
  ledStrips[stripIndex].strip->setPixelColor(ledIndex, r, g, b);
  ledStrips[stripIndex].strip->show();
#else
  // For non-addressable LEDs, we'll use the first LED of each strip
  if (ledIndex == 0) {
    int pin = ledStrips[stripIndex].pin;
    analogWrite(pin, (r + g + b) / 3); // Average of RGB for single-color LED
  }
#endif
}

void LEDManager::update() {
  for (int stripIndex = 0; stripIndex < ledStrips.size(); stripIndex++) {
    for (int ledIndex = 0; ledIndex < ledStrips[stripIndex].ledStates.size(); ledIndex++) {
      auto& state = ledStrips[stripIndex].ledStates[ledIndex];
      switch (state.mode) {
        case BLINK:
          handleBlink(stripIndex, ledIndex);
          break;
        case PULSE:
          handlePulse(stripIndex, ledIndex);
          break;
        case STATIC:
          handleStatic(stripIndex, ledIndex);
          break;
        case OFF:
          setLEDColor(stripIndex, ledIndex, 0);
          break;
      }
    }
#ifdef LED_ADDRESSABLE
    ledStrips[stripIndex].strip->show();
#endif
  }
}

void LEDManager::handleBlink(int stripIndex, int ledIndex) {
  auto& state = ledStrips[stripIndex].ledStates[ledIndex];
  unsigned long currentTime = millis();
  if (currentTime - state.lastUpdateTime >= BLINK_INTERVAL) {
    state.lastUpdateTime = currentTime;
    if (state.durationOrBlinkCount > 0) {
      if (state.blinkCounter == 0) {
        state.blinkCounter = state.durationOrBlinkCount * 2;
      } else {
        state.blinkCounter--;
        if (state.blinkCounter == 0) {
          if (state.queuedState.isQueued) {
            applyQueuedState(stripIndex, ledIndex);
          } else {
            returnToPreviousState(stripIndex, ledIndex);
          }
          return;
        }
      }
    }

    state.isBlinkOn = !state.isBlinkOn;
    if (state.isBlinkOn) {
      setLEDColor(stripIndex, ledIndex, state.color);
    } else {
      setLEDColor(stripIndex, ledIndex, 0);
    }
  }
}

void LEDManager::handlePulse(int stripIndex, int ledIndex) {
  auto& state = ledStrips[stripIndex].ledStates[ledIndex];
  unsigned long currentTime = millis();
  if (currentTime - state.lastUpdateTime >= PULSE_INTERVAL) {
    state.lastUpdateTime = currentTime;
    state.brightness += state.fadeAmount;
    if (state.brightness <= 0 || state.brightness >= 255) {
      state.fadeAmount = -state.fadeAmount;
      state.brightness = constrain(state.brightness, 0, 255);
      if (state.durationOrBlinkCount > 0) {
        state.durationOrBlinkCount--;
        if (state.durationOrBlinkCount == 0) {
          if (state.queuedState.isQueued) {
            applyQueuedState(stripIndex, ledIndex);
          } else {
            returnToPreviousState(stripIndex, ledIndex);
          }
          return;
        }
      }
    }
    uint32_t adjustedColor = (state.color & 0xFF000000) |
                             (((state.color & 0x00FF0000) * state.brightness / 255) & 0x00FF0000) |
                             (((state.color & 0x0000FF00) * state.brightness / 255) & 0x0000FF00) |
                             (((state.color & 0x000000FF) * state.brightness / 255) & 0x000000FF);
    setLEDColor(stripIndex, ledIndex, adjustedColor);
  }
}

void LEDManager::handleStatic(int stripIndex, int ledIndex) {
  auto& state = ledStrips[stripIndex].ledStates[ledIndex];
  unsigned long currentTime = millis();
  if (state.durationOrBlinkCount > 0) {
    if (currentTime - state.lastUpdateTime >= static_cast<unsigned long>(state.durationOrBlinkCount) * 1000) {
      if (state.queuedState.isQueued) {
        applyQueuedState(stripIndex, ledIndex);
      } else {
        returnToPreviousState(stripIndex, ledIndex);
      }
      return;
    }
  }
  setLEDColor(stripIndex, ledIndex, state.color);
}

void LEDManager::returnToPreviousState(int stripIndex, int ledIndex) {
  auto& state = ledStrips[stripIndex].ledStates[ledIndex];
  state.mode = state.previousMode;
  state.color = state.previousColor;
  state.durationOrBlinkCount = -1; // Set to infinite for the previous state
  state.lastUpdateTime = millis();
  state.blinkCounter = 0;
  state.isBlinkOn = false;
  state.brightness = globalBrightness;
  state.fadeAmount = FADE_AMOUNT;
}

void LEDManager::applyQueuedState(int stripIndex, int ledIndex) {
  auto& state = ledStrips[stripIndex].ledStates[ledIndex];
  state.mode = state.queuedState.mode;
  state.color = state.queuedState.color;
  state.durationOrBlinkCount = state.queuedState.durationOrBlinkCount;
  state.lastUpdateTime = millis();
  state.blinkCounter = 0;
  state.isBlinkOn = false;
  state.brightness = globalBrightness;
  state.fadeAmount = FADE_AMOUNT;
  state.queuedState.isQueued = false;
}

void LEDManager::extractRGB(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b) {
  r = (color >> 16) & 0xFF;
  g = (color >> 8) & 0xFF;
  b = color & 0xFF;
}