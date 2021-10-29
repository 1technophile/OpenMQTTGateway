/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal
 - leverage the rtl_433 device decoders on a ESP32 device

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
#ifdef ZgatewayRTL_433
#  ifndef ZradioCC1101
#    error "CC1101 is the only supported receiver module for RTL_433 and needs to be enabled."
#  endif

#  include <rtl_433_ESP.h>

char messageBuffer[JSON_MSG_BUFFER];

rtl_433_ESP rtl_433(-1); // use -1 to disable transmitter

#  include <ELECHOUSE_CC1101_SRC_DRV.h>

void rtl_433_Callback(char* message) {
  DynamicJsonDocument jsonBuffer2(JSON_MSG_BUFFER);
  JsonObject RFrtl_433_ESPdata = jsonBuffer2.to<JsonObject>();
  auto error = deserializeJson(jsonBuffer2, message);
  if (error) {
    Log.error(F("deserializeJson() failed: %s" CR), error.c_str());
    return;
  }

  String topic = String(subjectRTL_433toMQTT);
#  ifdef valueAsASubject
  String model = RFrtl_433_ESPdata["model"];
  String id = RFrtl_433_ESPdata["id"];
  if (model != 0) {
    topic = topic + "/" + model;
    if (id != 0) {
      topic = topic + "/" + id;
    }
  }
#  endif

  pub((char*)topic.c_str(), RFrtl_433_ESPdata);
#  ifdef MEMORY_DEBUG
  Log.trace(F("Post rtl_433_Callback: %d" CR), ESP.getFreeHeap());
#  endif
}

void setupRTL_433() {
  rtl_433.initReceiver(RF_RECEIVER_GPIO, receiveMhz);
  rtl_433.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);
  Log.trace(F("ZgatewayRTL_433 command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoRTL_433);
  Log.notice(F("ZgatewayRTL_433 setup done " CR));
}

void RTL_433Loop() {
  rtl_433.loop();
}

extern void MQTTtoRTL_433(char* topicOri, JsonObject& RTLdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRTL_433)) {
    float tempMhz = RTLdata["mhz"];
    bool success = false;
    if (RTLdata.containsKey("mhz") && validFrequency(tempMhz)) {
      receiveMhz = tempMhz;
      Log.notice(F("RTL_433 Receive mhz: %F" CR), receiveMhz);
      success = true;
    }
    if (RTLdata.containsKey("active")) {
      Log.trace(F("RTL_433 active:" CR));
      activeReceiver = ACTIVE_RTL; // Enable RTL_433 Gateway
      success = true;
    }
    if (RTLdata.containsKey("rssi")) {
      int minimumRssi = RTLdata["rssi"] | 0;
      Log.notice(F("RTL_433 minimum RSSI: %d" CR), minimumRssi);
      rtl_433.setMinimumRSSI(minimumRssi);
      success = true;
    }
    if (RTLdata.containsKey("debug")) {
      int debug = RTLdata["debug"] | -1;
      Log.notice(F("RTL_433 set debug: %d" CR), debug);
      rtl_433.setDebug(debug);
      rtl_433.initReceiver(RF_RECEIVER_GPIO, receiveMhz);
      success = true;
    }
    if (RTLdata.containsKey("status")) {
      Log.notice(F("RTL_433 get status:" CR));
      rtl_433.getStatus(1);
      success = true;
    }
    if (success) {
      pub(subjectRTL_433toMQTT, RTLdata);
    } else {
      pub(subjectRTL_433toMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("MQTTtoRTL_433 Fail json" CR));
    }
    enableActiveReceiver(false);
  }
}

extern void enableRTLreceive() {
  Log.notice(F("Switching to RTL_433 Receiver: %FMhz" CR), receiveMhz);
#  ifdef ZgatewayRF
  disableRFReceive();
#  endif
#  ifdef ZgatewayRF2
  disableRF2Receive();
#  endif
#  ifdef ZgatewayPilight
  disablePilightReceive();
#  endif
  ELECHOUSE_cc1101.SetRx(receiveMhz); // set Receive on
  rtl_433.enableReceiver(RF_RECEIVER_GPIO);
  pinMode(RF_EMITTER_GPIO, OUTPUT); // Set this here, because if this is the RX pin it was reset to INPUT by Serial.end();
}

extern void disableRTLreceive() {
  Log.trace(F("disableRTLreceive" CR));
  rtl_433.enableReceiver(-1);
  rtl_433.disableReceiver();
}

extern int getRTLMinimumRSSI() {
  return rtl_433.minimumRssi;
}

extern int getRTLCurrentRSSI() {
  return rtl_433.currentRssi;
}

extern int getRTLMessageCount() {
  return rtl_433.messageCount;
}

#endif