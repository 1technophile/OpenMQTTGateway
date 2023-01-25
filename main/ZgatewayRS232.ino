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

#  ifndef RS232_UART // software serial mode
#    include <SoftwareSerial.h>
SoftwareSerial RS232SoftSerial(RS232_RX_GPIO, RS232_TX_GPIO); // RX, TX
#  endif

// use pointer to stream class for serial communication to make code
// compatible with both softwareSerial as hardwareSerial.
Stream* RS232Stream = NULL;

void setupRS232() {
//Initalize serial port
#  ifdef RS232_UART // Hardware serial
#    if RS232_UART == 0 // init UART0
  Serial.end(); // stop if already initialized
#      ifdef ESP32
  Serial.begin(RS232Baud, SERIAL_8N1, RS232_RX_GPIO, RS232_TX_GPIO);
#      else
  Serial.begin(RS232Baud, SERIAL_8N1);
#      endif
#      if defined(ESP8266) && defined(RS232_UART0_SWAP)
  Serial.swap(); // swap UART0 ports from (GPIO1,GPIO3) to (GPIO15,GPIO13)
#      endif
  RS232Stream = &Serial;
  Log.notice(F("RS232 HW UART0" CR));

#    elif RS232_UART == 1 // init UART1
  Serial1.end(); // stop if already initialized
#      ifdef ESP32
  Serial1.begin(RS232Baud, SERIAL_8N1, RS232_RX_GPIO, RS232_TX_GPIO);
#      else
  Serial1.begin(RS232Baud, SERIAL_8N1);
#      endif
  RS232Stream = &Serial1;
  Log.notice(F("RS232 HW UART1" CR));

#    elif RS232_UART == 2 // init UART2
  Serial2.end(); // stop if already initialized
#      ifdef ESP32
  Serial2.begin(RS232Baud, SERIAL_8N1, RS232_RX_GPIO, RS232_TX_GPIO);
#      else
  Serial2.begin(RS232Baud, SERIAL_8N1);
#      endif
  RS232Stream = &Serial2;
  Log.notice(F("RS232 HW UART2" CR));

#    elif RS232_UART == 3 // init UART3
  Serial3.end(); // stop if already initialized
  Serial3.begin(RS232Baud, SERIAL_8N1);
  RS232Stream = &Serial3;
  Log.notice(F("RS232 HW UART3" CR));
#    endif

#  else // Software serial
  // define pin modes for RX, TX:
  pinMode(RS232_RX_GPIO, INPUT);
  pinMode(RS232_TX_GPIO, OUTPUT);
  RS232SoftSerial.begin(RS232Baud);
  RS232Stream = &RS232SoftSerial; // get stream of serial

  Log.notice(F("RS232_RX_GPIO: %d" CR), RS232_RX_GPIO);
  Log.notice(F("RS232_TX_GPIO: %d" CR), RS232_TX_GPIO);
#  endif

  // Flush all bytes in the "link" serial port buffer
  while (RS232Stream->available() > 0)
    RS232Stream->read();

  Log.notice(F("RS232Baud: %d" CR), RS232Baud);
  Log.trace(F("ZgatewayRS232 setup done" CR));
}

#  if RS232toMQTTmode == 0 // Convert received data to single MQTT topic
void RS232toMQTT() {
  // Send all RS232 output (up to RS232InPost char revieved) as MQTT message
  //This function is Blocking, but there should only ever be a few bytes, usually an ACK or a NACK.
  if (RS232Stream->available()) {
    Log.trace(F("RS232toMQTT" CR));
    static char RS232data[MAX_INPUT];
    static unsigned int input_pos = 0;
    static char inChar;
    do {
      if (RS232Stream->available()) {
        inChar = RS232Stream->read();
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

#  elif RS232toMQTTmode == 1 // Convert recievee JSON data to multiple MQTT topics based (nested) keys
void RS232toMQTT() {
  // Assumes valid JSON data at RS232 interface. Use (nested) keys to split JSON data in separate
  // sub-MQTT-topics up to the defined nesting level.
  if (RS232Stream->available()) {
    Log.trace(F("RS232toMQTT" CR));

    // Allocate the JSON document
    StaticJsonDocument<RS232JSONDocSize> doc;

    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(doc, *RS232Stream);

    if (err == DeserializationError::Ok) {
      // JSON received, send as MQTT topics
      char topic[mqtt_topic_max_size + 1] = subjectRS232toMQTT;
      sendMQTTfromNestedJson(doc.as<JsonVariant>(), topic, 0, RS232maxJSONlevel);
    } else {
      // Print error to serial log
      Log.error(F("Error in RS232JSONtoMQTT, deserializeJson() returned %s"), err.c_str());

      // Flush all bytes in the "link" serial port buffer
      while (RS232Stream->available() > 0)
        RS232Stream->read();
    }
  }
}

void sendMQTTfromNestedJson(JsonVariant obj, char* topic, int level, int maxLevel) {
  // recursively step through JSON data and send MQTT messages
  if (level < maxLevel && obj.is<JsonObject>()) {
    int topicLength = strlen(topic);
    // loop over fields
    for (JsonPair pair : obj.as<JsonObject>()) {
      // check if new key still fits in topic cstring
      const char* key = pair.key().c_str();
      Log.trace(F("level=%d, key='%s'" CR), level, pair.key().c_str());
      if (topicLength + 2 + strlen(key) <= mqtt_topic_max_size) {
        // add new level to existing topic cstring
        topic[topicLength] = '/'; // add slash
        topic[topicLength + 1] = '\0'; // terminate
        strncat(topic + topicLength, key, mqtt_topic_max_size - topicLength - 2);

        // step recursively into next level
        sendMQTTfromNestedJson(pair.value(), topic, level + 1, maxLevel);

        // restore topic
        topic[topicLength] = '\0';
      } else {
        Log.error(F("Nested key '%s' at level %d does not fit within max topic length of %d, skipping"),
                  key, level, mqtt_topic_max_size);
      }
    }

  } else {
    // output value at current json level
    char output[MAX_INPUT + 1];
    serializeJson(obj, output, MAX_INPUT);
    Log.notice(F("level=%d, topic=%s, value: %s\n"), level, topic, output);

    // send MQTT message
    pub(topic, &output[0]);
  }
}
#  endif

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
    RS232Stream->print(prefix);
    RS232Stream->print(data);
    RS232Stream->print(postfix);
  }
}
#endif