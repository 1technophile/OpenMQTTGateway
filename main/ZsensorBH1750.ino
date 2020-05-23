/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This is the Light Meter Addon:
   - Measures ambient Light Intensity in Lux (lx), Foot Candela (ftcd) and Watt/m^2 (wattsm2)
   - Required Hardware Module: BH1750

   Connection Schemata:
   --------------------

   BH1750 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> Vu (5V)
   GND ---------> GND ------------------> GND
   SCL ---------> Pin A5 ---------------> D1
   SDA ---------> Pin A4 ---------------> D2
   ADD ---------> N/C (Not Connected) --> N/C (Not Connected)
 
    Copyright: (c) Hans-Juergen Dinges
  
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

#ifdef ZsensorBH1750
#include "math.h" // Library for trig and exponential functions
#include "Wire.h" // Library for communication with I2C / TWI devices

void setupZsensorBH1750()
{
  Log.notice(F("Setup BH1750 on adress: %H" CR), BH1750_i2c_addr);
  Wire.begin();
  Wire.beginTransmission(BH1750_i2c_addr);
  Wire.write(0x10); // Set resolution to 1 Lux
  Wire.endTransmission();
  delay(300);
}

// void loop()
void MeasureLightIntensity()
{
  if (millis() > (timebh1750 + TimeBetweenReadingBH1750))
  { //retrieving value of Lux, FtCd and Wattsm2 from BH1750
    Log.trace(F("Creating BH1750 buffer" CR));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject &BH1750data = jsonBuffer.createObject();

    timebh1750 = millis();
    unsigned int i = 0;
    static float persistedll;
    static float persistedlf;
    static float persistedlw;
    unsigned int Lux;
    float ftcd;
    float Wattsm2;

    // Check if reads failed and exit early (to try again).
    Wire.requestFrom(BH1750_i2c_addr, 2);
    if (Wire.available() != 2)
    {
      Log.error(F("Failed to read from LightSensor BH1750!" CR));
    }
    else
    {
      i = Wire.read();
      i <<= 8;
      i |= Wire.read();

      // Calculate the Values
      Lux = i / 1.2; // Convert to Lux
      ftcd = Lux / 10.764;
      Wattsm2 = Lux / 683.0;

      // Generate Lux
      if (Lux != persistedll || bh1750_always)
      {
        BH1750data.set("lux", (unsigned int)Lux);
      }
      else
      {
        Log.trace(F("Same lux don't send it" CR));
      }

      // Generate FtCd
      if (ftcd != persistedlf || bh1750_always)
      {
        BH1750data.set("ftcd", (unsigned int)ftcd);
      }
      else
      {
        Log.trace(F("Same ftcd don't send it" CR));
      }

      // Generate Watts/m2
      if (Wattsm2 != persistedlw || bh1750_always)
      {
        BH1750data.set("wattsm2", (unsigned int)Wattsm2);
      }
      else
      {
        Log.trace(F("Same wattsm2 don't send it" CR));
      }
      if (BH1750data.size() > 0)
        pub(subjectBH1750toMQTT, BH1750data);
    }
    persistedll = Lux;
    persistedlf = ftcd;
    persistedlw = Wattsm2;
  }
}

#endif
