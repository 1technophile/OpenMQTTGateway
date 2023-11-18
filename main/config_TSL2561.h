/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker 
   Send and receiving command by MQTT
 
   This files exposes light level measured by a TSL2651 module
   Heavily inspired by the driver for the BH1750 light sensor

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
   
   Connection Schemata:
   --------------------

   TSL2561 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 3.3V -----------------> Vu (3.3V)
   GND ---------> GND ------------------> GND
   SCL ---------> Pin A5 ---------------> D1
   SDA ---------> Pin A4 ---------------> D2
   ADD ---------> N/C (Not Connected) --> N/C (Not Connected)

*/
#ifndef config_TSL2561_h
#define config_TSL2561_h

extern void setupZsensorTSL2561();
extern void MeasureLightIntensityTSL2561();

#define tsl2561_always            true // if false only send current value if it has changed
#define TimeBetweenReadingtsl2561 30000
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectTSL12561toMQTT "/LIGHTtoMQTT"

//Time used to wait for an interval before resending measured values
unsigned long timetsl2561 = 0;
//int TSL2561_i2c_addr = 0x37; // Light Sensor I2C Address (Set in Adafruit library)

#endif