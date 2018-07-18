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
#ifdef ZsensorSHT3x
#include "Adafruit_SHT31.h"
#ifdef m5stack
  #include "Free_Fonts.h" // Include the header file attached to this sketch
#endif

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setupZsensorSHT3x()
{
  trc(F("SHT31 Setup Begin"));
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    trc(F("ERROR - Couldn't find SHT31"));
  }
}

//Time used to wait for an interval before resending temp and hum
unsigned long timeSHT3x = 0;

  int xpos =  0;
  int ypos = 40;

void MeasureTempAndHum(){
  if (millis() > (timeSHT3x + TimeBetweenReadingSHT3x)) {//retriving value of temperature and humidity of the box from DHT every xUL
    timeSHT3x = millis();
    static float persistedh;
    static float persistedt;
    float h = sht31.readHumidity();
    // Read temperature as Celsius (the default)
    float t = sht31.readTemperature(); 
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      trc(F("Failed to read from SHT3x sensor!"));
    }else{
      if(h != persistedh || sht3x_always){
        trc(F("Sending Hum to MQTT"));
        trc(String(h));
        client.publish(HUM1,String(h).c_str());
       }else{
        trc(F("Same hum don't send it"));
       }
      if(t != persistedt || sht3x_always){
        trc(F("Sending Temp to MQTT"));
        trc(String(t));
        client.publish(TEMP1, String(t).c_str());
      }else{
        trc(F("Same temp don't send it"));
      }
    }
    persistedh = h;
    persistedt = t;
    #ifdef m5stack
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(xpos, ypos); 
    M5.Lcd.setFreeFont(FSB12);
    M5.Lcd.print("Temp: ");
    M5.Lcd.print(t);
    M5.Lcd.print(" Humidity: ");
    M5.Lcd.print(h);
    #endif
  }
}
#endif
