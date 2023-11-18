/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This is the Light Meter Addon based on modules with a TEMT6000:
   - Measures ambient Light Intensity in Lux (lx), Foot Candela (ftcd) and Watt/m^2 (wattsm2)
   - Required Hardware Module: TEMT6000
   - Dependencies: none

   Connection Scheme:
   --------------------

   TEMT6000 ----> ESP8266
   ==============================================
   Vcc ---------> 3.3V
   GND ---------> GND
   SIG ---------> A0

    Copyright: (c) Andre Greyling - this is a modified copy of Chris Broekema
  
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
#ifndef config_TEMT6000_h
#define config_TEMT6000_h

extern void setupZsensorTEMT6000();
extern void MeasureLightIntensityTEMT6000();

#define temt6000_always            true // if false only send current value if it has changed
#define TEMT6000LIGHTSENSORPIN     A0 //Ambient light sensor reading = ADC0
#define TimeBetweenReadingtemt6000 10000 //10000 ms

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectTEMT6000toMQTT "/LIGHTtoMQTT"

//Time used to wait for an interval before resending measured values
unsigned long timetemt6000 = 0;

#endif