/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This is the Light Meter Addon based on modules with a TEMT6000:
   - Measures ambient Light Intensity in Lux (lx), Foot Candela (ftcd) and Watt/m^2 (wattsm2)
   - Required Hardware Module: TEMT6000
   - Dependencies: none

   Connection Scheme:
   --------------------

   TEMT6000------> ESP8266
   ==============================================
   Vcc ---------> 3.3V
   GND ---------> GND
   SIG ---------> A0

    Copyright: (c) Andre Greyling - this is a modified copy of Chris Broekema
  
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

#ifdef ZsensorTEMT6000

#  include "Wire.h"
#  include "math.h"

void setupZsensorTEMT6000() {
  Log.notice(F("Setup TEMT6000 on pin: %i" CR), TEMT6000LIGHTSENSORPIN);
  pinMode(TEMT6000LIGHTSENSORPIN, INPUT);
}

void MeasureLightIntensityTEMT6000() {
  if (millis() > (timetemt6000 + TimeBetweenReadingtemt6000)) {
    static uint32_t persisted_lux;
    timetemt6000 = millis();

    Log.trace(F("Creating TEMT6000 buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject TEMT6000data = jsonBuffer.to<JsonObject>();

    float volts = analogRead(TEMT6000LIGHTSENSORPIN) * 5.0 / 1024.0;
    float amps = volts / 10000.0; // across 10,000 Ohms
    float microamps = amps * 1000000;
    float lux = microamps * 2.0;

    if (persisted_lux != lux || temt6000_always) {
      persisted_lux = lux;

      TEMT6000data["lux"] = (float)lux;
      TEMT6000data["ftcd"] = (float)(lux) / 10.764;
      TEMT6000data["wattsm2"] = (float)(lux) / 683.0;

      pub(subjectTEMT6000toMQTT, TEMT6000data);
    } else {
      Log.trace(F("Same lux value, do not send" CR));
    }
  }
}
#endif
