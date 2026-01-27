/*
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

   This is the Home Assistant MQTT Discovery addon.

    Copyright: (c) Rafal Herok / Florian Robert

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
#  include "TheengsCommon.h"

#  ifdef ESP8266
#    include <ESP8266WiFi.h>
#  elif defined(ESP32)
#    include <WiFi.h>

#    include "esp_mac.h"
#  endif
#  ifdef ESP32_ETHERNET
#    include <ETH.h>
#  endif
#  include "config_mqttDiscovery.h"

extern bool ethConnected;
extern JsonArray modules;

// Using Home Assistant MQTT abbreviations to shorten names as per https://github.com/home-assistant/core/blob/dev/homeassistant/components/mqtt/abbreviations.py

char discovery_prefix[parameters_size + 1] = discovery_Prefix;
// From https://github.com/home-assistant/core/blob/d7ac4bd65379e11461c7ce0893d3533d8d8b8cbf/homeassistant/const.py#L225
// List of classes available in Home Assistant
static const char* const availableHASSClasses[] = {
    HASS_CLASS_BATTERY_CHARGING,
    HASS_CLASS_BATTERY,
    HASS_CLASS_CARBON_DIOXIDE,
    HASS_CLASS_CARBON_MONOXIDE,
    HASS_CLASS_CONNECTIVITY,
    HASS_CLASS_CURRENT,
    HASS_CLASS_DATA_SIZE,
    HASS_CLASS_DISTANCE,
    HASS_CLASS_DOOR,
    HASS_CLASS_DURATION,
    HASS_CLASS_ENERGY,
    HASS_CLASS_ENUM,
    HASS_CLASS_FREQUENCY,
    HASS_CLASS_GAS,
    HASS_CLASS_HUMIDITY,
    HASS_CLASS_ILLUMINANCE,
    HASS_CLASS_IRRADIANCE,
    HASS_CLASS_LOCK,
    HASS_CLASS_MOTION,
    HASS_CLASS_MOVING,
    HASS_CLASS_OCCUPANCY,
    HASS_CLASS_PM1,
    HASS_CLASS_PM10,
    HASS_CLASS_PM25,
    HASS_CLASS_POWER_FACTOR,
    HASS_CLASS_POWER,
    HASS_CLASS_PRECIPITATION_INTENSITY,
    HASS_CLASS_PRECIPITATION,
    HASS_CLASS_PRESSURE,
    HASS_CLASS_PROBLEM,
    HASS_CLASS_RESTART,
    HASS_CLASS_SIGNAL_STRENGTH,
    HASS_CLASS_SOUND_PRESSURE,
    HASS_CLASS_TEMPERATURE,
    HASS_CLASS_TIMESTAMP,
    HASS_CLASS_VOLTAGE,
    HASS_CLASS_WATER,
    HASS_CLASS_WEIGHT,
    HASS_CLASS_WIND_SPEED,
    HASS_CLASS_WINDOW};

// From https://github.com/home-assistant/core/blob/d7ac4bd65379e11461c7ce0893d3533d8d8b8cbf/homeassistant/const.py#L379
// List of units available in Home Assistant
static const char* const availableHASSUnits[] = {
    HASS_UNIT_AMP,
    HASS_UNIT_BYTE,
    HASS_UNIT_UV_INDEX,
    HASS_UNIT_VOLT,
    HASS_UNIT_WATT,
    HASS_UNIT_BPM,
    HASS_UNIT_BAR,
    HASS_UNIT_CM,
    HASS_UNIT_DB,
    HASS_UNIT_DBM,
    HASS_UNIT_FT,
    HASS_UNIT_HOUR,
    HASS_UNIT_HPA,
    HASS_UNIT_HZ,
    HASS_UNIT_KG,
    HASS_UNIT_KW,
    HASS_UNIT_KWH,
    HASS_UNIT_KMH,
    HASS_UNIT_LB,
    HASS_UNIT_LX,
    HASS_UNIT_MS,
    HASS_UNIT_MS2,
    HASS_UNIT_M3,
    HASS_UNIT_MGM3,
    HASS_UNIT_MIN,
    HASS_UNIT_MM,
    HASS_UNIT_MMH,
    HASS_UNIT_MILLISECOND,
    HASS_UNIT_MV,
    HASS_UNIT_USCM,
    HASS_UNIT_UGM3,
    HASS_UNIT_OHM,
    HASS_UNIT_PERCENT,
    HASS_UNIT_PPM,
    HASS_UNIT_DEGREE,
    HASS_UNIT_CELSIUS,
    HASS_UNIT_FAHRENHEIT,
    HASS_UNIT_SECOND,
    HASS_UNIT_WB2};

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

/**
 * Create discover messages from a list of attributes
 * Full-featured version supporting all discovery parameters
 *
 * Array format:
 * [0] = component type
 * [1] = name  
 * [2] = unique id suffix (for gateway) or name (for BLE)
 * [3] = device class
 * [4] = value template
 * [5] = payload on
 * [6] = payload off
 * [7] = unit of measurement
 * [8] = state class
 * [9] = state_off (for switches)
 * [10] = state_on (for switches)
 * [11] = custom state topic (overrides default)
 * [12] = custom command topic (overrides default)
 */
void createDiscoveryFromList(const char* mac,
                             const char* sensorList[][13],
                             int sensorCount,
                             const char* device_name,
                             const char* device_manufacturer,
                             const char* device_model,
                             bool gateway_entity,
                             const char* state_topic,
                             const char* availability_topic,
                             const char* command_topic) {
  for (int i = 0; i < sensorCount; i++) {
    String unique_id;
    if (gateway_entity) {
      unique_id = getUniqueId(sensorList[i][2] ? sensorList[i][2] : sensorList[i][1], "");
    } else if (mac) {
      unique_id = String(mac) + "-" + sensorList[i][1];
    } else {
      unique_id = sensorList[i][1];
    }

    // Use custom state topic if provided, otherwise use default
    String discovery_topic;
    if (sensorList[i][11] && sensorList[i][11][0]) {
      discovery_topic = String(sensorList[i][11]);
    } else if (mac && !gateway_entity) {
      discovery_topic = String(state_topic) + "/" + String(mac);
    } else {
      discovery_topic = String(state_topic);
    }

    // Use custom command topic if provided
    const char* cmd_topic = (sensorList[i][12] && sensorList[i][12][0]) ? sensorList[i][12] : command_topic;

    createDiscovery(sensorList[i][0],
                    discovery_topic.c_str(), sensorList[i][1], unique_id.c_str(),
                    availability_topic ? availability_topic : will_Topic,
                    sensorList[i][3], sensorList[i][4],
                    sensorList[i][5], sensorList[i][6], sensorList[i][7],
                    0, Gateway_AnnouncementMsg, will_Message, gateway_entity,
                    cmd_topic ? cmd_topic : "",
                    device_name ? device_name : "",
                    device_manufacturer ? device_manufacturer : "",
                    device_model ? device_model : "",
                    mac ? mac : "", false,
                    sensorList[i][8] ? sensorList[i][8] : stateClassNone,
                    sensorList[i][9], sensorList[i][10], // state_off, state_on
                    nullptr, nullptr // enum_options, command_template
    );
  }
}

