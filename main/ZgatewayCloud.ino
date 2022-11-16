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

void cloudCallback(char* topic, byte* payload, unsigned int length) {
  byte* p = (byte*)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);
  // Conversion to a printable string
  p[length] = '\0';

  // cloudTopic = omgCloudSub/omgClient-636ef7c6149d798f3b71849e/bLOoakg7wF_Ip2Xk4YFazViBJMcTSm

  Log.notice(F("OMG Cloud message Received: %s" CR), topic);
  displayPrint("OMG Cloud message Received:", topic);

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
    Log.notice(F("Attempting to connect to OMG Cloud %s - %s - %s" CR), ("omgClient-" + String(clientId)).c_str(), device, token);
    displayPrint("Attempting OMG Cloud connection");
    if (cloud.connect(("omgClient-" + String(clientId)).c_str(), device, token, (cloudTopic + "/LWT").c_str(), will_QoS, will_Retain, will_Message)) {
      displayPrint("OMG Cloud connected");
      Log.notice(F("OMG Cloud connected %s" CR), cloudTopic.c_str());

      // ... and resubscribe
      cloud.setCallback(cloudCallback);
      String topic = "omgCloudSub/omgClient-" + String(clientId) + "/" + String(device) + "/#";
      if (!cloud.subscribe(topic.c_str())) {
        Log.error(F("Cloud subscribe failed %s, rc=%d" CR), topic.c_str(), cloud.state());
      };
      Log.notice(F("OMG Cloud subscribing to %s" CR), topic.c_str());
      if (!cloud.publish((cloudTopic + "/LWT").c_str(), Gateway_AnnouncementMsg, will_Retain)) {
        Log.error(F("Inital Cloud publish failed, rc=%d" CR), cloud.state());
      };
      if (!cloud.publish((cloudTopic + "/version").c_str(), OMG_VERSION)) {
        Log.error(F("Inital Cloud publish failed, rc=%d" CR), cloud.state());
      };
    } else {
      Log.error(F("OMG Cloud connection failed: rc=%d, retrying in 35 seconds." CR), cloud.state());
      delay(35000);
    }
  }
}

void setupCloud() {
  char deviceToken[90] = DEVICETOKEN; // clientid:device:token TODO: base64 encode the devicetoken
  strcpy(clientId, strtok(deviceToken, ":")); // TODO: strtok is not thread safe.....and needs to be replaced
  strcpy(device, strtok(0, ":")); // TODO: strtok is not thread safe.....and needs to be replaced
  strcpy(token, strtok(0, ":"));

  cloudTopic = "omgCloudPub/omgClient-" + String(clientId) + "/" + String(device);
  cloudWifi.setInsecure();
  cloud.setKeepAlive(300); // 5 minutes for keep alive

  cloud.setBufferSize(mqtt_max_packet_size);
  cloud.setServer("omgpoc.homebridge.ca", 8883);

  reconnect();
}

void CloudLoop() {
  if (!cloud.connected()) {
    reconnect();
  }
  cloud.loop();
}

void pubOmgCloud(const char* topic, const char* payload, bool retain) {
  if (cloud.connected()) {
    topic += strlen(mqtt_topic);
    topic += strlen(gateway_name);

    Log.trace(F("[ OMG->CLOUD ] topic: %s msg: %s " CR), (cloudTopic + String(topic)).c_str(), payload);
    if (!cloud.publish((cloudTopic + String(topic)).c_str(), payload, retain)) {
      Log.error(F("Cloud publish failed, rc=%d" CR), cloud.state());
    };
  } else {
    Log.warning(F("Cloud not connected, aborting this publication" CR));
  }
}

#endif