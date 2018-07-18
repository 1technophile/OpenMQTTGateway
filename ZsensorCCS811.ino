/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    CCS811 reading Addon
  
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
#ifdef ZsensorCCS811

#include "Adafruit_CCS811.h"

Adafruit_CCS811 ccs;

void setupZsensorCCS811() {
  trc(F("CCS811Setup Begin"));
  
  if(!ccs.begin()){
    trc(F("Failed to start sensor! Please check your wiring."));
  }

  //calibrate temperature sensor
  while(!ccs.available());
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);
}

unsigned long timeCCS811 = 0;

void MeasureAirQuality() {
    if (millis() > (timeCCS811 + TimeBetweenReadingCCS811)) {
    timeCCS811 = millis();
    static float persisted_co2;
    static float persisted_tvoc;
    float temp = ccs.calculateTemperature();
    float co2 = ccs.geteCO2();
    // Read temperature as Celsius (the default)
    float tvoc = ccs.getTVOC(); 
    // Check if any reads failed and exit early (to try again).
    if (isnan(co2) || isnan(tvoc)) {
      trc(F("Failed to read from CCS811 sensor!"));
    }else{
      if(co2 != persisted_co2 || CCS811_always){
        trc(F("Sending CO2 to MQTT"));
        trc(String(co2));
        client.publish(CO2_OUT,String(co2).c_str());
       }else{
        trc(F("Same CO2 don't send it"));
       }
      if(tvoc != persisted_tvoc || CCS811_always){
        trc(F("Sending TVOC to MQTT"));
        trc(String(tvoc));
        client.publish(TVOC_OUT, String(tvoc).c_str());
      }else{
        trc(F("Same TVOC don't send it"));
      }
    }
    persisted_co2 = co2;
    persisted_tvoc = tvoc;
  }
}
#endif
