/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the GPIOInput sensor
  
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
#ifndef config_GPIOMultiInput_h
#define config_GPIOMultiInput_h

extern void setupGPIOMultiInput();
extern void GPIOMInputtoMQTT();
void publishMeasureGPIOMultiInput(int _input_state, int GPIO_number);

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectGPIOMultiInputtoMQTT "/GPIOMultiInputtoMQTT"
#define GPIOMultiInputDebounceDelay 60 //debounce time, increase if there are issues

/*-------------------PIN DEFINITIONS----------------------*/
#ifndef INPUT_GPIO_1
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO_1 12
#  endif
#endif

#ifndef INPUT_GPIO_2
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO_2 25
#  endif
#endif

#ifndef INPUT_GPIO_3
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO_3 33
#  endif
#endif

#ifndef INPUT_GPIO_4
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO_4 32
#  endif
#endif

#ifndef INPUT_GPIO_5
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO_5 35
#  endif
#endif

#ifndef INPUT_GPIO_6
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO_6 34
#  endif
#endif

#ifndef INPUT_GPIO_7
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO_7 39
#  endif
#endif

#ifndef INPUT_GPIO_8
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO_8 36
#  endif
#endif

#define MULTI_INPUT_GPIO_ON_VALUE  "HIGH"
#define MULTI_INPUT_GPIO_OFF_VALUE "LOW"

#endif
