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
#include "User_config.h"

#ifdef Zgateway2G

#  include <A6lib.h> // library for controling A6 or A7 module
#  include <ArduinoJson.h>

// Instantiate the library with TxPin, RxPin.
A6lib A6l(_2G_TX_GPIO, _2G_RX_GPIO); //D6 to A6 RX, D7 to A6 TX

int unreadSMSLocs[50] = {0};
int unreadSMSNum = 0;
SMSmessage sms;

void setup2G() {
  Log.notice(F("_2G_TX_GPIO: %d " CR), _2G_TX_GPIO);
  Log.notice(F("_2G_RX_GPIO: %d " CR), _2G_RX_GPIO);
  setupGSM(false);
  Log.trace(F("Zgateway2G setup done " CR));
}

void setupGSM(bool deleteSMS) {
  Log.trace(F("Init 2G module: %d" CR), _2G_PWR_GPIO);
  delay(1000);
  // Power-cycle the module to reset it.
  A6l.powerCycle(_2G_PWR_GPIO);
  Log.notice(F("waiting for network connection at bd: %d" CR), _2G_MODULE_BAUDRATE);
  A6l.blockUntilReady(_2G_MODULE_BAUDRATE);
  Log.notice(F("A6/A7 gsm ready" CR));
  signalStrengthAnalysis();
  delay(1000);
  // deleting all sms
  if (deleteSMS) {
    if (A6l.deleteSMS(1, 4) == A6_OK) {
      Log.notice(F("delete SMS OK" CR));
    } else {
      Log.error(F("delete SMS KO" CR));
    }
  }
}

void signalStrengthAnalysis() {
  int signalStrength = 0;
  signalStrength = A6l.getSignalStrength();
  Log.trace(F("Signal strength: %d" CR), signalStrength);
  if (signalStrength < _2G_MIN_SIGNAL || signalStrength > _2G_MAX_SIGNAL) {
    Log.trace(F("Signal too low restart the module" CR));
    setupGSM(false); // if we are below or above a threshold signal we relaunch the setup of GSM module
  }
}

bool _2GtoMQTT() {
  // Get the memory locations of unread SMS messages.
  unreadSMSNum = A6l.getUnreadSMSLocs(unreadSMSLocs, 512);
  Log.trace(F("Creating SMS  buffer" CR));
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject SMSdata = jsonBuffer.to<JsonObject>();
  for (int i = 0; i < unreadSMSNum; i++) {
    Log.notice(F("New  message at index: %d" CR), unreadSMSNum);
    sms = A6l.readSMS(unreadSMSLocs[i]);
    SMSdata["message"] = (const char*)sms.message.c_str();
    SMSdata["date"] = (const char*)sms.date.c_str();
    SMSdata["phone"] = (const char*)sms.number.c_str();
    A6l.deleteSMS(unreadSMSLocs[i]); // we delete the SMS received
    Log.trace(F("Adv data 2GtoMQTT" CR));
    pub(subject2GtoMQTT, SMSdata);
    return true;
  }
  return false;
}
#  if simpleReceiving
void MQTTto2G(char* topicOri, char* datacallback) {
  String data = datacallback;
  String topic = topicOri;

  if (cmpToMainTopic(topicOri, subjectMQTTto2G)) {
    Log.trace(F("MQTTto2G data analysis" CR));
    // 2G DATA ANALYSIS
    String phone_number = "";
    int pos0 = topic.lastIndexOf(_2GPhoneKey);
    if (pos0 != -1) {
      pos0 = pos0 + strlen(_2GPhoneKey);
      phone_number = topic.substring(pos0);
      Log.notice(F("MQTTto2G phone: %s" CR), (char*)phone_number.c_str());
      Log.notice(F("MQTTto2G sms: %s" CR), (char*)data.c_str());
      if (A6l.sendSMS(phone_number, data) == A6_OK) {
        Log.notice(F("SMS OK" CR));
        // Acknowledgement to the GTW2G topic
        pub(subjectGTW2GtoMQTT, "SMS OK"); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      } else {
        Log.error(F("SMS KO" CR));
        // Acknowledgement to the GTW2G topic
        pub(subjectGTW2GtoMQTT, "SMS KO"); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      }
    } else {
      Log.error(F("MQTTto2G Fail reading phone number" CR));
    }
  }
}
#  endif

#  if jsonReceiving
void MQTTto2G(char* topicOri, JsonObject& SMSdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTto2G)) {
    const char* sms = SMSdata["message"];
    const char* phone = SMSdata["phone"];
    Log.trace(F("MQTTto2G json data analysis" CR));
    if (sms && phone) {
      Log.notice(F("MQTTto2G phone: %s" CR), phone);
      Log.notice(F("MQTTto2G sms: %s" CR), sms);
      if (A6l.sendSMS(String(phone), String(sms)) == A6_OK) {
        Log.notice(F("SMS OK" CR));
        // Acknowledgement to the GTW2G topic
        pub(subjectGTW2GtoMQTT, "SMS OK"); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      } else {
        Log.error(F("SMS KO" CR));
        pub(subjectGTW2GtoMQTT, "SMS KO"); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      }
    } else {
      Log.error(F("MQTTto2G failed json read" CR));
    }
  }
}
#  endif
#endif
