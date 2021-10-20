/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 
 
   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker
   Send and receiving command by MQTT
   
   This is the Climate Addon:
   - Measures Temperature, Humidity
   - Generates Values for: Temperature in degrees C and F, Humidity in %
   - Required Hardware Module: AHTX0 (AHT10 & AHT20)
   - Required Library: Adafruit AHTX0 by Adafruit

   Connection Schemata:
   --------------------

   AHT10 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> 3v3 (3V)
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

#ifdef ZsensorAHTx0
#  include <stdint.h>

#  include "Adafruit_AHTX0.h"
#  include "Wire.h" // Library for communication with I2C / TWI devices
#  include "config_AHTx0.h"

//Time used to wait for an interval before resending measured values
unsigned long timeAHTx0 = 0;

//Global sensor object
Adafruit_AHTX0 ahtSensor;

void setupZsensorAHTx0() {
  delay(10); // Gives the Sensor enough time to turn on
  Log.notice(F("AHTx0 Initialized - begin()" CR));

#  if defined(ESP32)
  Wire.begin(AHT_I2C_SDA, AHT_I2C_SCL);
  if (!ahtSensor.begin(&Wire)) {
    Log.error(F("Failed to initialize AHTx0 sensor!" CR));
  }
#  else
  if (!ahtSensor.begin()) {
    Log.error(F("Failed to initialize AHTx0 sensor!" CR));
  }
#  endif
}

void MeasureAHTTempHum() {
  if (millis() > (timeAHTx0 + TimeBetweenReadingAHTx0)) {
    Log.trace(F("Read AHTx0 Sensor" CR));

    timeAHTx0 = millis();
    static float persisted_aht_tempc;
    static float persisted_aht_hum;

    sensors_event_t ahtTempC, ahtHum;
    if (!ahtSensor.getEvent(&ahtHum, &ahtTempC)) // get sensor data
    {
      Log.error(F("Failed to read from sensor AHTx0!" CR));
      return;
    }

    // Check if reads failed and exit early (to try again).
    if (isnan(ahtTempC.temperature) || isnan(ahtHum.relative_humidity)) {
      Log.error(F("Failed to read from sensor AHTx0!" CR));
    } else {
      Log.notice(F("Creating AHTx0 buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject AHTx0data = jsonBuffer.to<JsonObject>();
      // Generate Temperature in degrees C
      if (ahtTempC.temperature != persisted_aht_tempc || AHTx0_always) {
        float ahtTempF = convertTemp_CtoF(ahtTempC.temperature);
        AHTx0data["tempc"] = (float)ahtTempC.temperature;
        AHTx0data["tempf"] = (float)ahtTempF;
      } else {
        Log.notice(F("Same Temp. Don't send it" CR));
      }

      // Generate Humidity in percent
      if (ahtHum.relative_humidity != persisted_aht_hum || AHTx0_always) {
        AHTx0data["hum"] = (float)ahtHum.relative_humidity;
      } else {
        Log.notice(F("Same Humidity. Don't send it" CR));
      }

      if (AHTx0data.size() > 0) {
        pub(AHTTOPIC, AHTx0data);
      }
    }
    persisted_aht_tempc = ahtTempC.temperature;
    persisted_aht_hum = ahtHum.relative_humidity;
  }
}

#endif
