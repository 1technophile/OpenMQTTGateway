/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
    For esp32, touch sensor reading add-on.
  
    Copyright: (c) Florian Xhumari
    
    This file is part of OpenMQTTGateway.
    
    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <HardwareSerial.h>

#include "ArduinoJson.h"
#include "User_config.h"
#include "config_Touch.h"

#ifdef ZsensorTouch

#  ifndef ESP32
#    error Only supported on esp32
#  endif

//Time used to wait for an interval before resending touch sensor value
static unsigned long touchTimeRead = 0;

// for each touch button
struct TouchStatus {
  uint8_t pin;
  uint16_t timePublished = 0;
  uint16_t timeStatusChanged = 0;
  uint16_t avg = 10 * 256; // starting value; multiplied by 256
  bool isOn = false;
  bool wasOn = false;
  int onCount = 0; // number of consecutive readings that were considered "ON"
  unsigned long onTime; // time the button was turned on
};

static TouchStatus status[TOUCH_SENSORS];

void setupTouch() {
#  if TOUCH_SENSORS > 0
  status[0].pin = TOUCH_GPIO_0;
#  endif
#  if TOUCH_SENSORS > 1
  status[1].pin = TOUCH_GPIO_1;
#  endif
#  if TOUCH_SENSORS > 2
  status[2].pin = TOUCH_GPIO_2;
#  endif
#  if TOUCH_SENSORS > 3
  status[3].pin = TOUCH_GPIO_3;
#  endif
#  if TOUCH_SENSORS > 4
  status[4].pin = TOUCH_GPIO_4;
#  endif
#  if TOUCH_SENSORS > 5
  status[5].pin = TOUCH_GPIO_5;
#  endif
#  if TOUCH_SENSORS > 6
  status[6].pin = TOUCH_GPIO_6;
#  endif
#  if TOUCH_SENSORS > 7
  status[7].pin = TOUCH_GPIO_7;
#  endif
#  if TOUCH_SENSORS > 8
  status[8].pin = TOUCH_GPIO_8;
#  endif
#  if TOUCH_SENSORS > 9
  status[9].pin = TOUCH_GPIO_9;
#  endif
  for (int i = 0; i < TOUCH_SENSORS; i++) {
    Log.notice(F("TOUCH_GPIO_%d: %d" CR), i, status[i].pin);
  }
  touchTimeRead = millis() - TOUCH_TIME_BETWEEN_READINGS; // so we get a first reading at the beginning
}

void MeasureTouch() {
  unsigned long now = millis();
  if (now - touchTimeRead < TOUCH_TIME_BETWEEN_READINGS) {
    return;
  }
  touchTimeRead = now;
  for (int i = 0; i < TOUCH_SENSORS; i++) {
    TouchStatus* button = &status[i];

    uint16_t val = touchRead(button->pin);
    if (val == 0) {
      // just ignore a 0 reading
      continue;
    }

    // we'll work in 1/265th of a unit value returned by touchRead(), in order
    // to have a better precision when performing averages and divisions, while
    // not using floating point calculations.
    uint16_t curValue = val * 256;
    bool isOn = (curValue < button->avg * (TOUCH_THRESHOLD * 256 / 100) / 256); // just so that we divide by 256 and not by 100
    if (!isOn) {
      button->avg = (button->avg * 127 + val * 256 * 1) / 128; // exponential averaging, factor = 1/128
    }
    int lastOnCount = button->onCount;
    if (isOn) {
      ++button->onCount;
    } else {
      button->onCount = 0;
    }

    bool isStatusChanged = false;
    if (((button->wasOn && !isOn) || (!button->wasOn && isOn && button->onCount >= TOUCH_MIN_DURATION / TOUCH_TIME_BETWEEN_READINGS)) && now - button->timeStatusChanged > TOUCH_DEBOUNCE_TIME) { // debounce
      isStatusChanged = true;
      button->wasOn = isOn;
      button->timeStatusChanged = now;
      if (isOn) {
        button->onTime = now;
      }
    }
    // keep timeStatusChanged from getting too old, so not to go
    // beyond timestamp wrapping
    if (now - button->timeStatusChanged > 100000) {
      button->timeStatusChanged = now - 100000;
    }
    if (isStatusChanged) {
      const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(4);
      StaticJsonDocument<JSON_MSG_CALC_BUFFER> jsonDoc;
      JsonObject touchData = jsonDoc.to<JsonObject>();
      touchData["id"] = i;
      touchData["on"] = (isOn ? 1 : 0);
      touchData["value"] = (int)(curValue / 256);
      if (!isOn) { // when turning off, send the duration the button was on
        touchData["onDuration"] = now - button->onTime + TOUCH_MIN_DURATION;
      }
      pub(TOUCHTOPIC, touchData);
      button->timePublished = now;
    }
    // adjust measure time so that min(avg of all buttons) is around 60 (between 40 and 80)
    // TODO

  } // loop
}
#endif
