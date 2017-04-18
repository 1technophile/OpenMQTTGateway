


void setupIR()
{

  //IR init parameters
#ifdef ESP8266
  irsend.begin();
#endif

  irrecv.enableIRIn(); // Start the receiver
}

void receivingMQTTIR(char * topicOri, char * datacallback) {
  
  // IR DATA ANALYSIS    
  //send received MQTT value by IR signal (example of signal sent data = 1086296175)
  boolean signalSent = false;
  #ifdef ESP8266 // send coolix not available for arduino IRRemote library
  if (topic.lastIndexOf("IR_COOLIX") != -1 ){
    irsend.sendCOOLIX(data, 24);
    signalSent = true;
  }
  #endif
  if (topic.lastIndexOf("IR_NEC")!= -1 || topic == subjectMQTTtoIR){
    irsend.sendNEC(data, 32);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Whynter")!=-1){
    irsend.sendWhynter(data, 32);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_LG")!=-1){
    irsend.sendLG(data, 28);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Sony")!=-1){
    irsend.sendSony(data, 12);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_DISH")!=-1){
    irsend.sendDISH(data, 16);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_RC5")!=-1){
    irsend.sendRC5(data, 12);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Sharp")!=-1){
    irsend.sendSharpRaw(data, 15);
    signalSent = true;
  }
   if (topic.lastIndexOf("IR_SAMSUNG")!=-1){
   irsend.sendSAMSUNG(data, 32);
    signalSent = true;
  }
  if (signalSent){
    boolean result = client.publish(subjectGTWIRtoMQTT, datacallback);
    if (result)trc(F("Acknowedgement of reception published"));
  }
   irrecv.enableIRIn(); // ReStart the IR receiver (if not restarted it is not able to receive data)
}
