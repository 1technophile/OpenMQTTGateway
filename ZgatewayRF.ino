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
  trc(F("ZgatewayRF setup done "));
}

boolean RFtoMQTT(){

  if (mySwitch.available()){
    trc(F("Rcv. RF"));
    #ifdef ESP32
      String taskMessage = "RF Task running on core ";
      taskMessage = taskMessage + xPortGetCoreID();
      trc(taskMessage);
    #endif
    unsigned long MQTTvalue = 0;
    String MQTTprotocol;
    String MQTTbits;
    String MQTTlength;
    MQTTvalue = mySwitch.getReceivedValue();
    MQTTprotocol = String(mySwitch.getReceivedProtocol());
    MQTTbits = String(mySwitch.getReceivedBitlength());
    MQTTlength = String(mySwitch.getReceivedDelay());
    mySwitch.resetAvailable();
    if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
        trc(F("Adv data RFtoMQTT"));
        client.publish(subjectRFtoMQTTprotocol,(char *)MQTTprotocol.c_str());
        client.publish(subjectRFtoMQTTbits,(char *)MQTTbits.c_str());    
        client.publish(subjectRFtoMQTTlength,(char *)MQTTlength.c_str());    
        trc(F("Sending RFtoMQTT"));
        String value = String(MQTTvalue);
        trc(value);
        boolean result = client.publish(subjectRFtoMQTT,(char *)value.c_str());
        if (repeatRFwMQTT){
            trc(F("Publish RF for repeat"));
            client.publish(subjectMQTTtoRF,(char *)value.c_str());
        }
        return result;
    } 
  }
  return false;
}

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
    boolean result = client.publish(subjectGTWRFtoMQTT, datacallback);
    if (result)trc(F("Ack pub."));
    
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
    boolean result = client.publish(subjectGTWRFtoMQTT, datacallback);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    if (result){
      trc(F("MQTTtoRF ack pub."));
      trc(data);
    }
  }
  
}
#endif
