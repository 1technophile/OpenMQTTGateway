/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a wifi or ethernet gateway between your SERIAL device and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the SERIAL gateway 
  
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
#ifndef config_SERIAL_h
#define config_SERIAL_h

extern void setupSERIAL();
extern void SERIALtoMQTT();
extern void MQTTtoSERIAL(char* topicOri, JsonObject& SERIALdata);

/*-------------------SERIAL topics & parameters----------------------*/

// Settings SERIAL to MQTT topic
#define subjectSERIALtoMQTT "/SERIALtoMQTT"

// setting to specify mode used for sending MQTT messages:
//   0: RAW: all recieved input at SERIAL interface is collected in single MQTT message
//      defined by SERIALtoMQTTsubject
//
//   1: JSON: Assumes input at SERIAL interface to be valid JSON. JSON keys are used to
//      split the JSON data in separate MQTT sub-topics. This will be repeated for
//      sub-keys up to the specified nesting level (SERIALmaxJSONlevel).
//
//      EXAMPLE: "{temperature: {sens1: 22, sens2: 23}, humidity: {sens1: 80, sens2: 60}}"
//        - with SERIALmaxJSONlevel=1:
//            ./SERIALtoMQTT/temperature  ==> "{sens1: 22, sens2: 23}"
//            ./SERIALtoMQTT/humidity     ==> "{sens1: 80, sens2: 60}"
//
//        - with SERIALmaxJSONlevel=2 (or higher):
//            ./SERIALtoMQTT/temperature/sens1 ==> 22
//            ./SERIALtoMQTT/temperature/sens2 ==> 23
//            ./SERIALtoMQTT/humidity/sens1 ==> 80
//            ./SERIALtoMQTT/humidity/sens2 ==> 60
//
#ifndef SERIALtoMQTTmode
#  define SERIALtoMQTTmode 0
#endif

// settings for SERIALTopicMode 0 (RAW)
#define SERIALInPost '\n' // Hacky way to get last character of postfix for incoming
#define MAX_INPUT    JSON_MSG_BUFFER // how much serial data we expect

// settings for SERIALTopicMode 1 (JSON)
#define SERIALmaxJSONlevel 2 // Max nested level in which JSON keys are converted to seperate sub-topics

// settings for MQTT to SERIAL
#define subjectMQTTtoSERIAL "/commands/MQTTtoSERIAL"
#ifndef SERIALPre
#  define SERIALPre "00" // The prefix for the SERIAL message
#endif
#ifndef SERIALPost
#  define SERIALPost "\r" // The postfix for the SERIAL message
#endif

//Setup for SERIAL
#ifndef SERIALBaud
#  define SERIALBaud 9600 // The serial connection Baud
#endif

/*-------------------PIN DEFINITIONS----------------------*/
#ifndef SERIAL_UART
// set hardware serial UART to be used for device communication or
// use software emulaton if not defined
//
// VALUES:
//  -  not defined: use software emulation (pins set by SERIAL_RX_GPIO & SERIAL_TX_GPIO)
//
//  -  0: use HW UART0 (serial), warning: default used for logging & usb
//        ESP8266:     (TX0 GPIO1,  RX0 GPIO3)
//                     (TX0 GPIO15, RX0 GPIO13) with UART0 swap enabled
//        ESP32:       (TX0 by SERIAL_TX_GPIO, RX0 by SERIAL_RX_GPIO)
//
//  -  1: use HW UART1 (serial1)
//        ESP8266:     (TX1 GPIO2,  RX none) only transmit available
//        ESP32:       (TX1 by SERIAL_TX_GPIO,  RX1 by SERIAL_RX_GPIO)
//
//  -  2: use HW UART2 (serial2)
//        ESP8266:     N/A
//        ESP32:       (TX2 by SERIAL_TX_GPIO,  RX2 by SERIAL_RX_GPIO)
//
//  -  3: use HW UART3 (serial3)
//        ESP8266:     N/A
//        ESP32:       N/A
//
// defaults
#  ifdef ESP32
#    define SERIAL_UART 1 // use HW UART ESP32
#  else
#    undef SERIAL_UART // default use software serial
//#  define SERIAL_UART 1 // define to use HW UART
#  endif
#endif

#ifndef SERIAL_UART0_SWAP
// option for ESP8266 only to swap UART0 ports from (GPIO1,GPIO3) to (GPIO15,GPIO13)
#  define SERIAL_UART0_SWAP
#endif

#ifndef SERIAL_RX_GPIO
// define receive pin (for software serial or ESP32 with configurable HW UART)
#  if defined(ESP8266) && !defined(SERIAL_UART)
#    define SERIAL_RX_GPIO 4 //D2
#  elif defined(ESP32)
#    define SERIAL_RX_GPIO 26
#  endif
#endif

#ifndef SERIAL_TX_GPIO
// define transmit pin (for software serial and/or ESP32)
#  if defined(ESP8266) && !defined(SERIAL_UART)
#    define SERIAL_TX_GPIO 2 //D4
#  elif ESP32
#    define SERIAL_TX_GPIO 14
#  endif
#endif

#endif