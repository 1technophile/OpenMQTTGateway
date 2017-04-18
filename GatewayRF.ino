

void setupRF()
{

  //RF init parameters
  mySwitch.enableTransmit(RF_EMITTER_PIN);
  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); 
  mySwitch.enableReceive(RF_RECEIVER_PIN); 
  
}

void receivingMQTTRF(char * topicOri, char * datacallback) {
  String topic = topicOri;

  trc(F("Receiving data by MQTT"));
  trc(topic);  
  trc(F("Callback value"));
  trc(String(datacallback));
  unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
  trc(F("Converted value to unsigned long"));
  trc(String(data));

  // Storing data received
  int pos0 = topic.lastIndexOf(subjectMultiGTWKey);
  if (pos0 != -1){
    trc(F("Storing signal"));
    storeValue(data);
    trc(F("Data stored"));
  }

  // RF DATA ANALYSIS
  //We look into the subject to see if a special RF protocol is defined 
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
  
  if ((topic == subjectMQTTto433) && (valuePRT == 0) && (valuePLSL  == 0)){
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
    if (result)trc(F("Acknowedgement of reception published"));
  }
  
}
