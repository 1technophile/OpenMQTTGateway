/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This is the Light Meter Addon:
   - Measures ambient Light Intensity in Lux (lx), Foot Candela (fc) and Watt/m^2 (wqm)
   - Required Hardware Module: BH1750

   Connection Schemata:
   --------------------

   BH1750 ------> Arduino Uno ----------> ESP8266
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

#ifdef ZsensorBH1750
#include "math.h" // Library for trig and exponential functions
#include "Wire.h" // Library for communication with I2C / TWI devices
#define bh1750_always true // if false when the current value for temp or hum is the same as previous one don't send it by MQTT
#define TimeBetweenReadingBH1750 8000

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define LUX   "home/433toMQTT/bh17501/lux"
#define FTCD  "home/433toMQTT/bh17501/ftcd"
#define WATTSM2  "home/433toMQTT/bh17501/wattsm2"


/*
lux (lx) # 1 lx = 1 lm/m² = 1 cd·sr·m⁻².
meter-candle (m-cd) #  1 m·cd = 1 lx = 1 lm/m² = 1 cd·sr·m⁻².
centimeter-candle (cm-sd) #  1 m·cd = 1 lx = 1 lm/m² = 1 cd·sr·m⁻².
foot-candle (ft-c) # 
phot (ph) # 1 ph = 1 lm/cm² = 10,000 lm/m² - 10,000 lx = 10 klx
nox (nx) #  1 nox = 1 millilux
candela steradin/meter2 (cd·sr·m⁻²) # 1 lx = 1 lm/m² = 1 cd·sr·m⁻²
lumen/meter2 (lm·m⁻²) # 1 lx = 1 lm/m² = 1 cd·sr·m⁻²
lumen/centimeter2  (lm·cm⁻²) # 1 lm/cm² = 10,000 lx = 10,000 cd·sr·m⁻²
lumen/foot2 (lm·ft⁻²) # (lm·ft⁻²)
watt/centimeter2 at 555nm  (W·cm⁻²) # 
*/


//Time used to wait for an interval before resending measured values
unsigned long timebh1750 = 0;
int BH1750_i2c_addr = 0x23; // Light Sensor I2C Address

 
void setupZsensorBH1750()
{
  Wire.begin();
  Wire.beginTransmission(BH1750_i2c_addr);
  Wire.write(0x10);      // Set resolution to 1 Lux
  Wire.endTransmission();
  delay(300);
}
 
// void loop() 
void MeasureLightIntensity()
{
  if (millis() > (timebh1750 + TimeBetweenReadingBH1750)) {//retriving value of Lux, FtCd and Wattsm2 from BH1750 every xUL
    timebh1750 = millis();
    unsigned int Lux, Scaled_FtCd;
    float FtCd, Wattsm2;
    unsigned int i=0;
    // static float persistedh;
    // static float persistedt;
    // float h = dht.readHumidity();


    // Lux = MeasureLight();
    
    Wire.beginTransmission(BH1750_i2c_addr);
    Wire.requestFrom(BH1750_i2c_addr, 2);
    // Wire.write(B1000000); 
    while(Wire.available()) //
    {
      i <<=8;
      i|= Wire.read();  
    }
    Wire.endTransmission();  

    Lux = i/1.2;  // Convert to Lux

    FtCd = Lux/10.764;
    Wattsm2 = Lux/683.0;
    
    Serial.print(Lux,DEC);     
    Serial.println("[lx]"); 
    
    Serial.print(FtCd,2);     
    Serial.println("[FC]");
    
    Serial.print(Wattsm2,4);     
    Serial.println("[Watts/m^2]"); 
    
    Serial.println("-------------------------OK-OK-------------------------\n\r"); 
    delay(2000);
  }
}

#endif
