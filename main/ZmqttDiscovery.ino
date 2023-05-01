/*
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

   This is the Home Assistant MQTT Discovery addon.

    Copyright: (c) Rafal Herok

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

#ifdef ZmqttDiscovery

String getMacAddress() {
  uint8_t baseMac[6];
  char baseMacChr[13] = {0};
#  if defined(ESP8266)
  WiFi.macAddress(baseMac);
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
#  elif defined(ESP32)
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
#  else
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#  endif
  return String(baseMacChr);
}

String getUniqueId(String name, String sufix) {
  String uniqueId = (String)getMacAddress() + "-" + name + sufix;
  return String(uniqueId);
}

#  ifdef ZgatewayBT
/**
 * Create a discover messages form a list of attribute
 * 
 * @param mac the MAC address
 * @param sensorList[][0] = component type
 * @param sensorList[][1] = name
 * @param sensorList[][2] = availability topic
 * @param sensorList[][3] = device class
 * @param sensorList[][4] = value template
 * @param sensorList[][5] = payload on
 * @param sensorList[][6] = payload off
 * @param sensorList[][7] = unit of measurement
 * @param sensorList[][8] = unit of measurement
 * @param sensorCount number of sensor
 * @param device_name name of sensors
 * @param device_manufacturer name of manufacturer
 * @param device_model the model
 * */
void createDiscoveryFromList(const char* mac,
                             const char* sensorList[][9],
                             int sensorCount,
                             const char* device_name,
                             const char* device_manufacturer,
                             const char* device_model) {
  for (int i = 0; i < sensorCount; i++) {
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + sensorList[i][1];

    createDiscovery(sensorList[i][0],
                    discovery_topic.c_str(), sensorList[i][1], unique_id.c_str(),
                    will_Topic, sensorList[i][3], sensorList[i][4],
                    sensorList[i][5], sensorList[i][6], sensorList[i][7],
                    0, "", "", false, "",
                    device_name, device_manufacturer, device_model, mac, false,
                    sensorList[i][8] //The state class
    );
  }
}
#  endif

/**
 * @brief Create a message for Discovery Device Trigger. For HA @see https://www.home-assistant.io/integrations/device_trigger.mqtt/
 * @param use_gateway_info      Boolean where true mean use the OMG information as Device Information
 * @param topic                 The Topic  where the trigger will publish the content
 * @param type                  The type of the trigger, e.g. button_short_press. Entries supported by the HA Frontend: button_short_press, button_short_release, button_long_press, button_long_release, button_double_press, button_triple_press, button_quadruple_press, button_quintuple_press. If set to an unsupported value, will render as subtype type, e.g. button_1 spammed with type set to spammed and subtype set to button_1
 * @param subtype               The subtype of the trigger, e.g. button_1. Entries supported by the HA frontend: turn_on, turn_off, button_1, button_2, button_3, button_4, button_5, button_6. If set to an unsupported value, will render as subtype type, e.g. left_button pressed with type set to button_short_press and subtype set to left_button
 * @param unique_id             Valid only if gateway entry is false, The IDs that uniquely identify the device. For example a serial number.
 * @param device_name           Valid only if gateway entry is false, The name of the device.
 * @param device_manufacturer   Valid only if gateway entry is false, The manufacturer of the device.
 * @param device_model          Valid only if gateway entry is false, The model of the device.
 * @param device_id            Valid only if gateway entry is false, The connection of the device to the outside world
 */
void announceDeviceTrigger(bool use_gateway_info, char* topic, char* type, char* subtype, char* unique_id, char* device_name, char* device_manufacturer, char* device_model, char* device_id) {
  //Create The Json
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject sensor = jsonBuffer.to<JsonObject>();

  // SET Default Configuration
  sensor["automation_type"] = "trigger"; // The type of automation, must be ‘trigger’.

  //SET TYPE
  if (type && type[0] != 0) {
    sensor["type"] = type;
  } else {
    sensor["type"] = "button_short_press";
  }

  //SET SUBTYPE
  if (subtype && subtype[0] != 0) {
    sensor["subtype"] = subtype;
  } else {
    sensor["subtype"] = "turn_on";
  }

  /* Set The topic */
  if (topic && topic[0]) {
    char state_topic[mqtt_topic_max_size];

    strcpy(state_topic, mqtt_topic);
    strcat(state_topic, gateway_name);

    strcat(state_topic, topic);
    sensor["topic"] = state_topic;
  }

  /* Set The Devices */
  StaticJsonDocument<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject device = jsonDeviceBuffer.to<JsonObject>();
  JsonArray identifiers = device.createNestedArray("identifiers");

  if (use_gateway_info) {
    device["name"] = gateway_name;
#  ifndef GATEWAY_MODEL
    String model = "";
    serializeJson(modules, model);
    device["model"] = model;
#  else
    device["model"] = GATEWAY_MODEL;
#  endif

    device["manufacturer"] = GATEWAY_MANUFACTURER;
    device["sw_version"] = OMG_VERSION;
    identifiers.add(getMacAddress());

  } else {
    char deviceid[13];
    memcpy(deviceid, &unique_id[0], 12);
    deviceid[12] = '\0';

    identifiers.add(deviceid);

    /*Set Connection */
    if (device_id && device_id[0] != 0) {
      JsonArray connections = device.createNestedArray("connections");
      JsonArray connection_mac = connections.createNestedArray();
      connection_mac.add("mac");
      connection_mac.add(device_id);
    }

    //Set manufacturer
    if (device_manufacturer && device_manufacturer[0]) {
      device["manufacturer"] = device_manufacturer;
    }

    //Set name
    if (device_name && device_name[0]) {
      device["name"] = device_name;
    }

    // set The Model
    if (device_model && device_model[0]) {
      device["model"] = device_model;
    }

    device["via_device"] = gateway_name; //device name of the board
  }
  sensor["device"] = device; //device representing the board

  /* Publish on the topic */
  String topic_to_publish = String(discovery_Topic) + "/device_automation/" + String(unique_id) + "/config";
  Log.trace(F("Announce Device Trigger  %s" CR), topic_to_publish.c_str());
  pub_custom_topic((char*)topic_to_publish.c_str(), sensor, true);
}

