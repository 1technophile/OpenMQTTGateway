/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 
   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    GPIO Multi Input derived from
    GPIO Input derived from HC SR-501 reading Addon and https://www.arduino.cc/en/Tutorial/Debounce

    This reads a high (open) or low (closed) through a circuit (switch, float sensor, etc.) connected to ground.

    Copyright: (c)Florian ROBERT, Artiom FEDOROV
    
    Contributors:
    - 1technophile
    - QuagmireMan
    - ArtiomFedorov
  
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
#include "User_config.h"

#ifdef ZsensorGPIOMultiInput

#ifdef INPUT_GPIO_1
unsigned long lastDebounceTime_1 = 0;
int InputState_1 = GPIOMultiInputState; // Set to 3 so that it reads on startup
int lastInputState_1 = GPIOMultiInputState;
#endif

#ifdef INPUT_GPIO_2
unsigned long lastDebounceTime_2 = 0;
int InputState_2 = GPIOMultiInputState; // Set to 3 so that it reads on startup
int lastInputState_2 = GPIOMultiInputState;
#endif

#ifdef INPUT_GPIO_3
unsigned long lastDebounceTime_3 = 0;
int InputState_3 = GPIOMultiInputState; // Set to 3 so that it reads on startup
int lastInputState_3 = GPIOMultiInputState;
#endif

#ifdef INPUT_GPIO_4
unsigned long lastDebounceTime_4 = 0;
int InputState_4 = GPIOMultiInputState; // Set to 3 so that it reads on startup
int lastInputState_4 = GPIOMultiInputState;
#endif

#ifdef INPUT_GPIO_5
unsigned long lastDebounceTime_5 = 0;
int InputState_5 = GPIOMultiInputState; // Set to 3 so that it reads on startup
int lastInputState_5 = GPIOMultiInputState;
#endif

#ifdef INPUT_GPIO_6
unsigned long lastDebounceTime_6 = 0;
int InputState_6 = GPIOMultiInputState; // Set to 3 so that it reads on startup
int lastInputState_6 = GPIOMultiInputState;
#endif

#ifdef INPUT_GPIO_7
unsigned long lastDebounceTime_7 = 0;
int InputState_7 = GPIOMultiInputState; // Set to 3 so that it reads on startup
int lastInputState_7 = GPIOMultiInputState;
#endif

#ifdef INPUT_GPIO_8
unsigned long lastDebounceTime_8 = 0;
int InputState_8 = GPIOMultiInputState; // Set to 3 so that it reads on startup
int lastInputState_8 = GPIOMultiInputState;
#endif

void setupGPIOMultiInput() {

#ifdef INPUT_GPIO_1
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_1);
  pinMode(INPUT_GPIO_1, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
#endif

#ifdef INPUT_GPIO_2
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_2);
  pinMode(INPUT_GPIO_2, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
#endif

#ifdef INPUT_GPIO_3
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_3);
  pinMode(INPUT_GPIO_3, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
#endif

#ifdef INPUT_GPIO_4
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_4);
  pinMode(INPUT_GPIO_4, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
#endif

#ifdef INPUT_GPIO_5
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_5);
  pinMode(INPUT_GPIO_5, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
#endif

#ifdef INPUT_GPIO_6
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_6);
  pinMode(INPUT_GPIO_6, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
#endif

#ifdef INPUT_GPIO_7
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_7);
  pinMode(INPUT_GPIO_7, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
#endif

#ifdef INPUT_GPIO_8
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_8);
  pinMode(INPUT_GPIO_8, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
#endif

}



