  /*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This is the Light Meter Addon based on modules with a TSL2561:
   - Measures ambient Light Intensity in Lux (lx), Foot Candela (ftcd) and Watt/m^2 (wattsm2)
   - Required Hardware Module: TSL2561 (for instance Banggood.com product ID 1129550)
   - Dependencies: Adafruit_TSL2561 and Adafruit_Sensor

   Connection Scheme:
   --------------------

   TSL2561------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 3.3V -----------------> 3.3V
   GND ---------> GND ------------------> GND
   SCL ---------> Pin A5 ---------------> D1
   SDA ---------> Pin A4 ---------------> D2
   ADD ---------> N/C (Not Connected) --> N/C (Not Connected)
 
    Copyright: (c) Chris Broekema
  
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

#ifdef ZsensorTSL2561
#include "math.h"
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void displaySensorDetails(void)
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  trc(F("------------------------------------"));
  trc("Sensor:       " + String(sensor.name));
  trc("Driver Ver:   " + String(sensor.version));
  trc("Unique ID:    " + String(sensor.sensor_id));
  trc("Max Value:    " + String(sensor.max_value) + " lux");
  trc("Min Value:    " + String(sensor.min_value) + " lux");
  trc("Resolution:   " + String(sensor.resolution) + " lux");
  trc(F("------------------------------------"));
  trc(F(""));
  delay(500);
}

void setupZsensorTSL2561()
{
  Wire.begin();
  Wire.beginTransmission(TSL2561_ADDR_FLOAT);

  if (!tsl.begin())
  {
    trc(F("No TSL2561 detected"));
  }
  
  // enable auto ranging
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  // since we're slowly sampling, enable high resolution but slow mode TSL2561
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);

  trc(F("TSL2561 Initialized. Printing detials now."));
  displaySensorDetails();  
}

void MeasureLightIntensityTSL2561()
{
  if (millis() > (timetsl2561 + TimeBetweenReadingtsl2561)) {
    static uint32_t persisted_lux;
    timetsl2561 = millis();

    trc(F("Creating TSL2561 buffer"));
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(3);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
    JsonObject& TSL2561data = jsonBuffer.createObject();
    
    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light)
      // if event.light == 0 the sensor is clipping, do not send
      {
	if (persisted_lux != event.light || tsl2561_always ) {
	  persisted_lux = event.light;

    TSL2561data.set("lux", (float)event.light);
    TSL2561data.set("ftcd", (float)(event.light)/10.764);
    TSL2561data.set("wattsm2", (float)(event.light)/683.0);

    pub(subjectTSL12561toMQTT,TSL2561data);
	} else {
	  trc(F("Same lux value, do not send"));
	}
      } else {
      trc(F("Failed to read from TSL2561"));
    }
  }
}
#endif
