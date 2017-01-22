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

#ifndef FanRemoteGPIO_h
#define FanRemoteGPIO_h

#include <stdint.h>
#include "Queue.h"
#include "Arduino.h"

class FanRemoteGPIO
{
  public:
    FanRemoteGPIO();
    void enableLight(int pin);
    void enableLight(int pinOn, int pinOff);
    void enableFanSpeeds(int pinFast, int pinMedium, int pinSlow, int pinOff);
    void disableLight();
    void disableFanSpeeds();
    void turnLight(bool on);
    void setBrightness(int brightness);
    void turnFan(bool on);
    void setSpeed(int speed);
    void holdLightButton(int time);
    int getBrightness();
    int getSpeed();
    bool isLightOn();
    bool isFanOn();
    bool available();
  private:
    void initPin(int pin);
    void press(int pin);
    void press(int pin, int time);
    int lightPin;
    int lightPinOff;
    int fanPinFast;
    int fanPinMedium;
    int fanPinSlow;
    int fanPinOff;
    int heldPin;
    unsigned long heldUntil;
    bool looping;
    bool bLightOn;
    bool bFanOn;
    uint8_t nBrightness;
    uint8_t nMaxBrightness;
    uint8_t nFanSpeed;
    struct QueuedPress {
        int pin;
        int time;
    };
    Queue<QueuedPress> queue;
};

#endif
