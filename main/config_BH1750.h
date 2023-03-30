/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the BH1750 sensor

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

   Connection Schemata:
   --------------------

   BH1750 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> Vu (5V)
   GND ---------> GND ------------------> GND
   SCL ---------> Pin A5 ---------------> D1
   SDA ---------> Pin A4 ---------------> D2
   ADD ---------> N/C (Not Connected) --> N/C (Not Connected)

*/
#ifndef config_BH1750_h
#define config_BH1750_h

extern void setupZsensorBH1750();
extern void MeasureLightIntensity();

#define bh1750_always            true // if false when the current value for light Level (Lux) is the same as previous one don't send it by MQTT
#define TimeBetweenReadingBH1750 30000

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectBH1750toMQTT "/BH1750toMQTT"
//Time used to wait for an interval before resending measured values
unsigned long timebh1750 = 0;
int BH1750_i2c_addr = 0x23; // Light Sensor I2C Address

#endif
