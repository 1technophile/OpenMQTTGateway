/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data based on ESPilight library
 - publish MQTT data to a different topic related to received 433Mhz signal based on ESPilight library

    Copyright: (c)Florian ROBERT
    Pilight Gateway made by steadramon, improvments with the help of puuu
  
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

#ifdef ZgatewayPilight

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

#  include <ESPiLight.h>
ESPiLight rf(RF_EMITTER_GPIO); // use -1 to disable transmitter

void pilightCallback(const String& protocol, const String& message, int status,
                     size_t repeats, const String& deviceID) {
  if (status == VALID) {
    Log.trace(F("Creating RF PiLight buffer" CR));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject& RFPiLightdata = jsonBuffer.createObject();
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer2;
    JsonObject& msg = jsonBuffer2.parseObject(message);
    RFPiLightdata.set("message", msg);
    RFPiLightdata.set("protocol", (char*)protocol.c_str());
    RFPiLightdata.set("length", (char*)deviceID.c_str());
    RFPiLightdata.set("repeats", (int)repeats);
    RFPiLightdata.set("status", (int)status);
    pub(subjectPilighttoMQTT, RFPiLightdata);
    if (repeatPilightwMQTT) {
      Log.trace(F("Pub Pilight for rpt" CR));
      pub(subjectMQTTtoPilight, RFPiLightdata);
    }
  }
}

void setupPilight() {
#  ifndef ZgatewayRF&& ZgatewayRF2 //receiving with Pilight is not compatible with ZgatewayRF or RF2 or RF315 as far as I can tell
#    ifdef ZradioCC1101 //receiving with CC1101
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);
  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY);
  rf.enableReceiver();
#    endif
  rf.setCallback(pilightCallback);
  rf.initReceiver(RF_RECEIVER_GPIO);
  pinMode(RF_EMITTER_GPIO, OUTPUT); // Set this here, because if this is the RX pin it was reset to INPUT by Serial.end();
  Log.notice(F("RF_EMITTER_GPIO: %d " CR), RF_EMITTER_GPIO);
  Log.notice(F("RF_RECEIVER_GPIO: %d " CR), RF_RECEIVER_GPIO);
  Log.trace(F("ZgatewayPilight setup done " CR));
#  else
  Log.trace(F("ZgatewayPilight setup cannot be done, comment first ZgatewayRF && ZgatewayRF2" CR));
#  endif
}

void PilighttoMQTT() {
  rf.loop();
}

void MQTTtoPilight(char* topicOri, JsonObject& Pilightdata) {
#  ifdef ZradioCC1101
  ELECHOUSE_cc1101.SetTx(CC1101_FREQUENCY); // set Transmit on
  rf.disableReceiver();
#  endif
  int result = 0;

  if (cmpToMainTopic(topicOri, subjectMQTTtoPilight)) {
    Log.trace(F("MQTTtoPilight json data analysis" CR));
    const char* message = Pilightdata["message"];
    const char* protocol = Pilightdata["protocol"];
    const char* raw = Pilightdata["raw"];
    if (raw) {
      int msgLength = 0;
      uint16_t codes[MAXPULSESTREAMLENGTH];
      msgLength = rf.stringToPulseTrain(
          raw,
          codes, MAXPULSESTREAMLENGTH);
      if (msgLength > 0) {
        rf.sendPulseTrain(codes, msgLength);
        Log.notice(F("MQTTtoPilight raw ok" CR));
        result = msgLength;
      } else {
        Log.trace(F("MQTTtoPilight raw KO" CR));
        switch (result) {
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_C:
            Log.trace(F("'c' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_P:
            Log.trace(F("'p' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_END:
            Log.trace(F("';' or '@' not found in data string" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_TYPE:
            Log.trace(F("pulse type not defined" CR));
            break;
        }
      }
    } else if (message && protocol) {
      Log.trace(F("MQTTtoPilight msg & protocol ok" CR));
      result = rf.send(protocol, message);
    } else {
      Log.error(F("MQTTtoPilight failed json read" CR));
    }

    if (result > 0) {
      Log.trace(F("Adv data MQTTtoPilight push state via PilighttoMQTT" CR));
      pub(subjectGTWPilighttoMQTT, Pilightdata);
    } else {
      switch (result) {
        case ESPiLight::ERROR_UNAVAILABLE_PROTOCOL:
          Log.error(F("protocol is not available" CR));
          break;
        case ESPiLight::ERROR_INVALID_PILIGHT_MSG:
          Log.error(F("message is invalid" CR));
          break;
        case ESPiLight::ERROR_INVALID_JSON:
          Log.error(F("message is not a proper json object" CR));
          break;
        case ESPiLight::ERROR_NO_OUTPUT_PIN:
          Log.error(F("no transmitter pin" CR));
          break;
        default:
          Log.error(F("invalid json data, can't read raw or message/protocol" CR));
          break;
      }
    }
  }
#  ifdef ZradioCC1101
  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY); // set Receive on
  rf.enableReceiver();
#  endif
}
#endif
