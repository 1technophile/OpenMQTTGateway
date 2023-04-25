/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your RF/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT
   
    This file sets parameters for the integration of C-37 YL-83 HM-RD water detection sensors
  
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
#ifndef config_C37_YL83_HMRD_h
#define config_C37_YL83_HMRD_h

extern void setupZsensorC37_YL83_HMRD();
extern void C37_YL83_HMRDtoMQTT();
extern void MeasureC37_YL83_HMRDWater();

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define C37_YL83_HMRD_TOPIC  "/WATERtoMQTT/c37_yk83_hmrd"
#define C37_YL83_HMRD_ALWAYS true // if false only published current water leak detection if has changed from previous reading
#ifndef C37_YL83_HMRD_INTERVAL_SEC
#  define C37_YL83_HMRD_INTERVAL_SEC 10000 // time between C-37, YL-83, HM-RD readings (ms)
#endif
#ifndef C37_YL83_HMRD_RESOLUTION
#  define C37_YL83_HMRD_RESOLUTION 10 // conversion times: 9 bit (93.75 ms), 10 bit (187.5 ms), 11 bit (375 ms), 12 bit (750 ms)
#endif
/*-------------------PIN DEFINITIONS----------------------*/

#ifndef C37_YL83_HMRD_Digital_GPIO
#  if defined(ESP8266)
#    define C37_YL83_HMRD_Digital_GPIO 14
#  elif defined(ESP32)
#    define C37_YL83_HMRD_Digital_GPIO GPIO_NUM_14
#  else
#    define C37_YL83_HMRD_Digital_GPIO 14
#  endif
#endif

#ifndef C37_YL83_HMRD_Analog_GPIO
#  if defined(ESP8266)
#    define C37_YL83_HMRD_Analog_GPIO A0
#  elif defined(ESP32)
#    define C37_YL83_HMRD_Analog_GPIO GPIO_NUM_35
#  else
#    define C37_YL83_HMRD_Analog_GPIO A0
#  endif
#endif

#endif
