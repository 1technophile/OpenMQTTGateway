/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This is the MQ2 GAS Sensor Addon based on modules with a MQ:
   - Measures flammable gas
   - Required Hardware Module: MQ2

   Connection Scheme:
   --------------------

   MQ-2 -------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 3.3V or 5V------------> 3.3V or 5V
   GND ---------> GND ------------------> GND
   A0 ----------> Pin A0 ---------------> A0
   D0 ----------> Pin D4 ---------------> D4
  
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
#ifndef config_MQ2_h
#define config_MQ2_h

extern void setupZsensorMQ2();
extern void MQ2toMQTT();

#ifndef MQ2SENSORADCPIN
#  ifdef ESP32
#    define MQ2SENSORADCPIN 5 //MQ2 Gas sensor reading = ADC0
#  else
#    define MQ2SENSORADCPIN A0 //MQ2 Gas sensor reading = ADC0
#  endif
#endif

#ifndef MQ2SENSORDETECTPIN
#  ifdef ESP32
#    define MQ2SENSORDETECTPIN 24 //MQ2 Gas sensor detected flag
#  else
#    define MQ2SENSORDETECTPIN D4 //MQ2 Gas sensor detected flag
#  endif
#endif

#ifndef TimeBetweenReadingmq2
#  define TimeBetweenReadingmq2 10000
#endif
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectMQ2toMQTT "/GAStoMQTT/mq2"

//Time used to wait for an interval before resending measured values
unsigned long timemq2 = 0;

#endif