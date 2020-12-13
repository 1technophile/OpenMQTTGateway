/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 
   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This actor enables to:
 - receive MQTT data from a topic and send Somfy RTS remote control signals corresponding to the received MQTT data
  
    Copyright (C) 2020 Leon Kiefer

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
#ifndef config_Somfy_h
#define config_Somfy_h

#include <Arduino.h>
#include <ArduinoJson.h>

extern void setupSomfy();
extern void MQTTtoSomfy(char* topicOri, JsonObject& RFdata);
/*----------------------------USER PARAMETERS-----------------------------*/
#define EEPROM_ADDRESS_START 0
#define SOMFY_REMOTE_NUM     1

// Do not change the order of the remotes, because based on the index the rolling codes are stored
const uint32_t somfyRemotes[SOMFY_REMOTE_NUM] = {0x5184c8};

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectMQTTtoSomfy "/commands/MQTTtoSomfy"

/*-------------------INTERNAL DEFINITIONS----------------------*/
#define CC1101_FREQUENCY_SOMFY 433.42

#endif