#  if defined(ZgatewayBT) || defined(SecondaryModule)
#    include "config_BT.h"
// Backward compatibility overload for BLE devices using 9-column format
void createDiscoveryFromList(const char* mac,
                             const char* sensorList[][9],
                             int sensorCount,
                             const char* device_name,
                             const char* device_manufacturer,
                             const char* device_model) {
  // Create temporary extended array
  const char* extendedList[sensorCount][13];
  for (int i = 0; i < sensorCount; i++) {
    for (int j = 0; j < 9; j++) {
      extendedList[i][j] = sensorList[i][j];
    }
    for (int j = 9; j < 13; j++) {
      extendedList[i][j] = nullptr;
    }
  }

  createDiscoveryFromList(mac, extendedList, sensorCount,
                          device_name, device_manufacturer, device_model,
                          false, subjectBTtoMQTT, will_Topic, nullptr);
}
#  endif

#  ifdef ZgatewayRF
/**
 * @brief Announce that the Gateway have the ability to raise Trigger.
 * This function provide the configuration of the MQTT Device trigger ( @see https://www.home-assistant.io/integrations/device_trigger.mqtt/ ).
 * All messages published by this function will be interpreted as configuration messages of Gateway Triggers.
 * Instead, all messages published on the "triggerTopic" will be interpreted as Gateway trigger.
 *
 * @param triggerTopic          Mandatory - The MQTT topic subscribed to receive trigger events.
 * @param type                  The type of the trigger, e.g. button_short_press. Entries supported by the HA Frontend: button_short_press, button_short_release, button_long_press, button_long_release, button_double_press, button_triple_press, button_quadruple_press, button_quintuple_press. If set to an unsupported value, will render as subtype type, e.g. button_1 spammed with type set to spammed and subtype set to button_1
 * @param subtype               The subtype of the trigger, e.g. button_1. Entries supported by the HA frontend: turn_on, turn_off, button_1, button_2, button_3, button_4, button_5, button_6. If set to an unsupported value, will render as subtype type, e.g. left_button pressed with type set to button_short_press and subtype set to left_button
 * @param object_id             The object_id of the trigger.
 * @param value_template        The template to render the value of the trigger. The template can use the variables trigger.id, trigger.type, trigger.subtype, trigger.payload, trigger.payload_json, trigger.topic, trigger.timestamp, trigger.value, trigger.value_json. The template can be a string or a JSON object. If the template is a JSON object, it must be a valid JSON object. If the template is a string, it will be rendered as a string. If the template is a JSON object, it will be rendered as a JSON object.
 */
void announceGatewayTrigger(const char* triggerTopic,
                            const char* type,
                            const char* subtype,
                            const char* object_id,
                            const char* value_template) {
  //Create The Json
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject sensor = jsonBuffer.to<JsonObject>();

  /**
   * The type of automation, must be 'trigger'.
   * @see https://www.home-assistant.io/integrations/device_trigger.mqtt/#automation_type
   */
  sensor["atype"] = "trigger"; // automation_type

  /**
   * Must be device_automation. Only allowed and required in MQTT auto discovery device messages.
   * @see https://www.home-assistant.io/integrations/device_trigger.mqtt/#platform
   * @see https://www.home-assistant.io/integrations/mqtt/#device-discovery-payload
   */
  sensor["p"] = "device_automation"; // platform

  // The MQTT topic subscribed to receive trigger events.
  if (triggerTopic && triggerTopic[0]) {
    char state_topic[mqtt_topic_max_size];

    strcpy(state_topic, mqtt_topic);
    strcat(state_topic, gateway_name);
    strcat(state_topic, triggerTopic);

    /**
     * "info_topic" is not a standard field, for the message is required the filed "topic", but this filed is reserved and it is used to know where to publish the topic.
     * If we want to send on the message the topic information is usefull to use this "info_topic" that will be not delete by the send function but converted to "topic"
     */
    sensor["info_topic"] = state_topic;
  } else {
    THEENGS_LOG_ERROR(F("[RF] Error: topic is mandatory for device trigger Discovery" CR));
    return;
  }

  /**
   * The type of the trigger, e.g. button_short_press.
   * Entries supported by the HA frontend: button_short_press, button_short_release, button_long_press, button_long_release, button_double_press, button_triple_press, button_quadruple_press, button_quintuple_press.
   * If set to an unsupported value, will render as subtype type, e.g. button_1 spammed with type set to spammed and subtype set to button_1
   */
  if (type && type[0] != 0) {
    sensor["type"] = type;
  } else {
    sensor["type"] = "button_short_press";
  }

  /**
   * The subtype of the trigger, e.g. turn_on.
   * Entries supported by the frontend: turn_on, turn_off, button_1, button_2, button_3, button_4, button_5, button_6.
   * If set to an unsupported value, will render as subtype type, e.g. left_button pressed with type set to button_short_press and subtype set to left_button
   */
  if (subtype && subtype[0] != 0) {
    sensor["stype"] = subtype; // subtype
  } else {
    sensor["stype"] = "turn_on";
  }

  // ------------------   START DEVICE DECLARATION  --------------------------------------------------
  // TODO: This section, like the almost identical one in createDiscovery, should be placed in a
  //       separate function and managed specifically to avoid errors in representing the device
  //       in the HASS world.
  // -------------------------------------------------------------------------------------------------

  // Information about the device: this device trigger is a part of to tie it into the HA device registry.
  StaticJsonDocument<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject device = jsonDeviceBuffer.to<JsonObject>();

  // A link to the webpage that can manage the configuration of this device.
  if (ethConnected) {
#    ifdef ESP32_ETHERNET
    device["cu"] = String("http://") + String(ETH.localIP().toString()) + String("/"); // configuration_url
#    endif
  } else {
    device["cu"] = String("http://") + String(WiFi.localIP().toString()) + String("/"); // configuration_url
  }

  /*
  * A list of connections of the device to the outside world as a list of tuples [connection_type, connection_identifier].
  * For example the MAC address of a network interface: "connections": [["mac", "02:5b:26:a8:dc:12"]].
  */
  JsonArray connections = device.createNestedArray("cns"); // connections
  JsonArray connection_mac = connections.createNestedArray();
  connection_mac.add("mac");
  connection_mac.add(getMacAddress());

  // A list of IDs that uniquely identify the device. For example a serial number.
  String unique_id = String(getMacAddress());
  JsonArray identifiers = device.createNestedArray("ids"); // identifiers
  identifiers.add(unique_id);

  // The manufacturer of the device.
  device["mf"] = GATEWAY_MANUFACTURER;

  // The model of the device.
#    ifndef GATEWAY_MODEL
  String model = "";
  serializeJson(modules, model);
  device["mdl"] = model;
#    else
  device["mdl"] = GATEWAY_MODEL;
#    endif

  // The name of the device.
  device["name"] = String(gateway_name);
  device["sw"] = OMG_VERSION;
  // ------------------   END DEVICE DECLARATION  ------------------ //

  sensor["dev"] = device; //device representing the board

  if (value_template && value_template[0]) {
    sensor["val_tpl"] = String(value_template);
  }

  /* Publish on the topic
     The discovery topic needs to be: <discovery_prefix>/device_automation/[<node_id>/]<object_id>/config.

     Note that only one trigger may be defined per unique discovery topic.
     Also note that the combination of type and subtype should be unique for a device.

   */

  String topic_to_publish = String(discovery_prefix) + "/device_automation/" + String(unique_id) + "/" + object_id + "/config";
  THEENGS_LOG_TRACE(F("Announce Gatewy Trigger  %s" CR), topic_to_publish.c_str());
  sensor["topic"] = topic_to_publish;
  sensor["retain"] = true;
  enqueueJsonObject(sensor);
}
#  endif // ZgatewayRF

