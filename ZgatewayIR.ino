/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send IR signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received IR signal

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
#ifdef ZgatewayIR

#ifdef ESP8266
  #include <IRremoteESP8266.h>
  #include <IRsend.h>  // Needed if you want to send IR commands.
  #include <IRrecv.h>  // Needed if you want to receive IR commands.
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
  
trc(F("IR_EMITTER_PIN "));
trc(IR_EMITTER_PIN);
trc(F("IR_RECEIVER_PIN "));
trc(IR_RECEIVER_PIN);
trc(F("ZgatewayIR setup done "));

}
boolean IRtoMQTT(){
  decode_results results;
  
  if (irrecv.decode(&results)){
  trc(F("Rcv. IR"));
    #ifdef ESP32
      String taskMessage = "Task running on core ";
      taskMessage = taskMessage + xPortGetCoreID();
      trc(taskMessage);
    #endif
    unsigned long MQTTvalue = 0;
    String MQTTprotocol;
    String MQTTbits;
    MQTTvalue = results.value;
    MQTTprotocol = String(results.decode_type);
    MQTTbits = String(results.bits);
    String rawCode = "";
    // Dump data
    for (uint16_t i = 1;  i < results.rawlen;  i++) {
       #ifdef ESP8266
          if (i % 100 == 0) yield();  // Preemptive yield every 100th entry to feed the WDT.
          rawCode = rawCode + (results.rawbuf[i] * RAWTICK);
       #else
          rawCode = rawCode + (results.rawbuf[i] * USECPERTICK);
       #endif
      if ( i < results.rawlen-1 ) rawCode = rawCode + ","; // ',' not needed on last one
    }
    trc(rawCode);
    // if needed we directly resend the raw code
    if (RawDirectForward){
      #ifdef ESP8266
        uint16_t rawsend[results.rawlen];
        for (uint16_t i = 1;  i < results.rawlen;  i++) {
          if (i % 100 == 0) yield();  // Preemptive yield every 100th entry to feed the WDT.
      #else
        unsigned int rawsend[results.rawlen];
        for (int i = 1;  i < results.rawlen;  i++) {
      #endif
          rawsend[i] = results.rawbuf[i];
      }
      irsend.sendRaw(rawsend, results.rawlen, RawFrequency); 
      trc(F("raw signal redirected"));
    }
    irrecv.resume(); // Receive the next value

    if (pubIRunknownPrtcl == false && MQTTprotocol == "-1"){ // don't publish unknown IR protocol
      trc(F("--no pub. unknown protocol--"));
    } else if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
        trc(F("Adv data IRtoMQTT"));
        client.publish(subjectIRtoMQTTprotocol,(char *)MQTTprotocol.c_str());
        client.publish(subjectIRtoMQTTbits,(char *)MQTTbits.c_str());
        client.publish(subjectIRtoMQTTRaw,(char *)rawCode.c_str());          
        trc(F("Sending IRtoMQTT"));
        String value = String(MQTTvalue);
        trc(value);
        boolean result = client.publish(subjectIRtoMQTT,(char *)value.c_str());
        if (repeatIRwMQTT){
            trc(F("Pub. IR for repeat"));
            client.publish(subjectMQTTtoIR,(char *)value.c_str());
        }
        return result;
    }
  }
  return false;  
}

