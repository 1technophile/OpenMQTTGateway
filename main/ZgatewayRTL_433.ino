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

#  include <rtl_433_ESP.h>

#  define CC1101_FREQUENCY 433.92
#  define ONBOARD_LED      2

char messageBuffer[JSON_MSG_BUFFER];

rtl_433_ESP rtl_433(-1); // use -1 to disable transmitter

#  include <ELECHOUSE_CC1101_SRC_DRV.h>

void rtl_433_Callback(char* message) {
  DynamicJsonBuffer jsonBuffer2(JSON_MSG_BUFFER);
  JsonObject& RFrtl_433_ESPdata = jsonBuffer2.parseObject(message);

  pub(subjectRTL_433toMQTT, RFrtl_433_ESPdata);
#  ifdef MEMORY_DEBUG
  Log.trace(F("Post rtl_433_Callback: %d" CR), ESP.getFreeHeap());
#  endif
}

void setupRTL_433() {
  rtl_433.initReceiver(RF_RECEIVER_GPIO, receiveMhz);
  rtl_433.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);
  Log.notice(F("ZgatewayRTL_433 command topic: %s%s" CR), mqtt_topic, subjectMQTTtoRTL_433);
  enableRTLreceive();
  Log.trace(F("ZgatewayRTL_433 setup done " CR));
}

void RTL_433Loop() {
  rtl_433.loop();
}

extern void MQTTtoRTL_433(char* topicOri, JsonObject& RTLdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRTL_433)) {
    Log.trace(F("MQTTtoRTL_433 %s" CR), topicOri);
    float tempMhz = RTLdata["mhz"] | 0;
    int minimumRssi = RTLdata["rssi"] | 0;
    int debug = RTLdata["debug"] | -1;
    int status = RTLdata["status"] | -1;
    if (tempMhz != 0 && validFrequency((int)tempMhz)) {
      //  activeReceiver = RTL; // Enable RTL_433 Gateway
      receiveMhz = tempMhz;
      Log.notice(F("RTL_433 Receive mhz: %F" CR), receiveMhz);
      pub(subjectRTL_433toMQTT, RTLdata);
    } else if (minimumRssi != 0) {
      Log.notice(F("RTL_433 minimum RSSI: %d" CR), minimumRssi);
      rtl_433.setMinimumRSSI(minimumRssi);
      pub(subjectRTL_433toMQTT, RTLdata);
    } else if (debug >= 0 && debug <= 4) {
      Log.notice(F("RTL_433 set debug: %d" CR), debug);
      rtl_433.setDebug(debug);
      rtl_433.initReceiver(RF_RECEIVER_GPIO, receiveMhz);
      pub(subjectRTL_433toMQTT, RTLdata);
    } else if (status >= 0) {
      Log.notice(F("RTL_433 get status: %d" CR), status);
      rtl_433.getStatus(status);
      pub(subjectRTL_433toMQTT, RTLdata);
    } else {
      pub(subjectRTL_433toMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("MQTTtoRTL_433 Fail json" CR));
    }
  }
}

extern void enableRTLreceive() {
  Log.trace(F("enableRTLreceive: %F" CR), receiveMhz);
  ELECHOUSE_cc1101.SetRx(receiveMhz); // set Receive on
  rtl_433.enableReceiver(RF_RECEIVER_GPIO);
  // rtl_433.initReceiver(RF_RECEIVER_GPIO);
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