/*
  * Remove a substring p from a given string s
*/
std::string remove_substring(std::string s, const std::string& p) {
  std::string::size_type n = p.length();

  for (std::string::size_type i = s.find(p);
       i != std::string::npos;
       i = s.find(p))
    s.erase(i, n);

  return s;
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
 * @param state_off state off value
 * @param state_on state on value
 * @param enum_options options
 * @param command_template command template
 * @param diagnostic_entity true if entity_category is diagnostic
 *
 * */
void createDiscovery(const char* sensor_type,
                     const char* st_topic, const char* s_name, const char* unique_id,
                     const char* availability_topic, const char* device_class, const char* value_template,
                     const char* payload_on, const char* payload_off, const char* unit_of_meas,
                     int off_delay,
                     const char* payload_available, const char* payload_not_available, bool gateway_entity, const char* cmd_topic,
                     const char* device_name, const char* device_manufacturer, const char* device_model, const char* device_id, bool retainCmd,
                     const char* state_class, const char* state_off, const char* state_on, const char* enum_options,
                     const char* command_template, bool diagnostic_entity) {
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject sensor = jsonBuffer.to<JsonObject>();

  // If a component cannot render it's state (f.i. KAKU relays) no state topic
  // should be added. Without a state topic HA will use optimistic mode for the
  // component by default. The Home Assistant UI for optimistic switches
  // (separate on and off icons) allows for multiple
  // subsequent on commands. This is required for dimming on KAKU relays like
  // the ACM-300.
  if (st_topic && st_topic[0]) {
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
    if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "blind") == 0) {
      sensor["tilt_status_t"] = state_topic; // tilt_status_topic for blind
    } else if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "curtain") == 0) {
      sensor["pos_t"] = state_topic; // position_topic for curtain
    } else {
      sensor["stat_t"] = state_topic;
    }
  }

  if (availability_topic && availability_topic[0] && gateway_entity) {
    char avty_topic[mqtt_topic_max_size];
    strcpy(avty_topic, mqtt_topic);
    strcat(avty_topic, gateway_name);
    strcat(avty_topic, availability_topic);
    sensor["avty_t"] = avty_topic;
  }

  if (device_class && device_class[0]) {
    // We check if the class belongs to HAAS classes list
    int num_classes = sizeof(availableHASSClasses) / sizeof(availableHASSClasses[0]);
    for (int i = 0; i < num_classes; i++) { // see class list and size into config_mqttDiscovery.h
      if (strcmp(availableHASSClasses[i], device_class) == 0) {
        sensor["dev_cla"] = device_class; //device_class
      }
    }
  }

  if (unit_of_meas && unit_of_meas[0]) {
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
  if (value_template && value_template[0]) {
    if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "blind") == 0) {
      sensor["tilt_status_tpl"] = value_template; // tilt_status_template for blind
    } else if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "curtain") == 0) {
      sensor["pos_tpl"] = value_template; // position_template for curtain
    } else {
      sensor["val_tpl"] = value_template; //HA Auto discovery
    }
  }
  if (payload_on && payload_on[0]) {
    if (strcmp(sensor_type, HASS_TYPE_BUTTON) == 0) {
      sensor["pl_prs"] = payload_on; // payload_press for a button press
    } else if (strcmp(sensor_type, HASS_TYPE_NUMBER) == 0) {
      sensor["cmd_tpl"] = payload_on; // payload_on for a switch
    } else if (strcmp(sensor_type, HASS_TYPE_UPDATE) == 0) {
      sensor["pl_inst"] = payload_on; // payload_install for update
    } else if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "blind") == 0) {
      int value = std::stoi(payload_on);
      sensor["tilt_opnd_val"] = value; // tilt_open_value for blind
    } else if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "curtain") == 0) {
      int value = std::stoi(payload_on);
      sensor["pos_open"] = value; // open value for curtain
    } else {
      if (strcmp(payload_on, "True") == 0 || strcmp(payload_on, "true") == 0) {
        sensor["pl_on"] = true;
      } else {
        sensor["pl_on"] = payload_on; // payload_on for the rest
      }
    }
  }
  if (payload_off && payload_off[0]) {
    if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "blind") == 0) {
      sensor["pl_cls"] = payload_off; // payload_close for cover
    } else if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "curtain") == 0) {
      int value = std::stoi(payload_off);
      sensor["pos_clsd"] = value; // closed value for curtain
    } else {
      if (strcmp(payload_off, "False") == 0 || strcmp(payload_off, "false") == 0) {
        sensor["pl_off"] = false;
      } else {
        sensor["pl_off"] = payload_off; //payload_off for the rest
      }
    }
  }
  if (command_template && command_template[0]) {
    if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "blind") == 0) {
      sensor["tilt_cmd_tpl"] = command_template; //command_template
    } else if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "curtain") == 0) {
      sensor["set_pos_tpl"] = command_template; //command_template
    } else {
      sensor["cmd_tpl"] = command_template; //command_template
    }
  }
  if (strcmp(sensor_type, "device_tracker") == 0)
    sensor["src_type"] = "bluetooth_le"; // source_type - payload_install for update
  if (off_delay != 0)
    sensor["off_dly"] = off_delay; // off_delay
  if (payload_available[0])
    sensor["pl_avail"] = payload_available; // payload_on
  if (payload_not_available[0])
    sensor["pl_not_avail"] = payload_not_available; //payload_off
  if (state_class && state_class[0])
    sensor["stat_cla"] = state_class; //add the state class on the sensors ( https://developers.home-assistant.io/docs/core/entity/sensor/#available-state-classes )
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
    if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "blind") == 0) {
      sensor["tilt_cmd_t"] = command_topic; // tilt_command_topic for cover
    } else if (strcmp(sensor_type, HASS_TYPE_COVER) == 0 && strcmp(state_class, "curtain") == 0) {
      sensor["set_pos_t"] = command_topic; // position_command_topic for curtain
    } else {
      sensor["cmd_t"] = command_topic; //command_topic
    }
  }

  if (diagnostic_entity) {  // entity_category
    sensor["ent_cat"] = "diagnostic";
  }

  if (enum_options != nullptr) {
    sensor["ops"] = enum_options; // options
  }

  StaticJsonDocument<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject device = jsonDeviceBuffer.to<JsonObject>();
  JsonArray identifiers = device.createNestedArray("ids");

  if (gateway_entity) {
    //device representing the board
    device["name"] = String(gateway_name);
#  ifndef GATEWAY_MODEL
    String model = "";
    serializeJson(modules, model);
    device["mdl"] = model;
#  else
    device["mdl"] = GATEWAY_MODEL;
#  endif
    device["mf"] = GATEWAY_MANUFACTURER;
    if (ethConnected) {
#  ifdef ESP32_ETHERNET
      device["cu"] = String("http://") + String(ETH.localIP().toString()) + String("/"); //configuration_url
#  endif
    } else {
      device["cu"] = String("http://") + String(WiFi.localIP().toString()) + String("/"); //configuration_url
    }

    device["sw"] = OMG_VERSION;
    identifiers.add(String(getMacAddress()));
  } else {
    //The Connections
    if (device_id[0]) {
      JsonArray connections = device.createNestedArray("cns");
      JsonArray connection_mac = connections.createNestedArray();
      connection_mac.add("mac");
      connection_mac.add(device_id);
      //Device representing the actual sensor/switch device
      //The Device ID
      identifiers.add(device_id);
    }

    if (device_manufacturer[0]) {
      device["mf"] = device_manufacturer;
    }

    if (device_model[0]) {
      device["mdl"] = device_model;
    }

    // generate unique device name by adding the second half of the device_id only if device_name and device_id are different and we don't want to use the BLE name
    if (device_name[0]) {
      if (strcmp(device_id, device_name) != 0 && device_id[0] && !ForceDeviceName) {
        device["name"] = device_name + String("-") + String(device_id + 6);
      } else {
        device["name"] = device_name;
      }
    }

    device["via_device"] = String(getMacAddress()); //mac address of the gateway so that the devices link to the gateway
  }

  sensor["dev"] = device; // device

  String topic = String(discovery_prefix) + "/" + String(sensor_type) + "/" + String(unique_id) + "/config";
  THEENGS_LOG_TRACE(F("Announce Device %s on  %s" CR), String(sensor_type).c_str(), topic.c_str());
  sensor["topic"] = topic;
  sensor["retain"] = true;
  enqueueJsonObject(sensor);
}

