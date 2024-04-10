/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
    HC SR-501 reading Addon
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - 1technophile
  
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

#ifdef ZsensorHCSR501

void setupHCSR501() {
  Log.notice(F("HCSR501 pin: %d" CR), HCSR501_GPIO);
  pinMode(HCSR501_GPIO, INPUT); // declare HC SR-501 GPIO as input
#  ifdef HCSR501_LED_NOTIFY_GPIO
  pinMode(HCSR501_LED_NOTIFY_GPIO, OUTPUT);
  digitalWrite(HCSR501_LED_NOTIFY_GPIO, LOW);
#  endif
}

void MeasureHCSR501() {
  if (millis() > TimeBeforeStartHCSR501) { //let time to init the PIR sensor
    StaticJsonDocument<JSON_MSG_BUFFER> HCSR501dataBuffer;
    JsonObject HCSR501data = HCSR501dataBuffer.to<JsonObject>();
    static int pirState = LOW;
    int PresenceValue = digitalRead(HCSR501_GPIO);
#  if defined(ESP8266) || defined(ESP32)
    yield();
#  endif
    if (PresenceValue == HIGH) {
      if (pirState == LOW) {
        //turned on
        HCSR501data["presence"] = "true";
        pirState = HIGH;
      }
    } else {
      if (pirState == HIGH) {
        // turned off
        HCSR501data["presence"] = "false";
        pirState = LOW;
      }
    }
#  ifdef HCSR501_LED_NOTIFY_GPIO
    digitalWrite(HCSR501_LED_NOTIFY_GPIO, pirState == HCSR501_LED_ON);
#  endif
    if (HCSR501data.size() > 0) {
      HCSR501data["origin"] = subjectHCSR501toMQTT;
      handleJsonEnqueue(HCSR501data);
    }
  }
}
#endif
