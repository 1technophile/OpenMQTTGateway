/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT

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
#ifdef ZgatewayCloud

#  include <ArduinoJson.h>
#  include <PubSubClient.h>

#  include "ArduinoLog.h"

// client link to pubsub MQTT
WiFiClientSecure cloudWifi;

PubSubClient cloud(cloudWifi);

char clientId[parameters_size];
char device[parameters_size];
char token[parameters_size];
String cloudTopic;

// Queue messages to be sent via loop

QueueHandle_t omgCloudQueue;
SemaphoreHandle_t omgCloudSemaphore;

typedef struct cloudMsg {
  char topic[mqtt_topic_max_size];
  char payload[mqtt_max_packet_size];
  bool retain;
} cloudMsg_t;

void cloudCallback(char* topic, byte* payload, unsigned int length) {
  byte* p = (byte*)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);
  // Conversion to a printable string
  p[length] = '\0';

  Log.trace(F("[ CLOUD->OMG ] message Received: %s" CR), topic);
  displayPrint("[ CLOUD->OMG ] Received:", topic);

  // normalize cloud topic to be similar to local mqtt topic
  topic += cloudTopic.length();

  char localTopic[mqtt_max_packet_size];
  (String(mqtt_topic) + String(gateway_name) + String(topic)).toCharArray(localTopic, mqtt_max_packet_size);
  // Log.notice(F("Normalized topic: %s" CR), localTopic);

  //launch the function to treat received data if this data concern OpenMQTTGateway
  if ((strstr(localTopic, subjectMultiGTWKey) != NULL) ||
      (strstr(localTopic, subjectGTWSendKey) != NULL) ||
      (strstr(localTopic, subjectFWUpdate) != NULL))
    receivingMQTT(localTopic, (char*)p);

  // Free the memory
  free(p);
}

void reconnect() {
  // Loop until we're reconnected
  while (!cloud.connected()) {
    Log.notice(F("[ CLOUD ] Connecting OMG Cloud %s - %s - %s" CR), ("omgClient-" + String(clientId)).c_str(), device, token);
    if (cloud.connect(("omgClient-" + String(clientId)).c_str(), device, token, (cloudTopic + "/LWT").c_str(), will_QoS, will_Retain, will_Message)) {
      displayPrint("[ CLOUD ] connected");
      Log.verbose(F("[ CLOUD ] OMG Cloud connected %s" CR), cloudTopic.c_str());

      // ... and resubscribe
      cloud.setCallback(cloudCallback);
      String topic = "omgCloudSub/omgClient-" + String(clientId) + "/" + String(device) + "/#";
      if (!cloud.subscribe(topic.c_str())) {
        Log.error(F("[ CLOUD ] Cloud subscribe failed %s, rc=%d" CR), topic.c_str(), cloud.state());
      };
      Log.verbose(F("[ CLOUD ] OMG Cloud subscribing to %s" CR), topic.c_str());
      // Need to use the central cloud.publish
      pubOmgCloud((String(mqtt_topic) + String(gateway_name) + will_Topic).c_str(), Gateway_AnnouncementMsg, will_Retain);
    } else {
      Log.error(F("[ CLOUD ] OMG Cloud connection failed: rc=%d, retrying in 35 seconds." CR), cloud.state());
      delay(35000);
    }
  }
}

void setupCloud() {
  Log.trace(F("ZgatewayCloud setup start" CR));

  char deviceToken[90] = DEVICETOKEN; // clientid:device:token TODO: base64 encode the devicetoken
  strcpy(clientId, strtok(deviceToken, ":")); // TODO: strtok is not thread safe.....and needs to be replaced
  strcpy(device, strtok(0, ":")); // TODO: strtok is not thread safe.....and needs to be replaced
  strcpy(token, strtok(0, ":"));

  cloudTopic = "omgCloudPub/omgClient-" + String(clientId) + "/" + String(device);
  cloudWifi.setInsecure();
  cloud.setKeepAlive(300); // 5 minutes for keep alive

  cloud.setBufferSize(mqtt_max_packet_size);
  cloud.setServer("omgpoc.homebridge.ca", 8883);

  // Create queue for cloud messages
  omgCloudQueue = xQueueCreate(5, sizeof(cloudMsg_t*));
  omgCloudSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(omgCloudSemaphore);

  reconnect();
  Log.notice(F("ZgatewayCloud setup done" CR));
}

void CloudLoop() {
  if (!cloud.connected()) {
    reconnect();
  }
  cloud.loop();

  if (uxQueueMessagesWaiting(omgCloudQueue) && uxSemaphoreGetCount(omgCloudSemaphore)) { // && uxSemaphoreGetCount(omgCloudSemaphore)
    if (xSemaphoreTake(omgCloudSemaphore, (TickType_t)10) == pdTRUE) {
      cloudMsg_t* omgCloudMessage = nullptr;
      if (cloud.connected() && xQueueReceive(omgCloudQueue,
                                             &omgCloudMessage,
                                             (TickType_t)10) == pdPASS) {
        Log.verbose(F("[ OMG->CLOUD ] unQueue topic: %s msg: %s " CR), omgCloudMessage->topic, omgCloudMessage->payload);
        if (!cloud.publish(omgCloudMessage->topic, omgCloudMessage->payload, omgCloudMessage->retain)) {
          Log.error(F("Cloud publish failed, rc=%d, topic: %s msg: %s " CR), cloud.state(), omgCloudMessage->topic, omgCloudMessage->payload);
        } else {
          Log.trace(F("[ OMG->CLOUD ] topic: %s msg: %s " CR), omgCloudMessage->topic, omgCloudMessage->payload);
        }
      }
      free(omgCloudMessage);
      xSemaphoreGive(omgCloudSemaphore);
    }
  }
}

void pubOmgCloud(const char* topic, const char* payload, bool retain) {
  topic += strlen(mqtt_topic);
  topic += strlen(gateway_name);

  Log.verbose(F("[ OMG->CLOUD ] place on queue topic: %s msg: %s " CR), (cloudTopic + String(topic)).c_str(), payload);

  cloudMsg_t* omgCloudMessage = (cloudMsg_t*)calloc(1, sizeof(cloudMsg_t));
  strcpy(omgCloudMessage->topic, (cloudTopic + String(topic)).c_str());
  strcpy(omgCloudMessage->payload, payload);
  omgCloudMessage->retain = retain;
  if (xQueueSend(omgCloudQueue, &omgCloudMessage, 0) != pdTRUE) {
    Log.warning(F("omgCloudQueue full, discarding signal topic: %s msg: %s " CR), (cloudTopic + String(topic)).c_str(), payload);
    free(omgCloudMessage);
    displayPrint("[ CLOUD ] discarding");
  } else {
  }
}

#endif