void eraseTopic(const char* sensor_type, const char* unique_id) {
  if (sensor_type == NULL || unique_id == NULL) {
    return;
  }
  String topic = String(discovery_prefix) + "/" + String(sensor_type) + "/" + String(unique_id) + "/config";
  THEENGS_LOG_TRACE(F("Erase entity discovery %s on  %s" CR), String(sensor_type).c_str(), topic.c_str());
  pubMQTT((char*)topic.c_str(), "", true);
}

#  if defined(ZgatewayBT) || defined(SecondaryModule)
void btPresenceParametersDiscovery() {
  createDiscovery(HASS_TYPE_NUMBER, //set Type
                  subjectBTtoMQTT, "BT: Presence/Tracker timeout", (char*)getUniqueId("presenceawaytimer", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.presenceawaytimer/60000 }}", //set availability_topic,device_class,value_template,
                  "{\"presenceawaytimer\":{{value*60000}},\"save\":true}", "", HASS_UNIT_MIN, //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain,
                  stateClassNone //State Class
  );
}
void btScanParametersDiscovery() {
  const char* btScanParams[][13] = {
      {HASS_TYPE_NUMBER, "BT: Interval between scans", "interval", "", "{{ value_json.interval/1000 }}", "{\"interval\":{{value*1000}},\"save\":true}", "", HASS_UNIT_SECOND, stateClassNone, nullptr, nullptr, nullptr, subjectMQTTtoBTset},
      {HASS_TYPE_NUMBER, "BT: Interval between active scans", "intervalacts", "", "{{ value_json.intervalacts/1000 }}", "{\"intervalacts\":{{value*1000}},\"save\":true}", "", HASS_UNIT_SECOND, stateClassNone, nullptr, nullptr, nullptr, subjectMQTTtoBTset},
  };

  createDiscoveryFromList(nullptr, btScanParams, 2, nullptr, nullptr, nullptr,
                          true, subjectBTtoMQTT, will_Topic, nullptr);
}
#  endif