/**
 * @brief Generate message and publish it on an MQTT discovery explorer. For HA @see https://www.home-assistant.io/docs/mqtt/discovery/
 * 
 * @param sensor_type the Type
 * @param st_topic set state topic,
 * @param s_name set name,
 * @param unique_id set uniqueId
 * @param availability_topic set availability_topic,
 * @param device_class set device_class,
 * @param value_template set value_template,
 * @param payload_on set payload_on,
 * @param payload_off set payload_off,
 * @param unit_of_meas set unit_of_meas,
 * @param off_delay set off_delay
 * @param payload_available set payload_available,
 * @param payload_not_available set payload_not_available
 * @param gateway_entity set is a gateway entity, 
 * @param cmd_topic set command topic
 * @param device_name set device name, 
 * @param device_manufacturer set device manufacturer, 
 * @param device_model set device model, 
 * @param device_id set device(BLE)/entity(RTL_433) identification, 
 * @param retainCmd set retain
 * @param state_class set state class
 * 
 * */
void createDiscovery(const char* sensor_type,
                     const char* st_topic, const char* s_name, const char* unique_id,
                     const char* availability_topic, const char* device_class, const char* value_template,
                     const char* payload_on, const char* payload_off, const char* unit_of_meas,
                     int off_delay,
                     const char* payload_available, const char* payload_not_available, bool gateway_entity, const char* cmd_topic,
                     const char* device_name, const char* device_manufacturer, const char* device_model, const char* device_id, bool retainCmd,
                     const char* state_class, const char* state_off, const char* state_on) {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject sensor = jsonBuffer.to<JsonObject>();

  // If a component cannot render it's state (f.i. KAKU relays) no state topic
  // should be added. Without a state topic HA will use optimistic mode for the
  // component by default. The Home Assistant UI for optimistic switches
  // (separate on and off icons) allows for multiple
  // subsequent on commands. This is required for dimming on KAKU relays like
  // the ACM-300.
  if (st_topic[0]) {
    char state_topic[mqtt_topic_max_size];
    // If not an entity belonging to the gateway we put wild card for the location and gateway name
    // allowing to have the entity detected by several gateways and a consistent discovery topic among the gateways
    if (gateway_entity) {
      strcpy(state_topic, mqtt_topic);
      strcat(state_topic, gateway_name);
    } else {
      strcpy(state_topic, "+/+");
    }
    strcat(state_topic, st_topic);
    sensor["stat_t"] = state_topic;
  }

  if (availability_topic[0] && gateway_entity) {
    char avty_topic[mqtt_topic_max_size];
    strcpy(avty_topic, mqtt_topic);
    strcat(avty_topic, gateway_name);
    strcat(avty_topic, availability_topic);
    sensor["avty_t"] = avty_topic;
  }

  if (device_class[0]) {
    // We check if the class belongs to HAAS classes list
    int num_classes = sizeof(availableHASSClasses) / sizeof(availableHASSClasses[0]);
    for (int i = 0; i < num_classes; i++) { // see class list and size into config_mqttDiscovery.h
      if (strcmp(availableHASSClasses[i], device_class) == 0) {
        sensor["dev_cla"] = device_class; //device_class
      }
    }
  }

  if (unit_of_meas[0]) {
    // We check if the class belongs to HAAS units list
    int num_units = sizeof(availableHASSUnits) / sizeof(availableHASSUnits[0]);
    for (int i = 0; i < num_units; i++) { // see units list and size into config_mqttDiscovery.h
      if (strcmp(availableHASSUnits[i], unit_of_meas) == 0) {
        sensor["unit_of_meas"] = unit_of_meas; //unit_of_measurement*/
      }
    }
  }
  sensor["name"] = s_name; //name
  sensor["uniq_id"] = unique_id; //unique_id
  if (retainCmd)
    sensor["retain"] = retainCmd; // Retain command
  if (value_template[0])
    sensor["val_tpl"] = value_template; //value_template
  if (payload_on[0]) {
    if (strcmp(sensor_type, "button") == 0) {
      sensor["pl_prs"] = payload_on; // payload_press for a button press
    } else if (strcmp(sensor_type, "number") == 0) {
      sensor["cmd_tpl"] = payload_on; // payload_on for a switch
    } else if (strcmp(sensor_type, "update") == 0) {
      sensor["payload_install"] = payload_on; // payload_install for update
    } else {
      sensor["pl_on"] = payload_on; // payload_on for the rest
    }
  }
  if (payload_off[0])
    sensor["pl_off"] = payload_off; //payload_off
  if (strcmp(sensor_type, "device_tracker") == 0)
    sensor["source_type "] = "bluetooth_le"; // payload_install for update
  if (off_delay != 0)
    sensor["off_delay"] = off_delay; //off_delay
  if (payload_available[0])
    sensor["pl_avail"] = payload_available; // payload_on
  if (payload_not_available[0])
    sensor["pl_not_avail"] = payload_not_available; //payload_off
  if (state_class && state_class[0])
    sensor["state_class"] = state_class; //add the state class on the sensors ( https://developers.home-assistant.io/docs/core/entity/sensor/#available-state-classes )
  if (state_on != nullptr)
    if (strcmp(state_on, "true") == 0) {
      sensor["stat_on"] = true;
    } else {
      sensor["stat_on"] = state_on;
    }
  if (state_off != nullptr)
    if (strcmp(state_off, "false") == 0) {
      sensor["stat_off"] = false;
    } else {
      sensor["stat_off"] = state_off;
    }
  if (cmd_topic[0]) {
    char command_topic[mqtt_topic_max_size];
    strcpy(command_topic, mqtt_topic);
    strcat(command_topic, gateway_name);
    strcat(command_topic, cmd_topic);
    sensor["cmd_t"] = command_topic; //command_topic
  }

  StaticJsonDocument<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject device = jsonDeviceBuffer.to<JsonObject>();
  JsonArray identifiers = device.createNestedArray("identifiers");

  if (gateway_entity) {
    //device representing the board
    device["name"] = String(gateway_name);
#  ifndef GATEWAY_MODEL
    String model = "";
    serializeJson(modules, model);
    device["model"] = model;
#  else
    device["model"] = GATEWAY_MODEL;
#  endif
    device["manufacturer"] = GATEWAY_MANUFACTURER;
    device["sw_version"] = OMG_VERSION;
    identifiers.add(String(getMacAddress()));
  } else {
    //The Connections
    if (device_id[0]) {
      JsonArray connections = device.createNestedArray("connections");
      JsonArray connection_mac = connections.createNestedArray();
      connection_mac.add("mac");
      connection_mac.add(device_id);
      //Device representing the actual sensor/switch device
      //The Device ID
      identifiers.add(device_id);
    }

    if (device_manufacturer[0]) {
      device["manufacturer"] = device_manufacturer;
    }

    if (device_model[0]) {
      device["model"] = device_model;
    }

    // generate unique device name by adding the second half of the device_id only if device_name and device_id are different
    if (device_name[0]) {
      if (strcmp(device_id, device_name) != 0 && device_id[0]) {
        device["name"] = device_name + String("-") + String(device_id + 6);
      } else {
        device["name"] = device_name;
      }
    }

    device["via_device"] = String(gateway_name); //device name of the board
  }

  sensor["device"] = device;

  String topic = String(discovery_Topic) + "/" + String(sensor_type) + "/" + String(unique_id) + "/config";
  Log.trace(F("Announce Device %s on  %s" CR), String(sensor_type).c_str(), topic.c_str());
  pub_custom_topic((char*)topic.c_str(), sensor, true);
}

