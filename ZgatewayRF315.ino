/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send RF 315Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 315Mhz signal

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
#ifdef ZgatewayRF315

#include <RCSwitch.h> // library for controling Radio frequency switch

RCSwitch mySwitch315 = RCSwitch();

void setupRF315(){

  //RF init parameters
  mySwitch315.enableTransmit(RF315_EMITTER_PIN);
  trc(F("RF315_EMITTER_PIN "));
  trc(RF315_EMITTER_PIN);
  mySwitch315.setRepeatTransmit(RF315_EMITTER_REPEAT); 
  mySwitch315.enableReceive(RF315_RECEIVER_PIN); 
  trc(F("RF315_RECEIVER_PIN "));
  trc(RF315_RECEIVER_PIN);
  trc(F("ZgatewayRF315 setup done "));
}

boolean RF315toMQTT(){
  if (mySwitch315.available()){
    trc(F("Creating RF315 buffer"));
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(4);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
    JsonObject& RF315data = jsonBuffer.createObject();
    trc(F("Rcv. RF315"));
    #ifdef ESP32
      String taskMessage = "RF Task running on core ";
      taskMessage = taskMessage + xPortGetCoreID();
      trc(taskMessage);
    #endif
    RF315data.set("value", (unsigned long)mySwitch315.getReceivedValue());
    RF315data.set("protocol",(int)mySwitch315.getReceivedProtocol());
    RF315data.set("length", (int)mySwitch315.getReceivedBitlength());
    RF315data.set("delay", (int)mySwitch315.getReceivedDelay());
    mySwitch315.resetAvailable();
    
    unsigned long MQTTvalue = RF315data.get<unsigned long>("value");
    if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
        trc(F("Adv data RF315toMQTT")); 
        pub(subjectRF315toMQTT,RF315data);
        trc(F("Store to avoid duplicate"));
        storeValue(MQTTvalue);
        if (repeatRF315wMQTT){
            trc(F("Publish RF315 for repeat"));
            pub(subjectMQTTtoRF315,RF315data);
        }
    }
  }
}

#ifdef simpleReceiving
  void MQTTtoRF315(char * topicOri, char * datacallback) {
  
    unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
  
    // RF315 DATA ANALYSIS
    //We look into the subject to see if a special RF protocol is defined 
    String topic = topicOri;
    int valuePRT = 0;
    int valuePLSL  = 0;
    int valueBITS  = 0;
    int pos = topic.lastIndexOf(RF315protocolKey);       
    if (pos != -1){
      pos = pos + +strlen(RF315protocolKey);
      valuePRT = (topic.substring(pos,pos + 1)).toInt();
      trc(F("RF315 Protocol:"));
      trc(valuePRT);
    }
    //We look into the subject to see if a special RF pulselength is defined 
    int pos2 = topic.lastIndexOf(RF315pulselengthKey);
    if (pos2 != -1) {
      pos2 = pos2 + strlen(RF315pulselengthKey);
      valuePLSL = (topic.substring(pos2,pos2 + 3)).toInt();
      trc(F("RF315 Pulse Lgth:"));
      trc(valuePLSL);
    }
    int pos3 = topic.lastIndexOf(RF315bitsKey);       
    if (pos3 != -1){
      pos3 = pos3 + strlen(RF315bitsKey);
      valueBITS = (topic.substring(pos3,pos3 + 2)).toInt();
      trc(F("Bits nb:"));
      trc(valueBITS);
    }
    
    if ((topic == subjectMQTTtoRF315) && (valuePRT == 0) && (valuePLSL  == 0) && (valueBITS == 0)){
      trc(F("MQTTtoRF315 dflt"));
      mySwitch315.setProtocol(1,350);
      mySwitch315.send(data, 24);
      // Acknowledgement to the GTWRF topic
      pub(subjectGTWRF315toMQTT, datacallback);    
    } else if ((valuePRT != 0) || (valuePLSL  != 0)|| (valueBITS  != 0)){
      trc(F("MQTTtoRF315 usr par."));
      if (valuePRT == 0) valuePRT = 1;
      if (valuePLSL == 0) valuePLSL = 350;
      if (valueBITS == 0) valueBITS = 24;
      trc(valuePRT);
      trc(valuePLSL);
      trc(valueBITS);
      mySwitch315.setProtocol(valuePRT,valuePLSL);
      mySwitch315.send(data, valueBITS);
      // Acknowledgement to the GTWRF topic 
      pub(subjectGTWRF315toMQTT, datacallback);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    }
  }
#endif

#ifdef jsonReceiving
  void MQTTtoRF315(char * topicOri, JsonObject& RF315data) { // json object decoding
  
   if (strcmp(topicOri,subjectMQTTtoRF315) == 0){
      trc(F("MQTTtoRF315 json"));
      unsigned long data = RF315data["value"];
      if (data != 0) {
        int valuePRT =  RF315data["protocol"]|1;
        int valuePLSL = RF315data["delay"]|350;
        int valueBITS = RF315data["length"]|24;
        mySwitch315.setProtocol(valuePRT,valuePLSL);
        mySwitch315.send(data, valueBITS);
        trc(F("MQTTtoRF315 OK"));
        pub(subjectGTWRF315toMQTT, RF315data);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      }else{
        trc(F("MQTTtoRF315 Fail json"));
      }
    }
  }
#endif
#endif
