/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your RS232 device and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send RS232 signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received RS232 signal

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

#ifdef ZgatewayRS232

#  include <SoftwareSerial.h>

SoftwareSerial RS232Serial(RS232_RX_GPIO, RS232_TX_GPIO); // RX, TX

void setupRS232() {
  // define pin modes for RX, TX:
  pinMode(RS232_RX_GPIO, INPUT);
  pinMode(RS232_TX_GPIO, OUTPUT);
  // set the data rate for the SoftwareSerial port
  RS232Serial.begin(RS232Baud);

  Log.notice(F("RS232_RX_GPIO: %d " CR), RS232_RX_GPIO);
  Log.notice(F("RS232_TX_GPIO: %d " CR), RS232_TX_GPIO);
  Log.notice(F("RS232Baud: %d " CR), RS232Baud);
  Log.trace(F("ZgatewayRS232 setup done " CR));
}

void RS232toMQTT() {
  //This function is Blocking, but there should only ever be a few bytes, usually an ACK or a NACK.
  if (RS232Serial.available()) {
    Log.trace(F("RS232toMQTT" CR));
    static char RS232data[MAX_INPUT];
    static unsigned int input_pos = 0;
    static char inChar;
    do {
      if (RS232Serial.available()) {
        inChar = RS232Serial.read();
        RS232data[input_pos] = inChar;
        input_pos++;
        Log.trace(F("Received %c" CR), inChar);
      }
    } while (inChar != RS232InPost && input_pos < MAX_INPUT);
    RS232data[input_pos] = 0;
    input_pos = 0;
    Log.trace(F("Publish %s" CR), RS232data);
    char* output = RS232data + sizeof(RS232Pre) - 1;
    pub(subjectRS232toMQTT, output);
  }
}

void MQTTtoRS232(char* topicOri, JsonObject& RS232data) {
  Log.trace(F("json" CR));
  if (cmpToMainTopic(topicOri, subjectMQTTtoRS232)) {
    Log.trace(F("MQTTtoRS232 json" CR));
    const char* data = RS232data["value"];
    const char* prefix = RS232data["prefix"] | RS232Pre;
    const char* postfix = RS232data["postfix"] | RS232Post;
    Log.trace(F("Value set: %s" CR), data);
    Log.trace(F("Prefix set: %s" CR), prefix);
    Log.trace(F("Postfix set: %s" CR), postfix);
    RS232Serial.print(prefix);
    RS232Serial.print(data);
    RS232Serial.print(postfix);
  }
}
#endif