/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - publish MQTT data to a different topic related to received 433Mhz signal DIO/new kaku protocol

    Receiving cannot be used with this gateway as it is not currently compatible with pubsubclient, emitting can be used with both RF gateway on the same program.
    Note that repeater and acknowledgement are not available with this gateway.
    -->So as to know the code of your devices use ShowReceivedCode from https://github.com/1technophile/NewRemoteSwitch
    
    Copyright: (c)Florian ROBERT
    Copyright: (c)Randy Simons http://randysimons.nl/
  
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

/*Command example for switching off:
sudo mosquitto_pub -t home/commands/MQTTtoRF2/CODE_8233372/UNIT_0/PERIOD_272 -m 0
Command example for switching on:
sudo mosquitto_pub -t home/commands/MQTTtoRF2/CODE_8233372/UNIT_0/PERIOD_272 -m 1
*/

#ifdef ZgatewayRF2

#include <NewRemoteTransmitter.h>

void MQTTtoRF2(char * topicOri, char * datacallback) {

  // RF DATA ANALYSIS
  //We look into the subject to see if a special RF protocol is defined 
  String topic = topicOri;
  bool boolSWITCHTYPE;
  boolSWITCHTYPE = to_bool(datacallback);
  
  int valueCODE  = 0;
  int valueUNIT = -1;
  int valuePERIOD = 0;
  int valueBIT  = 0;
  
  int pos = topic.lastIndexOf(RF2codeKey);       
  if (pos != -1){
    pos = pos + +strlen(RF2codeKey);
    valueCODE = (topic.substring(pos,pos + 7)).toInt();
    trc(F("RF2 code:"));
    trc(String(valueCODE));
  }
  int pos2 = topic.lastIndexOf(RF2periodKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RF2periodKey);
    valuePERIOD = (topic.substring(pos2,pos2 + 3)).toInt();
    trc(F("RF2 Period:"));
    trc(String(valuePERIOD));
  }
  int pos3 = topic.lastIndexOf(RF2unitKey);       
  if (pos3 != -1){
    pos3 = pos3 + strlen(RF2unitKey);
    valueUNIT = (topic.substring(pos3,pos3 + 1)).toInt();
    trc(F("Unit:"));
    trc(String(valueUNIT));
  }
  
  if ((topic == subjectMQTTtoRF2) || (valueCODE != 0) || (valueUNIT  != -1)|| (valuePERIOD  != 0)){
    trc(F("MQTTtoRF2 user parameters"));
    if (valueCODE == 0) valueCODE = 8233378;
    if (valueUNIT == -1) valueUNIT = 0;
    if (valuePERIOD == 0) valuePERIOD = 272;
    trc(String(valueCODE));
    trc(String(valueUNIT));
    trc(String(valuePERIOD));
    trc(String(boolSWITCHTYPE));
    trc(F("Creating transmitter"));
    NewRemoteTransmitter transmitter(valueCODE, RF_EMITTER_PIN, valuePERIOD);
    trc(F("Sending data "));
    transmitter.sendUnit(valueUNIT, boolSWITCHTYPE); 
    trc(F("Data sent"));
  }
}

bool to_bool(std::string const& s) { // thanks Chris Jester-Young from stackoverflow
     return s != "0";
}
#endif
