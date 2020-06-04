/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your RF/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT
   
   This file sets parameters for the integration of 1-wire DS1820 temperature sensors
  
    Copyright (c) 2020 Lars Wessels
  
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
#ifndef config_DS1820_h
#define config_DS1820_h

extern void setupZsensorDS1820();
extern void DS1820toMQTT();
extern void pubOneWire_HADiscovery();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define OW_TOPIC    "/OneWiretoMQTT/ds1820"
#define OW_MAX_SENSORS 8 // query max. sensors on 1-wire bus
#define DS1820_ALWAYS  true // if false only published current temperature if has changed from previous reading
#define DS1820_INTERVAL_SEC 60 // time between DS1820 readings (seconds)
#define DS1820_RESOLUTION  10  // conversion times: 9 bit (93.75 ms), 10 bit (187.5 ms), 11 bit (375 ms), 12 bit (750 ms)
#ifndef DS1820_FAHRENHEIT
  #define DS1820_FAHRENHEIT  false // defaults to Celcius
#endif
#define DS1820_DETAILS  true // publish extented info for each sensor (resolution, address, type)
#define DS1820_CONV_TIME  2000  // trigger conversion before requesting temperature readings (ms)
/*-------------------PIN DEFINITIONS----------------------*/

#ifndef DS1820_OWBUS_PIN
  #if defined(ESP8266)
    #define DS1820_OWBUS_PIN 2
  #elif defined(ESP32)
    #define DS1820_OWBUS_PIN 2
  #else
    #define DS1820_OWBUS_PIN 2
  #endif
#endif

#endif