void pubMqttDiscovery() {
  THEENGS_LOG_TRACE(F("omgStatusDiscovery" CR));

  // System sensors and controls - using extended 13-column format with macros
  const char* systemEntities[][13] = {
      // Sensors
      {HASS_TYPE_BINARY_SENSOR, "SYS: Connectivity", "connectivity", HASS_CLASS_CONNECTIVITY, "", Gateway_AnnouncementMsg, will_Message, "", stateClassNone, nullptr, nullptr, will_Topic, nullptr},
      {HASS_TYPE_SENSOR, "SYS: Uptime", "uptime", HASS_CLASS_DURATION, "{{ value_json.uptime }}", "", "", HASS_UNIT_SECOND, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "SYS: Free memory", "freemem", HASS_CLASS_DATA_SIZE, "{{ value_json.freemem }}", "", "", HASS_UNIT_BYTE, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "SYS: IP", "ip", "", "{{ value_json.ip }}", "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr},
#  ifndef ESP32_ETHERNET
      {HASS_TYPE_SENSOR, "SYS: RSSI", "rssi", HASS_CLASS_SIGNAL_STRENGTH, "{{ value_json.rssi }}", "", "", HASS_UNIT_DB, stateClassNone, nullptr, nullptr, nullptr, nullptr},
#  endif
      // Switch with state_on/state_off
      {HASS_TYPE_SWITCH, "SYS: Auto discovery", "disc", "", "{{ value_json.disc }}", "{\"disc\":true,\"save\":true}", "{\"disc\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoSYSset},
      // Buttons
      {HASS_TYPE_BUTTON, "SYS: Restart gateway", "restart", HASS_CLASS_RESTART, "", "{\"cmd\":\"restart\"}", "", "", stateClassNone, nullptr, nullptr, will_Topic, subjectMQTTtoSYSset},
      {HASS_TYPE_BUTTON, "SYS: Erase credentials", "erase", "", "", "{\"cmd\":\"erase\"}", "", "", stateClassNone, nullptr, nullptr, will_Topic, subjectMQTTtoSYSset},
  };

  int entityCount = sizeof(systemEntities) / sizeof(systemEntities[0]);
  createDiscoveryFromList(nullptr, systemEntities, entityCount, nullptr, nullptr, nullptr,
                          true, subjectSYStoMQTT, will_Topic, nullptr);

#  ifdef SecondaryModule
  // Secondary module system sensors - dynamic string handling required
  String secondaryPrefix = String(SecondaryModule);
  String uptimeName = "SYS: Uptime " + secondaryPrefix;
  String uptimeId = "uptime-" + secondaryPrefix;
  String freememName = "SYS: Free memory " + secondaryPrefix;
  String freememId = "freemem-" + secondaryPrefix;
  String restartName = "SYS: Restart " + secondaryPrefix;
  String restartId = "restart-" + secondaryPrefix;

  const char* secondarySensors[][13] = {
      {HASS_TYPE_SENSOR, uptimeName.c_str(), uptimeId.c_str(), HASS_CLASS_DURATION, "{{ value_json.uptime }}", "", "", HASS_UNIT_SECOND, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, freememName.c_str(), freememId.c_str(), HASS_CLASS_DATA_SIZE, "{{ value_json.freemem }}", "", "", HASS_UNIT_BYTE, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_BUTTON, restartName.c_str(), restartId.c_str(), HASS_CLASS_RESTART, "", "{\"cmd\":\"restart\"}", "", "", stateClassNone, nullptr, nullptr, will_Topic, subjectMQTTtoSYSsetSecondaryModule},
  };

  createDiscoveryFromList(nullptr, secondarySensors, 3, nullptr, nullptr, nullptr,
                          true, subjectSYStoMQTTSecondaryModule, will_Topic, nullptr);
#  endif

#  ifdef LED_ADDRESSABLE
  createDiscovery(HASS_TYPE_NUMBER, //set Type
                  subjectSYStoMQTT, "SYS: LED Brightness", (char*)getUniqueId("rgbb", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ (value_json.rgbb/2.55) | round(0) }}", //set availability_topic,device_class,value_template,
                  "{\"rgbb\":{{ (value*2.55) | round(0) }},\"save\":true}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain,
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZdisplaySSD1306
#    include "config_SSD1306.h"
  const char* ssd1306Entities[][13] = {
      {HASS_TYPE_SWITCH, "SSD1306: Control", "onstate", "", "{{ value_json.onstate }}", "{\"onstate\":true,\"save\":true}", "{\"onstate\":false,\"save\":true}", "", stateClassNone, "false", "true", subjectSSD1306toMQTT, subjectMQTTtoSSD1306set},
      {HASS_TYPE_SWITCH, "SSD1306: Display metric", "displayMetric", "", "{{ value_json.displayMetric }}", "{\"displayMetric\":true,\"save\":true}", "{\"displayMetric\":false,\"save\":true}", "", stateClassNone, "false", "true", subjectWebUItoMQTT, subjectMQTTtoWebUIset},
      {HASS_TYPE_NUMBER, "SSD1306: Brightness", "brightness", "", "{{ value_json.brightness }}", "{\"brightness\":{{value}},\"save\":true}", "", "", stateClassNone, nullptr, nullptr, subjectSSD1306toMQTT, subjectMQTTtoSSD1306set},
  };

  createDiscoveryFromList(nullptr, ssd1306Entities, 3, nullptr, nullptr, nullptr,
                          true, subjectSSD1306toMQTT, will_Topic, nullptr);
#  endif

#  if defined(ESP32) && !defined(NO_INT_TEMP_READING)
  const char* esp32Sensors[][13] = {
      {HASS_TYPE_SENSOR, "SYS: Internal temperature", "tempc", HASS_CLASS_TEMPERATURE, "{{ value_json.tempc  | round(1)}}", "", "", HASS_UNIT_CELSIUS, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
#    if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP) || defined(ZboardM5TOUGH)
      {HASS_TYPE_SENSOR, "SYS: Bat voltage", "m5batvoltage", HASS_CLASS_VOLTAGE, "{{ value_json.m5batvoltage }}", "", "", HASS_UNIT_VOLT, stateClassNone, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "SYS: Bat current", "m5batcurrent", HASS_CLASS_CURRENT, "{{ value_json.m5batcurrent }}", "", "", HASS_UNIT_AMP, stateClassNone, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "SYS: Vin voltage", "m5vinvoltage", HASS_CLASS_VOLTAGE, "{{ value_json.m5vinvoltage }}", "", "", HASS_UNIT_VOLT, stateClassNone, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "SYS: Vin current", "m5vincurrent", HASS_CLASS_CURRENT, "{{ value_json.m5vincurrent }}", "", "", HASS_UNIT_AMP, stateClassNone, nullptr, nullptr, nullptr, nullptr},
#    endif
#    ifdef ZboardM5STACK
      {HASS_TYPE_SENSOR, "SYS: Batt level", "m5battlevel", HASS_CLASS_BATTERY, "{{ value_json.m5battlevel }}", "", "", HASS_UNIT_PERCENT, stateClassNone, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_BINARY_SENSOR, "SYS: Is Charging", "m5ischarging", "", "{{ value_json.m5ischarging }}", "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_BINARY_SENSOR, "SYS: Is Charge Full", "m5ischargefull", "", "{{ value_json.m5ischargefull }}", "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr},
#    endif
  };

  int esp32SensorCount = sizeof(esp32Sensors) / sizeof(esp32Sensors[0]);
  createDiscoveryFromList(nullptr, esp32Sensors, esp32SensorCount, nullptr, nullptr, nullptr,
                          true, subjectSYStoMQTT, will_Topic, nullptr);
#  endif

#  ifdef MQTT_HTTPS_FW_UPDATE
  createDiscovery(HASS_TYPE_UPDATE, //set Type
                  subjectRLStoMQTT, "SYS: Firmware Update", (char*)getUniqueId(HASS_TYPE_UPDATE, "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "firmware", "", //set availability_topic,device_class,value_template,
                  LATEST_OR_DEV, "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSupdate, //set,payload_available,payload_not available   ,is a gateway entity, command topic
                  "", "", "", "", false, // device name, device manufacturer, device model, device ID, retain
                  stateClassNone //State Class
  );
#  endif

#  ifdef ZsensorBME280
#    include "config_BME280.h"
  const char* BMEsensor[][13] = {
      {HASS_TYPE_SENSOR, "BME: Temp", "bme-temp", HASS_CLASS_TEMPERATURE, jsonTempc, "", "", HASS_UNIT_CELSIUS, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "BME: Pressure", "bme-pressure", HASS_CLASS_PRESSURE, jsonPa, "", "", HASS_UNIT_HPA, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "BME: Humidity", "bme-humidity", HASS_CLASS_HUMIDITY, jsonHum, "", "", HASS_UNIT_PERCENT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "BME: Altitude", "bme-altim", "", jsonAltim, "", "", HASS_UNIT_METER, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "BME: Altitude (ft)", "bme-altift", "", jsonAltif, "", "", HASS_UNIT_FT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("bme280Discovery" CR));
  createDiscoveryFromList(nullptr, BMEsensor, 5, nullptr, nullptr, nullptr,
                          true, BMETOPIC, will_Topic, nullptr);
#  endif

#  ifdef ZsensorHTU21
#    include "config_HTU21.h"
  const char* HTUsensor[][13] = {
      {HASS_TYPE_SENSOR, "HTU: Temperature", "htu-temp", HASS_CLASS_TEMPERATURE, jsonTempc, "", "", HASS_UNIT_CELSIUS, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "HTU: Humidity", "htu-hum", HASS_CLASS_HUMIDITY, jsonHum, "", "", HASS_UNIT_PERCENT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("htu21Discovery" CR));
  createDiscoveryFromList(nullptr, HTUsensor, 2, nullptr, nullptr, nullptr,
                          true, HTUTOPIC, will_Topic, nullptr);
#  endif

#  ifdef ZsensorLM75
  THEENGS_LOG_TRACE(F("LM75Discovery" CR));
  const char* LM75sensor[][13] = {
      {HASS_TYPE_SENSOR, "LM75: Temperature", "lm75-temp", HASS_CLASS_TEMPERATURE, jsonTempc, "", "", HASS_UNIT_CELSIUS, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, LM75sensor, 1, nullptr, nullptr, nullptr,
                          true, LM75TOPIC, will_Topic, nullptr);
#  endif

#  ifdef ZsensorAHTx0
#    include "config_AHTx0.h"
  const char* AHTsensor[][13] = {
      {HASS_TYPE_SENSOR, "AHT: Temperature", "aht-temp", HASS_CLASS_TEMPERATURE, jsonTempc, "", "", HASS_UNIT_CELSIUS, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "AHT: Humidity", "aht-hum", HASS_CLASS_HUMIDITY, jsonHum, "", "", HASS_UNIT_PERCENT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("AHTx0Discovery" CR));
  createDiscoveryFromList(nullptr, AHTsensor, 2, nullptr, nullptr, nullptr,
                          true, AHTTOPIC, will_Topic, nullptr);
#  endif

#  ifdef ZsensorDHT
#    include "config_DHT.h"
  const char* DHTsensor[][13] = {
      {HASS_TYPE_SENSOR, "DHT: Temperature", "dht-temp", HASS_CLASS_TEMPERATURE, jsonTempc, "", "", HASS_UNIT_CELSIUS, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "DHT: Humidity", "dht-hum", HASS_CLASS_HUMIDITY, jsonHum, "", "", HASS_UNIT_PERCENT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("DHTDiscovery" CR));
  createDiscoveryFromList(nullptr, DHTsensor, 2, nullptr, nullptr, nullptr,
                          true, DHTTOPIC, will_Topic, nullptr);
#  endif

#  ifdef ZsensorADC
#    include "config_ADC.h"
  THEENGS_LOG_TRACE(F("ADCDiscovery" CR));
  const char* ADCsensor[][13] = {
      {HASS_TYPE_SENSOR, "ADC", "adc", "", jsonAdc, "", "", "", stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, ADCsensor, 1, nullptr, nullptr, nullptr,
                          true, ADCTOPIC, will_Topic, nullptr);
#  endif

#  ifdef ZsensorBH1750
#    include "config_BH1750.h"
  const char* BH1750sensor[][13] = {
      {HASS_TYPE_SENSOR, "BH1750: Lux", "BH1750-lux", HASS_CLASS_ILLUMINANCE, jsonLux, "", "", HASS_UNIT_LX, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "BH1750: ftCd", "BH1750-ftcd", HASS_CLASS_IRRADIANCE, jsonFtcd, "", "", "", stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "BH1750: wattsm2", "BH1750-wm2", HASS_CLASS_IRRADIANCE, jsonWm2, "", "", HASS_UNIT_WM2, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("BH1750Discovery" CR));
  createDiscoveryFromList(nullptr, BH1750sensor, 3, nullptr, nullptr, nullptr,
                          true, subjectBH1750toMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZsensorMQ2
#    include "config_MQ2.h"
  const char* MQ2sensor[][13] = {
      {HASS_TYPE_SENSOR, "MQ2: gas", "MQ2-gas", HASS_CLASS_GAS, jsonVal, "", "", HASS_UNIT_PPM, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_BINARY_SENSOR, "MQ2", "", HASS_CLASS_GAS, jsonPresence, "true", "false", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("MQ2Discovery" CR));
  createDiscoveryFromList(nullptr, MQ2sensor, 2, nullptr, nullptr, nullptr,
                          true, subjectMQ2toMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZsensorTEMT6000
#    include "config_TEMT6000.h"
  const char* TEMT6000sensor[][13] = {
      {HASS_TYPE_SENSOR, "TEMT6000: Lux", "TEMT6000-lux", HASS_CLASS_ILLUMINANCE, jsonLux, "", "", HASS_UNIT_LX, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "TEMT6000: ftCd", "TEMT6000-ftcd", HASS_CLASS_IRRADIANCE, jsonFtcd, "", "", "", stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "TEMT6000: wattsm2", "TEMT6000-wm2", HASS_CLASS_IRRADIANCE, jsonWm2, "", "", HASS_UNIT_WM2, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("TEMT6000Discovery" CR));
  createDiscoveryFromList(nullptr, TEMT6000sensor, 3, nullptr, nullptr, nullptr,
                          true, subjectTEMT6000toMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZsensorTSL2561
#    include "config_TSL2561.h"
  const char* TSL2561sensor[][13] = {
      {HASS_TYPE_SENSOR, "TSL2561: Lux", "TSL2561-lux", HASS_CLASS_ILLUMINANCE, jsonLux, "", "", HASS_UNIT_LX, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "TSL2561: ftCd", "TSL2561-ftcd", HASS_CLASS_IRRADIANCE, jsonFtcd, "", "", "", stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "TSL2561: wattsm2", "TSL2561-wm2", HASS_CLASS_IRRADIANCE, jsonWm2, "", "", HASS_UNIT_WM2, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("TSL2561Discovery" CR));
  createDiscoveryFromList(nullptr, TSL2561sensor, 3, nullptr, nullptr, nullptr,
                          true, subjectTSL12561toMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZsensorHCSR501
#    include "config_HCSR501.h"
  THEENGS_LOG_TRACE(F("HCSR501Discovery" CR));
  const char* HCSR501sensor[][13] = {
      {HASS_TYPE_BINARY_SENSOR, "hcsr501", "", HASS_CLASS_MOTION, jsonPresence, "true", "false", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, HCSR501sensor, 1, nullptr, nullptr, nullptr,
                          true, subjectHCSR501toMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZsensorGPIOInput
#    include "config_GPIOInput.h"
  THEENGS_LOG_TRACE(F("GPIOInputDiscovery" CR));
  const char* GPIOInputsensor[][13] = {
      {HASS_TYPE_BINARY_SENSOR, "GPIOInput", "", "", jsonGpio, INPUT_GPIO_ON_VALUE, INPUT_GPIO_OFF_VALUE, "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, GPIOInputsensor, 1, nullptr, nullptr, nullptr,
                          true, subjectGPIOInputtoMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZsensorINA226
#    include "config_INA226.h"
  const char* INA226sensor[][13] = {
      {HASS_TYPE_SENSOR, "INA226: volt", "INA226-volt", HASS_CLASS_VOLTAGE, jsonVolt, "", "", HASS_UNIT_VOLT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "INA226: current", "INA226-current", HASS_CLASS_CURRENT, jsonCurrent, "", "", HASS_UNIT_AMP, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "INA226: power", "INA226-power", HASS_CLASS_POWER, jsonPower, "", "", HASS_UNIT_WATT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("INA226Discovery" CR));
  createDiscoveryFromList(nullptr, INA226sensor, 3, nullptr, nullptr, nullptr,
                          true, subjectINA226toMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZsensorDS1820
  extern void pubOneWire_HADiscovery();
  // Publish any DS1820 sensors found on the OneWire bus
  pubOneWire_HADiscovery();
#  endif

#  ifdef ZactuatorONOFF
#    include "config_ONOFF.h"
  THEENGS_LOG_TRACE(F("actuatorONOFFDiscovery" CR));
  const char* actuatorONOFF[][13] = {
      {HASS_TYPE_SWITCH, "actuatorONOFF", "actuatorONOFF", "", "{{ value_json.cmd }}", "{\"cmd\":1}", "{\"cmd\":0}", "", stateClassNone, "0", "1", nullptr, subjectMQTTtoONOFF}};

  createDiscoveryFromList(nullptr, actuatorONOFF, 1, nullptr, nullptr, nullptr,
                          true, subjectGTWONOFFtoMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZsensorRN8209
#    include "config_RN8209.h"
  const char* RN8209sensor[][13] = {
      {HASS_TYPE_SENSOR, "NRG: volt", "volt", HASS_CLASS_VOLTAGE, jsonVolt, "", "", HASS_UNIT_VOLT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "NRG: current", "current", HASS_CLASS_CURRENT, jsonCurrent, "", "", HASS_UNIT_AMP, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_SENSOR, "NRG: power", "power", HASS_CLASS_POWER, jsonPower, "", "", HASS_UNIT_WATT, stateClassMeasurement, nullptr, nullptr, nullptr, nullptr},
      {HASS_TYPE_BINARY_SENSOR, "NRG: inUse", "inUse", HASS_CLASS_POWER, jsonInuseRN8209, "on", "off", "", stateClassMeasurement, nullptr, nullptr, nullptr, nullptr}};

  THEENGS_LOG_TRACE(F("RN8209Discovery" CR));
  createDiscoveryFromList(nullptr, RN8209sensor, 4, nullptr, nullptr, nullptr,
                          true, subjectRN8209toMQTT, will_Topic, nullptr);
#  endif

// Gateway sensors for various modules
#  if defined(ZgatewayRF) && defined(RF_on_HAS_as_MQTTSensor)
  THEENGS_LOG_TRACE(F("gatewayRFDiscovery" CR));
  const char* gatewayRF[][13] = {
      {HASS_TYPE_SENSOR, "gatewayRF", "", "", jsonVal, "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, gatewayRF, 1, nullptr, nullptr, nullptr,
                          true,
#    if valueAsATopic
                          subjectRFtoMQTTvalueAsATopic,
#    else
                          subjectRFtoMQTT,
#    endif
                          will_Topic, nullptr);
#  endif

#  ifdef ZgatewayRF2
#    include "config_RF.h"
  THEENGS_LOG_TRACE(F("gatewayRF2Discovery" CR));
  const char* gatewayRF2[][13] = {
      {HASS_TYPE_SENSOR, "gatewayRF2", "", "", jsonAddress, "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, gatewayRF2, 1, nullptr, nullptr, nullptr,
                          true,
#    if valueAsATopic
                          subjectRF2toMQTTvalueAsATopic,
#    else
                          subjectRF2toMQTT,
#    endif
                          will_Topic, nullptr);
#  endif

#  ifdef ZgatewayRFM69
#    include "config_RFM69.h"
  THEENGS_LOG_TRACE(F("gatewayRFM69Discovery" CR));
  const char* gatewayRFM69[][13] = {
      {HASS_TYPE_SENSOR, "gatewayRFM69", "", "", jsonVal, "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, gatewayRFM69, 1, nullptr, nullptr, nullptr,
                          true, subjectRFM69toMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZgatewayLORA
#    include "config_LORA.h"
  THEENGS_LOG_TRACE(F("gatewayLORADiscovery" CR));
  const char* gatewayLORA[][13] = {
      {HASS_TYPE_SENSOR, "gatewayLORA", "", "", jsonMsg, "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, gatewayLORA, 1, nullptr, nullptr, nullptr,
                          true, subjectLORAtoMQTT, will_Topic, nullptr);

  const char* LORAswitches[][13] = {
      {HASS_TYPE_SWITCH, "LORA: CRC", "enablecrc", "", "{{ value_json.enablecrc }}", "{\"enablecrc\":true,\"save\":true}", "{\"enablecrc\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoLORAset},
      {HASS_TYPE_SWITCH, "LORA: Invert IQ", "invertiq", "", "{{ value_json.invertiq }}", "{\"invertiq\":true,\"save\":true}", "{\"invertiq\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoLORAset},
      {HASS_TYPE_SWITCH, "LORA: Only Known", "onlyknown", "", "{{ value_json.onlyknown }}", "{\"onlyknown\":true,\"save\":true}", "{\"onlyknown\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoLORAset},
  };

  createDiscoveryFromList(nullptr, LORAswitches, 3, nullptr, nullptr, nullptr,
                          true, subjectLORAtoMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZgatewaySRFB
#    include "config_SRFB.h"
  THEENGS_LOG_TRACE(F("gatewaySRFBDiscovery" CR));
  const char* gatewaySRFB[][13] = {
      {HASS_TYPE_SENSOR, "gatewaySRFB", "", "", jsonVal, "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, gatewaySRFB, 1, nullptr, nullptr, nullptr,
                          true, subjectSRFBtoMQTT, will_Topic, nullptr);
#  endif

#  ifdef ZgatewayPilight
#    include "config_RF.h"
  THEENGS_LOG_TRACE(F("gatewayPilightDiscovery" CR));
  const char* gatewayPilight[][13] = {
      {HASS_TYPE_SENSOR, "gatewayPilight", "", "", jsonMsg, "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, gatewayPilight, 1, nullptr, nullptr, nullptr,
                          true,
#    if valueAsATopic
                          subjectPilighttoMQTTvalueAsATopic,
#    else
                          subjectPilighttoMQTT,
#    endif
                          will_Topic, nullptr);
#  endif

#  ifdef ZgatewayIR
#    include "config_IR.h"
  THEENGS_LOG_TRACE(F("gatewayIRDiscovery" CR));
  const char* gatewayIR[][13] = {
      {HASS_TYPE_SENSOR, "gatewayIR", "", "", jsonVal, "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, gatewayIR, 1, nullptr, nullptr, nullptr,
                          true, subjectIRtoMQTT, will_Topic, nullptr);
#  endif

#  ifdef Zgateway2G
#    include "config_2G.h"
  THEENGS_LOG_TRACE(F("gateway2GDiscovery" CR));
  const char* gateway2G[][13] = {
      {HASS_TYPE_SENSOR, "gateway2G", "", "", jsonMsg, "", "", "", stateClassNone, nullptr, nullptr, nullptr, nullptr}};

  createDiscoveryFromList(nullptr, gateway2G, 1, nullptr, nullptr, nullptr,
                          true, subject2GtoMQTT, will_Topic, nullptr);
#  endif

#  if defined(ZgatewayBT) || defined(SecondaryModule)
#    ifdef ESP32

  // BT configuration entities - all in arrays now with macros
  const char* btConfigEntities[][13] = {
      // Numbers
      {HASS_TYPE_NUMBER, "BT: Connect interval", "intervalcnct", "", "{{ value_json.intervalcnct/60000 }}", "{\"intervalcnct\":{{value*60000}},\"save\":true}", "", HASS_UNIT_MIN, stateClassNone, nullptr, nullptr, nullptr, subjectMQTTtoBTset},
      {HASS_TYPE_NUMBER, "BT: Scan duration", "scanduration", "", "{{ value_json.scanduration/1000 }}", "{\"scanduration\":{{value*1000}},\"save\":true}", "", HASS_UNIT_SECOND, stateClassNone, nullptr, nullptr, nullptr, subjectMQTTtoBTset},
      // Buttons
      {HASS_TYPE_BUTTON, "BT: Force scan", "force_scan", "", "", "{\"interval\":0}", "", "", stateClassNone, nullptr, nullptr, will_Topic, subjectMQTTtoBTset},
      {HASS_TYPE_BUTTON, "BT: Erase config", "erase_bt_config", "", "", "{\"erase\":true}", "", "", stateClassNone, nullptr, nullptr, will_Topic, subjectMQTTtoBTset},
      // Switches with state_on/state_off
      {HASS_TYPE_SWITCH, "BT: Publish only sensors", "only_sensors", "", "{{ value_json.onlysensors }}", "{\"onlysensors\":true,\"save\":true}", "{\"onlysensors\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoBTset},
      {HASS_TYPE_SWITCH, "BT: Adaptive scan", "adaptive_scan", "", "{{ value_json.adaptivescan }}", "{\"adaptivescan\":true,\"save\":true}", "{\"adaptivescan\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoBTset},
      {HASS_TYPE_SWITCH, "BT: Enabled", "enabled", "", "{{ value_json.enabled }}", "{\"enabled\":true,\"save\":true}", "{\"enabled\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoBTset},
      {HASS_TYPE_SWITCH, "BT: Publish HASS presence", "hasspresence", "", "{{ value_json.hasspresence }}", "{\"hasspresence\":true,\"save\":true}", "{\"hasspresence\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoBTset},
      {HASS_TYPE_SWITCH, "BT: Publish Advertisement data", "pubadvdata", "", "{{ value_json.pubadvdata }}", "{\"pubadvdata\":true,\"save\":true}", "{\"pubadvdata\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoBTset},
      {HASS_TYPE_SWITCH, "BT: Connect to devices", "bleconnect", "", "{{ value_json.bleconnect }}", "{\"bleconnect\":true,\"save\":true}", "{\"bleconnect\":false,\"save\":true}", "", stateClassNone, "false", "true", nullptr, subjectMQTTtoBTset},
  };

  createDiscoveryFromList(nullptr, btConfigEntities, 10, nullptr, nullptr, nullptr,
                          true, subjectBTtoMQTT, will_Topic, nullptr);

#      define EntitiesCount 9
  const char* obsoleteEntities[EntitiesCount][2] = {
      // Remove previously created entities for version < 1.4.0
      {HASS_TYPE_SWITCH, "active_scan"}, // Replaced by adaptive scan
      // Remove previously created entities for version < 1.3.0
      {HASS_TYPE_NUMBER, "scanbcnct"}, // Now a connect interval
      // Remove previously created entities for version < 1.2.0
      {HASS_TYPE_SWITCH, "restart"}, // Now a button
      {HASS_TYPE_SWITCH, "erase"}, // Now a button
      {HASS_TYPE_SWITCH, "force_scan"}, // Now a button
      {HASS_TYPE_SENSOR, "interval"}, // Now a number
      {HASS_TYPE_SENSOR, "scanbcnct"}, // Now a number
      {HASS_TYPE_SWITCH, "ohdiscovery"}, // Now a new key
      {HASS_TYPE_SWITCH, "discovery"}}; // Now a new key

  for (int i = 0; i < EntitiesCount; i++) {
    eraseTopic(obsoleteEntities[i][0], (char*)getUniqueId(obsoleteEntities[i][1], "").c_str());
  }

  btScanParametersDiscovery();
  btPresenceParametersDiscovery();

#      if DEFAULT_LOW_POWER_MODE != DEACTIVATED
  createDiscovery(HASS_TYPE_SWITCH, //set Type
                  subjectSYStoMQTT, "SYS: Low Power Mode command", (char*)getUniqueId("powermode", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "{{ value_json.powermode | bool }}", //set availability_topic,device_class,value_template,
                  "{\"powermode\":1,\"save\":true}", "{\"powermode\":0,\"save\":true}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset, //set,payload_available,payload_not available,is a gateway entity, command topic
                  "", "", "", "", true, // device name, device manufacturer, device model, device MAC, retain
                  stateClassNone, //State Class
                  "false", "true" //state_off, state_on
  );
#      else
  // Remove previously created switch for version < 1.4.0
  eraseTopic(HASS_TYPE_SWITCH, (char*)getUniqueId("powermode", "").c_str());
#      endif
#    endif
#  endif
}
#else
void pubMqttDiscovery() {}
#endif