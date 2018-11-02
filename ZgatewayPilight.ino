/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 
    Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - publish MQTT data to a different topic related to received 433Mhz signal using ESPiLight
     Copyright: (c)Florian ROBERT
    Copyright: (c)Randy Simons http://randysimons.nl/
  
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
        trc(F("ZgatewayPilight setup cannot be done, comment first ZgatewayRF && ZgatewayRF2"));
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
   }
}

void MQTTtoPilight(char * topicOri, JsonObject& Pilightdata) {
String topic = topicOri;
int result = 0;
 
if (topic == subjectMQTTtoPilight) {
    trc(F("MQTTtoPilight json data analysis"));
    const char * message = Pilightdata["message"];
    const char * protocol = Pilightdata["protocol"];
    if (message && protocol) {
      trc(F("MQTTtoPilight msg & protocol ok"));
      result = rf.send(protocol, message);
    }else{
    trc(F("MQTTtoPilight fail reading from json"));
    }
    int msgLength = 0;
    uint16_t codes[MAXPULSESTREAMLENGTH];
    msgLength = rf.stringToPulseTrain(
        message,
        codes, MAXPULSESTREAMLENGTH);
    if (msgLength > 0) {
      trc(F("MQTTtoPilight raw ok"));
      rf.sendPulseTrain(codes, msgLength);
      result = msgLength;
    }else{
      trc(F("MQTTtoPilight raw KO"));
      result = -9999;
    }
    
   String MQTTmessage;
   if (result > 0) {
      trc(F("Adv data MQTTtoPilight push state via PilighttoMQTT"));
      pub(subjectGTWPilighttoMQTT, Pilightdata);
   } else {
    switch (result) {
      case ESPiLight::ERROR_UNAVAILABLE_PROTOCOL:
        MQTTmessage = "protocol is not avaiable";
        break;
      case ESPiLight::ERROR_INVALID_PILIGHT_MSG:
        MQTTmessage = "message is invalid";
        break;
      case ESPiLight::ERROR_INVALID_JSON:
        MQTTmessage = "message is not a proper json object";
        break;
      case ESPiLight::ERROR_NO_OUTPUT_PIN:
        MQTTmessage = "no transmitter pin";
        break;
      case -9999:
        MQTTmessage = "invalid pulse train message";
        break;
    }
    trc(F("ESPiLight Error: "));
    trc(String(MQTTmessage));
    pub(subjectGTWPilighttoMQTT, MQTTmessage);
   }
 }
}
 #endif
