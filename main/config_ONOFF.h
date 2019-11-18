/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the ON OFF actuator
  
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
#ifndef config_ONOFF_h
#define config_ONOFF_h

extern void setupONOFF();
extern void MQTTtoONOFF(char * topicOri, char * datacallback);
extern void MQTTtoONOFF(char * topicOri, JsonObject& RFdata);
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectMQTTtoONOFF  Base_Topic Gateway_Name "/commands/MQTTtoONOFF"
#define subjectGTWONOFFtoMQTT  Base_Topic Gateway_Name "/ONOFFtoMQTT"

#define ONKey "setON"
#define OFFKey "setOFF"

/*-------------------PIN DEFINITIONS----------------------*/
// default pin, if not set into the MQTT json
#ifndef ACTUATOR_ONOFF_PIN
    #ifdef ESP8266
            #define ACTUATOR_ONOFF_PIN 15 //12 for sonoff basic relay
    #elif ESP32
        #define ACTUATOR_ONOFF_PIN 15 
    #else
        #define ACTUATOR_ONOFF_PIN 13
    #endif
#endif

#endif