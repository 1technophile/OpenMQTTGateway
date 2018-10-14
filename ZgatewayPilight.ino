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
    #ifndef ZgatewayRF && ZgatewayRF2 //receiving with Pilight is not compatible with ZgatewayRF or RF2 as far as I can tell 
        rf.setCallback(pilightCallback);
        rf.initReceiver(RF_RECEIVER_PIN);
        trc(F("RF_EMITTER_PIN "));
        trc(String(RF_EMITTER_PIN));
        trc(F("RF_RECEIVER_PIN "));
        trc(String(RF_RECEIVER_PIN));
        trc(F("ZgatewayPilight setup done "));
    #endif 
}

boolean PilighttoMQTT(){
  rf.loop();
}

void pilightCallback(const String &protocol, const String &message, int status,
                size_t repeats, const String &deviceID) {

  if (status == VALID) {

    trc(F("Rcv. Pilight"));
    String MQTTprotocol;
    String MQTTdeviceID;
    String MQTTmessage;

    MQTTprotocol = String(protocol);
    MQTTdeviceID = String(deviceID);
    MQTTmessage = String(message);
    String MQTTPilightstring;
    MQTTPilightstring = subjectPilighttoMQTT+String("/")+MQTTprotocol+String("/")+MQTTdeviceID;
    trc(F("Adv data PilighttoMQTT"));
    client.publish((char *)MQTTPilightstring.c_str(),(char *)MQTTmessage.c_str());      

  }
}

void MQTTtoPilight(char * topicOri, char * datacallback) {

  String topic = topicOri;

  int pos = topic.lastIndexOf(subjectMQTTtoPilight);
  if (pos == -1){
    return;
  }

  trc(F("Adv data PilighttoMQTT"));

  char *last = strrchr(topicOri, '/');
  String protocol = "";

  if (last+1 != NULL) {
    protocol = last+1;
  }

  int result = 0;

  if (protocol == PilightRAW){
    trc(F("RAW:"));
    trc(String(datacallback));

    uint16_t rawpulses[MAXPULSESTREAMLENGTH];
    int rawlen =
        rf.stringToPulseTrain(datacallback, rawpulses, MAXPULSESTREAMLENGTH);
    if (rawlen > 0) {
      rf.sendPulseTrain(rawpulses, rawlen);
      result = rawlen;
    } else {
      result = -9999;
    }
  } else {
    trc(F("PROTO:"));
    trc(protocol);
    result = rf.send(protocol, datacallback);
  }

  String MQTTmessage;
  String MQTTprotocol = String(protocol);
  String MQTTPilightstring = subjectPilighttoMQTT+String("/")+MQTTprotocol+String("/");

  if (result > 0) {
      trc(F("Adv data MQTTtoPilight push state via PilighttoMQTT"));

      MQTTmessage = String(datacallback);

      char *deviceID = "0";

      DynamicJsonBuffer jb;
      JsonObject& rjson = jb.parseObject(MQTTmessage);
      if (rjson.success()) {
        strcpy(deviceID, rjson["id"]);
      }

      MQTTmessage = String(datacallback);
      MQTTPilightstring += String(deviceID);

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
    MQTTPilightstring += String("error");

  }

  client.publish((char *)MQTTPilightstring.c_str(),(char *)MQTTmessage.c_str());

}

#endif

