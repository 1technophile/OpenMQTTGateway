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

/*-------------------IR topics & parameters----------------------*/
//RS232 MQTT Subjects
#define subjectGTWRS232toMQTT     "/RS232toMQTT"
#define subjectRS232toMQTT        "/RS232toMQTT"
#define subjectMQTTtoRS232        "/commands/MQTTtoRS232"
#define subjectForwardMQTTtoRS232 "home/gateway2/commands/MQTTtoRS232"

//Setup for RS232
#define MAX_INPUT   50 // how much serial data we expect
#define RS232Baud   9600 // The serial connection Baud
#define RS232Pre    "00" // The prefix for the RS232 message
#define RS232Post   "\r" // The postfix for the RS232 message
#define RS232InPost '\r' // Hacky way to get last character of postfix for incoming

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
