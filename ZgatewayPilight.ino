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
 #ifdef ZgatewayPilight
 #include <ESPiLight.h>
 ESPiLight rf(RF_EMITTER_PIN);  // use -1 to disable transmitter
void setupPilight(){
    #ifndef ZgatewayRF && ZgatewayRF2 && ZgatewayRF315 //receiving with Pilight is not compatible with ZgatewayRF or RF2 or RF315 as far as I can tell 
        rf.setCallback(pilightCallback);
        rf.initReceiver(RF_RECEIVER_PIN);
        trc(F("RF_EMITTER_PIN "));
        trc(String(RF_EMITTER_PIN));
        trc(F("RF_RECEIVER_PIN "));
        trc(String(RF_RECEIVER_PIN));
        trc(F("ZgatewayPilight setup done "));
    #else
        trc(F("ZgatewayPilight setup cannot be done, comment first ZgatewayRF && ZgatewayRF2 && ZgatewayRF315"));
    #endif
}

void PilighttoMQTT(){
  rf.loop();
}

void pilightCallback(const String &protocol, const String &message, int status,
                size_t repeats, const String &deviceID) {
   if (status == VALID) {
    trc(F("Creating RF PiLight buffer"));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject& RFPiLightdata = jsonBuffer.createObject();
    trc(F("Rcv. Pilight"));
    RFPiLightdata.set("message", (char *)message.c_str());
    RFPiLightdata.set("protocol",(char *)protocol.c_str());
    RFPiLightdata.set("length", (char *)deviceID.c_str());
    RFPiLightdata.set("repeats", (int)repeats);
    RFPiLightdata.set("status", (int)status);
    pub(subjectPilighttoMQTT,RFPiLightdata);
    if (repeatPilightwMQTT){
        trc(F("Pub Pilight for rpt"));
        pub(subjectMQTTtoPilight,RFPiLightdata);
    }
   }
}

void MQTTtoPilight(char * topicOri, JsonObject& Pilightdata) {

  int result = 0;
  
  if (strcmp(topicOri,subjectMQTTtoPilight) == 0){
    trc(F("MQTTtoPilight json data analysis"));
    const char * message = Pilightdata["message"];
    const char * protocol = Pilightdata["protocol"];
    const char * raw = Pilightdata["raw"];
    if(raw){
      int msgLength = 0;
      uint16_t codes[MAXPULSESTREAMLENGTH];
      msgLength = rf.stringToPulseTrain(
          raw,
          codes, MAXPULSESTREAMLENGTH);
      if (msgLength > 0) {
        trc(F("MQTTtoPilight raw ok"));
        rf.sendPulseTrain(codes, msgLength);
        result = msgLength;
      }else{
        trc(F("MQTTtoPilight raw KO"));
        switch (result) {
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_C:
            trc(F("'c' not found in string, or has no data"));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_P:
            trc(F("'p' not found in string, or has no data"));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_END:
            trc(F("';' or '@' not found in data string"));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_TYPE:
            trc(F("pulse type not defined"));
            break;
        }
      }
    }else if (message && protocol) {
      trc(F("MQTTtoPilight msg & protocol ok"));
      result = rf.send(protocol, message);
    }else{
      trc(F("MQTTtoPilight fail reading from json"));
    }

    if (result > 0) {
      trc(F("Adv data MQTTtoPilight push state via PilighttoMQTT"));
      pub(subjectGTWPilighttoMQTT, Pilightdata);
    } else {
      switch (result) {
        case ESPiLight::ERROR_UNAVAILABLE_PROTOCOL:
          trc(F("protocol is not available"));
          break;
        case ESPiLight::ERROR_INVALID_PILIGHT_MSG:
          trc(F("message is invalid"));
          break;
        case ESPiLight::ERROR_INVALID_JSON:
          trc(F("message is not a proper json object"));
          break;
        case ESPiLight::ERROR_NO_OUTPUT_PIN:
          trc(F("no transmitter pin"));
          break;
        default:
          trc(F("invalid json data, can't read raw or message/protocol"));
          break;   
      }
    }
  }
}
 #endif
