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

struct Pilightrxd
{
  String protocol;
  String deviceID;
  String status;
  String message;
  bool hasNewData;
};

Pilightrxd pilightrd;

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

  if(pilightrd.hasNewData){
    
    pilightrd.hasNewData=false;
    trc(F("Rcv. Pilight"));
    String MQTTprotocol;
    String MQTTdeviceID;
    String MQTTstatus;
    String MQTTmessage;

    MQTTprotocol = String(pilightrd.protocol);
    MQTTdeviceID = String(pilightrd.deviceID);
    MQTTstatus = String(pilightrd.status);
    MQTTmessage = String(pilightrd.message);
    String MQTTPilightstring;
    MQTTPilightstring = subjectPilighttoMQTT+String("/")+MQTTprotocol+String("/")+MQTTdeviceID;
    trc(F("Adv data PilighttoMQTT"));
    client.publish((char *)MQTTPilightstring.c_str(),(char *)MQTTmessage.c_str());  
    return true;
    
  }
  return false;
}

void pilightCallback(const String &protocol, const String &message, int status,
                size_t repeats, const String &deviceID) {

  pilightrd.protocol=protocol;
  pilightrd.deviceID=deviceID;
  pilightrd.status=status;
  pilightrd.message=message;
  pilightrd.hasNewData=true;

}

void MQTTtoPilight(char * topicOri, char * datacallback) {

}

#endif
