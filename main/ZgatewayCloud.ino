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
#if defined(ZgatewayCloud) && defined(ESP32)

#  include <ArduinoJson.h>
#  include <PubSubClient.h>

#  include "ArduinoLog.h"
#  include "config_Cloud.h"
#  include "config_mqttDiscovery.h"

// client link to pubsub MQTT
WiFiClientSecure cloudWifi;

PubSubClient cloud(cloudWifi);

char account[parameters_size];
char device[parameters_size];
char token[parameters_size];

String deviceToken;
bool cloudEnabled;

String cloudTopic;

// Queue messages to be sent via loop

QueueHandle_t omgCloudQueue;
SemaphoreHandle_t omgCloudSemaphore;

boolean cloudReady = false;

typedef struct cloudMsg {
  char* topic;
  char* payload;
  bool retain;
};

unsigned long reconnectTime = uptime();

void cloudCallback(char* topic, byte* payload, unsigned int length) {
  byte* p = (byte*)malloc(length + 1);
  if (p != NULL) {
    // Copy the payload to the new buffer
    memcpy(p, payload, length);
    // Conversion to a printable string
    p[length] = '\0';

    Log.trace(F("[ CLOUD->OMG ] message Received: %s" CR), topic);
    // displayPrint("[ CLOUD->OMG ] Received:", topic);

    // normalize cloud topic to be similar to local mqtt topic
    topic += cloudTopic.length() + 1;

    char localTopic[mqtt_max_packet_size];
    (String(topic)).toCharArray(localTopic, mqtt_max_packet_size);
    // Log.notice(F("Normalized topic: %s" CR), localTopic);

    //launch the function to treat received data if this data concern OpenMQTTGateway
    if ((strstr(localTopic, subjectMultiGTWKey) != NULL) ||
        (strstr(localTopic, subjectGTWSendKey) != NULL) ||
        (strstr(localTopic, subjectMQTTtoSYSupdate) != NULL))
      receivingMQTT(localTopic, (char*)p);

    // Free the memory
    free(p);
  } else {
    Log.error(F("[ CLOUD->OMG ] insufficent memory " CR));
  }
}

bool cloudConnect() {
  if (deviceToken.length() == CLOUD_DEVICE_TOKEN_LENGTH) {
    cloudActive = true;
    deviceToken.substring(0, deviceToken.indexOf(':')).toCharArray(account, parameters_size);
    deviceToken.substring(deviceToken.indexOf(':') + 1, deviceToken.lastIndexOf(':')).toCharArray(device, parameters_size);
    deviceToken.substring(deviceToken.lastIndexOf(':') + 1).toCharArray(token, parameters_size);
    // strcpy(account, strtok(deviceToken.c_str(), ":")); // TODO: strtok is not thread safe.....and needs to be replaced
    // strcpy(device, strtok(0, ":")); // TODO: strtok is not thread safe.....and needs to be replaced
    // strcpy(token, strtok(0, ":"));

    cloudTopic = "omgClient-" + String(account);
    Log.warning(F("[ CLOUD ] Connecting OMG Cloud %s - %s - %s" CR), account, device, token);
    if (cloud.connect(("omgDevice-" + String(gateway_name)).c_str(), device, token, (cloudTopic + '/' + String(mqtt_topic) + String(gateway_name) + "/LWT").c_str(), will_QoS, will_Retain, will_Message)) {
      displayPrint("[ CLOUD ] connected");
      Log.verbose(F("[ CLOUD ] OMG Cloud connected %s" CR), cloudTopic.c_str());
      Log.verbose(F("[ CLOUD ] OMG Cloud LWT %s" CR), (cloudTopic + '/' + mqtt_topic + gateway_name + "/LWT").c_str());
      cloud.setCallback(cloudCallback);
      String topic = cloudTopic + '/' + mqtt_topic + gateway_name + subjectMQTTtoX;
      if (!cloud.subscribe(topic.c_str())) {
        Log.error(F("[ CLOUD ] Cloud subscribe failed %s, rc=%d" CR), topic.c_str(), cloud.state());
      };
      Log.verbose(F("[ CLOUD ] OMG Cloud subscribing to %s" CR), topic.c_str());
      // Need to use the central cloud.publish
      pubOmgCloud((String(mqtt_topic) + String(gateway_name) + will_Topic).c_str(), Gateway_AnnouncementMsg, will_Retain);
    } else {
      Log.error(F("[ CLOUD ] OMG Cloud connection failed: rc=%d, retrying in %d seconds." CR), cloud.state(), RECONNECT_DELAY);
      reconnectTime = uptime() + RECONNECT_DELAY;
    }
  } else {
    Log.error(F("[ CLOUD ] Invalid devicetoken %d" CR), deviceToken.length());
    cloudActive = false;
  }
  return cloudActive;
}

void cloudDisconnect() {
  cloudActive = false;
  cloudEnabled = false;
  cloud.disconnect();
  Log.notice(F("[ CLOUD ] OMG Cloud disconected" CR));
}

