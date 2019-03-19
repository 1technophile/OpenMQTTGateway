/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR/BLE signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send LORA signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received LORA signal

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
#ifdef ZgatewayLORA

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  

void setupLORA() {
  SPI.begin(LORA_SCK,LORA_MISO,LORA_MOSI,LORA_SS);
  LoRa.setPins(LORA_SS,LORA_RST,LORA_DI0);  
  
  if (!LoRa.begin(LORA_BAND)) {
    trc(F("ZgatewayLORA setup failed!"));
    while (1);
  }
  LoRa.receive();
  trc(F("LORA_SCK"));
  trc(LORA_SCK);
  trc(F("LORA_MISO"));
  trc(LORA_MISO);
  trc(F("LORA_MOSI"));
  trc(LORA_MOSI);
  trc(F("LORA_SS"));
  trc(LORA_SS);
  trc(F("LORA_RST"));
  trc(LORA_RST);
  trc(F("LORA_DI0"));
  trc(LORA_DI0);
  trc(F("ZgatewayLORA setup done"));
   
}

void LORAtoMQTT(){
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject& LORAdata = jsonBuffer.createObject();
    trc(F("Rcv. LORA"));
    #ifdef ESP32
      String taskMessage = "LORA Task running on core ";
      taskMessage = taskMessage + xPortGetCoreID();
      trc(taskMessage);
    #endif
    String packet ;
    packet ="";
    for (int i = 0; i < packetSize; i++) { packet += (char) LoRa.read(); }
    LORAdata.set("rssi", (int)LoRa.packetRssi());
    LORAdata.set("snr",(float)LoRa.packetSnr());
    LORAdata.set("pferror",(float)LoRa.packetFrequencyError());
    LORAdata.set("packetSize", (int)packetSize);
    LORAdata.set("message", (char *)packet.c_str());
    pub(subjectLORAtoMQTT,LORAdata);
    }
}

#ifdef jsonReceiving
  void MQTTtoLORA(char * topicOri, JsonObject& LORAdata) { // json object decoding
   if (strcmp(topicOri,subjectMQTTtoLORA) == 0){
      trc(F("MQTTtoLORA json"));
      const char * message = LORAdata["message"];
      int txPower = LORAdata["txpower"]|LORA_TX_POWER;
      int spreadingFactor = LORAdata["spreadingfactor"]|LORA_SPREADING_FACTOR;
      long int frequency  = LORAdata["frequency "]|LORA_BAND;
      long int signalBandwidth = LORAdata["signalbandwidth"]|LORA_SIGNAL_BANDWIDTH; 
      int codingRateDenominator = LORAdata["codingrate"]|LORA_CODING_RATE;
      int preambleLength = LORAdata["preamblelength"]|LORA_PREAMBLE_LENGTH;
      byte syncWord = LORAdata["syncword"]|LORA_SYNC_WORD;
      bool Crc = LORAdata["enablecrc"];
      if (message) {
        LoRa.setTxPower(txPower);
        LoRa.setFrequency(frequency);
        LoRa.setSpreadingFactor(spreadingFactor);
        LoRa.setSignalBandwidth(signalBandwidth);
        LoRa.setCodingRate4(codingRateDenominator);
        LoRa.setPreambleLength(preambleLength);
        LoRa.setSyncWord(syncWord);
        if (Crc) LoRa.enableCrc();
        LoRa.beginPacket();
        LoRa.print(message);
        LoRa.endPacket();
        trc(F("MQTTtoLORA OK"));
        pub(subjectGTWLORAtoMQTT, LORAdata);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      }else{
        trc(F("MQTTtoLORA Fail json"));
      }
    }
  }
#endif
#ifdef simpleReceiving
  void MQTTtoLORA(char * topicOri, char * LORAdata) { // json object decoding
   if (strcmp(topicOri,subjectMQTTtoLORA) == 0){
        LoRa.beginPacket();
        LoRa.print(LORAdata);
        LoRa.endPacket();
        trc(F("MQTTtoLORA OK"));
        pub(subjectGTWLORAtoMQTT, LORAdata);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    }
  }
#endif
#endif
