/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
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
extern void LORAtoX();
extern void XtoLORA(const char* topicOri, const char* datacallback);
extern void XtoLORA(const char* topicOri, JsonObject& RFdata);
/*----------------------LORA topics & parameters-------------------------*/
#define subjectLORAtoMQTT    "/LORAtoMQTT"
#define subjectMQTTtoLORA    "/commands/MQTTtoLORA"
#define subjectGTWLORAtoMQTT "/LORAtoMQTT"
#define subjectMQTTtoLORAset "/commands/MQTTtoLORA/config"

//Default parameters used when the parameters are not set in the json data
#ifndef LORA_BAND
#  define LORA_BAND 868E6
#endif
#ifndef LORA_SIGNAL_BANDWIDTH
#  define LORA_SIGNAL_BANDWIDTH 125E3
#endif
#ifndef LORA_TX_POWER
#  define LORA_TX_POWER 14
#endif
#ifndef LORA_SPREADING_FACTOR
#  define LORA_SPREADING_FACTOR 7
#endif
#ifndef LORA_CODING_RATE
#  define LORA_CODING_RATE 5
#endif
#ifndef LORA_PREAMBLE_LENGTH
#  define LORA_PREAMBLE_LENGTH 8
#endif
#ifndef LORA_SYNC_WORD
#  define LORA_SYNC_WORD 0x12
#endif
#ifndef DEFAULT_CRC
#  define DEFAULT_CRC true
#endif
#ifndef INVERT_IQ
#  define INVERT_IQ false
#endif
#ifndef LORA_ONLY_KNOWN
#  define LORA_ONLY_KNOWN false
#endif

#define repeatLORAwMQTT false // do we repeat a received signal by using MQTT with LORA gateway

/*-------------------PIN DEFINITIONS----------------------*/

//TTGO LORA BOARD ESP32 PIN DEFINITION

#ifndef LORA_SCK
#  define LORA_SCK 5 // GPIO5  -- SX1278's SCK
#endif

#ifndef LORA_MISO
#  define LORA_MISO 19 // GPIO19 -- SX1278's MISO
#endif

#ifndef LORA_MOSI
#  define LORA_MOSI 27 // GPIO27 -- SX1278's MOSI
#endif

#ifndef LORA_SS
#  define LORA_SS 18 // GPIO18 -- SX1278's CS
#endif

#ifndef LORA_RST
#  define LORA_RST 14 // GPIO14 -- SX1278's RESET
#endif

#ifndef LORA_DI0
#  define LORA_DI0 26 // GPIO26 -- SX1278's IRQ(Interrupt Request)
#endif

struct LORAConfig_s {
  long frequency;
  int txPower;
  int spreadingFactor;
  long signalBandwidth;
  int codingRateDenominator;
  int preambleLength;
  byte syncWord;
  bool crc;
  bool invertIQ;
  bool onlyKnown;
};

#ifdef ZmqttDiscovery
extern void launchLORADiscovery(bool overrideDiscovery);
// This structure stores the entities of the devices and is they have been discovered or not
// The uniqueId is composed of the device id + the key

#  define uniqueIdSize  30
#  define modelNameSize 30

struct LORAdevice {
  char uniqueId[uniqueIdSize];
  char modelName[modelNameSize];
  bool isDisc;
};

const char LORAparameters[5][4][12] = {
    // LORA key, name, unit, device_class
    {"tempc", "temperature", "°C", "temperature"},
    {"hum", "humidity", "%", "humidity"},
    {"moi", "moisture", "%", "humidity"},
    {"batt", "battery", "%", "battery"},
    {"count", "counter", "L", "water"}};

#endif

#endif