void eraseTopic(const char* sensor_type, const char* unique_id) {
  String topic = String(discovery_Topic) + "/" + String(sensor_type) + "/" + String(unique_id) + "/config";
  Log.trace(F("Erase entity discovery %s on  %s" CR), String(sensor_type).c_str(), topic.c_str());
  pubMQTT((char*)topic.c_str(), "", true);
}

#  ifdef ZgatewayBT
void btPresenceParametersDiscovery() {
  if (BTConfig.presenceEnable) {
    createDiscovery("number", //set Type
                    subjectBTtoMQTT, "BT: Presence detection timer", (char*)getUniqueId("presenceawaytimer", "").c_str(), //set state_topic,name,uniqueId
                    will_Topic, "", "{{ value_json.presenceawaytimer/60000 }}", //set availability_topic,device_class,value_template,
                    "{\"presenceawaytimer\":{{value*60000}},\"save\":true}", "", "min", //set,payload_on,payload_off,unit_of_meas,
                    0, //set  off_delay
                    Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain,
                    stateClassNone //State Class
    );
  }
}

void btScanParametersDiscovery() {
  if (!BTConfig.adaptiveScan) {
    createDiscovery("number", //set Type
                    subjectBTtoMQTT, "BT: Interval between scans", (char*)getUniqueId("interval", "").c_str(), //set state_topic,name,uniqueId
                    will_Topic, "", "{{ value_json.interval/1000 }}", //set availability_topic,device_class,value_template,
                    "{\"interval\":{{value*1000}},\"save\":true}", "", "s", //set,payload_on,payload_off,unit_of_meas,
                    0, //set  off_delay
                    Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain,
                    stateClassNone //State Class
    );
    createDiscovery("number", //set Type
                    subjectBTtoMQTT, "BT: Interval between active scans", (char*)getUniqueId("intervalacts", "").c_str(), //set state_topic,name,uniqueId
                    will_Topic, "", "{{ value_json.intervalacts/1000 }}", //set availability_topic,device_class,value_template,
                    "{\"intervalacts\":{{value*1000}},\"save\":true}", "", "s", //set,payload_on,payload_off,unit_of_meas,
                    0, //set  off_delay
                    Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain,
                    stateClassNone //State Class
    );
  }
}
#  endif

