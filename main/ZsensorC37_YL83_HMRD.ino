/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR/BLE signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    This file sets parameters for the integration of C-37 YL-83 HM-RD water detection sensors
  
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

#ifdef ZsensorC37_YL83_HMRD

//Time used to wait for an interval before resending temp and hum
unsigned long timeC37YL83HMRD = 0;
unsigned int persistedanalog = 0;
unsigned int persisteddigital = 0;

void setupZsensorC37_YL83_HMRD() {
  pinMode(C37_YL83_HMRD_Digital_GPIO, INPUT);
  Log.trace(F("C37_YL83_HMRD: digital configured pin: %d" CR), C37_YL83_HMRD_Digital_GPIO);

  pinMode(C37_YL83_HMRD_Analog_GPIO, INPUT);
  Log.trace(F("C37_YL83_HMRD: Analog configured pin: %d" CR), C37_YL83_HMRD_Analog_GPIO);

#  ifdef C37_YL83_HMRD_Analog_RESOLUTION
  analogReadResolution(C37_YL83_HMRD_Analog_RESOLUTION);
  Log.trace(F("C37_YL83_HMRD: resolution: %d" CR), C37_YL83_HMRD_Analog_RESOLUTION);
#  endif
}

void MeasureC37_YL83_HMRDWater() {
  if (millis() > (timeC37YL83HMRD + C37_YL83_HMRD_INTERVAL_SEC)) { //retrieving value of water sensor every xUL
    timeC37YL83HMRD = millis();
    static int persistedanalog;
    static int persisteddigital;

    int sensorDigitalValue = digitalRead(C37_YL83_HMRD_Digital_GPIO); // Read the analog value from sensor
    int sensorAnalogValue = analogRead(C37_YL83_HMRD_Analog_GPIO);

    Log.trace(F("Creating C37_YL83_HMRD buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject C37_YL83_HMRDdata = jsonBuffer.to<JsonObject>();
    if (sensorDigitalValue != persisteddigital || C37_YL83_HMRD_ALWAYS) {
      C37_YL83_HMRDdata["detected"] = (sensorDigitalValue == 1 ? "false" : "true");
    } else {
      Log.trace(F("Same digital don't send it" CR));
    }
    if (sensorAnalogValue != persistedanalog || C37_YL83_HMRD_ALWAYS) {
      C37_YL83_HMRDdata["reading"] = sensorAnalogValue;
    } else {
      Log.trace(F("Same analog don't send it" CR));
    }
    if (C37_YL83_HMRDdata.size() > 0) {
      pub(C37_YL83_HMRD_TOPIC, C37_YL83_HMRDdata);
      delay(10);
#  if defined(DEEP_SLEEP_IN_US) || defined(ESP32_EXT0_WAKE_PIN)
      if (sensorDigitalValue == 1) //No water detected, all good we can sleep
        ready_to_sleep = true;
#  endif
    }
    persistedanalog = sensorAnalogValue;
    persisteddigital = sensorDigitalValue;
  }
}
#endif