void setupCloud() {
  Log.trace(F("ZgatewayCloud setup start" CR));

  CloudConfig_init();
  CloudConfig_load();

  cloudWifi.setInsecure();
  cloud.setKeepAlive(300); // 5 minutes for keep alive

  cloud.setBufferSize(mqtt_max_packet_size);
  cloud.setServer("cloudbeta.openmqttgateway.com", 8883);

  // Create queue for cloud messages
  omgCloudQueue = xQueueCreate(30, sizeof(cloudMsg*));
  omgCloudSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(omgCloudSemaphore);
  stateCLOUDStatus();
  if (cloudEnabled) {
    cloudConnect();
  }
  Log.notice(F("ZgatewayCloud setup done" CR));
  cloudReady = true;
}

void CloudConfig_init() {
  deviceToken = DEVICETOKEN; // account:device:token TODO: base64 encode the devicetoken
  cloudEnabled = CLOUDENABLED;
  Log.notice(F("Cloud config initialised" CR));
}

bool CloudConfig_load() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  preferences.begin(Gateway_Short_Name, true);
  String exists = preferences.getString("CLOUDConfig", "{}");
  if (exists != "{}") {
    auto error = deserializeJson(jsonBuffer, preferences.getString("CLOUDConfig", "{}"));
    preferences.end();
    if (error) {
      Log.error(F("CLOUD config deserialization failed: %s, buffer capacity: %u" CR), error.c_str(), jsonBuffer.capacity());
      return false;
    }
    if (jsonBuffer.isNull()) {
      Log.warning(F("CLOUD config is null" CR));
      return false;
    }
    JsonObject jo = jsonBuffer.as<JsonObject>();
    deviceToken = jo["deviceToken"].as<String>();
    cloudEnabled = jo["cloudEnabled"].as<bool>();
    return true;
  } else {
    preferences.end();
    Log.notice(F("No CLOUD config to load" CR));
    return false;
  }
}

bool CloudConfig_save() {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject jo = jsonBuffer.to<JsonObject>();
  jo["deviceToken"] = deviceToken;
  jo["cloudEnabled"] = cloudEnabled;
  // Save config into NVS (non-volatile storage)
  String conf = "";
  serializeJson(jsonBuffer, conf);
  preferences.begin(Gateway_Short_Name, false);
  preferences.putString("CLOUDConfig", conf);
  preferences.end();
  return true;
}

void MQTTtoCLOUD(char* topicOri, JsonObject& CLOUDdata) { // json object decoding
  bool success = false;
  if (cmpToMainTopic(topicOri, subjectMQTTtoCLOUDset)) {
    Log.trace(F("MQTTtoCLOUD json set" CR));
    // properties
    if (CLOUDdata.containsKey("deviceToken")) {
      success = setCloudDeviceToken(CLOUDdata["deviceToken"].as<String>());
    }
    if (CLOUDdata.containsKey("cloudEnabled")) {
      success = setCloudEnabled(CLOUDdata["cloudEnabled"].as<bool>());
    }
    /*
    if (success) { // If cloud enabled or device token updated
      if (cloudEnabled) {
        cloudConnect();
      } else {
        cloudDisconnect();
      }
    }*/
    if (CLOUDdata.containsKey("status")) {
      success = true;
    }
    // save, load, init, erase
    if (CLOUDdata.containsKey("save") && CLOUDdata["save"]) {
      success = CloudConfig_save();
      if (success) {
        Log.notice(F("CLOUD config saved" CR));
      }
    } else if (CLOUDdata.containsKey("load") && CLOUDdata["load"]) {
      success = CloudConfig_load();
      if (success) {
        Log.notice(F("CLOUD config loaded" CR));
      }
    } else if (CLOUDdata.containsKey("init") && CLOUDdata["init"]) {
      CloudConfig_init();
      if (cloudEnabled) {
        cloudConnect();
      } else {
        cloudDisconnect();
      }
      success = true;
      if (success) {
        Log.notice(F("CLOUD config initialised" CR));
      }
    } else if (CLOUDdata.containsKey("erase") && CLOUDdata["erase"]) {
      // Erase config from NVS (non-volatile storage)
      preferences.begin(Gateway_Short_Name, false);
      success = preferences.remove("CLOUDConfig");
      preferences.end();
      if (success) {
        Log.notice(F("CLOUD config erased" CR));
      }
    }
    if (success) {
      stateCLOUDStatus();
    } else {
      pub(subjectCLOUDtoMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("[ CLOUD ] MQTTtoCLOUD Fail json" CR), CLOUDdata);
    }
  }
}

String stateCLOUDStatus() {
  //Publish display state
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject CLOUDdata = jsonBuffer.to<JsonObject>();
  CLOUDdata["cloudEnabled"] = (bool)cloudEnabled;
  if (deviceToken.length() == CLOUD_DEVICE_TOKEN_LENGTH) {
    CLOUDdata["cloudDeviceToken"] = true;
  } else {
    CLOUDdata["cloudDeviceToken"] = false;
  }
  CLOUDdata["cloudActive"] = (bool)cloudActive;
  CLOUDdata["cloudState"] = cloud.state();
  CLOUDdata["cloudQueue"] = uxQueueMessagesWaiting(omgCloudQueue);
  pub(subjectCLOUDtoMQTT, CLOUDdata);
  // apply
  String output;
  serializeJson(CLOUDdata, output);
  return output;
}

