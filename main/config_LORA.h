/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the LORA gateway
  
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
#ifndef config_LORA_h
#define config_LORA_h

extern void setupLORA();
extern void LORAtoMQTT();
extern void MQTTtoLORA(char* topicOri, char* datacallback);
extern void MQTTtoLORA(char* topicOri, JsonObject& RFdata);
/*----------------------LORA topics & parameters-------------------------*/
#define subjectLORAtoMQTT    "/LORAtoMQTT"
#define subjectMQTTtoLORA    "/commands/MQTTtoLORA"
#define subjectGTWLORAtoMQTT "/LORAtoMQTT"

//Default parameters used when the parameters are not set in the json data
#ifndef LORA_BAND
#  define LORA_BAND 868E6
#endif
#define LORA_SIGNAL_BANDWIDTH 125E3
#define LORA_TX_POWER         17
#define LORA_SPREADING_FACTOR 7
#define LORA_CODING_RATE      5
#define LORA_PREAMBLE_LENGTH  8
#define LORA_SYNC_WORD        0x12
#define DEFAULT_CRC           true
#define INVERT_IQ             false

#define repeatLORAwMQTT false // do we repeat a received signal by using MQTT with LORA gateway

/*-------------------PIN DEFINITIONS----------------------*/
//TTGO LORA BOARD ESP32 PIN DEFINITION
#define LORA_SCK  5 // GPIO5  -- SX1278's SCK
#define LORA_MISO 19 // GPIO19 -- SX1278's MISO
#define LORA_MOSI 27 // GPIO27 -- SX1278's MOSI
#define LORA_SS   18 // GPIO18 -- SX1278's CS
#define LORA_RST  14 // GPIO14 -- SX1278's RESET
#define LORA_DI0  26 // GPIO26 -- SX1278's IRQ(Interrupt Request)

#endif