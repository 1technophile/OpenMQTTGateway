/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send IR signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received IR signal

  Copyright: (c)1technophile

IMPORTANT NOTE: On arduino connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/IRremoteInt.h so as to free pin D3 for RF RECEIVER PIN
  
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
#ifdef ZgatewayIR

#ifdef ESP8266
  #include <IRremoteESP8266.h>
  IRrecv irrecv(IR_RECEIVER_PIN);
  IRsend irsend(IR_EMITTER_PIN);
#else
  #include <IRremote.h>
  IRrecv irrecv(IR_RECEIVER_PIN);
  IRsend irsend; //connect IR emitter pin to D9 on arduino, you need to comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library IRremote.h so as to free pin D3 for RF RECEIVER PIN
#endif

void setupIR()
{
  //IR init parameters
#ifdef ESP8266
  irsend.begin();
#endif

  irrecv.enableIRIn(); // Start the receiver
  
}
boolean IRtoMQTT(){
  decode_results results;
  
  if (irrecv.decode(&results)){
  trc(F("Receiving IR signal"));
    unsigned long MQTTvalue = 0;
    String MQTTprotocol;
    String MQTTbits;
    MQTTvalue = results.value;
    MQTTprotocol = String(results.decode_type);
    MQTTbits = String(results.bits);
    irrecv.resume(); // Receive the next value
    if (pubIRunknownPrtcl == false && MQTTprotocol == "-1"){ // don't publish unknown IR protocol
      trc(F("--------------don't publish the received code unknown protocol--------------"));
    } else if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
        trc(F("Sending advanced data to MQTT"));
        client.publish(subjectIRtoMQTTprotocol,(char *)MQTTprotocol.c_str());
        client.publish(subjectIRtoMQTTbits,(char *)MQTTbits.c_str());        
        trc(F("Sending IR to MQTT"));
        String value = String(MQTTvalue);
        trc(value);
        boolean result = client.publish(subjectIRtoMQTT,(char *)value.c_str());
        return result;
    }
  }
  return false;  
}

void MQTTtoIR(char * topicOri, char * datacallback) {

  unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
  
  // IR DATA ANALYSIS    
  //send received MQTT value by IR signal (example of signal sent data = 1086296175)
  boolean signalSent = false;
  #ifdef ESP8266 // send coolix not available for arduino IRRemote library
  if (strstr(topicOri, "IR_COOLIX") != NULL){
    irsend.sendCOOLIX(data, 24);
    signalSent = true;
  }
  #endif
  if (strstr(topicOri, "IR_NEC") != NULL || strstr(topicOri, subjectMQTTtoIR) != NULL ){
    irsend.sendNEC(data, 32);
    signalSent = true;
  }
  if (strstr(topicOri, "IR_Whynter") != NULL){
    irsend.sendWhynter(data, 32);
    signalSent = true;
  }
  if (strstr(topicOri, "IR_LG") != NULL){
    irsend.sendLG(data, 28);
    signalSent = true;
  }
  if (strstr(topicOri, "IR_Sony") != NULL){
    irsend.sendSony(data, 12);
    signalSent = true;
  }
  if (strstr(topicOri, "IR_DISH") != NULL){
    irsend.sendDISH(data, 16);
    signalSent = true;
  }
  if (strstr(topicOri, "IR_RC5") != NULL){
    irsend.sendRC5(data, 12);
    signalSent = true;
  }
  if (strstr(topicOri, "IR_Sharp") != NULL){
    irsend.sendSharpRaw(data, 15);
    signalSent = true;
  }
  if (strstr(topicOri, "IR_SAMSUNG") != NULL){
   irsend.sendSAMSUNG(data, 32);
    signalSent = true;
  }
  if (signalSent){
    boolean result = client.publish(subjectGTWIRtoMQTT, datacallback);
    if (result){
      trc(F("Signal below sent by IR and acknowledgment published"));
      trc(String(data));
      };
  }
   irrecv.enableIRIn(); // ReStart the IR receiver (if not restarted it is not able to receive data)
}
#endif
