/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    DHT reading Addon
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - prahjister
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
#ifdef ZsensorDHT
#include <DHT.h>
#include <DHT_U.h>

DHT dht(DHT_RECEIVER_PIN,DHT_SENSOR_TYPE); 

//Time used to wait for an interval before resending temp and hum
unsigned long timedht = 0;

void MeasureTempAndHum(){
  if (millis() > (timedht + TimeBetweenReadingDHT)) {//retriving value of temperature and humidity of the box from DHT every xUL
    timedht = millis();
    static float persistedh;
    static float persistedt;
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature(); 
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      trc(F("Failed to read from DHT sensor!"));
    }else{
      if(h != persistedh || dht_always){
        trc(F("Sending Hum to MQTT"));
        trc(h);
        client.publish(HUM1,String(h).c_str());
       }else{
        trc(F("Same hum don't send it"));
       }
      if(t != persistedt || dht_always){
        trc(F("Sending Temp to MQTT"));
        trc(t);
        client.publish(TEMP1, String(t).c_str());
      }else{
        trc(F("Same temp don't send it"));
      }
    }
    persistedh = h;
    persistedt = t;
  }
}
#endif
