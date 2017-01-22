/*
  FanRemoteGPIO - Arduino libary to control a fan remote connected via GPIO pins.
  Copyright (c) 2017 Ricky Brent

  Contributors:
  - Ricky Brent

  Project home: https://github.com/rickybrent/fan-remote-gpio/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ****************************************************/

#include "FanRemoteGPIO.h"

FanRemoteGPIO::FanRemoteGPIO() {
  this->lightPin = -1;
  this->lightPinOff = -1;
  this->fanPinFast = -1;
  this->fanPinMedium = -1;
  this->fanPinSlow = -1;
  this->fanPinOff = -1;
  this->nFanSpeed = 0;
  this->nBrightness = 0;
  this->nMaxBrightness = 3;
  this->bLightOn = false;
  this->bFanOn = false;
  this->looping = false;
  this->heldPin = -1;
  this->heldUntil = 0;
  this->queue = Queue<QueuedPress>(20);
}

/**
 * Enable a single light button and assign the pin.
 *
 * @param pin  Arduino Pin to which the ligh button is connected to
 */
void FanRemoteGPIO::enableLight(int pin) {
  this->lightPin = pin;
  this->lightPinOff = -1;
  initPin(this->lightPin);
}

/**
 * Enable controlling the light, with separate pins for on and off.
 *
 * @param pinOn   Arduino Pin to which the light on button is connected to
 * @param pinOff  Arduino Pin to which the light off button is connected to
 */
void FanRemoteGPIO::enableLight(int pinOn, int pinOff) {
  this->lightPin = pinOn;
  this->lightPinOff = pinOff;
  initPin(this->lightPin);
  initPin(this->lightPinOff);
  // If we have an on and off button, we can guarentee the proper sync state:
  this->turnLight(false);
}

/**
  * Disable controlling the light
  */
void FanRemoteGPIO::disableLight() {
  this->lightPin = -1;
  this->lightPinOff = -1;
}

/**
 * Enable controlling the fan speeds by separate pins per speed.
 *
 * @param pinFast    Arduino Pin connected to the fast button
 * @param pinMedium  Arduino Pin connected to the medium button
 * @param pinSlow    Arduino Pin connected to the slow button
 * @param pinOff     Arduino Pin connected to the off button
  */
void FanRemoteGPIO::enableFanSpeeds(int pinFast, int pinMedium, int pinSlow, int pinOff) {
  this->fanPinFast = pinFast;
  this->fanPinMedium = pinMedium;
  this->fanPinSlow = pinSlow;
  this->fanPinOff = pinOff;
  initPin(this->fanPinFast);
  initPin(this->fanPinMedium);
  initPin(this->fanPinSlow);
  initPin(this->fanPinOff);
  this->turnFan(false); // Easiest way to keep the real fan speed identical.
}

/**
  * Disable controlling the fan speeds
  */
void FanRemoteGPIO::disableFanSpeeds() {
  this->fanPinFast = -1;
  this->fanPinMedium = -1;
  this->fanPinSlow = -1;
  this->fanPinOff = -1;
}


/**
 * Set the fan light brightness.
 * Ideally, this should track the brightness level, then adjust it by timing
 * how long the button is held down for. This is hard to get right.
 */
void FanRemoteGPIO::setBrightness(int level) {
    if (level == 0) {
        this->bLightOn = false;
        if (this->lightPinOff < 0) {
            press(this->lightPin);
        } else {
            press(this->lightPinOff);
        }
    } else if (level > 0) {
        this->bLightOn = true;
        // Visible changes at: 1600, 1800, 2000, 2200, 2400. Difficult to time
        // correctly matched with a level, and I rarely want the lights dimmed.
        this->nBrightness = level;
        press(this->lightPin);
    }
}


/**
 * Power the light back on without attempting to change its brightness level.
 */
void FanRemoteGPIO::turnLight(bool on) {
    if (!on) {
        setBrightness(0);
    } else {
        this->nBrightness = this->nMaxBrightness;
        this->bLightOn = true;
        press(this->lightPin, 200);
    }
}

/**
 * Set the fan light brightness.
 */
void FanRemoteGPIO::setSpeed(int speed) {
    if (speed == 0) {
        this->bFanOn = false;
        press(this->fanPinOff);
    } else {
        this->bFanOn = true;
        this->nFanSpeed = speed;
        if (speed == 1) {
            press(this->fanPinSlow);
        } else if (speed == 2) {
            press(this->fanPinMedium);
        } else if (speed == 3) {
            press(this->fanPinFast);
        }
    }
}

/**
 * Power the fan back on to its last set speed.
 */
void FanRemoteGPIO::turnFan(bool on) {
    if (!on) {
        setSpeed(0);
    } else {
        setSpeed(this->nFanSpeed > 0 ? this->nFanSpeed : 3);
    }
}

/**
 * Release any buttons being held down long enough.
 */
bool FanRemoteGPIO::available() {
    this->looping = true;
    if (this->heldPin >= 0 && this->heldUntil != 0 && this->heldUntil <= millis()) {
        digitalWrite(this->heldPin, HIGH);
        this->heldPin = -1;
        if ((queue.count()) > 0) {
            QueuedPress queued = queue.pop();
            press(queued.pin, queued.time);
        }
        return true;
    }
    return false;
}


int FanRemoteGPIO::getBrightness() {
    return ((this->bLightOn) ? this->nBrightness : 0);
}

int FanRemoteGPIO::getSpeed() {
    return ((this->bFanOn) ? this->nFanSpeed : 0);
}

bool FanRemoteGPIO::isLightOn() {
    return this->bLightOn;
}

bool FanRemoteGPIO::isFanOn() {
    return this->bFanOn;
}

/**
 * Set pin to output and HIGH.
 */
void FanRemoteGPIO::initPin(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

/**
 * Hold down the light button for the given ms. Useful for testing.
 */
void FanRemoteGPIO::holdLightButton(int time) {
    press(this->lightPin, time);
}


/**
 * Press a button.
 */
void FanRemoteGPIO::press(int pin) {
    press(pin, 200); // Too long, and the light will begin adjusting brightness.
}

/**
 * Press a button, hold for the specified time, then release.
 * @param pin   Pin to press.
 * @param time  Delay before releasing, in milliseconds.
 */
void FanRemoteGPIO::press(int pin, int time) {
    if (pin < 0)
        return;
    if  (this->heldPin > -1) {
        struct QueuedPress queued;
        queued.pin = pin;
        queued.time = time;
        this->queue.push(queued);
        return;
    }
    digitalWrite(pin, LOW);
    if (this->looping) {
        this->heldPin = pin;
        this->heldUntil = millis() + time;
    } else {
        if (time > 0) delay(time);
        digitalWrite(pin, HIGH);
    }
}
