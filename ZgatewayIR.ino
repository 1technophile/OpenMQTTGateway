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
  #ifdef DumpMode // in dump mode we increase the size of the buffer to catch big codes
    IRrecv irrecv(IR_RECEIVER_PIN, 1024, 15U, true);
  #else
    IRrecv irrecv(IR_RECEIVER_PIN);
  #endif
  IRsend irsend(IR_EMITTER_PIN);
#else
  #include <IRremote.h>
  IRrecv irrecv(IR_RECEIVER_PIN);
  IRsend irsend; //connect IR emitter pin to D9 on arduino, you need to comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library IRremote.h so as to free pin D3 for RF RECEIVER PIN
#endif

// IR protocol bits definition for Arduino (for ESP9266 they are defined in IRRemoteESP8266.h)
#ifndef NEC_BITS
  #define NEC_BITS                    32U
#endif
#ifndef SAMSUNG_BITS
  #define SAMSUNG_BITS                32U
#endif
#ifndef SHARP_BITS
  #define SHARP_ADDRESS_BITS           5U
  #define SHARP_COMMAND_BITS           8U
  #define SHARP_BITS (SHARP_ADDRESS_BITS + SHARP_COMMAND_BITS + 2)  // 15U
#endif
#ifndef RC5_BITS
  #define RC5_RAW_BITS                14U
  #define RC5_BITS      RC5_RAW_BITS - 2U
#endif
#ifndef DISH_BITS
  #define DISH_BITS                   16U
#endif
#ifndef SONY_12_BITS
  #define SONY_12_BITS                12U
#endif
#ifndef LG_BITS
  #define LG_BITS                     28U
#endif
#ifndef WHYNTER_BITS
  #define WHYNTER_BITS                32U
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
void IRtoMQTT(){
  decode_results results;
  
  if (irrecv.decode(&results)){
  trc(F("Creating IR buffer"));
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& IRdata = jsonBuffer.createObject();
  
  trc(F("Rcv. IR"));
    #ifdef ESP32
      String taskMessage = "Task running on core ";
      taskMessage = taskMessage + xPortGetCoreID();
      trc(taskMessage);
    #endif
    IRdata.set("value", (unsigned long)(results.value));
    IRdata.set("protocol", (int)(results.decode_type));
    IRdata.set("bits",(int)(results.bits));
    
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
    IRdata.set("raw", rawCode);
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
    unsigned long MQTTvalue = IRdata.get<unsigned long>("value");
    if (pubIRunknownPrtcl == false && IRdata.get<int>("protocol") == -1){ // don't publish unknown IR protocol
      trc(F("--no pub. unknown protocol--"));
    } else if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
        trc(F("Adv data IRtoMQTT"));         
        pub(subjectIRtoMQTT,IRdata);
        trc(F("Store to avoid duplicate"));
        storeValue(MQTTvalue);
        if (repeatIRwMQTT){
            trc(F("Pub. IR for repeat"));
            pub(subjectMQTTtoIR,MQTTvalue);
        }
    }
  } 
}

#ifdef simpleReceiving
  void MQTTtoIR(char * topicOri, char * datacallback) {
    
    // IR DATA ANALYSIS    
    //send received MQTT value by IR signal
    boolean signalSent = false;
    uint64_t data = 0;
    String strcallback = String(datacallback);
    trc(datacallback);
    unsigned int s = strcallback.length();
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
      trc(F("IR_GC"));
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
      trc(F("IR_Raw"));
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
    uint16_t  valueRPT = 0;
    int pos2 = topic.lastIndexOf(IRRptKey);
    if (pos2 != -1) {
      pos2 = pos2 + strlen(IRRptKey);
      valueRPT = (topic.substring(pos2,pos2 + 1)).toInt();
      trc(F("IR repeat:"));
      trc(valueRPT);
    }
    
    if(topicOri && (strstr(topicOri, "IR_NEC") == NULL)){
        signalSent = sendIdentifiedProtocol(topicOri, data, (unsigned char *)datacallback, valueBITS, valueRPT);
    }else{
        trc(F("Using NEC protocol"));
        if (valueBITS == 0) valueBITS = NEC_BITS;
          #ifdef ESP8266
              irsend.sendNEC(data, valueBITS, valueRPT);
          #else
              for (int i=0; i <= valueRPT; i++) irsend.sendNEC(data, valueBITS);
          #endif
        signalSent = true;
     }
  
    if (signalSent){ // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      pub(subjectGTWIRtoMQTT, datacallback);
    }
     irrecv.enableIRIn(); // ReStart the IR receiver (if not restarted it is not able to receive data)
  }