void MeasureGPIOMultiInput() {

#ifdef INPUT_GPIO_1
  int reading_1 = digitalRead(INPUT_GPIO_1);
  if (reading_1 != lastInputState_1) {
    lastDebounceTime_1 = millis();
  }
  if ((millis() - lastDebounceTime_1) > GPIOMultiInputDebounceDelay) {
#  ifdef ESP32
    yield();
#  endif
    if (reading_1 != InputState_1) {
      InputState_1 = reading_1;
      publishMeasureGPIOMultiInput(InputState_1, 1);
    }
  }
  lastInputState_1 = reading_1;
#endif

#ifdef INPUT_GPIO_2
  int reading_2 = digitalRead(INPUT_GPIO_2);
  if (reading_2 != lastInputState_2) {
    lastDebounceTime_2 = millis();
  }
  if ((millis() - lastDebounceTime_2) > GPIOMultiInputDebounceDelay) {
#  ifdef ESP32
    yield();
#  endif
    if (reading_2 != InputState_2) {
      InputState_2 = reading_2;
      publishMeasureGPIOMultiInput(InputState_2, 2);
    }
  }
  lastInputState_2 = reading_2;
#endif

#ifdef INPUT_GPIO_3
  int reading_3 = digitalRead(INPUT_GPIO_3);
  if (reading_3 != lastInputState_3) {
    lastDebounceTime_3 = millis();
  }
  if ((millis() - lastDebounceTime_3) > GPIOMultiInputDebounceDelay) {
#  ifdef ESP32
    yield();
#  endif
    if (reading_3 != InputState_3) {
      InputState_3 = reading_3;
      publishMeasureGPIOMultiInput(InputState_3, 3);
    }
  }
  lastInputState_3 = reading_3;
#endif

#ifdef INPUT_GPIO_4
  int reading_4 = digitalRead(INPUT_GPIO_4);
  if (reading_4 != lastInputState_4) {
    lastDebounceTime_4 = millis();
  }
  if ((millis() - lastDebounceTime_4) > GPIOMultiInputDebounceDelay) {
#  ifdef ESP32
    yield();
#  endif
    if (reading_4 != InputState_4) {
      InputState_4 = reading_4;
      publishMeasureGPIOMultiInput(InputState_4, 4);
    }
  }
  lastInputState_4 = reading_4;
#endif

#ifdef INPUT_GPIO_5
  int reading_5 = digitalRead(INPUT_GPIO_5);
  if (reading_5 != lastInputState_5) {
    lastDebounceTime_5 = millis();
  }
  if ((millis() - lastDebounceTime_5) > GPIOMultiInputDebounceDelay) {
#  ifdef ESP32
    yield();
#  endif
    if (reading_5 != InputState_5) {
      InputState_5 = reading_5;
      publishMeasureGPIOMultiInput(InputState_5, 5);
    }
  }
  lastInputState_5 = reading_5;
#endif

#ifdef INPUT_GPIO_6
  int reading_6 = digitalRead(INPUT_GPIO_6);
  if (reading_6 != lastInputState_6) {
    lastDebounceTime_6 = millis();
  }
  if ((millis() - lastDebounceTime_6) > GPIOMultiInputDebounceDelay) {
#  ifdef ESP32
    yield();
#  endif
    if (reading_6 != InputState_6) {
      InputState_6 = reading_6;
      publishMeasureGPIOMultiInput(InputState_6, 6);
    }
  }
  lastInputState_6 = reading_6;
#endif

#ifdef INPUT_GPIO_7
  int reading_7 = digitalRead(INPUT_GPIO_7);
  if (reading_7 != lastInputState_7) {
    lastDebounceTime_7 = millis();
  }
  if ((millis() - lastDebounceTime_7) > GPIOMultiInputDebounceDelay) {
#  ifdef ESP32
    yield();
#  endif
    if (reading_7 != InputState_7) {
      InputState_7 = reading_7;
      publishMeasureGPIOMultiInput(InputState_7, 7);
    }
  }
  lastInputState_7 = reading_7;
#endif

#ifdef INPUT_GPIO_8
  int reading_8 = digitalRead(INPUT_GPIO_8);
  if (reading_8 != lastInputState_8) {
    lastDebounceTime_8 = millis();
  }
  if ((millis() - lastDebounceTime_8) > GPIOMultiInputDebounceDelay) {
#  ifdef ESP32
    yield();
#  endif
    if (reading_8 != InputState_8) {
      InputState_8 = reading_8;
      publishMeasureGPIOMultiInput(InputState_8, 8);
    }
  }
  lastInputState_8 = reading_8;
#endif

}


void publishMeasureGPIOMultiInput(int _input_state, int GPIO_number) {
  Log.trace(F("Creating GPIOInput buffer" CR));
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject GPIOdata = jsonBuffer.to<JsonObject>();
  if (_input_state == HIGH) {
    GPIOdata["gpio_" + String(GPIO_number)] = MULTI_INPUT_GPIO_ON_VALUE;
  }
  if (_input_state == LOW) {
    GPIOdata["gpio_" + String(GPIO_number)] = MULTI_INPUT_GPIO_OFF_VALUE;
  }
  if (GPIOdata.size() > 0)
    pub(subjectGPIOMultiInputtoMQTT, GPIOdata);
}


#endif
