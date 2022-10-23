/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 
   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    GPIO Input derived from HC SR-501 reading Addon and https://www.arduino.cc/en/Tutorial/Debounce

    This reads a high (open) or low (closed) through a circuit (switch, float sensor, etc.) connected to ground.

    Copyright: (c)Florian ROBERT
    
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
int InputState_1 = 3; // Set to 3 so that it reads on startup
int lastInputState_1 = 3;
#endif


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


void setupGPIOMultiInput() {


#ifdef INPUT_GPIO_1
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO_1);
  pinMode(INPUT_GPIO_1, INPUT_PULLUP); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
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
      publishMeasureGPIOMultiInput(InputState_1, 1)
    }
  }
  lastInputState_1 = reading_1;
#endif



}
#endif