void MQTTtoIR(char * topicOri, char * datacallback) {
  
  // IR DATA ANALYSIS    
  //send received MQTT value by IR signal
  boolean signalSent = false;
  uint64_t data = 0;
  String strcallback = String(datacallback);
  trc(datacallback);
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
    uint16_t  GC[count+1];
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
  #ifdef IR_Raw
  else if(strstr(topicOri, "IR_Raw") != NULL){ // sending Raw data
    trc("IR_Raw");
    //buffer allocation from char datacallback
    #ifdef ESP8266
      uint16_t  Raw[count+1];
    #else
      unsigned int Raw[count+1];
    #endif
    String value = "";
    int j = 0;
    for(int i = 0; i < s; i++)
    {
     if (datacallback[i] != ',') {
        value = value + String(datacallback[i]);
      }
      if ((datacallback[i] == ',') || (i == s - 1))
      {
        Raw[j]= value.toInt();
        value = "";
        j++;
      }
    }
      irsend.sendRaw(Raw, j, RawFrequency);
      signalSent = true;
  }
#endif
    
    //We look into the subject to see if a special Bits number is defined 
  String topic = topicOri;
  unsigned int valueBITS  = 0;
  int pos = topic.lastIndexOf(IRbitsKey);       
  if (pos != -1){
    pos = pos + +strlen(IRbitsKey);
    valueBITS = (topic.substring(pos,pos + 2)).toInt();
    trc(F("Bits nb:"));
    trc(valueBITS);
  }
  //We look into the subject to see if a special repeat number is defined 
  int valueRPT = 0;
  int pos2 = topic.lastIndexOf(IRRptKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(IRRptKey);
    valueRPT = (topic.substring(pos2,pos2 + 1)).toInt();
    trc(F("IR repeat:"));
    trc(valueRPT);
  }
  
  #ifdef ESP8266 // send coolix not available for arduino IRRemote library
  #ifdef IR_COOLIX
  if (strstr(topicOri, "IR_COOLIX") != NULL){
    if (valueBITS == 0) valueBITS = 24;
    irsend.sendCOOLIX(data, valueBITS, valueRPT);
    signalSent = true;
  }
  #endif
  #endif
  if (strstr(topicOri, "IR_NEC") != NULL || strstr(topicOri, subjectMQTTtoIR) != NULL ){
    if (valueBITS == 0) valueBITS = 32;
#ifdef ESP8266
    irsend.sendNEC(data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendNEC(data, valueBITS);
#endif
    signalSent = true;
  }
  #ifdef IR_Whynter
  if (strstr(topicOri, "IR_Whynter") != NULL){
    if (valueBITS == 0) valueBITS = 32;
#ifdef ESP8266
    irsend.sendWhynter(data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendWhynter(data, valueBITS);
#endif
    signalSent = true;
  }
  #endif
  #ifdef IR_LG
  if (strstr(topicOri, "IR_LG") != NULL){
    if (valueBITS == 0) valueBITS = 28;
#ifdef ESP8266
    irsend.sendLG(data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendLG(data, valueBITS);
#endif
    signalSent = true;
  }
  #endif
  #ifdef IR_Sony
  if (strstr(topicOri, "IR_Sony") != NULL){
    if (valueBITS == 0) valueBITS = 12;
    if (valueRPT == 0) valueRPT = 2;
#ifdef ESP8266
    irsend.sendSony(data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendSony(data, valueBITS);
#endif
    signalSent = true;
  }
  #endif
  #ifdef IR_DISH
  if (strstr(topicOri, "IR_DISH") != NULL){
    if (valueBITS == 0) valueBITS = 16;
#ifdef ESP8266
    irsend.sendDISH(data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendDISH(data, valueBITS);
#endif
    signalSent = true;
  }
  #endif
  #ifdef IR_RC5
  if (strstr(topicOri, "IR_RC5") != NULL){
    if (valueBITS == 0) valueBITS = 12;
#ifdef ESP8266
    irsend.sendRC5(data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendRC5(data, valueBITS);
#endif
    signalSent = true;
  }
  #endif
  #ifdef IR_Sharp
  if (strstr(topicOri, "IR_Sharp") != NULL){
    if (valueBITS == 0) valueBITS = 15;
#ifdef ESP8266
    irsend.sendSharpRaw(data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendSharpRaw(data, valueBITS);
#endif
    signalSent = true;
  }
  #endif
  #ifdef IR_SAMSUNG
  if (strstr(topicOri, "IR_SAMSUNG") != NULL){
    if (valueBITS == 0) valueBITS = 32;
#ifdef ESP8266
    irsend.sendSAMSUNG(data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendSAMSUNG(data, valueBITS);
#endif
    signalSent = true;
  }
  #endif
  #ifdef IR_PANASONIC
  if (strstr(topicOri, "IR_PANASONIC") != NULL){
    if (valueBITS == 0) valueBITS = PanasonicBits;
    if (valueRPT == 0) valueRPT = 2;
#ifdef ESP8266
    irsend.sendPanasonic(PanasonicAddress, data, valueBITS, valueRPT);
#else
    for (int i=0; i <= valueRPT; i++) irsend.sendPanasonic(PanasonicAddress, data);
#endif
    signalSent = true;
  }
  #endif

#ifdef ESP8266  // IR_RCMM not available on arduino
  #ifdef IR_RCMM
  if (strstr(topicOri, "IR_RCMM") != NULL){
    if (valueBITS == 0) valueBITS = 32;
    irsend.sendRCMM(data, valueBITS, valueRPT);
    signalSent = true;
  }
  #endif
#endif

  if (signalSent){ // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    boolean result = client.publish(subjectGTWIRtoMQTT, datacallback);
    if (result){
      trc(F("MQTTtoIR ack pub."));
      };
  }
   irrecv.enableIRIn(); // ReStart the IR receiver (if not restarted it is not able to receive data)
}
#endif
