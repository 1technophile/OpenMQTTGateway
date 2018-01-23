/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared FASTLED signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the infrared gateway 
  
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

/*-------------------FASTLED topics & parameters----------------------*/
//FASTLED MQTT Subjects
#define subjectFASTLEDtoMQTT "home/FASTLEDtoMQTT"

// How many leds in your strip?
#define FASTLED_NUM_LEDS 1

// Uncomment/edit one of the following lines for your leds arrangement.

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

//#define FASTLED_TYPE WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE SM16716, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE LPD8806, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE P9813, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
//#define FASTLED_TYPE DOTSTAR, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
//#define DATA_PIN 3
//#define CLOCK_PIN 13

#ifdef ESP8266

#elif ESP32

#else

#endif

/*-------------------PIN DEFINITIONS----------------------*/
// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN

#ifdef ESP8266
  #define FASTLED_ESP8266_RAW_PIN_ORDER
  //#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
  //#define FASTLED_ESP8266_D1_PIN_ORDER
  #define FASTLED_DATA_PIN D8
  //#define FASTLED_CLOCK_PIN 13
#elif ESP32
  #define FASTLED_DATA_PIN 16
  #define FASTLED_CLOCK_PIN 13
#else
  #define FASTLED_DATA_PIN 10
  #define FASTLED_CLOCK_PIN 13
#endif

