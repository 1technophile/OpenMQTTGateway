/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the DHT11/22 sensor
  
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
#ifndef config_DHT_h
#define config_DHT_h

extern void setupDHT();
extern void MeasureTempAndHum();

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define DHTTOPIC              "/DHTtoMQTT/dht1"
#define dht_always            true // if false when the current value for temp or hum is the same as previous one don't send it by MQTT
#define TimeBetweenReadingDHT 30000 // time between 2 DHT readings
/*-------------DHT SENSOR TYPE-------------*/
//#define DHT_SENSOR_TYPE DHT11 //uncomment for DHT11 Sensor
//#define DHT_SENSOR_TYPE DHT21 //uncomment for DHT21 Sensor
#ifndef DHT_SENSOR_TYPE
#  define DHT_SENSOR_TYPE DHT22 //uncomment for DHT22 Sensor (default for backwards compatibility)
#endif
/*-------------------PIN DEFINITIONS----------------------*/
#ifndef DHT_RECEIVER_GPIO
#  if defined(ESP8266)
#    define DHT_RECEIVER_GPIO 5 //5 = D1 you can put 14 = D5 if you don't use HCSR501 sensor and the RFM69
#  elif defined(ESP32)
#    define DHT_RECEIVER_GPIO 16
#  else
#    define DHT_RECEIVER_GPIO 8
#  endif
#endif

#endif