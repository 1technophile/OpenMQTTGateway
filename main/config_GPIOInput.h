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
#ifndef config_GPIOInput_h
#define config_GPIOInput_h

extern void setupGPIOInput();
extern void GPIOInputtoMQTT();
extern void MeasureGPIOInput();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectGPIOInputtoMQTT "/GPIOInputtoMQTT"
#define GPIOInputDebounceDelay 60 //debounce time, increase if there are issues

/*-------------------PIN DEFINITIONS----------------------*/
#ifndef INPUT_GPIO
#  if defined(ESP8266) || defined(ESP32)
#    define INPUT_GPIO 13
#  else
#    define INPUT_GPIO 7
#  endif
#endif

#ifndef GPIO_INPUT_TYPE
#  define GPIO_INPUT_TYPE INPUT_PULLUP
#endif

#define INPUT_GPIO_ON_VALUE  "HIGH"
#define INPUT_GPIO_OFF_VALUE "LOW"

#endif
