/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This is the MQ2 GAS Sensor Addon based on modules with a MQ:
   - Measures flammable gas
   - Required Hardware Module: MQ2

   Connection Scheme:
   --------------------

   MQ-2 -------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 3.3V or 5V------------> 3.3V or 5V
   GND ---------> GND ------------------> GND
   A0 ----------> Pin A0 ---------------> A0
   D0 ----------> Pin D4 ---------------> D4
  
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

#ifdef ZsensorMQ2

#  include "Wire.h"
#  include "math.h"

void setupZsensorMQ2() {
  Log.notice(F("Setup MQ2 detection on pin: %d" CR), MQ2SENSORDETECTPIN);
  pinMode(MQ2SENSORDETECTPIN, INPUT); // declare GPIOInput pin as input_pullup to prevent floating. Pin will be high when not connected to ground

  Log.notice(F("Starting MQ2 calibration on pin: %d" CR), MQ2SENSORADCPIN);

  //Simple calibrate
  float sensorValue;
  for (int x = 0; x < 1000; x++) {
    analogRead(MQ2SENSORADCPIN);
  }
  delay(1000);

  Log.trace(F("MQ2 Initialized." CR));
}

void MeasureGasMQ2() {
  if (millis() > (timemq2 + TimeBetweenReadingmq2)) {
    timemq2 = millis();

    Log.trace(F("Creating MQ2 buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject MQ2data = jsonBuffer.to<JsonObject>();

    MQ2data["gas"] = analogRead(MQ2SENSORADCPIN);
    MQ2data["detected"] = digitalRead(MQ2SENSORDETECTPIN) == HIGH ? "false" : "true";

    pub(subjectMQ2toMQTT, MQ2data);
  }
}
#endif
