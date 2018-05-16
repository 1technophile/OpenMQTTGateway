/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - publish MQTT data to a different topic related to received 433Mhz signal DIO/new kaku protocol

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
#include <NewRemoteReceiver.h>

struct RF2rxd
{
  unsigned int period;
  unsigned long address;
  unsigned long groupBit;
  unsigned long unit;
  unsigned long switchType;
  bool hasNewData;
};

RF2rxd rf2rd;

void setupRF2(){
    #ifndef ZgatewayRF //receiving with RF2 is not compatible with ZgatewayRF
      NewRemoteReceiver::init(RF_RECEIVER_PIN, 2, rf2Callback);
        trc(F("RF_EMITTER_PIN "));
        trc(RF_EMITTER_PIN);
        trc(F("RF_RECEIVER_PIN "));
        trc(RF_RECEIVER_PIN);
        trc(F("ZgatewayRF2 setup done "));   
    #endif 
    pinMode(RF_EMITTER_PIN, OUTPUT);
    digitalWrite(RF_EMITTER_PIN, LOW); 
}

boolean RF2toMQTT(){

  if(rf2rd.hasNewData){
    
    rf2rd.hasNewData=false;
    trc(F("Rcv. RF2"));
    String MQTTAddress;
    String MQTTperiod;
    String MQTTunit;
    String MQTTgroupBit;
    String MQTTswitchType;

    MQTTAddress = String(rf2rd.address);
    MQTTperiod = String(rf2rd.period);
    MQTTunit = String(rf2rd.unit);
    MQTTgroupBit = String(rf2rd.groupBit);
    MQTTswitchType = String(rf2rd.switchType);
    String MQTTRF2string;
    MQTTRF2string = subjectRF2toMQTT+String("/")+RF2codeKey+MQTTAddress+String("/")+RF2unitKey+MQTTunit+String("/")+RF2groupKey+MQTTgroupBit+String("/")+RF2periodKey+MQTTperiod;
    trc(F("Adv data RF2toMQTT"));
    client.publish((char *)MQTTRF2string.c_str(),(char *)MQTTswitchType.c_str());  
    return true;
    
  }
  return false;
}

void rf2Callback(unsigned int period, unsigned long address, unsigned long groupBit, unsigned long unit, unsigned long switchType) {

  rf2rd.period=period;
  rf2rd.address=address;
  rf2rd.groupBit=groupBit;
  rf2rd.unit=unit;
  rf2rd.switchType=switchType;
  rf2rd.hasNewData=true;

}

void MQTTtoRF2(char * topicOri, char * datacallback) {

  // RF DATA ANALYSIS
  //We look into the subject to see if a special RF protocol is defined 
  String topic = topicOri;
  bool boolSWITCHTYPE;
  boolSWITCHTYPE = to_bool(datacallback);
  bool isDimCommand = false;
  
  long valueCODE  = 0;
  int valueUNIT = -1;
  int valuePERIOD = 0;
  int valueGROUP  = 0;
  int valueDIM  = -1;
  
  int pos = topic.lastIndexOf(RF2codeKey);       
  if (pos != -1){
    pos = pos + +strlen(RF2codeKey);
    valueCODE = (topic.substring(pos,pos + 8)).toInt();
    trc(F("RF2 code:"));
    trc(valueCODE);
  }
  int pos2 = topic.lastIndexOf(RF2periodKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RF2periodKey);
    valuePERIOD = (topic.substring(pos2,pos2 + 3)).toInt();
    trc(F("RF2 Period:"));
    trc(valuePERIOD);
  }
  int pos3 = topic.lastIndexOf(RF2unitKey);       
  if (pos3 != -1){
    pos3 = pos3 + strlen(RF2unitKey);
    valueUNIT = (topic.substring(pos3, topic.indexOf("/", pos3))).toInt();
    trc(F("Unit:"));
    trc(valueUNIT);
  }
  int pos4 = topic.lastIndexOf(RF2groupKey);
  if (pos4 != -1) {
    pos4 = pos4 + strlen(RF2groupKey);
    valueGROUP = (topic.substring(pos4,pos4 + 1)).toInt();
    trc(F("RF2 Group:"));
    trc(valueGROUP);
  }
  int pos5 = topic.lastIndexOf(RF2dimKey);
  if (pos5 != -1) {
    isDimCommand = true;
    valueDIM = atoi(datacallback);
    trc(F("RF2 Dim:"));
    trc(valueDIM);
  }
  
  if ((topic == subjectMQTTtoRF2) || (valueCODE != 0) || (valueUNIT  != -1)|| (valuePERIOD  != 0)){
    trc(F("MQTTtoRF2"));
    if (valueCODE == 0) valueCODE = 8233378;
    if (valueUNIT == -1) valueUNIT = 0;
    if (valuePERIOD == 0) valuePERIOD = 272;
    trc(valueCODE);
    trc(valueUNIT);
    trc(valuePERIOD);
    trc(valueGROUP);
    trc(boolSWITCHTYPE);
    trc(valueDIM);
    NewRemoteReceiver::disable();
    trc(F("Creating transmitter"));
    NewRemoteTransmitter transmitter(valueCODE, RF_EMITTER_PIN, valuePERIOD);
    trc(F("Sending data"));
    if (valueGROUP) {
      if (isDimCommand) {
        transmitter.sendGroupDim(valueDIM); 
      }
      else {
        transmitter.sendGroup(boolSWITCHTYPE); 
      }
    }
    else {
      if (isDimCommand) {
        transmitter.sendDim(valueUNIT, valueDIM); 
      }
      else {    
        transmitter.sendUnit(valueUNIT, boolSWITCHTYPE); 
      }
    }
    trc(F("Data sent"));
    NewRemoteReceiver::enable();

    // Publish state change back to MQTT
    String MQTTAddress;
    String MQTTperiod;
    String MQTTunit;
    String MQTTgroupBit;
    String MQTTswitchType;
    String MQTTdimLevel;

    MQTTAddress = String(valueCODE);
    MQTTperiod = String(valuePERIOD);
    MQTTunit = String(valueUNIT);
    MQTTgroupBit = String(rf2rd.groupBit);
    MQTTswitchType = String(boolSWITCHTYPE);
    MQTTdimLevel = String(valueDIM);
    String MQTTRF2string;
    trc(F("Adv data MQTTtoRF2 push state via RF2toMQTT"));
    if (isDimCommand) {
      MQTTRF2string = subjectRF2toMQTT+String("/")+RF2codeKey+MQTTAddress+String("/")+RF2unitKey+MQTTunit+String("/")+RF2groupKey+MQTTgroupBit+String("/")+RF2dimKey+String("/")+RF2periodKey+MQTTperiod;
      client.publish((char *)MQTTRF2string.c_str(),(char *)MQTTdimLevel.c_str());  
    }
    else {
      MQTTRF2string = subjectRF2toMQTT+String("/")+RF2codeKey+MQTTAddress+String("/")+RF2unitKey+MQTTunit+String("/")+RF2groupKey+MQTTgroupBit+String("/")+RF2periodKey+MQTTperiod;
      client.publish((char *)MQTTRF2string.c_str(),(char *)MQTTswitchType.c_str());  
    }
  }
}

#endif
