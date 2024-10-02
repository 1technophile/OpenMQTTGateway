/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a wifi or ethernet gateway between your SERIAL device and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send SERIAL signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received SERIAL signal

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

#ifdef ZgatewaySERIAL

#  ifndef SERIAL_UART // software serial mode
#    include <SoftwareSerial.h>
SoftwareSerial SERIALSoftSerial(SERIAL_RX_GPIO, SERIAL_TX_GPIO); // RX, TX
#  endif

// use pointer to stream class for serial communication to make code
// compatible with both softwareSerial as hardwareSerial.
Stream* SERIALStream = NULL;
//unsigned long msgCount = 0;

bool receiverReady = false;
unsigned long lastHeartbeatReceived = 0;
unsigned long lastHeartbeatAckReceived = 0;
unsigned long lastHeartbeatSent = 0;
const unsigned long heartbeatTimeout = 15000; // 15 seconds timeout for ack
const unsigned long maxHeartbeatInterval = 60000; // Maximum interval of 1 minute
unsigned long heartbeatInterval = 5000; // 5 seconds
bool isOverflow = false;

void setupSERIAL() {
//Initalize serial port
#  ifdef SERIAL_UART // Hardware serial
#    if SERIAL_UART == 0 // init UART0
  Serial.end(); // stop if already initialized
#      ifdef ESP32
  Serial.begin(SERIALBaud, SERIAL_8N1, SERIAL_RX_GPIO, SERIAL_TX_GPIO);
#      else
  Serial.begin(SERIALBaud, SERIAL_8N1);
#      endif
#      if defined(ESP8266) && defined(SERIAL_UART0_SWAP)
  Serial.swap(); // swap UART0 ports from (GPIO1,GPIO3) to (GPIO15,GPIO13)
#      endif
  SERIALStream = &Serial;
  Log.notice(F("SERIAL HW UART0" CR));

#    elif SERIAL_UART == 1 // init UART1
  Serial1.end(); // stop if already initialized
#      ifdef ESP32
  Serial1.begin(SERIALBaud, SERIAL_8N1, SERIAL_RX_GPIO, SERIAL_TX_GPIO);
#      else
  Serial1.begin(SERIALBaud, SERIAL_8N1);
#      endif
  SERIALStream = &Serial1;
  Log.notice(F("SERIAL HW UART1" CR));

#    elif SERIAL_UART == 2 // init UART2
  Serial2.end(); // stop if already initialized
#      ifdef ESP32
  Serial2.begin(SERIALBaud, SERIAL_8N1, SERIAL_RX_GPIO, SERIAL_TX_GPIO);
#      else
  Serial2.begin(SERIALBaud, SERIAL_8N1);
#      endif
  SERIALStream = &Serial2;
  Log.notice(F("SERIAL HW UART2" CR));

#    elif SERIAL_UART == 3 // init UART3
  Serial3.end(); // stop if already initialized
  Serial3.begin(SERIALBaud, SERIAL_8N1);
  SERIALStream = &Serial3;
  Log.notice(F("SERIAL HW UART3" CR));
#    endif

#  else // Software serial
  // define pin modes for RX, TX:
  pinMode(SERIAL_RX_GPIO, INPUT);
  pinMode(SERIAL_TX_GPIO, OUTPUT);
  SERIALSoftSerial.begin(SERIALBaud);
  SERIALStream = &SERIALSoftSerial; // get stream of serial

  Log.notice(F("SERIAL_RX_GPIO: %d" CR), SERIAL_RX_GPIO);
  Log.notice(F("SERIAL_TX_GPIO: %d" CR), SERIAL_TX_GPIO);
#  endif

  // Flush all bytes in the "link" serial port buffer
  while (SERIALStream->available() > 0)
    SERIALStream->read();

  Log.notice(F("SERIALBaud: %d" CR), SERIALBaud);
  Log.trace(F("ZgatewaySERIAL setup done" CR));
}

#  if SERIALtoMQTTmode == 0 // Convert received data to single MQTT topic
void SERIALtoX() {
  // Send all SERIAL output (up to SERIALInPost char revieved) as MQTT message
  //This function is Blocking, but there should only ever be a few bytes, usually an ACK or a NACK.
  if (SERIALStream->available()) {
    Log.trace(F("SERIALtoMQTT" CR));
    static char SERIALdata[MAX_INPUT];
    static unsigned int input_pos = 0;
    static char inChar;
    do {
      if (SERIALStream->available()) {
        inChar = SERIALStream->read();
        SERIALdata[input_pos] = inChar;
        input_pos++;
      }
    } while (inChar != SERIALInPost && input_pos < MAX_INPUT);
    SERIALdata[input_pos] = 0;
    input_pos = 0;

    char* output = SERIALdata + sizeof(SERIALPre) - 1;
    Log.notice(F("SERIAL data: %s" CR), output);
    pub(subjectSERIALtoMQTT, output);
  }
}

#  elif SERIALtoMQTTmode == 1 // Convert received JSON data to one or multiple MQTT topics
void sendHeartbeat() {
  Log.trace(F("Sending Serial heartbeat" CR));
  SERIALStream->print(SERIALPre);
  SERIALStream->print("{\"type\":\"heartbeat\"}");
  SERIALStream->print(SERIALPost);
  SERIALStream->flush();
}

void sendHeartbeatAck() {
  SERIALStream->print(SERIALPre);
  SERIALStream->print("{\"type\":\"heartbeat_ack\"}");
  SERIALStream->print(SERIALPost);
  SERIALStream->flush();
  Log.trace(F("Sent heartbeat ack" CR));
}

