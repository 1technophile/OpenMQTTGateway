/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker 
   Send and receiving command by MQTT
 
    HC SR-04 reading Addon
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - 1technophile
    - mpember
  
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

#ifdef ZsensorHCSR04

unsigned long timeHCSR04 = 0;

void setupHCSR04() {
  Log.notice(F("HCSR04 trigger pin: %d" CR), HCSR04_TRI_GPIO);
  Log.notice(F("HCSR04 echo pin: %d" CR), HCSR04_ECH_GPIO);
  pinMode(HCSR04_TRI_GPIO, OUTPUT); // declare HC SR-04 trigger GPIO as output
  pinMode(HCSR04_ECH_GPIO, INPUT); // declare HC SR-04 echo GPIO as input
}

void MeasureDistance() {
  if (millis() > (timeHCSR04 + TimeBetweenReadingHCSR04)) {
    timeHCSR04 = millis();
    Log.trace(F("Creating HCSR04 buffer" CR));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject& HCSR04data = jsonBuffer.createObject();
    digitalWrite(HCSR04_TRI_GPIO, LOW);
    delayMicroseconds(2);
    digitalWrite(HCSR04_TRI_GPIO, HIGH);
    delayMicroseconds(10);
    digitalWrite(HCSR04_TRI_GPIO, LOW);
    unsigned long duration = pulseIn(HCSR04_ECH_GPIO, HIGH);
    if (isnan(duration)) {
      Log.error(F("Failed to read from HC SR04 sensor!" CR));
    } else {
      static unsigned int distance = 99999;
      unsigned int d = duration / 58.2;
      HCSR04data.set("distance", (int)d);
      if (d > distance) {
        HCSR04data.set("direction", "away");
        Log.trace(F("HC SR04 Distance changed" CR));
      } else if (d < distance) {
        HCSR04data.set("direction", "towards");
        Log.trace(F("HC SR04 Distance changed" CR));
      } else if (HCSR04_always) {
        HCSR04data.set("direction", "static");
        Log.trace(F("HC SR04 Distance hasn't changed" CR));
      }
      distance = d;
      if (HCSR04data.size() > 0)
        pub(subjectHCSR04, HCSR04data);
    }
  }
}
#endif
