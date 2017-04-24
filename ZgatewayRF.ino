/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal

  Copyright: (c)1technophile

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifdef ZgatewayRF

#include <RCSwitch.h> // library for controling Radio frequency switch

RCSwitch mySwitch = RCSwitch();

void setupRF()
{

  //RF init parameters
  mySwitch.enableTransmit(RF_EMITTER_PIN);
  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); 
  mySwitch.enableReceive(RF_RECEIVER_PIN); 
  
}

boolean RFtoMQTT(){

  if (mySwitch.available()){
    trc(F("Receiving 433Mhz signal"));
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
        trc(F("Sending advanced signal to MQTT"));
        client.publish(subjectRFtoMQTTprotocol,(char *)MQTTprotocol.c_str());
        client.publish(subjectRFtoMQTTbits,(char *)MQTTbits.c_str());    
        client.publish(subjectRFtoMQTTlength,(char *)MQTTlength.c_str());    
        trc(F("Sending RF to MQTT"));
        String value = String(MQTTvalue);
        trc(value);
        boolean result = client.publish(subjectRFtoMQTT,(char *)value.c_str());
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
  int pos = topic.lastIndexOf(RFprotocolKey);       
  if (pos != -1){
    pos = pos + +strlen(RFprotocolKey);
    valuePRT = (topic.substring(pos,pos + 1)).toInt();
    trc(F("RF Protocol number:"));
    trc(String(valuePRT));
  }
  //We look into the subject to see if a special RF pulselength is defined 
  int pos2 = topic.lastIndexOf(RFpulselengthKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RFpulselengthKey);
    valuePLSL = (topic.substring(pos2,pos2 + 3)).toInt();
    trc(F("RF Pulse Length:"));
    trc(String(valuePLSL));
  }
  
  if ((topic == subjectMQTTtoRF) && (valuePRT == 0) && (valuePLSL  == 0)){
    trc(F("Sending data by RF, default parameters"));
    mySwitch.setProtocol(1,350);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    boolean result = client.publish(subjectGTWRFtoMQTT, datacallback);
    if (result)trc(F("Acknowedgement of reception published"));
    
  } else if ((valuePRT != 0) || (valuePLSL  != 0)){
    trc(F("Sending data by RF, user defined parameters"));
    if (valuePRT == 0) valuePRT = 1;
    if (valuePLSL == 0) valuePLSL = 350;
    trc(String(valuePRT));
    trc(String(valuePLSL));
    mySwitch.setProtocol(valuePRT,valuePLSL);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    boolean result = client.publish(subjectGTWRFtoMQTT, datacallback);
    if (result){
      trc(F("Signal below sent by RF and acknowledgment published"));
      trc(String(data));
      };
  }
  
}
#endif
