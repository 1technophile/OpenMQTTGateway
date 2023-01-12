/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to use M5Stick C board components (display)
  
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
#ifndef config_M5_h
#define config_M5_h

#ifdef ZboardM5STICKC
#  include <M5StickC.h>
#endif
#ifdef ZboardM5STICKCP
#  include <M5StickCPlus.h>
#endif
#ifdef ZboardM5STACK
#  include <M5Stack.h>
#endif
#ifdef ZboardM5TOUGH
#  include <M5Tough.h>
#endif

extern void setupM5();
extern void loopM5();
/*----------------------------USER PARAMETERS-----------------------------*/
/*---------------DEFINE SCREEN BRIGHTNESS------------------*/
#ifdef ZboardM5TOUGH // Sleep brightness doesn't seem to work for the moment on Tough
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
#ifndef LOG_LEVEL_LCD
#  define LOG_LEVEL_LCD LOG_LEVEL_WARNING // Default to only display Warning level messages, if we go down below warning the size of the text to display can make the M5 restarting
#endif
#ifndef LOG_TO_LCD
#  define LOG_TO_LCD true // Default to display log messages on display
#endif
/*-------------------DEFINE MQTT TOPIC FOR CONFIG----------------------*/
#define subjectMQTTtoM5set "/commands/MQTTtoM5/config"

// Simple print macro

// Simple construct for displaying message in lcd and oled displays

#define displayPrint(...) \
  if (lowpowermode < 2) M5Print(__VA_ARGS__) // only print if not in low power mode
#define lpDisplayPrint(...) M5Print(__VA_ARGS__) // print in low power mode

void M5Print(char*, char* = "", char* = "");

#endif