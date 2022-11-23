/*
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

   This is a Temperature Addon:
   - Measures Temperature
   - Generates Values for: Temperature in degrees C and F
   - Required Hardware Module: LM75 or NCT75
   - Required Library: jeremycole/I2C Temperature Sensors derived from the LM75

   Connection Schemata:
   --------------------

   LM75 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> Vu (5V)
   GND ---------> GND ------------------> GND
   SCL ---------> Pin A5 ---------------> D1
   SDA ---------> Pin A4 ---------------> D2

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

#ifdef ZsensorLM75
#  include <stdint.h>

#  include "Temperature_LM75_Derived.h"
#  include "Wire.h" // Library for communication with I2C / TWI devices
#  include "config_LM75.h"

//Time used to wait for an interval before resending measured values
unsigned long timelm75 = 0;

//Global sensor object
Generic_LM75 lm75Sensor;

void setupZsensorLM75() {
  delay(10); // Gives the Sensor enough time to turn on
  Log.notice(F("LM75 Initialized - begin()" CR));

#  if defined(ESP32)
  Wire.begin(I2C_SDA, I2C_SCL);
#  elif defined(ESP8266)
  Wire.begin();
#  endif
}

void MeasureTemp() {
  if (millis() > (timelm75 + TimeBetweenReadinglm75)) {
    Log.trace(F("Read LM75 Sensor" CR));

    timelm75 = millis();
    static float persisted_lm75_tempc;

    float lm75TempC = lm75Sensor.readTemperatureC();

    if (lm75TempC >= 998) {
      Log.error(F("Failed to read from sensor LM75!" CR));
      return;
    }

    // Check if reads failed and exit early (to try again).
    if (isnan(lm75TempC)) {
      Log.error(F("Failed to read from sensor HLM75!" CR));
    } else {
      Log.notice(F("Creating LM75 buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject LM75data = jsonBuffer.to<JsonObject>();
      // Generate Temperature in degrees C
      if (lm75TempC != persisted_lm75_tempc || lm75_always) {
        float lm75TempF = (lm75TempC * 1.8) + 32;
        LM75data["tempc"] = (float)lm75TempC;
        LM75data["tempf"] = (float)lm75TempF;
        pub(LM75TOPIC, LM75data);
      } else {
        Log.notice(F("Same Temp. Don't send it" CR));
      }
    }
    persisted_lm75_tempc = lm75TempC;
  }
}

#endif
