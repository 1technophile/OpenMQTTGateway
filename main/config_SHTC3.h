/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the SHTC311/22 sensor
  
    Copyright: (c)Florian ROBERT
  
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
#ifndef config_SHTC3_h
#define config_SHTC3_h

extern void setupSHTC3();
extern void SHTC3toMQTT();

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define SHTC3TOPIC              "/SHTC3toMQTT/shtc31"
#define shtc3_always            true // if false when the current value for temp or hum is the same as previous one don't send it by MQTT
#define TimeBetweenReadingSHTC3 30000 // time between 2 SHTC3 readings
/*-------------SHTC3 SENSOR TYPE-------------*/
//#define SHTC3_SENSOR_TYPE SHTC311 //uncomment for SHTC311 Sensor
//#define SHTC3_SENSOR_TYPE SHTC321 //uncomment for SHTC321 Sensor
#define SHTC3_SENSOR_TYPE SHTC322 //uncomment for SHTC322 Sensor (default for backwards compatibility)
/*-------------------PIN DEFINITIONS----------------------*/
#ifndef SHTC3_RECEIVER_GPIO
#  if defined(ESP8266)
#    define SHTC3_RECEIVER_GPIO 5 //5 = D1 you can put 14 = D5 if you don't use HCSR501 sensor and the RFM69
#  elif defined(ESP32)
#    define SHTC3_RECEIVER_GPIO 16
#  else
#    define SHTC3_RECEIVER_GPIO 8
#  endif
#endif

#endif