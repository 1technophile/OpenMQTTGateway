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
/*----------------------LORA topics & parameters-------------------------*/
#define subjectLORAtoMQTT     Base_Topic Gateway_Name "/LORAtoMQTT"
#define subjectMQTTtoLORA     Base_Topic Gateway_Name "/commands/MQTTtoLORA"
#define subjectGTWLORAtoMQTT  Base_Topic Gateway_Name "/LORAtoMQTT"

//Default parameters used when the parameters are not set in the json data
#define LORA_BAND             868E6
#define LORA_SIGNAL_BANDWIDTH 125E3
#define LORA_TX_POWER         17
#define LORA_SPREADING_FACTOR 7
#define LORA_CODING_RATE      5
#define LORA_PREAMBLE_LENGTH  8
#define LORA_SYNC_WORD        0x12


/*-------------------PIN DEFINITIONS----------------------*/
//TTGO LORA BOARD ESP32 PIN DEFINITION
#define LORA_SCK     5    // GPIO5  -- SX1278's SCK
#define LORA_MISO    19   // GPIO19 -- SX1278's MISO
#define LORA_MOSI    27   // GPIO27 -- SX1278's MOSI
#define LORA_SS      18   // GPIO18 -- SX1278's CS
#define LORA_RST     14   // GPIO14 -- SX1278's RESET
#define LORA_DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
