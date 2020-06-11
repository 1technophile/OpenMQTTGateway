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
#include "User_config.h"

#ifdef ZgatewayRF

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

#  include <RCSwitch.h> // library for controling Radio frequency switch

RCSwitch mySwitch = RCSwitch();

#  ifdef ZmqttDiscovery
void RFtoMQTTdiscovery(unsigned long MQTTvalue) { //on the fly switch creation from received RF values
  char val[11];
  sprintf(val, "%lu", MQTTvalue);
  Log.trace(F("switchRFDiscovery" CR));
  char* switchRF[8] = {"switch", val, "", "", "", val, "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySwitch: %s" CR), switchRF[1]);
  createDiscovery(switchRF[0],
                  subjectRFtoMQTT, switchRF[1], (char*)getUniqueId(switchRF[1], switchRF[2]).c_str(),
                  will_Topic, switchRF[3], switchRF[4],
                  switchRF[5], switchRF[6], switchRF[7],
                  0, "", "", true, subjectMQTTtoRF);
}
#  endif

void setupRF() {
  //RF init parameters
  Log.notice(F("RF_EMITTER_GPIO: %d " CR), RF_EMITTER_GPIO);
  Log.notice(F("RF_RECEIVER_GPIO: %d " CR), RF_RECEIVER_GPIO);
#  ifdef ZradioCC1101 //receiving with CC1101
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);
  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY);
#  endif
  mySwitch.enableTransmit(RF_EMITTER_GPIO);
  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT);
  mySwitch.enableReceive(RF_RECEIVER_GPIO);
  Log.trace(F("ZgatewayRF setup done" CR));
}

void RFtoMQTT() {
  if (mySwitch.available()) {
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(4);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
    JsonObject& RFdata = jsonBuffer.createObject();
    Log.trace(F("Rcv. RF" CR));
#  ifdef ESP32
    Log.trace(F("RF Task running on core :%d" CR), xPortGetCoreID());
#  endif
    RFdata.set("value", (unsigned long)mySwitch.getReceivedValue());
    RFdata.set("protocol", (int)mySwitch.getReceivedProtocol());
    RFdata.set("length", (int)mySwitch.getReceivedBitlength());
    RFdata.set("delay", (int)mySwitch.getReceivedDelay());
    mySwitch.resetAvailable();

    unsigned long MQTTvalue = RFdata.get<unsigned long>("value");
    if (!isAduplicate(MQTTvalue) && MQTTvalue != 0) { // conditions to avoid duplications of RF -->MQTT
#  ifdef ZmqttDiscovery //component creation for HA
      RFtoMQTTdiscovery(MQTTvalue);
#  endif
      pub(subjectRFtoMQTT, RFdata);
      Log.trace(F("Store val: %lu" CR), MQTTvalue);
      storeValue(MQTTvalue);
      if (repeatRFwMQTT) {
        Log.trace(F("Pub RF for rpt" CR));
        pub(subjectMQTTtoRF, RFdata);
      }
    }
  }
}

#  ifdef simpleReceiving
void MQTTtoRF(char* topicOri, char* datacallback) {
#    ifdef ZradioCC1101 // set Receive off and Transmitt on
  ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY);
  mySwitch.disableReceive();
  mySwitch.enableTransmit(RF_EMITTER_GPIO);
#    endif
  unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295

  // RF DATA ANALYSIS
  //We look into the subject to see if a special RF protocol is defined
  String topic = topicOri;
  int valuePRT = 0;
  int valuePLSL = 0;
  int valueBITS = 0;
  int pos = topic.lastIndexOf(RFprotocolKey);
  if (pos != -1) {
    pos = pos + +strlen(RFprotocolKey);
    valuePRT = (topic.substring(pos, pos + 1)).toInt();
  }
  //We look into the subject to see if a special RF pulselength is defined
  int pos2 = topic.lastIndexOf(RFpulselengthKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RFpulselengthKey);
    valuePLSL = (topic.substring(pos2, pos2 + 3)).toInt();
  }
  int pos3 = topic.lastIndexOf(RFbitsKey);
  if (pos3 != -1) {
    pos3 = pos3 + strlen(RFbitsKey);
    valueBITS = (topic.substring(pos3, pos3 + 2)).toInt();
  }

  if ((cmpToMainTopic(topicOri, subjectMQTTtoRF)) && (valuePRT == 0) && (valuePLSL == 0) && (valueBITS == 0)) {
    Log.trace(F("MQTTtoRF dflt" CR));
    mySwitch.setProtocol(1, 350);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWRFtoMQTT, datacallback);
  } else if ((valuePRT != 0) || (valuePLSL != 0) || (valueBITS != 0)) {
    Log.trace(F("MQTTtoRF usr par." CR));
    if (valuePRT == 0)
      valuePRT = 1;
    if (valuePLSL == 0)
      valuePLSL = 350;
    if (valueBITS == 0)
      valueBITS = 24;
    Log.notice(F("RF Protocol:%d" CR), valuePRT);
    Log.notice(F("RF Pulse Lgth: %d" CR), valuePLSL);
    Log.notice(F("Bits nb: %d" CR), valueBITS);
    mySwitch.setProtocol(valuePRT, valuePLSL);
    mySwitch.send(data, valueBITS);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWRFtoMQTT, datacallback); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
  }
#    ifdef ZradioCC1101 // set Receive on and Transmitt off
  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY);
  mySwitch.disableTransmit();
  mySwitch.enableReceive(RF_RECEIVER_GPIO);
#    endif
}
#  endif

#  ifdef jsonReceiving
void MQTTtoRF(char* topicOri, JsonObject& RFdata) { // json object decoding
#    ifdef ZradioCC1101 // set Receive off and Transmitt on
  ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY);
  mySwitch.disableReceive();
  mySwitch.enableTransmit(RF_EMITTER_GPIO);
#    endif
  if (cmpToMainTopic(topicOri, subjectMQTTtoRF)) {
    Log.trace(F("MQTTtoRF json" CR));
    unsigned long data = RFdata["value"];
    if (data != 0) {
      int valuePRT = RFdata["protocol"] | 1;
      int valuePLSL = RFdata["delay"] | 350;
      int valueBITS = RFdata["length"] | 24;
      int valueRPT = RFdata["repeat"] | RF_EMITTER_REPEAT;
      Log.notice(F("RF Protocol:%d" CR), valuePRT);
      Log.notice(F("RF Pulse Lgth: %d" CR), valuePLSL);
      Log.notice(F("Bits nb: %d" CR), valueBITS);
      mySwitch.setRepeatTransmit(valueRPT);
      mySwitch.setProtocol(valuePRT, valuePLSL);
      mySwitch.send(data, valueBITS);
      Log.notice(F("MQTTtoRF OK" CR));
      pub(subjectGTWRFtoMQTT, RFdata); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); // Restore the default value
    } else {
      Log.error(F("MQTTtoRF Fail json" CR));
    }
  }
#    ifdef ZradioCC1101 // set Receive on and Transmitt off
  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY);
  mySwitch.disableTransmit();
  mySwitch.enableReceive(RF_RECEIVER_GPIO);
#    endif
}
#  endif
#endif
