/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send SMS corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received SMS

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
#ifdef Zgateway2G

#include <A6lib.h> // library for controling A6 or A7 module

// Instantiate the library with TxPin, RxPin.
A6lib A6l(_2G_TX_PIN, _2G_RX_PIN); //D6 to A6 RX, D7 to A6 TX 
  
int unreadSMSLocs[50] = {0};
int unreadSMSNum = 0;
SMSmessage sms;

void setup2G(){

  trc(F("_2G_TX_PIN "));
  trc(_2G_TX_PIN);
  trc(F("_2G_RX_PIN "));
  trc(_2G_RX_PIN);
  setupGSM(false);
  trc(F("Zgateway2G setup done "));
}

void setupGSM(boolean deleteSMS){
    trc(F("Init 2G module: "));
    trc(_2G_PWR_PIN);
    delay(1000);
    // Power-cycle the module to reset it.
    A6l.powerCycle(_2G_PWR_PIN);
    trc(F("waiting for network connection"));
    A6l.blockUntilReady(_2G_MODULE_BAUDRATE);
    trc(F("A6/A7 gsm ready"));
    signalStrengthAnalysis();
    delay(1000);
    // deleting all sms
    if (deleteSMS){
      if (A6l.deleteSMS(1,4) == A6_OK ) {
        trc(F("delete SMS OK"));
      }else{
        trc(F("delete SMS KO"));
      }
    }
}

void signalStrengthAnalysis(){
    int signalStrength = 0;
    signalStrength = A6l.getSignalStrength();
    trc(F("Signal strength: "));
    trc(signalStrength);
    if (signalStrength < _2G_MIN_SIGNAL || signalStrength > _2G_MAX_SIGNAL ) {
      trc(F("Signal too low restart the module"));
      setupGSM(false); // if we are below or above a threshold signal we relaunch the setup of GSM module
    }
}

boolean _2GtoMQTT(){
    // Get the memory locations of unread SMS messages.
    unreadSMSNum = A6l.getUnreadSMSLocs(unreadSMSLocs, 512);
    trc(F("Creating SMS  buffer"));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject& SMSdata = jsonBuffer.createObject();
    for (int i = 0; i < unreadSMSNum; i++) {
        trc(F("New  message at index: "));
        trc(unreadSMSNum);
        sms = A6l.readSMS(unreadSMSLocs[i]);
        SMSdata.set("message", (char *)sms.message.c_str());
        SMSdata.set("date", (char *)sms.date.c_str());
        SMSdata.set("phone", (char *)sms.number.c_str());
        A6l.deleteSMS(unreadSMSLocs[i]); // we delete the SMS received
        trc(F("Adv data 2GtoMQTT"));
        pub(subject2GtoMQTT,SMSdata);
        return true;   
    }
    return false;
}
#ifdef simpleReceiving
  void MQTTto2G(char * topicOri, char * datacallback) {
  
    String data = datacallback;
    String topic = topicOri;
    
    if (topic == subjectMQTTto2G) {
      trc(F("MQTTto2G data analysis"));
      // 2G DATA ANALYSIS
      String phone_number = "";
      int pos0 = topic.lastIndexOf(_2GPhoneKey);
      if (pos0 != -1) {
        pos0 = pos0 + strlen(_2GPhoneKey);
        phone_number = topic.substring(pos0);
        trc(F("MQTTto2G phone ok"));
        trc(phone_number);
        trc(F("MQTTto2G sms"));
        trc(data);
        if (A6l.sendSMS(phone_number,data) == A6_OK ) {
          trc(F("SMS OK"));
          // Acknowledgement to the GTW2G topic
          pub(subjectGTW2GtoMQTT, datacallback);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
        }else{
          trc(F("SMS KO"));
          // Acknowledgement to the GTW2G topic
          pub(subjectGTW2GtoMQTT, "SMS KO");// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
        }
     }else{
        trc(F("MQTTto2G Fail reading phone number"));
     }
    }
  }
#endif

#ifdef jsonReceiving
  void MQTTto2G(char * topicOri, JsonObject& SMSdata) {
    
   if (strcmp(topicOri,subjectMQTTto2G) == 0){
      const char * sms = SMSdata["message"];
      const char * phone = SMSdata["phone"];
      trc(F("MQTTto2G json data analysis"));
      if (sms && phone) {
        trc(F("MQTTto2G sms & phone ok"));
        trc(sms);
        if (A6l.sendSMS(String(phone),String(sms)) == A6_OK ) {
          trc(F("SMS OK"));
          // Acknowledgement to the GTW2G topic
          pub(subjectGTW2GtoMQTT, SMSdata);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
        }else{
          trc(F("SMS KO"));
          pub(subjectGTW2GtoMQTT, "SMS KO");// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
        }
      }else{
        trc(F("MQTTto2G Fail reading from json"));
      }
    }
    
  }
#endif
#endif
