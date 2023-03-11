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

#ifdef ZsensorGPIOInput
#  if defined(TRIGGER_GPIO) && INPUT_GPIO == TRIGGER_GPIO
unsigned long resetTime = 0;
#  endif
unsigned long lastDebounceTime = 0;
int InputState = 3; // Set to 3 so that it reads on startup
int previousInputState = 3;

void setupGPIOInput() {
  Log.notice(F("Reading GPIO at pin: %d" CR), INPUT_GPIO);
  pinMode(INPUT_GPIO, GPIO_INPUT_TYPE); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground
}

void MeasureGPIOInput() {
  int reading = digitalRead(INPUT_GPIO);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != previousInputState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > GPIOInputDebounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
#  if defined(ESP8266) || defined(ESP32)
    yield();
#  endif
#  if defined(TRIGGER_GPIO) && INPUT_GPIO == TRIGGER_GPIO && !defined(ESPWifiManualSetup)
    if (reading == LOW) {
      if (resetTime == 0) {
        resetTime = millis();
      } else if ((millis() - resetTime) > 3000) {
        Log.trace(F("Button Held" CR));
        Log.notice(F("Erasing ESP Config, restarting" CR));
        setup_wifimanager(true);
      }
    } else {
      resetTime = 0;
    }
#  endif
    // if the Input state has changed:
    if (reading != InputState) {
      Log.trace(F("Creating GPIOInput buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject GPIOdata = jsonBuffer.to<JsonObject>();
      if (InputState == HIGH) {
        GPIOdata["gpio"] = "HIGH";
      }
      if (InputState == LOW) {
        GPIOdata["gpio"] = "LOW";
      }
      if (GPIOdata.size() > 0)
        pub(subjectGPIOInputtoMQTT, GPIOdata);

#  if defined(ZactuatorONOFF) && defined(ACTUATOR_TRIGGER)
      //Trigger the actuator if we are not at startup
      if (InputState != 3) {
#    if defined(ACTUATOR_BUTTON_TRIGGER_LEVEL)
        if (InputState == ACTUATOR_BUTTON_TRIGGER_LEVEL)
          ActuatorTrigger(); // Button press trigger
#    else
        ActuatorTrigger(); // Switch trigger
#    endif
      }
#  endif
      InputState = reading;
    }
  }

  // save the reading. Next time through the loop, it'll be the previousInputState:
  previousInputState = reading;
}
#endif
