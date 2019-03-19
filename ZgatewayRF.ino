/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal

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
#ifdef ZgatewayRF

#include <RCSwitch.h> // library for controling Radio frequency switch

RCSwitch mySwitch = RCSwitch();

void setupRF(){

  //RF init parameters
  mySwitch.enableTransmit(RF_EMITTER_PIN);
  trc(F("RF_EMITTER_PIN "));
  trc(RF_EMITTER_PIN);
  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); 
  mySwitch.enableReceive(RF_RECEIVER_PIN); 
  trc(F("RF_RECEIVER_PIN "));
  trc(RF_RECEIVER_PIN);
  trc(F("RF setup ok"));
}

void RFtoMQTT(){

  if (mySwitch.available()){
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(4);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
    JsonObject& RFdata = jsonBuffer.createObject();
    trc(F("Rcv. RF"));
    #ifdef ESP32
      String taskMessage = "RF Task running on core ";
      taskMessage = taskMessage + xPortGetCoreID();
      trc(taskMessage);
    #endif
    RFdata.set("value", (unsigned long)mySwitch.getReceivedValue());
    RFdata.set("protocol",(int)mySwitch.getReceivedProtocol());
    RFdata.set("length", (int)mySwitch.getReceivedBitlength());
    RFdata.set("delay", (int)mySwitch.getReceivedDelay());
    mySwitch.resetAvailable();
    
    unsigned long MQTTvalue = RFdata.get<unsigned long>("value");
    if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
        #ifdef ZmqttDiscovery //component creation for HA
            RFtoMQTTdiscovery(MQTTvalue);
        #endif
        pub(subjectRFtoMQTT,RFdata);
        trc(F("Store to avoid duplicate"));
        storeValue(MQTTvalue);
        if (repeatRFwMQTT){
            trc(F("Pub RF for rpt"));
            pub(subjectMQTTtoRF,RFdata);
        }
    } 
  }
}

#ifdef ZmqttDiscovery
void RFtoMQTTdiscovery(unsigned long MQTTvalue){//on the fly switch creation from received RF values
  char val[11];
  sprintf(val, "%lu", MQTTvalue);
  trc(F("switchRFDiscovery"));
  char * switchRF[8] = {"switch", val, "", "","",val, "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySwitch"));
   trc(switchRF[1]);
    createDiscovery(switchRF[0],
                    subjectRFtoMQTT, switchRF[1], (char *)getUniqueId(switchRF[1], switchRF[2]).c_str(),
                    will_Topic, switchRF[3], switchRF[4],
                    switchRF[5], switchRF[6], switchRF[7],
                    0,"","",true,subjectMQTTtoRF);
}
#endif

#ifdef simpleReceiving
void MQTTtoRF(char * topicOri, char * datacallback) {

  unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295

  // RF DATA ANALYSIS
  //We look into the subject to see if a special RF protocol is defined 
  String topic = topicOri;
  int valuePRT = 0;
  int valuePLSL  = 0;
  int valueBITS  = 0;
  int pos = topic.lastIndexOf(RFprotocolKey);       
  if (pos != -1){
    pos = pos + +strlen(RFprotocolKey);
    valuePRT = (topic.substring(pos,pos + 1)).toInt();
    trc(F("RF Protocol:"));
    trc(valuePRT);
  }
  //We look into the subject to see if a special RF pulselength is defined 
  int pos2 = topic.lastIndexOf(RFpulselengthKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RFpulselengthKey);
    valuePLSL = (topic.substring(pos2,pos2 + 3)).toInt();
    trc(F("RF Pulse Lgth:"));
    trc(valuePLSL);
  }
  int pos3 = topic.lastIndexOf(RFbitsKey);       
  if (pos3 != -1){
    pos3 = pos3 + strlen(RFbitsKey);
    valueBITS = (topic.substring(pos3,pos3 + 2)).toInt();
    trc(F("Bits nb:"));
    trc(valueBITS);
  }
  
  if ((topic == subjectMQTTtoRF) && (valuePRT == 0) && (valuePLSL  == 0) && (valueBITS == 0)){
    trc(F("MQTTtoRF dflt"));
    mySwitch.setProtocol(1,350);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWRFtoMQTT, datacallback);  
  } else if ((valuePRT != 0) || (valuePLSL  != 0)|| (valueBITS  != 0)){
    trc(F("MQTTtoRF usr par."));
    if (valuePRT == 0) valuePRT = 1;
    if (valuePLSL == 0) valuePLSL = 350;
    if (valueBITS == 0) valueBITS = 24;
    trc(valuePRT);
    trc(valuePLSL);
    trc(valueBITS);
    mySwitch.setProtocol(valuePRT,valuePLSL);
    mySwitch.send(data, valueBITS);
    // Acknowledgement to the GTWRF topic 
    pub(subjectGTWRFtoMQTT, datacallback);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
  } 
}
#endif

#ifdef jsonReceiving
  void MQTTtoRF(char * topicOri, JsonObject& RFdata) { // json object decoding
  
   if (strcmp(topicOri,subjectMQTTtoRF) == 0){
      trc(F("MQTTtoRF json"));
      unsigned long data = RFdata["value"];
      if (data != 0) {
        int valuePRT =  RFdata["protocol"]|1;
        int valuePLSL = RFdata["delay"]|350;
        int valueBITS = RFdata["length"]|24;
        int valueRPT = RFdata["repeat"]|RF_EMITTER_REPEAT;
        mySwitch.setRepeatTransmit(valueRPT);
        mySwitch.setProtocol(valuePRT,valuePLSL);
        mySwitch.send(data, valueBITS);
        trc(F("MQTTtoRF OK"));
        pub(subjectGTWRFtoMQTT, RFdata);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
        mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); // Restore the default value
      }else{
        trc(F("MQTTtoRF Fail json"));
      }
    }
  }
#endif
#endif