void SERIALtoX() {
  static String buffer = ""; // Static buffer to store incomplete messages

  unsigned long currentTime = millis();

  // Check if it's time to send a heartbeat and we're not in overflow
  if (!isOverflow && currentTime - lastHeartbeatSent > heartbeatInterval) {
    // Check if we received an ack for the last heartbeat
    if (currentTime - lastHeartbeatAckReceived > heartbeatTimeout) {
      // No ack received, increase the interval (with a maximum limit)
      heartbeatInterval = min(heartbeatInterval * 2, maxHeartbeatInterval);
      Log.warning(F("No heartbeat ack received. Increasing interval to %lu ms" CR), heartbeatInterval);
    } else {
      // Ack received, reset the interval
      heartbeatInterval = 5000;
    }
    sendHeartbeat();
    lastHeartbeatSent = currentTime;
  }

  while (SERIALStream->available()) {
    char c = SERIALStream->read();
    buffer += c;

    // Check if we have a complete message
    if (buffer.startsWith(SERIALPre) && buffer.endsWith(SERIALPost)) {
      isOverflow = false;
      // Remove prefix and postfix
      String jsonString = buffer.substring(strlen(SERIALPre), buffer.length() - strlen(SERIALPost));

      // Allocate the JSON document
      StaticJsonDocument<JSON_MSG_BUFFER> SERIALBuffer;
      JsonObject SERIALdata = SERIALBuffer.to<JsonObject>();

      // Deserialize the JSON string
      DeserializationError err = deserializeJson(SERIALBuffer, jsonString);

      if (err == DeserializationError::Ok) {
        // Check if this is a heartbeat message
        if (SERIALdata.containsKey("type") && strcmp(SERIALdata["type"], "heartbeat") == 0) {
          handleHeartbeat();
        } else if (SERIALdata.containsKey("type") && strcmp(SERIALdata["type"], "heartbeat_ack") == 0) {
          lastHeartbeatAckReceived = currentTime;
          Log.notice(F("Heartbeat ack received" CR));
        } else {
          // Process normal messages
          Log.notice(F("SERIAL msg received: %s" CR), jsonString.c_str());
#    if jsonPublishing
          if (SERIALdata.containsKey("target")) {
            receivingDATA("", jsonString.c_str());
          } else {
            // send as json
            if (SERIALdata.containsKey("origin") || SERIALdata.containsKey("topic")) {
#      ifdef ZmqttDiscovery
              // We need to assign the discovery message to the primary module instead of the secondary module
              if (SERIALdata.containsKey("device") && SERIALdata["device"].containsKey("via_device")) {
                SERIALdata["device"]["via_device"] = gateway_name;
              }
#      endif
              enqueueJsonObject(SERIALdata);
            } else {
              SERIALdata["origin"] = subjectSERIALtoMQTT;
              enqueueJsonObject(SERIALdata);
            }
          }
#    endif
#    if simplePublishing
          // send as MQTT topics
          char topic[mqtt_topic_max_size + 1] = subjectSERIALtoMQTT;
          sendMQTTfromNestedJson(SERIALBuffer.as<JsonVariant>(), topic, 0, SERIALmaxJSONlevel);
#    endif
        }
      } else {
        // Print error to serial log
        Log.error(F("Error in SERIALJSONtoMQTT, deserializeJson() returned %s" CR), err.c_str());
      }

      // Clear the buffer for the next message
      buffer = "";
    } else if (buffer.length() > JSON_MSG_BUFFER) {
      // If the buffer gets too large without finding a complete message, clear it
      Log.error(F("Buffer overflow, clearing buffer. Partial content: %s" CR), buffer.c_str());
      buffer = "";
      isOverflow = true;
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

bool XtoSERIAL(const char* topicOri, JsonObject& SERIALdata) {
  bool res = false;
  unsigned long currentTime = millis();

  // Check if receiver is still ready (heartbeat check)
  if (currentTime - lastHeartbeatReceived > heartbeatTimeout) {
    receiverReady = false;
    Log.error(F("Heartbeat timeout. Receiver is not ready." CR));
  }

  if (receiverReady && (cmpToMainTopic(topicOri, subjectMQTTtoSERIAL) || (SYSConfig.serial && SERIALdata.containsKey("origin") && SERIALdata["origin"].is<const char*>()) || (SYSConfig.serial && SERIALdata.containsKey("topic") && SERIALdata["topic"].is<const char*>()))) {
    Log.trace(F("XtoSERIAL" CR));

    // Prepare the data string
    std::string data;
    if (SYSConfig.serial ||
        (SERIALdata.containsKey("origin") && SERIALdata["origin"].is<const char*>()) || // Module like BT to SERIAL
        (SERIALdata.containsKey("target") && SERIALdata["target"].is<const char*>())) { // Command to send to a specific target example MQTTtoBT through SERIAL
      //SERIALdata["msgcount"] = msgCount++;
      serializeJson(SERIALdata, data);
    } else if (SERIALdata.containsKey("value")) {
      data = SERIALdata["value"].as<std::string>();
    }

    // Send the message
    const char* prefix = SERIALdata["prefix"] | SERIALPre;
    const char* postfix = SERIALdata["postfix"] | SERIALPost;

    SERIALStream->print(prefix);
    SERIALStream->print(data.c_str());
    SERIALStream->print(postfix);
    SERIALStream->flush();

    Log.notice(F("[ OMG->SERIAL ] data sent: %s" CR), data.c_str());
    res = true;
    delay(100);
  }
  return res;
}

bool isSerialReady() {
  return receiverReady;
}

// This function should be called when a heartbeat is received from the receiver
void handleHeartbeat() {
  receiverReady = true;
  lastHeartbeatReceived = millis();
  Log.trace(F("Heartbeat received. Receiver is ready." CR));
  sendHeartbeatAck();
}
#endif