/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the HC SR-501 sensor
  
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
#ifndef config_HCSR501_h
#define config_HCSR501_h

extern void setupHCSR501();
extern void HCSR501toX();
extern void MeasureHCSR501();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectHCSR501toMQTT "/HCSR501toMQTT"
//#define HCSR501_LED_NOTIFY_GPIO 4 //Uncomment this line to mirror the state of the PIR sensor to the specified GPIO

#if defined(HCSR501_LED_NOTIFY_GPIO) && !defined(HCSR501_LED_ON)
#  define HCSR501_LED_ON HIGH
#endif

#ifndef TimeBeforeStartHCSR501
#  define TimeBeforeStartHCSR501 60000 //define the time necessary for HC SR501 init
#endif

/*-------------------PIN DEFINITIONS----------------------*/
#ifndef HCSR501_GPIO
#  if defined(ESP8266)
#    define HCSR501_GPIO D5
#  elif defined(ESP32)
#    define HCSR501_GPIO 5
#  else
#    define HCSR501_GPIO 7
#  endif
#endif

#endif