void pubMqttDiscovery() {
  Log.trace(F("omgStatusDiscovery" CR));
  createDiscovery("binary_sensor", //set Type
                  will_Topic, "SYS: Connectivity", (char*)getUniqueId("connectivity", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "connectivity", "", //set availability_topic,device_class,value_template,
                  Gateway_AnnouncementMsg, will_Message, "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Uptime", (char*)getUniqueId("uptime", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.uptime }}", //set availability_topic,device_class,value_template,
                  "", "", "s", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  if defined(ESP8266) || defined(ESP32)
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Free memory", (char*)getUniqueId("freemem", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.freemem }}", //set availability_topic,device_class,value_template,
                  "", "", "B", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: IP", (char*)getUniqueId("ip", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.ip }}", //set availability_topic,device_class,value_template,
                  "", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("switch", //set Type
                  subjectSYStoMQTT, "SYS: Auto discovery", (char*)getUniqueId("discovery", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.discovery }}", //set availability_topic,device_class,value_template,
                  "{\"discovery\":true}", "{\"discovery\":false}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset, //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", true, // device name, device manufacturer, device model, device MAC, retain,
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );

#    ifdef ZdisplaySSD1306
  createDiscovery("switch", //set Type
                  subjectSSD1306toMQTT, "SSD1306: Control", (char*)getUniqueId("onstate", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.onstate }}", //set availability_topic,device_class,value_template,
                  "{\"onstate\":true,\"save\":true}", "{\"onstate\":false,\"save\":true}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSSD1306set, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );
  createDiscovery("switch", //set Type
                  subjectSSD1306toMQTT, "SSD1306: Display metric", (char*)getUniqueId("displaymetric", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.displaymetric }}", //set availability_topic,device_class,value_template,
                  "{\"displaymetric\":true,\"save\":true}", "{\"displaymetric\":false,\"save\":true}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSSD1306set, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );
  createDiscovery("number", //set Type
                  subjectSSD1306toMQTT, "SSD1306: Brightness", (char*)getUniqueId("brightness", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.brightness }}", //set availability_topic,device_class,value_template,
                  "{\"brightness\":{{value}},\"save\":true}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSSD1306set, //set,payload_available,payload_not available,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#    endif

#    ifndef ESP32_ETHERNET
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Rssi", (char*)getUniqueId("rssi", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.rssi }}", //set availability_topic,device_class,value_template,
                  "", "", "dB", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#    endif
#  endif
#  if defined(ESP32) && !defined(NO_INT_TEMP_READING)
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Internal temperature", (char*)getUniqueId("tempc", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "temperature", "{{ value_json.tempc }}", //set availability_topic,device_class,value_template,
                  "", "", "°C", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC
                  stateClassMeasurement //State Class
  );
#    if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Bat voltage", (char*)getUniqueId("m5batvoltage", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.m5batvoltage }}", //set availability_topic,device_class,value_template,
                  "", "", "V", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a child device, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Bat current", (char*)getUniqueId("m5batcurrent", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.m5batcurrent }}", //set availability_topic,device_class,value_template,
                  "", "", "A", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a child device, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Vin voltage", (char*)getUniqueId("m5vinvoltage", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.m5vinvoltage }}", //set availability_topic,device_class,value_template,
                  "", "", "V", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a child device, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Vin current", (char*)getUniqueId("m5vincurrent", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.m5vincurrent }}", //set availability_topic,device_class,value_template,
                  "", "", "A", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a child device, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#    endif