#endif

#ifdef jsonReceiving
  void MQTTtoIR(char * topicOri, JsonObject& IRdata) {

     if (strcmp(topicOri,subjectMQTTtoIR) == 0){ 
      trc(F("MQTTtoIR json"));
      unsigned long data = IRdata["value"];
      const char * raw = IRdata["raw"];
      const char * datastring = IRdata["datastring"];
      if (data != 0||raw) {
        trc(F("MQTTtoIR data || raw ok"));
        boolean signalSent = false;
        trc(F("value"));
        trc(data);
        trc(F("raw"));
        trc(raw);
        const char * protocol_name = IRdata["protocol_name"];
        unsigned int valueBITS  = IRdata["bits"];
        uint16_t  valueRPT = IRdata["repeat"]|repeatIRwNumber;

        if(raw){
            unsigned int s = strlen(raw);
            //number of "," value count
            int count = 0;
            for(int i = 0; i < s; i++)
            {
             if (raw[i] == ',') {
              count++;
              }
            }
            #ifdef IR_GC
             if(strstr(protocol_name, "IR_GC") != NULL){ // sending GC data from https://irdb.globalcache.com
              trc(F("IR_GC"));
              //buffer allocation from char datacallback
              uint16_t  GC[count+1];
              String value = "";
              int j = 0;
              for(int i = 0; i < s; i++)
              {
               if (raw[i] != ',') {
                  value = value + String(raw[i]);
                }
                if ((raw[i] == ',') || (i == s - 1))
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
            if(strstr(protocol_name, "IR_Raw") != NULL){ // sending Raw data
              trc(F("IR_Raw"));
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
             if (raw[i] != ',') {
                value = value + String(raw[i]);
              }
              if ((raw[i] == ',') || (i == s - 1))
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
        }else if(protocol_name && (strstr(protocol_name, "IR_NEC") == NULL)){
            signalSent = sendIdentifiedProtocol(protocol_name, data, (unsigned char*)datastring, valueBITS, valueRPT);
        }else{
            trc(F("Using NEC protocol"));
            if (valueBITS == 0) valueBITS = NEC_BITS;
              #ifdef ESP8266
                  irsend.sendNEC(data, valueBITS, valueRPT);
              #else
                  for (int i=0; i <= valueRPT; i++) irsend.sendNEC(data, valueBITS);
              #endif
            signalSent = true;
          }
          if (signalSent){ // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
            trc(F("MQTTtoIR OK"));
            pub(subjectGTWIRtoMQTT, IRdata);
          }
          irrecv.enableIRIn(); // ReStart the IR receiver (if not restarted it is not able to receive data)
        }else{
          trc(F("MQTTtoIR Fail reading from json"));
        }
     }
  }
#endif

boolean sendIdentifiedProtocol(const char * protocol_name, unsigned long data, unsigned char * datastring, unsigned int valueBITS, uint16_t valueRPT){
  #ifdef IR_Whynter
    if (strstr(protocol_name, "IR_Whynter") != NULL){
      if (valueBITS == 0) valueBITS = WHYNTER_BITS;
        #ifdef ESP8266
            irsend.sendWhynter(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendWhynter(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_LG
    if (strstr(protocol_name, "IR_LG") != NULL){
      if (valueBITS == 0) valueBITS = LG_BITS;
        #ifdef ESP8266
            irsend.sendLG(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendLG(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_Sony
    if (strstr(protocol_name, "IR_Sony") != NULL){
      if (valueBITS == 0) valueBITS = SONY_12_BITS;
        #ifdef ESP8266
            irsend.sendSony(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendSony(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_DISH
    if (strstr(protocol_name, "IR_DISH") != NULL){
      if (valueBITS == 0) valueBITS = DISH_BITS;
        #ifdef ESP8266
            irsend.sendDISH(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendDISH(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_RC5
    if (strstr(protocol_name, "IR_RC5") != NULL){
      if (valueBITS == 0) valueBITS = RC5_BITS;
        #ifdef ESP8266
            irsend.sendRC5(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendRC5(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_RC6
    if (strstr(protocol_name, "IR_RC6") != NULL){
      if (valueBITS == 0) valueBITS = RC6_MODE0_BITS;
        #ifdef ESP8266
            irsend.sendRC6(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendRC6(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_Sharp
    if (strstr(protocol_name, "IR_Sharp") != NULL){
      if (valueBITS == 0) valueBITS = SHARP_BITS;
        #ifdef ESP8266
            irsend.sendSharpRaw(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendSharpRaw(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_SAMSUNG
    if (strstr(protocol_name, "IR_SAMSUNG") != NULL){
      if (valueBITS == 0) valueBITS = SAMSUNG_BITS;
        #ifdef ESP8266
            irsend.sendSAMSUNG(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendSAMSUNG(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_JVC
    if (strstr(protocol_name, "IR_JVC") != NULL){
      if (valueBITS == 0) valueBITS = JVC_BITS;
        #ifdef ESP8266
            irsend.sendJVC(data, valueBITS, valueRPT);
        #else
            for (int i=0; i <= valueRPT; i++) irsend.sendJVC(data, valueBITS);
        #endif
      return true;
    }
  #endif
  #ifdef IR_PANASONIC
    if (strstr(protocol_name, "IR_PANASONIC") != NULL){
      #ifdef ESP8266
          if (valueBITS == 0) valueBITS = PANASONIC_BITS;
          irsend.sendPanasonic(PanasonicAddress, data, valueBITS, valueRPT);
      #else
          for (int i=0; i <= valueRPT; i++) irsend.sendPanasonic(PanasonicAddress, data);
      #endif
    return true;
    }
  #endif
  
  #ifdef ESP8266  // sendings not available on arduino
    #ifdef IR_COOLIX
      if (strstr(protocol_name, "IR_COOLIX") != NULL){
        if (valueBITS == 0) valueBITS = kCoolixBits;
        irsend.sendCOOLIX(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_RCMM
      if (strstr(protocol_name, "IR_RCMM") != NULL){
        if (valueBITS == 0) valueBITS = kRCMMBits;
        irsend.sendRCMM(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_DENON
      if (strstr(protocol_name, "IR_DENON") != NULL){
        if (valueBITS == 0) valueBITS = DENON_BITS;
        irsend.sendDenon(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_GICABLE
      if (strstr(protocol_name, "IR_GICABLE") != NULL){
        if (valueBITS == 0) valueBITS = kGicableBits;
        if (valueRPT == repeatIRwNumber) valueRPT = std::max(valueRPT, kGicableMinRepeat);
        irsend.sendGICable(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_SHERWOOD
      if (strstr(protocol_name, "IR_SHERWOOD") != NULL){
        if (valueBITS == 0) valueBITS = kSherwoodBits;
        if (valueRPT == repeatIRwNumber) valueRPT = std::max(valueRPT, kSherwoodMinRepeat);
        irsend.sendSherwood(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_MITSUBISHI
      if (strstr(protocol_name, "IR_MITSUBISHI") != NULL){
        if (valueBITS == 0) valueBITS = kMitsubishiBits;
        if (valueRPT == repeatIRwNumber) valueRPT = std::max(valueRPT, kMitsubishiMinRepeat);
        irsend.sendMitsubishi(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_NIKAI
      if (strstr(protocol_name, "IR_NIKAI") != NULL){
        if (valueBITS == 0) valueBITS = kNikaiBits;
        irsend.sendNikai(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_MIDEA
      if (strstr(protocol_name, "IR_MIDEA") != NULL){
        if (valueBITS == 0) valueBITS = kMideaBits;
        irsend.sendMidea(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_MAGIQUEST
      if (strstr(protocol_name, "IR_MAGIQUEST") != NULL){
        if (valueBITS == 0) valueBITS = kMagiquestBits;
        irsend.sendMagiQuest(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_LASERTAG
      if (strstr(protocol_name, "IR_LASERTAG") != NULL){
        if (valueBITS == 0) valueBITS = kLasertagBits;
        irsend.sendLasertag(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_CARRIER_AC
      if (strstr(protocol_name, "IR_CARRIER_AC") != NULL){
        if (valueBITS == 0) valueBITS = kCarrierAcBits;
        irsend.sendCarrierAC(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_MITSUBISHI2
      if (strstr(protocol_name, "IR_MITSUBISHI2") != NULL){
        if (valueBITS == 0) valueBITS = kMitsubishiBits;
        if (valueRPT == repeatIRwNumber) valueRPT = std::max(valueRPT, kMitsubishiMinRepeat);
        irsend.sendMitsubishi2(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_AIWA_RC_T501
      if (strstr(protocol_name, "IR_AIWA_RC_T501") != NULL){
        if (valueBITS == 0) valueBITS = kAiwaRcT501Bits;
        if (valueRPT == repeatIRwNumber) valueRPT = std::max(valueRPT, kAiwaRcT501MinRepeats);
        irsend.sendAiwaRCT501(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_SANYO
      if (strstr(protocol_name, "IR_SANYO") != NULL){
        if (valueBITS == 0) valueBITS = kSanyoLC7461Bits;
        irsend.sendSanyoLC7461(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_DAIKIN
      if (strstr(protocol_name, "IR_DAIKIN") != NULL){
        if (valueBITS == 0) valueBITS = kDaikinStateLength;
        irsend.sendDaikin(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_KELVINATOR
      if (strstr(protocol_name, "IR_KELVINATOR") != NULL){
        if (valueBITS == 0) valueBITS = kKelvinatorStateLength;
        irsend.sendKelvinator(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_MITSUBISHI_AC
      if (strstr(protocol_name, "IR_MITSUBISHI_AC") != NULL){
        if (valueBITS == 0) valueBITS = kMitsubishiACStateLength;
        irsend.sendMitsubishiAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_SANYO
      if (strstr(protocol_name, "IR_SANYO") != NULL){
        if (valueBITS == 0) valueBITS = kSanyoLC7461Bits;
        irsend.sendSanyoLC7461(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_GREE
      if (strstr(protocol_name, "IR_GREE") != NULL){
        if (valueBITS == 0) valueBITS = kGreeStateLength;
        irsend.sendGree(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_ARGO
      if (strstr(protocol_name, "IR_ARGO") != NULL){
        if (valueBITS == 0) valueBITS = kArgoStateLength;
        irsend.sendArgo(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_TROTEC
      if (strstr(protocol_name, "IR_TROTEC") != NULL){
        if (valueBITS == 0) valueBITS = kTrotecStateLength;
        irsend.sendTrotec(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_TOSHIBA_AC
      if (strstr(protocol_name, "IR_TOSHIBA_AC") != NULL){
        if (valueBITS == 0) valueBITS = kToshibaACBits;
        irsend.sendToshibaAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_FUJITSU_AC
      if (strstr(protocol_name, "IR_FUJITSU_AC") != NULL){
        irsend.sendFujitsuAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_MAGIQUEST
      if (strstr(protocol_name, "IR_MAGIQUEST") != NULL){
        if (valueBITS == 0) valueBITS = kMagiquestBits;
        irsend.sendMagiQuest(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_HAIER_AC
      if (strstr(protocol_name, "IR_HAIER_AC") != NULL){
        if (valueBITS == 0) valueBITS = kHaierACStateLength;
        irsend.sendHaierAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_HITACHI_AC
      if (strstr(protocol_name, "IR_HITACHI_AC") != NULL){
        if (valueBITS == 0) valueBITS = kHitachiAc2StateLength;
        irsend.sendHitachiAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_HITACHI_AC1
      if (strstr(protocol_name, "IR_HITACHI_AC1") != NULL){
        if (valueBITS == 0) valueBITS = kHitachiAc2StateLength;
        irsend.sendHitachiAC1(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_HITACHI_AC2
      if (strstr(protocol_name, "IR_HITACHI_AC2") != NULL){
        if (valueBITS == 0) valueBITS = kHitachiAc2StateLength;
        irsend.sendHitachiAC2(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_GICABLE
      if (strstr(protocol_name, "IR_GICABLE") != NULL){
        if (valueBITS == 0) valueBITS = kGicableBits;
        irsend.sendGICable(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_HAIER_AC_YRW02
      if (strstr(protocol_name, "IR_HAIER_AC_YRW02") != NULL){
        if (valueBITS == 0) valueBITS = kHaierACYRW02StateLength;
        irsend.sendHaierACYRW02(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_WHIRLPOOL_AC
      if (strstr(protocol_name, "IR_WHIRLPOOL_AC") != NULL){
        if (valueBITS == 0) valueBITS = kWhirlpoolAcStateLength;
        irsend.sendWhirlpoolAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_SAMSUNG_AC
      if (strstr(protocol_name, "IR_SAMSUNG_AC") != NULL){
        if (valueBITS == 0) valueBITS = kSamsungAcStateLength;
        irsend.sendSamsungAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_LUTRON
      if (strstr(protocol_name, "IR_LUTRON") != NULL){
        if (valueBITS == 0) valueBITS = kLutronBits;
        irsend.sendLutron(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_ELECTRA_AC
      if (strstr(protocol_name, "IR_ELECTRA_AC") != NULL){
        if (valueBITS == 0) valueBITS = kElectraAcStateLength;
        irsend.sendElectraAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_PANASONIC_AC
      if (strstr(protocol_name, "IR_PANASONIC_AC") != NULL){
        if (valueBITS == 0) valueBITS = kPanasonicAcStateLength;
        irsend.sendPanasonicAC(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_PIONEER
      if (strstr(protocol_name, "IR_PIONEER") != NULL){
        if (valueBITS == 0) valueBITS = kPioneerBits;
        irsend.sendPioneer(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_LG2
      if (strstr(protocol_name, "IR_LG2") != NULL){
        if (valueBITS == 0) valueBITS = kLgBits;
        irsend.sendLG2(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_MWM
      if (strstr(protocol_name, "IR_MWM") != NULL){
        irsend.sendMWM(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_DAIKIN2
      if (strstr(protocol_name, "IR_DAIKIN2") != NULL){
        if (valueBITS == 0) valueBITS = kDaikin2StateLength;
        irsend.sendDaikin2(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_VESTEL_AC
      if (strstr(protocol_name, "IR_VESTEL_AC") != NULL){
        if (valueBITS == 0) valueBITS = kVestelAcBits;
        irsend.sendVestelAc(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_SAMSUNG36
      if (strstr(protocol_name, "IR_SAMSUNG36") != NULL){
        if (valueBITS == 0) valueBITS = kSamsung36Bits;
        irsend.sendSamsung36(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_TCL112AC
      if (strstr(protocol_name, "IR_TCL112AC") != NULL){
        if (valueBITS == 0) valueBITS = kTcl112AcStateLength;
        irsend.sendTcl112Ac(datastring, valueBITS, valueRPT);
        return true;
      }
    #endif
    #ifdef IR_TECO
      if (strstr(protocol_name, "IR_TECO") != NULL){
        if (valueBITS == 0) valueBITS = kTecoBits;
        irsend.sendTeco(data, valueBITS, valueRPT);
        return true;
      }
    #endif
    return false;
  #endif
}
#endif
