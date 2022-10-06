/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    HELTEC ESP32 LORA - SSD1306 / Onboard 0.96-inch 128*64 dot matrix OLED display
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - 1technophile
    - NorthernMan54
  
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
#ifndef config_HELTEC_h
#define config_HELTEC_h
#ifdef ZboardHELTEC
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#endif

#define OLEDPrint(a,b,c) HELTECPrint((String) a)

extern void setupHELTEC();
extern void loopHELTEC();
/*----------------------------USER PARAMETERS-----------------------------*/
/*---------------DEFINE SCREEN BRIGHTNESS------------------*/
#ifdef ZboardHELTECTOUGH // Sleep brightness doesn't seem to work for the moment on Tough
#  ifndef SLEEP_LCD_BRIGHTNESS
#    define SLEEP_LCD_BRIGHTNESS 15 // 0 to 100
#  endif
#else
#  ifndef SLEEP_LCD_BRIGHTNESS
#    define SLEEP_LCD_BRIGHTNESS 2 // 0 to 100
#  endif
#endif
#ifndef NORMAL_LCD_BRIGHTNESS
#  define NORMAL_LCD_BRIGHTNESS 100 // 0 to 100
#endif
/*---------------DEFINE SLEEP BUTTON------------------*/
#ifndef SLEEP_BUTTON
#  define SLEEP_BUTTON 33
#endif
/*-------------------DEFINE LOG LEVEL----------------------*/
#define LOG_LEVEL_LCD LOG_LEVEL_TRACE // if we go down below warning the size of the text to display can make the HELTEC restarting
#define LOG_TO_LCD    true //set to false if you want to use serial monitor for the log per default instead of the HELTEC screen
/*-------------------DEFINE MQTT TOPIC FOR CONFIG----------------------*/
#define subjectMQTTtoHELTECset "/commands/MQTTtoHELTEC/config"

#endif