#    ifdef ZboardM5STACK
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Batt level", (char*)getUniqueId("m5battlevel", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.m5battlevel }}", //set availability_topic,device_class,value_template,
                  "", "", "%", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a child device, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("binary_sensor", //set Type
                  subjectSYStoMQTT, "SYS: Is Charging", (char*)getUniqueId("m5ischarging", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "{{ value_json.m5ischarging }}", "", //set availability_topic,device_class,value_template,
                  "", "", "%", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a child device, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("binary_sensor", //set Type
                  subjectSYStoMQTT, "SYS: Is Charge Full", (char*)getUniqueId("m5ischargefull", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "{{ value_json.m5ischargefull }}", "", //set availability_topic,device_class,value_template,
                  "", "", "%", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_available,payload_not available   ,is a child device, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#    endif
#  endif
  createDiscovery("button", //set Type
                  will_Topic, "SYS: Restart gateway", (char*)getUniqueId("restart", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "", //set availability_topic,device_class,value_template,
                  "{\"cmd\":\"restart\"}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("button", //set Type
                  will_Topic, "SYS: Erase credentials", (char*)getUniqueId("erase", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "", //set availability_topic,device_class,value_template,
                  "{\"cmd\":\"erase\"}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("update", //set Type
                  subjectRLStoMQTT, "SYS: Firmware Update", (char*)getUniqueId("update", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "firmware", "", //set availability_topic,device_class,value_template,
                  LATEST_OR_DEV, "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSupdate, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  ifdef ZsensorBME280
#    define BMEparametersCount 5
  Log.trace(F("bme280Discovery" CR));
  char* BMEsensor[BMEparametersCount][8] = {
      {"sensor", "temp", "bme", "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "pa", "bme", "", jsonPa, "", "", "hPa"},
      {"sensor", "hum", "bme", "humidity", jsonHum, "", "", "%"},
      {"sensor", "altim", "bme", "", jsonAltim, "", "", "m"},
      {"sensor", "altift", "bme", "", jsonAltif, "", "", "ft"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < BMEparametersCount; i++) {
    createDiscovery(BMEsensor[i][0],
                    BMETOPIC, BMEsensor[i][1], (char*)getUniqueId(BMEsensor[i][1], BMEsensor[i][2]).c_str(),
                    will_Topic, BMEsensor[i][3], BMEsensor[i][4],
                    BMEsensor[i][5], BMEsensor[i][6], BMEsensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorHTU21
#    define HTUparametersCount 2
  Log.trace(F("htu21Discovery" CR));
  char* HTUsensor[HTUparametersCount][8] = {
      {"sensor", "temp", "htu", "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "hum", "htu", "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < HTUparametersCount; i++) {
    //trc(HTUsensor[i][1]);
    createDiscovery(HTUsensor[i][0],
                    HTUTOPIC, HTUsensor[i][1], (char*)getUniqueId(HTUsensor[i][1], HTUsensor[i][2]).c_str(),
                    will_Topic, HTUsensor[i][3], HTUsensor[i][4],
                    HTUsensor[i][5], HTUsensor[i][6], HTUsensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorLM75
  Log.trace(F("LM75Discovery" CR));
  char* LM75sensor[8] = {"sensor", "temp", "htu", "temperature", jsonTempc, "", "", "°C"};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  createDiscovery(LM75sensor[0],
                  LM75TOPIC, LM75sensor[1], (char*)getUniqueId(LM75sensor[1], LM75sensor[2]).c_str(),
                  will_Topic, LM75sensor[3], LM75sensor[4],
                  LM75sensor[5], LM75sensor[6], LM75sensor[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZsensorAHTx0
#    define AHTparametersCount 2
  Log.trace(F("AHTx0Discovery" CR));
  char* AHTsensor[AHTparametersCount][8] = {
      {"sensor", "temp", "aht", "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "hum", "aht", "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < AHTparametersCount; i++) {
    createDiscovery(AHTsensor[i][0],
                    AHTTOPIC, AHTsensor[i][1], (char*)getUniqueId(AHTsensor[i][1], AHTsensor[i][2]).c_str(),
                    will_Topic, AHTsensor[i][3], AHTsensor[i][4],
                    AHTsensor[i][5], AHTsensor[i][6], AHTsensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorDHT
#    define DHTparametersCount 2
  Log.trace(F("DHTDiscovery" CR));
  char* DHTsensor[DHTparametersCount][8] = {
      {"sensor", "temp", "dht", "temperature", jsonTempc, "", "", "°C"},
      {"sensor", "hum", "dht", "humidity", jsonHum, "", "", "%"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < DHTparametersCount; i++) {
    //trc(DHTsensor[i][1]);
    createDiscovery(DHTsensor[i][0],
                    DHTTOPIC, DHTsensor[i][1], (char*)getUniqueId(DHTsensor[i][1], DHTsensor[i][2]).c_str(),
                    will_Topic, DHTsensor[i][3], DHTsensor[i][4],
                    DHTsensor[i][5], DHTsensor[i][6], DHTsensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorADC
  Log.trace(F("ADCDiscovery" CR));
  char* ADCsensor[8] = {"sensor", "adc", "", "", jsonAdc, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(ADCsensor[1]);
  createDiscovery(ADCsensor[0],
                  ADCTOPIC, ADCsensor[1], (char*)getUniqueId(ADCsensor[1], ADCsensor[2]).c_str(),
                  will_Topic, ADCsensor[3], ADCsensor[4],
                  ADCsensor[5], ADCsensor[6], ADCsensor[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZsensorBH1750
#    define BH1750parametersCount 3
  Log.trace(F("BH1750Discovery" CR));
  char* BH1750sensor[BH1750parametersCount][8] = {
      {"sensor", "lux", "BH1750", "illuminance", jsonLux, "", "", "lx"},
      {"sensor", "ftCd", "BH1750", "", jsonFtcd, "", "", ""},
      {"sensor", "wattsm2", "BH1750", "", jsonWm2, "", "", "wm²"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < BH1750parametersCount; i++) {
    //trc(BH1750sensor[i][1]);
    createDiscovery(BH1750sensor[i][0],
                    subjectBH1750toMQTT, BH1750sensor[i][1], (char*)getUniqueId(BH1750sensor[i][1], BH1750sensor[i][2]).c_str(),
                    will_Topic, BH1750sensor[i][3], BH1750sensor[i][4],
                    BH1750sensor[i][5], BH1750sensor[i][6], BH1750sensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorMQ2
#    define MQ2parametersCount 2
  Log.trace(F("MQ2Discovery" CR));
  char* MQ2sensor[MQ2parametersCount][8] = {
      {"sensor", "gas", "MQ2", "", jsonVal, "", "", "ppm"},
      {"binary_sensor", "MQ2", "", "", jsonPresence, "true", "false", ""}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < MQ2parametersCount; i++) {
    createDiscovery(MQ2sensor[i][0],
                    subjectMQ2toMQTT, MQ2sensor[i][1], (char*)getUniqueId(MQ2sensor[i][1], MQ2sensor[i][2]).c_str(),
                    will_Topic, MQ2sensor[i][3], MQ2sensor[i][4],
                    MQ2sensor[i][5], MQ2sensor[i][6], MQ2sensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorTEMT6000
#    define TEMT6000parametersCount 3
  Log.trace(F("TEMT6000Discovery" CR));
  char* TEMT6000sensor[TEMT6000parametersCount][8] = {
      {"sensor", "lux", "TEMT6000", "illuminance", jsonLux, "", "", "lx"},
      {"sensor", "ftcd", "TEMT6000", "", jsonFtcd, "", "", ""},
      {"sensor", "wattsm2", "TEMT6000", "", jsonWm2, "", "", "wm²"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < TEMT6000parametersCount; i++) {
    //trc(TEMT6000sensor[i][1]);
    createDiscovery(TEMT6000sensor[i][0],
                    subjectTEMT6000toMQTT, TEMT6000sensor[i][1], (char*)getUniqueId(TEMT6000sensor[i][1], TEMT6000sensor[i][2]).c_str(),
                    will_Topic, TEMT6000sensor[i][3], TEMT6000sensor[i][4],
                    TEMT6000sensor[i][5], TEMT6000sensor[i][6], TEMT6000sensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorTSL2561
#    define TSL2561parametersCount 3
  Log.trace(F("TSL2561Discovery" CR));
  char* TSL2561sensor[TSL2561parametersCount][8] = {
      {"sensor", "lux", "TSL2561", "illuminance", jsonLux, "", "", "lx"},
      {"sensor", "ftcd", "TSL2561", "", jsonFtcd, "", "", ""},
      {"sensor", "wattsm2", "TSL2561", "", jsonWm2, "", "", "wm²"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < TSL2561parametersCount; i++) {
    //trc(TSL2561sensor[i][1]);
    createDiscovery(TSL2561sensor[i][0],
                    subjectTSL12561toMQTT, TSL2561sensor[i][1], (char*)getUniqueId(TSL2561sensor[i][1], TSL2561sensor[i][2]).c_str(),
                    will_Topic, TSL2561sensor[i][3], TSL2561sensor[i][4],
                    TSL2561sensor[i][5], TSL2561sensor[i][6], TSL2561sensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorHCSR501
  Log.trace(F("HCSR501Discovery" CR));
  char* HCSR501sensor[8] = {"binary_sensor", "hcsr501", "", "", jsonPresence, "true", "false", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(HCSR501sensor[1]);
  createDiscovery(HCSR501sensor[0],
                  subjectHCSR501toMQTT, HCSR501sensor[1], (char*)getUniqueId(HCSR501sensor[1], HCSR501sensor[2]).c_str(),
                  will_Topic, HCSR501sensor[3], HCSR501sensor[4],
                  HCSR501sensor[5], HCSR501sensor[6], HCSR501sensor[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZsensorGPIOInput
  Log.trace(F("GPIOInputDiscovery" CR));
  char* GPIOInputsensor[8] = {"binary_sensor", "GPIOInput", "", "", jsonGpio, INPUT_GPIO_ON_VALUE, INPUT_GPIO_OFF_VALUE, ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(GPIOInputsensor[1]);
  createDiscovery(GPIOInputsensor[0],
                  subjectGPIOInputtoMQTT, GPIOInputsensor[1], (char*)getUniqueId(GPIOInputsensor[1], GPIOInputsensor[2]).c_str(),
                  will_Topic, GPIOInputsensor[3], GPIOInputsensor[4],
                  GPIOInputsensor[5], GPIOInputsensor[6], GPIOInputsensor[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZsensorINA226
#    define INA226parametersCount 3
  Log.trace(F("INA226Discovery" CR));
  char* INA226sensor[INA226parametersCount][8] = {
      {"sensor", "volt", "INA226", "", jsonVolt, "", "", "V"},
      {"sensor", "current", "INA226", "", jsonCurrent, "", "", "A"},
      {"sensor", "power", "INA226", "", jsonPower, "", "", "W"}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < INA226parametersCount; i++) {
    //trc(INA226sensor[i][1]);
    createDiscovery(INA226sensor[i][0],
                    subjectINA226toMQTT, INA226sensor[i][1], (char*)getUniqueId(INA226sensor[i][1], INA226sensor[i][2]).c_str(),
                    will_Topic, INA226sensor[i][3], INA226sensor[i][4],
                    INA226sensor[i][5], INA226sensor[i][6], INA226sensor[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, true, "",
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassNone //State Class
    );
  }
#  endif

#  ifdef ZsensorDS1820
  // Publish any DS1820 sensors found on the OneWire bus
  pubOneWire_HADiscovery();
#  endif

#  ifdef ZactuatorONOFF
  Log.trace(F("actuatorONOFFDiscovery" CR));
  char* actuatorONOFF[8] = {"switch", "actuatorONOFF", "", "", "", "{\"cmd\":1}", "{\"cmd\":0}", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(actuatorONOFF[1]);
  createDiscovery(actuatorONOFF[0],
                  subjectGTWONOFFtoMQTT, actuatorONOFF[1], (char*)getUniqueId(actuatorONOFF[1], actuatorONOFF[2]).c_str(),
                  will_Topic, actuatorONOFF[3], actuatorONOFF[4],
                  actuatorONOFF[5], actuatorONOFF[6], actuatorONOFF[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoONOFF,
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZsensorRN8209
#    define RN8209parametersCount 4
  Log.trace(F("RN8209Discovery" CR));
  char* RN8209sensor[RN8209parametersCount][8] = {
      {"sensor", "volt", "RN8209", "voltage", jsonVolt, "", "", "V"},
      {"sensor", "current", "RN8209", "current", jsonCurrent, "", "", "A"},
      {"sensor", "power", "RN8209", "power", jsonPower, "", "", "W"},
      {"binary_sensor", "inUse", "RN8209", "power", jsonInuseRN8209, "on", "off", ""}
      //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };

  for (int i = 0; i < RN8209parametersCount; i++) {
    String name = "NRG: " + String(RN8209sensor[i][1]);
    createDiscovery(RN8209sensor[i][0],
                    subjectRN8209toMQTT, (char*)name.c_str(), (char*)getUniqueId(RN8209sensor[i][1], RN8209sensor[i][2]).c_str(), //set state_topic,name,uniqueId
                    will_Topic, RN8209sensor[i][3], RN8209sensor[i][4], //set availability_topic,device_class,value_template,
                    RN8209sensor[i][5], RN8209sensor[i][6], RN8209sensor[i][7], //set,payload_on,payload_off,unit_of_meas
                    0, Gateway_AnnouncementMsg, will_Message, true, "", //set  off_delay,payload_available,payload_not available   ,is a gateway entity, command topic
                    "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                    stateClassMeasurement //State Class
    );
  }
#  endif

#  ifdef ZgatewayRF
  // Sensor to display RF received value
  Log.trace(F("gatewayRFDiscovery" CR));
  char* gatewayRF[8] = {"sensor", "gatewayRF", "", "", jsonVal, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(gatewayRF[1]);
  createDiscovery(gatewayRF[0],
                  subjectRFtoMQTT, gatewayRF[1], (char*)getUniqueId(gatewayRF[1], gatewayRF[2]).c_str(),
                  will_Topic, gatewayRF[3], gatewayRF[4],
                  gatewayRF[5], gatewayRF[6], gatewayRF[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );

#  endif

#  ifdef ZgatewayRF2
  // Sensor to display RF received value
  Log.trace(F("gatewayRF2Discovery" CR));
  char* gatewayRF2[8] = {"sensor", "gatewayRF2", "", "", jsonAddress, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(gatewayRF2[1]);
  createDiscovery(gatewayRF2[0],
                  subjectRF2toMQTT, gatewayRF2[1], (char*)getUniqueId(gatewayRF2[1], gatewayRF2[2]).c_str(),
                  will_Topic, gatewayRF2[3], gatewayRF2[4],
                  gatewayRF2[5], gatewayRF2[6], gatewayRF2[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZgatewayRFM69
  // Sensor to display RF received value
  Log.trace(F("gatewayRFM69Discovery" CR));
  char* gatewayRFM69[8] = {"sensor", "gatewayRFM69", "", "", jsonVal, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(gatewayRFM69[1]);
  createDiscovery(gatewayRFM69[0],
                  subjectRFM69toMQTT, gatewayRFM69[1], (char*)getUniqueId(gatewayRFM69[1], gatewayRFM69[2]).c_str(),
                  will_Topic, gatewayRFM69[3], gatewayRFM69[4],
                  gatewayRFM69[5], gatewayRFM69[6], gatewayRFM69[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZgatewayLORA
  // Sensor to display RF received value
  Log.trace(F("gatewayLORADiscovery" CR));
  char* gatewayLORA[8] = {"sensor", "gatewayLORA", "", "", jsonMsg, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(gatewayLORA[1]);
  createDiscovery(gatewayLORA[0],
                  subjectLORAtoMQTT, gatewayLORA[1], (char*)getUniqueId(gatewayLORA[1], gatewayLORA[2]).c_str(),
                  will_Topic, gatewayLORA[3], gatewayLORA[4],
                  gatewayLORA[5], gatewayLORA[6], gatewayLORA[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZgatewaySRFB
  // Sensor to display RF received value
  Log.trace(F("gatewaySRFBDiscovery" CR));
  char* gatewaySRFB[8] = {"sensor", "gatewaySRFB", "", "", jsonVal, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(gatewaySRFB[1]);
  createDiscovery(gatewaySRFB[0],
                  subjectSRFBtoMQTT, gatewaySRFB[1], (char*)getUniqueId(gatewaySRFB[1], gatewaySRFB[2]).c_str(),
                  will_Topic, gatewaySRFB[3], gatewaySRFB[4],
                  gatewaySRFB[5], gatewaySRFB[6], gatewaySRFB[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZgatewayPilight
  // Sensor to display RF received value
  Log.trace(F("gatewayPilightDiscovery" CR));
  char* gatewayPilight[8] = {"sensor", "gatewayPilight", "", "", jsonMsg, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(gatewayPilight[1]);
  createDiscovery(gatewayPilight[0],
                  subjectPilighttoMQTT, gatewayPilight[1], (char*)getUniqueId(gatewayPilight[1], gatewayPilight[2]).c_str(),
                  will_Topic, gatewayPilight[3], gatewayPilight[4],
                  gatewayPilight[5], gatewayPilight[6], gatewayPilight[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZgatewayIR
  // Sensor to display IR received value
  Log.trace(F("gatewayIRDiscovery" CR));
  char* gatewayIR[8] = {"sensor", "gatewayIR", "", "", jsonVal, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(gatewayIR[1]);
  createDiscovery(gatewayIR[0],
                  subjectIRtoMQTT, gatewayIR[1], (char*)getUniqueId(gatewayIR[1], gatewayIR[2]).c_str(),
                  will_Topic, gatewayIR[3], gatewayIR[4],
                  gatewayIR[5], gatewayIR[6], gatewayIR[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef Zgateway2G
  // Sensor to display 2G received value
  Log.trace(F("gateway2GDiscovery" CR));
  char* gateway2G[8] = {"sensor", "gateway2G", "", "", jsonMsg, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  //trc(gateway2G[1]);
  createDiscovery(gateway2G[0],
                  subject2GtoMQTT, gateway2G[1], (char*)getUniqueId(gateway2G[1], gateway2G[2]).c_str(),
                  will_Topic, gateway2G[3], gateway2G[4],
                  gateway2G[5], gateway2G[6], gateway2G[7],
                  0, Gateway_AnnouncementMsg, will_Message, true, "",
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZgatewayBT
#    ifdef ESP32

  createDiscovery("number", //set Type
                  subjectBTtoMQTT, "BT: Connnect interval", (char*)getUniqueId("intervalcnct", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.intervalcnct/60000 }}", //set availability_topic,device_class,value_template,
                  "{\"intervalcnct\":{{value*60000}},\"save\":true}", "", "min", //set,payload_on,payload_off,unit_of_meas,
                  0, //set off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("button", //set Type
                  will_Topic, "BT: Force scan", (char*)getUniqueId("force_scan", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "", //set availability_topic,device_class,value_template,
                  "{\"interval\":0}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
  createDiscovery("button", //set Type
                  will_Topic, "BT: Erase config", (char*)getUniqueId("erase_bt_config", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "", //set availability_topic,device_class,value_template,
                  "{\"erase\":true}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone //State Class
  );
  createDiscovery("switch", //set Type
                  subjectBTtoMQTT, "BT: Publish only sensors", (char*)getUniqueId("only_sensors", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.onlysensors }}", //set availability_topic,device_class,value_template,
                  "{\"onlysensors\":true,\"save\":true}", "{\"onlysensors\":false,\"save\":true}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );
  createDiscovery("switch", //set Type
                  subjectBTtoMQTT, "BT: Adaptive scan", (char*)getUniqueId("adaptive_scan", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.adaptivescan }}", //set availability_topic,device_class,value_template,
                  "{\"adaptivescan\":true,\"save\":true}", "{\"adaptivescan\":false,\"save\":true}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );

#      define EntitiesCount 8
  const char* obsoleteEntities[EntitiesCount][2] = {
      // Remove previously created entities for version < 1.4.0
      {"switch", "active_scan"}, // Replaced by adaptive scan
      // Remove previously created entities for version < 1.3.0
      {"number", "scanbcnct"}, // Now a connect interval
      // Remove previously created entities for version < 1.2.0
      {"switch", "restart"}, // Now a button
      {"switch", "erase"}, // Now a button
      {"switch", "force_scan"}, // Now a button
      {"sensor", "interval"}, // Now a number
      {"sensor", "scanbcnct"}}; // Now a number

  for (int i = 0; i < EntitiesCount; i++) {
    eraseTopic(obsoleteEntities[i][0], (char*)getUniqueId(obsoleteEntities[i][1], "").c_str());
  }

  btScanParametersDiscovery();

  btPresenceParametersDiscovery();

  createDiscovery("switch", //set Type
                  subjectBTtoMQTT, "BT: Publish HASS presence", (char*)getUniqueId("hasspresence", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.hasspresence }}", //set availability_topic,device_class,value_template,
                  "{\"hasspresence\":true,\"save\":true}", "{\"hasspresence\":false,\"save\":true}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );
  createDiscovery("switch", //set Type
                  subjectBTtoMQTT, "BT: Publish Advertisement data", (char*)getUniqueId("pubadvdata", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.pubadvdata }}", //set availability_topic,device_class,value_template,
                  "{\"pubadvdata\":true,\"save\":true}", "{\"pubadvdata\":false,\"save\":true}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );
  createDiscovery("switch", //set Type
                  subjectBTtoMQTT, "BT: Connect to devices", (char*)getUniqueId("bleconnect", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.bleconnect }}", //set availability_topic,device_class,value_template,
                  "{\"bleconnect\":true,\"save\":true}", "{\"bleconnect\":false,\"save\":true}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );
#      if DEFAULT_LOW_POWER_MODE != -1
  createDiscovery("switch", //set Type
                  subjectSYStoMQTT, "SYS: Low Power Mode command", (char*)getUniqueId("lowpowermode", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.lowpowermode | bool }}", //set availability_topic,device_class,value_template,
                  "{\"lowpowermode\":2}", "{\"lowpowermode\":0}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available,is a gateway entity, command topic
                  "", "", "", "", true, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );
#      else
  // Remove previously created switch for version < 1.4.0
  eraseTopic("switch", (char*)getUniqueId("lowpowermode", "").c_str());
#      endif
#    endif
#  endif
}
#endif
