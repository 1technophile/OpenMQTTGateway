/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to set your parameters for the 2G gateway

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
#ifndef config_2G_h
#define config_2G_h

extern void setup2G();
extern bool _2GtoMQTT();
extern void MQTTto2G(char* topicOri, char* datacallback);
extern void MQTTto2G(char* topicOri, JsonObject& SMSdata);
/*-------------------2G topics & parameters----------------------*/
//433Mhz MQTT Subjects and keys
#define subjectMQTTto2G    "/commands/MQTTto2G"
#define subject2GtoMQTT    "/2GtoMQTT"
#define subjectGTW2GtoMQTT "/2GtoMQTT"
#define _2GPhoneKey        "PHO_" // phone number define the phone number to send the SMS MQTT->2G

#define _2G_MODULE_BAUDRATE 9600
#define _2G_MIN_SIGNAL      30
#define _2G_MAX_SIGNAL      1000

/*-------------------PIN DEFINITIONS----------------------*/
#if !defined(_2G_TX_GPIO) || !defined(_2G_RX_GPIO) || !defined(_2G_PWR_GPIO)
#  ifdef ESP8266
#    define _2G_TX_GPIO  D6 //D6 to A6 RX,
#    define _2G_RX_GPIO  D7 //D7 to A6 TX
#    define _2G_PWR_GPIO D5 // connect a MOSFET to power on and off your A6/7 module
#  elif defined(ESP32)
#    define _2G_TX_GPIO  16 //D16 to A6 RX,
#    define _2G_RX_GPIO  17 //D17 to A6 TX
#    define _2G_PWR_GPIO 5 // connect a MOSFET to power on and off your A6/7 module
#  else
#    define _2G_TX_GPIO  6 //D6 to A6 RX,
#    define _2G_RX_GPIO  7 //D7 to A6 TX
#    define _2G_PWR_GPIO 5 // connect a MOSFET to power on and off your A6/7 module
#  endif
#endif

#endif
