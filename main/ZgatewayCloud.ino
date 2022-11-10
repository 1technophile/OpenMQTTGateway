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

void cloudCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!cloud.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (cloud.connect("omgClient", CLUSERNAME, CLPASSWORD)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      cloud.publish("presence/" CLUSERNAME, OMG_VERSION);
      // ... and resubscribe
      cloud.subscribe("command/" CLUSERNAME "/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(cloud.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setupCloud() {
  Log.notice(F("Connecting to OMG Cloud: %s" CR), CLUSERNAME);
  cloud.setBufferSize(mqtt_max_packet_size);
  cloud.setServer("clone.homebridge.ca", 8883);
  cloud.setCallback(cloudCallback);
  cloudWifi.setInsecure();
  reconnect();
}

void CloudLoop() {
  if (!cloud.connected()) {
    reconnect();
  }
  cloud.loop();
}

void pubCloud(const char* topicori, JsonObject& data) {
  String dataAsString = "";
  serializeJson(data, dataAsString);
  String topic = "response/" CLUSERNAME + String(topicori);
  if (cloud.connected()) {
    Log.trace(F("[ OMG->CLOUD ] topic: %s msg: %s " CR), topic.c_str(), dataAsString.c_str());
    cloud.publish(topic.c_str(), dataAsString.c_str(), false);
  } else {
    Log.warning(F("Cloud not connected, aborting this publication" CR));
  }
}

#endif