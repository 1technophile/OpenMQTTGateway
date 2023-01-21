/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your RS232 device and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the RS232 gateway 
  
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
#ifndef config_RS232_h
#define config_RS232_h

extern void setupRS232();
extern void RS232toMQTT();
extern void MQTTtoRS232(char* topicOri, JsonObject& RS232data);

/*-------------------RS232 topics & parameters----------------------*/

// Settings RS232 to MQTT topic
#define RS232toMQTTsubject "/RS232toMQTT"

// setting to specify mode used for sending MQTT messages:
//   0: RAW: all recieved input at RS232 interface is collected in single MQTT message
//      defined by RS232toMQTTsubject
//
//   1: JSON: Assumes input at RS232 interface to be valid JSON. JSON keys are used to
//      split the JSON data in separate MQTT sub-topics. This will be repeated for
//      sub-keys up to the specified nesting level (RS232maxJSONlevel).
//
//      EXAMPLE: "{temperature: {sens1: 22, sens2: 23}, humidity: {sens1: 80, sens2: 60}}"
//        - with RS232maxJSONlevel=1:
//            ./RS232toMQTT/temperature  ==> "{sens1: 22, sens2: 23}"
//            ./RS232toMQTT/humidity     ==> "{sens1: 80, sens2: 60}"
//
//        - with RS232maxJSONlevel=2 (or higher):
//            ./RS232toMQTT/temperature/sens1 ==> 22
//            ./RS232toMQTT/temperature/sens2 ==> 23
//            ./RS232toMQTT/humidity/sens1 ==> 80
//            ./RS232toMQTT/humidity/sens2 ==> 60
//
#ifndef RS232toMQTTmode
#  define RS232toMQTTmode 1
#endif

// settings for RS232TopicMode 0 (RAW)
#define RS232InPost '\n' // Hacky way to get last character of postfix for incoming
#define MAX_INPUT   200 // how much serial data we expect

// settings for RS232TopicMode 1 (JSON)
#define RS232maxJSONlevel 2 // Max nested level in which JSON keys are converted to seperate sub-topics
#define RS232JSONDocSize  1024 // bytes to reserve for the JSON doc

// settings for MQTT to RS232
#define subjectMQTTtoRS232 "/commands/MQTTtoRS232"
#define RS232Pre           "00" // The prefix for the RS232 message
#define RS232Post          "\r" // The postfix for the RS232 message

//Setup for RS232
#ifndef RS232Baud
#  define RS232Baud 9600 // The serial connection Baud
#endif

/*-------------------PIN DEFINITIONS----------------------*/
#ifndef RS232_RX_GPIO
#  ifdef ESP8266
#    define RS232_RX_GPIO 4 //D2
#  elif ESP32
#    define RS232_RX_GPIO 26
#  elif __AVR_ATmega2560__
#    define RS232_RX_GPIO 2 // 2 = D2 on arduino mega
#  else
#    define RS232_RX_GPIO 0 // 0 = D2 on arduino UNO
#  endif
#endif

#ifndef RS232_TX_GPIO
#  ifdef ESP8266
#    define RS232_TX_GPIO 2 //D4
#  elif ESP32
#    define RS232_TX_GPIO 14
#  elif __AVR_ATmega2560__
#    define RS232_TX_GPIO 9
#  else
#    define RS232_TX_GPIO 9
#  endif
#endif

#endif
