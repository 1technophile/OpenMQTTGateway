/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the GPIOKeyCode multi input
  
    Copyright: (c)Grzegorz Rajtar
  
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

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectGPIOKeyCodetoMQTT    Base_Topic Gateway_Name "/keycode"
#define subjectGPIOKeyCodeStatetoMQTT subjectGPIOKeyCodetoMQTT "/status"
#define GPIOKeyCodeDebounceDelay 60 //debounce time, increase if there are issues

/*-------------------PIN DEFINITIONS----------------------*/
#if defined(ESP8266) || defined(ESP32)

#ifndef GPIOKeyCode_LATCH_PIN
  #define GPIOKeyCode_LATCH_PIN 12 //D6
#endif  
#ifndef GPIOKeyCode_D0_PIN
  #define GPIOKeyCode_D0_PIN 14 //D5
#endif  
#ifndef GPIOKeyCode_D1_PIN
  #define GPIOKeyCode_D1_PIN 5 //D1
#endif  
#ifndef GPIOKeyCode_D2_PIN
  #define GPIOKeyCode_D2_PIN 13 //D7
#endif  
#ifndef GPIOKeyCode_D3_PIN
  #define GPIOKeyCode_D3_PIN XX //??
#endif  
#else
// must define !!!  
#endif

