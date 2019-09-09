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
  pinMode(HCSR04_TRI_PIN, OUTPUT);    // declare HC SR-04 trigger pin as output
  pinMode(HCSR04_ECH_PIN, INPUT);     // declare HC SR-04 echo pin as input
}

void MeasureDistance() {
  if (millis() > (timeHCSR04 + TimeBetweenReadingHCSR04)) {
    timeHCSR04 = millis();
    trc(F("Creating HCSR04 buffer"));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject& HCSR04data = jsonBuffer.createObject();
    static unsigned int distance = 99999;
    digitalWrite(HCSR04_TRI_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(HCSR04_TRI_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(HCSR04_TRI_PIN, LOW);
    if (isnan(duration)) {
      trc(F("Failed to read from HC SR04 sensor!"));
    } else {
      unsigned long duration = pulseIn(HCSR04_ECH_PIN, HIGH);
      unsigned int d = duration/58.2;
      HCSR04data.set("distance", (int)d);
      if (d > distance) {
        HCSR04data.set("direction", "away");
        trc(F("HC SR04 Distance changed"));
      } else if (d < distance) {
        HCSR04data.set("direction", "towards");
        trc(F("HC SR04 Distance changed"));
      } else if (HCSR04_always) {
        HCSR04data.set("direction", "static");
        trc(F("HC SR04 Distance hasn't changed"));
      }
      distance = d;
      if(HCSR04data.size()>0) pub(subjectHCSR04,HCSR04data);
    }
  }
}
#endif
