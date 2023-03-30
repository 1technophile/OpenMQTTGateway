/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   This files enables you to set your parameters for the FASTLED actuator 
  
    Copyright: (c)
  
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

#ifndef config_FASTLED_h
#define config_FASTLED_h

extern void setupFASTLED();
extern void FASTLEDLoop();
extern void MQTTtoFASTLED(char*, char*);
extern void MQTTtoFASTLED(char*, JsonObject&);
/*-------------------FASTLED topics & parameters----------------------*/
//FASTLED MQTT Subjects
#define subjectMQTTtoFASTLED              "/commands/MQTTtoFASTLED"
#define subjectMQTTtoFASTLEDsetled        "/commands/MQTTtoFASTLED/setled" //set only one LED with JSON struct {"led":0-x,"hex":"#000000","blink":true/false}
#define subjectMQTTtoFASTLEDsetbrightness "/commands/MQTTtoFASTLED/setbrightness" //set the brightness 0-255
#define subjectMQTTtoFASTLEDsetanimation  "/commands/MQTTtoFASTLED/setanimation" //Animation Fire2012 by Mark Kriegsman
#define subjectGTWFASTLEDtoMQTT           "/FASTLEDtoMQTT" //same color on all LEDs in #RRGGBB

// How many leds in your strip?
#define FASTLED_NUM_LEDS 16

// Uncomment/edit one of the following lines for your LEDs arrangement.

//#define FASTLED_TYPE TM1803
//#define FASTLED_TYPE TM1804
//#define FASTLED_TYPE TM1809
//#define FASTLED_TYPE WS2811
//#define FASTLED_TYPE WS2812
//#define FASTLED_TYPE WS2812B
#define FASTLED_TYPE NEOPIXEL
//#define FASTLED_TYPE APA104
//#define FASTLED_TYPE UCS1903
//#define FASTLED_TYPE UCS1903B
//#define FASTLED_TYPE GW6205

//#define FASTLED_TYPE WS2801, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE SM16716, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE LPD8806, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE P9813, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE APA102, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE DOTSTAR, RGB>(leds, NUM_LEDS);

//#define FASTLED_TYPE WS2801, DATA_GPIO, CLOCK_GPIO, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE SM16716, DATA_GPIO, CLOCK_GPIO, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE LPD8806, DATA_GPIO, CLOCK_GPIO, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE P9813, DATA_GPIO, CLOCK_GPIO, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE APA102, DATA_GPIO, CLOCK_GPIO, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE DOTSTAR, DATA_GPIO, CLOCK_GPIO, RGB>(leds, NUM_LEDS);

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_GPIO.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_GPIO and CLOCK_GPIO
//#define DATA_GPIO 3
//#define CLOCK_GPIO 13

#ifdef ESP8266
//#define FASTLED_ESP8266_RAW_GPIO_ORDER
//#define FASTLED_ESP8266_NODEMCU_GPIO_ORDER
#  define FASTLED_ESP8266_D1_GPIO_ORDER
#  define FASTLED_DATA_GPIO D2 // only D2 works by me
//#define FASTLED_CLOCK_GPIO 13
#elif ESP32
#  define FASTLED_DATA_GPIO  16
#  define FASTLED_CLOCK_GPIO 13
#else
#  define FASTLED_DATA_GPIO  10
#  define FASTLED_CLOCK_GPIO 13
#endif

#endif
