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
#define subjectRS232toMQTT "/RS232toMQTT"

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
#  define RS232toMQTTmode 0
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
#ifndef RS232_UART
// set hardware serial UART to be used for device communication or
// use software emulaton if not defined
//
// VALUES:
//  -  not defined: use software emulation (pins set by RS232_RX_GPIO & RS232_TX_GPIO)
//
//  -  0: use HW UART0 (serial), warning: default used for logging & usb
//        ESP8266:     (TX0 GPIO1,  RX0 GPIO3)
//                     (TX0 GPIO15, RX0 GPIO13) with UART0 swap enabled
//        ESP32:       (TX0 by RS232_TX_GPIO, RX0 by RS232_RX_GPIO)
//        ATmega2560:  (TX0 pin1, RX0 pin0)
//        Arduino Uno: (TX0 pin1, RX0 pin0)
//
//  -  1: use HW UART1 (serial1)
//        ESP8266:     (TX1 GPIO2,  RX none) only transmit available
//        ESP32:       (TX1 by RS232_TX_GPIO,  RX1 by RS232_RX_GPIO)
//        ATmega2560:  (TX1 pin18, RX1 pin19)
//        Arduino Uno: N/A
//
//  -  2: use HW UART2 (serial2)
//        ESP8266:     N/A
//        ESP32:       (TX2 by RS232_TX_GPIO,  RX2 by RS232_RX_GPIO)
//        ATmega2560:  (TX2 pin16, RX1 pin17)
//        Arduino Uno: N/A
//
//  -  3: use HW UART3 (serial3)
//        ESP8266:     N/A
//        ESP32:       N/A
//        ATmega2560:  (TX2 pin14, RX1 pin15)
//        Arduino Uno: N/A
//
// defaults
#  ifdef ESP32
#    define RS232_UART 1 // use HW UART ESP32
#  else
#    undef RS232_UART // default use software serial
//#  define RS232_UART 1 // define to use HW UART
#  endif
#endif

#ifndef RS232_UART0_SWAP
// option for ESP8266 only to swap UART0 ports from (GPIO1,GPIO3) to (GPIO15,GPIO13)
#  define RS232_UART0_SWAP
#endif

#ifndef RS232_RX_GPIO
// define receive pin (for software serial or ESP32 with configurable HW UART)
#  if defined(ESP8266) && !defined(RS232_UART)
#    define RS232_RX_GPIO 4 //D2
#  elif defined(ESP32)
#    define RS232_RX_GPIO 26
#  elif defined(__AVR_ATmega2560__) && !defined(RS232_UART)
#    define RS232_RX_GPIO 2 // 2 = D2 on arduino mega
#  else
#    define RS232_RX_GPIO 0 // 0 = D2 on arduino UNO
#  endif
#endif

#ifndef RS232_TX_GPIO
// define transmit pin (for software serial and/or ESP32)
#  if defined(ESP8266) && !defined(RS232_UART)
#    define RS232_TX_GPIO 2 //D4
#  elif ESP32
#    define RS232_TX_GPIO 14
#  elif defined(__AVR_ATmega2560__) && !defined(RS232_UART)
#    define RS232_TX_GPIO 9
#  else
#    define RS232_TX_GPIO 9
#  endif
#endif

#endif