bool setCloudEnabled(bool value) {
  cloudEnabled = value;
  Log.notice(F("Set cloudEnabled: %d" CR), cloudEnabled);
  CloudConfig_save();
  if (cloudEnabled) {
    cloudConnect();
  } else {
    cloudDisconnect();
  }
  return true;
}

bool setCloudDeviceToken(String tempToken) {
  if (tempToken.length() == CLOUD_DEVICE_TOKEN_LENGTH) {
    deviceToken = tempToken;
    CloudConfig_save();
    // Log.notice(F("Set deviceToken: %d" CR), deviceToken);
    return true;
  } else {
    return false;
  }
}

bool isCloudEnabled() {
  return (bool)cloudEnabled;
}

bool isCloudDeviceTokenSupplied() {
  return (deviceToken.length() == CLOUD_DEVICE_TOKEN_LENGTH);
}

void CloudLoop() {
  if (cloudActive) {
    if (!cloud.connected()) {
      if (uptime() > reconnectTime) {
        cloudConnect();
      }
    } else {
      cloud.loop();

      if (uxQueueMessagesWaiting(omgCloudQueue) && uxSemaphoreGetCount(omgCloudSemaphore)) { // && uxSemaphoreGetCount(omgCloudSemaphore)
        if (xSemaphoreTake(omgCloudSemaphore, (TickType_t)10) == pdTRUE) {
          cloudMsg* omgCloudMessage = nullptr;
          if (xQueueReceive(omgCloudQueue,
                            &omgCloudMessage,
                            (TickType_t)10) == pdPASS) {
            Log.verbose(F("[ OMG->CLOUD ] unQueue topic: %s msg: %s " CR), omgCloudMessage->topic, omgCloudMessage->payload);
            if (!cloud.publish(omgCloudMessage->topic, omgCloudMessage->payload, omgCloudMessage->retain)) {
              Log.error(F("[ CLOUD ] Cloud publish failed, rc=%d, topic: %s msg: %s " CR), cloud.state(), omgCloudMessage->topic, omgCloudMessage->payload);
              reconnectTime = uptime() + RECONNECT_DELAY;
            } else {
              Log.trace(F("[ OMG->CLOUD ] topic: %s msg: %s " CR), omgCloudMessage->topic, omgCloudMessage->payload);
            }
            free(omgCloudMessage->topic);
            free(omgCloudMessage->payload);
            free(omgCloudMessage);
          }
          xSemaphoreGive(omgCloudSemaphore);
        }
      }
    }
  }
}

void pubOmgCloud(const char* topic, const char* payload, bool retain) {
  Log.trace(F("[ OMG->CLOUD ] place on queue topic: %s msg: %s " CR), (cloudTopic + '/' + String(topic)).c_str(), payload);

  if (cloudReady) {
    cloudMsg* omgCloudMessage = (cloudMsg*)malloc(sizeof(cloudMsg));
    if (omgCloudMessage != NULL) {
      String messageTopic;

      // if (strncmp(topic, discovery_Topic, strlen(discovery_Topic)) != 0) {
      //  messageTopic = cloudTopic + String(topic + (strlen(mqtt_topic) + strlen(gateway_name)));
      // } else {
      messageTopic = cloudTopic + '/' + String(topic);
      // }
      char* topicHolder = (char*)malloc(messageTopic.length() + 1);
      if (topicHolder == NULL) {
        free(omgCloudMessage);
        return;
      }
      messageTopic.toCharArray(topicHolder, messageTopic.length() + 1);
      omgCloudMessage->topic = topicHolder;

      omgCloudMessage->retain = retain;

      char* payloadHolder = (char*)malloc(strlen(payload) + 1);
      if (payloadHolder == NULL) {
        free(omgCloudMessage);
        free(topicHolder);
        return;
      }
      strlcpy(payloadHolder, payload, strlen(payload) + 1);
      omgCloudMessage->payload = payloadHolder;

      if (xQueueSend(omgCloudQueue, (void*)&omgCloudMessage, 0) != pdTRUE) {
        Log.warning(F("[ CLOUD ] omgCloudQueue full, discarding signal topic: %s " CR), omgCloudMessage->topic);
        free(omgCloudMessage);
        free(topicHolder);
        free(payloadHolder);
        displayPrint("[ CLOUD ] discarding");
      } else {
        // Log.trace(F("[ OMG->CLOUD ] placed on queue topic: %s msg: %s " CR), (cloudTopic + '/' + String(topic)).c_str(), payload);
      }
    } else {
      Log.error(F("[ OMG->CLOUD ] insufficent memory %s-%d" CR), topic, strlen(payload));
    }
  };
}

#endif