/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
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
  
  // IR DATA ANALYSIS    
  //send received MQTT value by IR signal
  boolean signalSent = false;
  unsigned long data = 0;
  String strcallback = String(datacallback);
  trc(String(datacallback));
  int s = strcallback.length();
  //number of "," value count
  int count = 0;
  for(int i = 0; i < s; i++)
  {
   if (datacallback[i] == ',') {
    count++;
    }
  }
  if(count == 0){
    data = strtoul(datacallback, NULL, 10); // standard sending with unsigned long, we will not be able to pass values > 4294967295
  }
  #ifdef IR_GC
  else if(strstr(topicOri, "IR_GC") != NULL){ // sending GC data from https://irdb.globalcache.com
    trc("IR_GC");
    //buffer allocation from char datacallback
    unsigned int GC[count+1];
    String value = "";
    int j = 0;
    for(int i = 0; i < s; i++)
    {
     if (datacallback[i] != ',') {
        value = value + String(datacallback[i]);
      }
      if ((datacallback[i] == ',') || (i == s - 1))
      {
        GC[j]= value.toInt();
        value = "";
        j++;
      }
    }
      irsend.sendGC(GC, j);
      signalSent = true;
  }
#endif
    
    //We look into the subject to see if a special Bits number is defined 
  String topic = topicOri;
  int valueRPT = 0;
  int valueBITS  = 0;
  int pos = topic.lastIndexOf(IRbitsKey);       
  if (pos != -1){
    pos = pos + +strlen(IRbitsKey);
    valueBITS = (topic.substring(pos,pos + 2)).toInt();
    trc(F("Bits number:"));
    trc(String(valueBITS));
  }
  //We look into the subject to see if a special repeat number is defined 
  int pos2 = topic.lastIndexOf(IRRptKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(IRRptKey);
    valueRPT = (topic.substring(pos2,pos2 + 1)).toInt();
    trc(F("IR repeat:"));
    trc(String(valueRPT));
  }
  
  #ifdef ESP8266 // send coolix not available for arduino IRRemote library
  #ifdef IR_COOLIX
  if (strstr(topicOri, "IR_COOLIX") != NULL){
    if (valueBITS == 0) valueBITS = 24;
    irsend.sendCOOLIX(data, valueBITS);
    signalSent = true;
  }
  #endif
  #endif
  if (strstr(topicOri, "IR_NEC") != NULL || strstr(topicOri, subjectMQTTtoIR) != NULL ){
    if (valueBITS == 0) valueBITS = 32;
    irsend.sendNEC(data, valueBITS);
    signalSent = true;
  }
  #ifdef IR_Whynter
  if (strstr(topicOri, "IR_Whynter") != NULL){
    if (valueBITS == 0) valueBITS = 32;
    irsend.sendWhynter(data, valueBITS);
    signalSent = true;
  }
  #endif
  #ifdef IR_LG
  if (strstr(topicOri, "IR_LG") != NULL){
    if (valueBITS == 0) valueBITS = 28;
    irsend.sendLG(data, valueBITS);
    signalSent = true;
  }
  #endif
  #ifdef IR_Sony
  if (strstr(topicOri, "IR_Sony") != NULL){
    if (valueBITS == 0) valueBITS = 12;
    if (valueRPT == 0) valueRPT = 2;
    irsend.sendSony(data, valueBITS, valueRPT);
    signalSent = true;
  }
  #endif
  #ifdef IR_DISH
  if (strstr(topicOri, "IR_DISH") != NULL){
    if (valueBITS == 0) valueBITS = 16;
    irsend.sendDISH(data, valueBITS);
    signalSent = true;
  }
  #endif
  #ifdef IR_RC5
  if (strstr(topicOri, "IR_RC5") != NULL){
    if (valueBITS == 0) valueBITS = 12;
    irsend.sendRC5(data, valueBITS);
    signalSent = true;
  }
  #endif
  #ifdef IR_Sharp
  if (strstr(topicOri, "IR_Sharp") != NULL){
    if (valueBITS == 0) valueBITS = 15;
    irsend.sendSharpRaw(data, valueBITS);
    signalSent = true;
  }
  #endif
  #ifdef IR_SAMSUNG
  if (strstr(topicOri, "IR_SAMSUNG") != NULL){
    if (valueBITS == 0) valueBITS = 32;
   irsend.sendSAMSUNG(data, valueBITS);
    signalSent = true;
  }
  #endif
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
