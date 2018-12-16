/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    Output pin High or Low
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - 1technophile
  
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
#ifdef ZactuatorONOFF

void setupONOFF(){
  trc(F("ACTUATOR_ONOFF_PIN"));
  trc(ACTUATOR_ONOFF_PIN);
  //init
  pinMode(ACTUATOR_ONOFF_PIN, OUTPUT);
  trc(F("Set to OFF"));
  digitalWrite(ACTUATOR_ONOFF_PIN, LOW);
  trc(F("ZactuatorONOFF setup done "));
}

  void MQTTtoONOFF(char * topicOri, char * datacallback){
  
    int boolSWITCHTYPE;
    if (strcmp(datacallback, "ON") == 0) boolSWITCHTYPE = 1;
    if (strcmp(datacallback, "OFF") == 0) boolSWITCHTYPE = 0; 

   if (strcmp(topicOri,subjectMQTTtoONOFF) == 0){
      trc(F("MQTTtoONOFF data analysis"));
      trc(boolSWITCHTYPE);
      digitalWrite(ACTUATOR_ONOFF_PIN, boolSWITCHTYPE);
      // we acknowledge the sending by publishing the value to an acknowledgement topic
      pub(subjectGTWONOFFtoMQTT, datacallback);
    }
  }
  void MQTTtoONOFF(char * topicOri, JsonObject& ONOFFdata){
   
   if (strcmp(topicOri,subjectMQTTtoONOFF) == 0){
      trc(F("MQTTtoONOFF json data analysis"));
      int boolSWITCHTYPE = ONOFFdata["switchType"] | 99;
      if (boolSWITCHTYPE != 99) {
        trc(F("MQTTtoONOFF boolSWITCHTYPE ok"));
        trc(boolSWITCHTYPE);
        digitalWrite(ACTUATOR_ONOFF_PIN, boolSWITCHTYPE);
        // we acknowledge the sending by publishing the value to an acknowledgement topic
        pub(subjectGTWONOFFtoMQTT, ONOFFdata);
      }else{
        trc(F("MQTTtoONOFF Fail reading from json"));
      }
    }
  }
#endif
