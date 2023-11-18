/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the HCSR04 sensor

    Copyright: (c) mpember
    
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

   HC-SR04 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> Vu (5V)
   GND ---------> GND ------------------> GND
   TRI ---------> Pin D6 ---------------> D6
   ECH ---------> Pin D7 ---------------> D7
   
*/
#ifndef config_HCSR04_h
#define config_HCSR04_h

extern void setupHCSR04();
extern void MeasureDistance();

#define HCSR04_always            true // If false, the current value of the parameter is the same as previous one don't send it by MQTT
#define TimeBetweenReadingHCSR04 5000

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectHCSR04 "/DISTtoMQTT/sr04"

/*-------------------PIN DEFINITIONS----------------------*/
#if defined(ESP8266)
#  define HCSR04_TRI_GPIO D6
#  define HCSR04_ECH_GPIO D7
#elif defined(ESP32)
#  define HCSR04_TRI_GPIO 16 // NOT TESTED
#  define HCSR04_ECH_GPIO 17 // NOT TESTED
#else
#  define HCSR04_TRI_GPIO 6 // NOT TESTED
#  define HCSR04_ECH_GPIO 5 // NOT TESTED
#endif

#endif