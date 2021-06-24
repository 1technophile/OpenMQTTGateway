/*
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

   This is the Home Assistant mqtt Discovery addon.

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
  String uniqueId = (String)getMacAddress() + name + sufix;
  return String(uniqueId);
}

#  ifdef ZgatewayBT
void createDiscoveryFromList(const char* mac, const char* sensorList[][8], int sensorCount,
                             const char* device_name, const char* device_manufacturer, const char* device_model) {
  for (int i = 0; i < sensorCount; i++) {
    Log.trace(F("CreateDiscoverySensor %s" CR), sensorList[i][1]);
    String discovery_topic = String(subjectBTtoMQTT) + "/" + String(mac);
    String unique_id = String(mac) + "-" + sensorList[i][1];
    createDiscovery(sensorList[i][0],
                    discovery_topic.c_str(), sensorList[i][1], unique_id.c_str(),
                    will_Topic, sensorList[i][3], sensorList[i][4],
                    sensorList[i][5], sensorList[i][6], sensorList[i][7],
                    0, "", "", false, "",
                    device_name, device_manufacturer, device_model, mac, false);
  }
}
#  endif

void createDiscovery(const char* sensor_type,
                     const char* st_topic, const char* s_name, const char* unique_id,
                     const char* availability_topic, const char* device_class, const char* value_template,
                     const char* payload_on, const char* payload_off, const char* unit_of_meas,
                     int off_delay,
                     const char* payload_available, const char* payload_not_avalaible, bool gateway_entity, const char* cmd_topic,
                     const char* device_name, const char* device_manufacturer, const char* device_model, const char* device_mac, bool retainCmd) {
  const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(14) + JSON_OBJECT_SIZE(5) + JSON_ARRAY_SIZE(1);
  StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
  JsonObject& sensor = jsonBuffer.createObject();

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
    sensor.set("stat_t", state_topic);
  }

  sensor.set("name", s_name); //name
  sensor.set("uniq_id", unique_id); //unique_id
  if (retainCmd)
    sensor.set("retain", retainCmd); // Retain command
  if (device_class[0])
    sensor.set("dev_cla", device_class); //device_class
  if (value_template[0])
    sensor.set("val_tpl", value_template); //value_template
  if (payload_on[0])
    sensor.set("pl_on", payload_on); // payload_on
  if (payload_off[0])
    sensor.set("pl_off", payload_off); //payload_off
  if (unit_of_meas[0])
    sensor.set("unit_of_meas", unit_of_meas); //unit_of_measurement*/
  if (off_delay != 0)
    sensor.set("off_delay", off_delay); //off_delay
  if (payload_available[0])
    sensor.set("pl_avail", payload_available); // payload_on
  if (payload_not_avalaible[0])
    sensor.set("pl_not_avail", payload_not_avalaible); //payload_off

  if (cmd_topic[0]) {
    char command_topic[mqtt_topic_max_size];
    strcpy(command_topic, mqtt_topic);
    strcat(command_topic, gateway_name);
    strcat(command_topic, cmd_topic);
    sensor.set("cmd_t", command_topic); //command_topic
  }

  if (gateway_entity) {
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonDeviceBuffer;
    JsonObject& device = jsonDeviceBuffer.createObject();
    char JSONmessageBuffer[JSON_MSG_BUFFER];
    modules.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Log.notice(F("Received json : %s" CR), JSONmessageBuffer);
    device.set("name", gateway_name);
    device.set("model", JSONmessageBuffer);
    device.set("manufacturer", DEVICEMANUFACTURER);
    device.set("sw_version", OMG_VERSION);
    JsonArray& identifiers = device.createNestedArray("identifiers");
    identifiers.add(getMacAddress());
    sensor.set("device", device); //device representing the board
  } else {
    char deviceid[13];
    memcpy(deviceid, &unique_id[0], 12);
    deviceid[12] = '\0';
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonDeviceBuffer;
    JsonObject& device = jsonDeviceBuffer.createObject();
    if (device_mac[0] != 0) {
      JsonArray& connections = device.createNestedArray("connections");
      JsonArray& connection_mac = connections.createNestedArray();
      connection_mac.add("mac");
      connection_mac.add(device_mac);
    }
    JsonArray& identifiers = device.createNestedArray("identifiers");
    identifiers.add(deviceid);
    if (device_manufacturer[0]) {
      device.set("manufacturer", device_manufacturer);
    }
    if (device_model[0]) {
      device.set("model", device_model);
    }
    if (device_name[0]) {
      device.set("name", device_name);
    }
    device.set("via_device", gateway_name); //device name of the board
    sensor.set("device", device); //device representing the actual sensor/switch device
  }
  String topic = String(discovery_Topic) + "/" + String(sensor_type) + "/" + String(unique_id) + "/config";
  pub_custom_topic((char*)topic.c_str(), sensor, true);
}

void pubMqttDiscovery() {
  Log.trace(F("omgStatusDiscovery" CR));
  createDiscovery("binary_sensor", //set Type
                  will_Topic, "SYS: Connectivity", (char*)getUniqueId("connectivity", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "connectivity", "", //set availability_topic,device_class,value_template,
                  Gateway_AnnouncementMsg, will_Message, "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Uptime", (char*)getUniqueId("uptime", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.uptime }}", //set availability_topic,device_class,value_template,
                  "", "", "s", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );

#  if defined(ESP8266) || defined(ESP32)
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Free memory", (char*)getUniqueId("freemem", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.freemem }}", //set availability_topic,device_class,value_template,
                  "", "", "B", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: IP", (char*)getUniqueId("ip", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.ip }}", //set availability_topic,device_class,value_template,
                  "", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#    ifndef ESP32_ETHERNET
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Rssi", (char*)getUniqueId("rssi", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.rssi }}", //set availability_topic,device_class,value_template,
                  "", "", "dB", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#    endif
#  endif
#  ifdef ESP32
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Low Power Mode", (char*)getUniqueId("lowpowermode", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.lowpowermode }}", //set availability_topic,device_class,value_template,
                  "", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac
  );
#    if defined(ZboardM5STICKC) || defined(ZboardM5STICKCP)
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Bat voltage", (char*)getUniqueId("m5batvoltage", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.m5batvoltage }}", //set availability_topic,device_class,value_template,
                  "", "", "V", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a child device, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Bat current", (char*)getUniqueId("m5batcurrent", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.m5batcurrent }}", //set availability_topic,device_class,value_template,
                  "", "", "A", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a child device, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Vin voltage", (char*)getUniqueId("m5vinvoltage", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.m5vinvoltage }}", //set availability_topic,device_class,value_template,
                  "", "", "V", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a child device, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Vin current", (char*)getUniqueId("m5vincurrent", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.m5vincurrent }}", //set availability_topic,device_class,value_template,
                  "", "", "A", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a child device, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#    endif
#    ifdef ZboardM5STACK
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "SYS: Batt level", (char*)getUniqueId("m5battlevel", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.m5battlevel }}", //set availability_topic,device_class,value_template,
                  "", "", "%", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a child device, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("binary_sensor", //set Type
                  subjectSYStoMQTT, "SYS: Is Charging", (char*)getUniqueId("m5ischarging", "").c_str(), //set state_topic,name,uniqueId
                  "", "{{ value_json.m5ischarging }}", "", //set availability_topic,device_class,value_template,
                  "", "", "%", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a child device, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("binary_sensor", //set Type
                  subjectSYStoMQTT, "SYS: Is Charge Full", (char*)getUniqueId("m5ischargefull", "").c_str(), //set state_topic,name,uniqueId
                  "", "{{ value_json.m5ischargefull }}", "", //set availability_topic,device_class,value_template,
                  "", "", "%", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a child device, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#    endif
#  endif
  createDiscovery("switch", //set Type
                  will_Topic, "SYS: Restart gateway", (char*)getUniqueId("restart", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "", //set availability_topic,device_class,value_template,
                  "{\"cmd\":\"restart\"}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset, //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("switch", //set Type
                  will_Topic, "SYS: Erase credentials", (char*)getUniqueId("erase", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "", //set availability_topic,device_class,value_template,
                  "{\"cmd\":\"erase\"}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset, //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("switch", //set Type
                  "", "SYS: Auto discovery", (char*)getUniqueId("discovery", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "", //set availability_topic,device_class,value_template,
                  "{\"discovery\":true}", "{\"discovery\":false}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoSYSset, //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", true // device name, device manufacturer, device model, device mac, retain
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
    Log.trace(F("CreateDiscoverySensor" CR));
    //trc(BMEsensor[i][1]);
    createDiscovery(BMEsensor[i][0],
                    BMETOPIC, BMEsensor[i][1], (char*)getUniqueId(BMEsensor[i][1], BMEsensor[i][2]).c_str(),
                    will_Topic, BMEsensor[i][3], BMEsensor[i][4],
                    BMEsensor[i][5], BMEsensor[i][6], BMEsensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
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
    Log.trace(F("CreateDiscoverySensor" CR));
    //trc(HTUsensor[i][1]);
    createDiscovery(HTUsensor[i][0],
                    HTUTOPIC, HTUsensor[i][1], (char*)getUniqueId(HTUsensor[i][1], HTUsensor[i][2]).c_str(),
                    will_Topic, HTUsensor[i][3], HTUsensor[i][4],
                    HTUsensor[i][5], HTUsensor[i][6], HTUsensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
    );
  }
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
    Log.trace(F("CreateDiscoverySensor" CR));
    createDiscovery(AHTsensor[i][0],
                    AHTTOPIC, AHTsensor[i][1], (char*)getUniqueId(AHTsensor[i][1], AHTsensor[i][2]).c_str(),
                    will_Topic, AHTsensor[i][3], AHTsensor[i][4],
                    AHTsensor[i][5], AHTsensor[i][6], AHTsensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
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
    Log.trace(F("CreateDiscoverySensor" CR));
    //trc(DHTsensor[i][1]);
    createDiscovery(DHTsensor[i][0],
                    DHTTOPIC, DHTsensor[i][1], (char*)getUniqueId(DHTsensor[i][1], DHTsensor[i][2]).c_str(),
                    will_Topic, DHTsensor[i][3], DHTsensor[i][4],
                    DHTsensor[i][5], DHTsensor[i][6], DHTsensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
    );
  }
#  endif

#  ifdef ZsensorADC
  Log.trace(F("ADCDiscovery" CR));
  char* ADCsensor[8] = {"sensor", "adc", "", "", jsonAdc, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(ADCsensor[1]);
  createDiscovery(ADCsensor[0],
                  ADCTOPIC, ADCsensor[1], (char*)getUniqueId(ADCsensor[1], ADCsensor[2]).c_str(),
                  will_Topic, ADCsensor[3], ADCsensor[4],
                  ADCsensor[5], ADCsensor[6], ADCsensor[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
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
    Log.trace(F("CreateDiscoverySensor" CR));
    //trc(BH1750sensor[i][1]);
    createDiscovery(BH1750sensor[i][0],
                    subjectBH1750toMQTT, BH1750sensor[i][1], (char*)getUniqueId(BH1750sensor[i][1], BH1750sensor[i][2]).c_str(),
                    will_Topic, BH1750sensor[i][3], BH1750sensor[i][4],
                    BH1750sensor[i][5], BH1750sensor[i][6], BH1750sensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
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
    Log.trace(F("CreateDiscoverySensor" CR));
    //trc(TSL2561sensor[i][1]);
    createDiscovery(TSL2561sensor[i][0],
                    subjectTSL12561toMQTT, TSL2561sensor[i][1], (char*)getUniqueId(TSL2561sensor[i][1], TSL2561sensor[i][2]).c_str(),
                    will_Topic, TSL2561sensor[i][3], TSL2561sensor[i][4],
                    TSL2561sensor[i][5], TSL2561sensor[i][6], TSL2561sensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
    );
  }
#  endif

#  ifdef ZsensorHCSR501
  Log.trace(F("HCSR501Discovery" CR));
  char* HCSR501sensor[8] = {"binary_sensor", "hcsr501", "", "", jsonPresence, "true", "false", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(HCSR501sensor[1]);
  createDiscovery(HCSR501sensor[0],
                  subjectHCSR501toMQTT, HCSR501sensor[1], (char*)getUniqueId(HCSR501sensor[1], HCSR501sensor[2]).c_str(),
                  will_Topic, HCSR501sensor[3], HCSR501sensor[4],
                  HCSR501sensor[5], HCSR501sensor[6], HCSR501sensor[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZsensorGPIOInput
  Log.trace(F("GPIOInputDiscovery" CR));
  char* GPIOInputsensor[8] = {"binary_sensor", "GPIOInput", "", "", jsonGpio, INPUT_GPIO_ON_VALUE, INPUT_GPIO_OFF_VALUE, ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(GPIOInputsensor[1]);
  createDiscovery(GPIOInputsensor[0],
                  subjectGPIOInputtoMQTT, GPIOInputsensor[1], (char*)getUniqueId(GPIOInputsensor[1], GPIOInputsensor[2]).c_str(),
                  will_Topic, GPIOInputsensor[3], GPIOInputsensor[4],
                  GPIOInputsensor[5], GPIOInputsensor[6], GPIOInputsensor[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
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
    Log.trace(F("CreateDiscoverySensor" CR));
    //trc(INA226sensor[i][1]);
    createDiscovery(INA226sensor[i][0],
                    subjectINA226toMQTT, INA226sensor[i][1], (char*)getUniqueId(INA226sensor[i][1], INA226sensor[i][2]).c_str(),
                    will_Topic, INA226sensor[i][3], INA226sensor[i][4],
                    INA226sensor[i][5], INA226sensor[i][6], INA226sensor[i][7],
                    0, "", "", true, "",
                    "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
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

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(actuatorONOFF[1]);
  createDiscovery(actuatorONOFF[0],
                  subjectGTWONOFFtoMQTT, actuatorONOFF[1], (char*)getUniqueId(actuatorONOFF[1], actuatorONOFF[2]).c_str(),
                  will_Topic, actuatorONOFF[3], actuatorONOFF[4],
                  actuatorONOFF[5], actuatorONOFF[6], actuatorONOFF[7],
                  0, "", "", true, subjectMQTTtoONOFF,
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZgatewayRF
  // Sensor to display RF received value
  Log.trace(F("gatewayRFDiscovery" CR));
  char* gatewayRF[8] = {"sensor", "gatewayRF", "", "", jsonVal, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(gatewayRF[1]);
  createDiscovery(gatewayRF[0],
                  subjectRFtoMQTT, gatewayRF[1], (char*)getUniqueId(gatewayRF[1], gatewayRF[2]).c_str(),
                  will_Topic, gatewayRF[3], gatewayRF[4],
                  gatewayRF[5], gatewayRF[6], gatewayRF[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZgatewayRF2
  // Sensor to display RF received value
  Log.trace(F("gatewayRF2Discovery" CR));
  char* gatewayRF2[8] = {"sensor", "gatewayRF2", "", "", jsonAddress, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(gatewayRF2[1]);
  createDiscovery(gatewayRF2[0],
                  subjectRF2toMQTT, gatewayRF2[1], (char*)getUniqueId(gatewayRF2[1], gatewayRF2[2]).c_str(),
                  will_Topic, gatewayRF2[3], gatewayRF2[4],
                  gatewayRF2[5], gatewayRF2[6], gatewayRF2[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZgatewayRFM69
  // Sensor to display RF received value
  Log.trace(F("gatewayRFM69Discovery" CR));
  char* gatewayRFM69[8] = {"sensor", "gatewayRFM69", "", "", jsonVal, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(gatewayRFM69[1]);
  createDiscovery(gatewayRFM69[0],
                  subjectRFM69toMQTT, gatewayRFM69[1], (char*)getUniqueId(gatewayRFM69[1], gatewayRFM69[2]).c_str(),
                  will_Topic, gatewayRFM69[3], gatewayRFM69[4],
                  gatewayRFM69[5], gatewayRFM69[6], gatewayRFM69[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZgatewayLORA
  // Sensor to display RF received value
  Log.trace(F("gatewayLORADiscovery" CR));
  char* gatewayLORA[8] = {"sensor", "gatewayLORA", "", "", jsonMsg, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(gatewayLORA[1]);
  createDiscovery(gatewayLORA[0],
                  subjectLORAtoMQTT, gatewayLORA[1], (char*)getUniqueId(gatewayLORA[1], gatewayLORA[2]).c_str(),
                  will_Topic, gatewayLORA[3], gatewayLORA[4],
                  gatewayLORA[5], gatewayLORA[6], gatewayLORA[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZgatewaySRFB
  // Sensor to display RF received value
  Log.trace(F("gatewaySRFBDiscovery" CR));
  char* gatewaySRFB[8] = {"sensor", "gatewaySRFB", "", "", jsonVal, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(gatewaySRFB[1]);
  createDiscovery(gatewaySRFB[0],
                  subjectSRFBtoMQTT, gatewaySRFB[1], (char*)getUniqueId(gatewaySRFB[1], gatewaySRFB[2]).c_str(),
                  will_Topic, gatewaySRFB[3], gatewaySRFB[4],
                  gatewaySRFB[5], gatewaySRFB[6], gatewaySRFB[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZgatewayPilight
  // Sensor to display RF received value
  Log.trace(F("gatewayPilightDiscovery" CR));
  char* gatewayPilight[8] = {"sensor", "gatewayPilight", "", "", jsonMsg, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(gatewayPilight[1]);
  createDiscovery(gatewayPilight[0],
                  subjectPilighttoMQTT, gatewayPilight[1], (char*)getUniqueId(gatewayPilight[1], gatewayPilight[2]).c_str(),
                  will_Topic, gatewayPilight[3], gatewayPilight[4],
                  gatewayPilight[5], gatewayPilight[6], gatewayPilight[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZgatewayIR
  // Sensor to display IR received value
  Log.trace(F("gatewayIRDiscovery" CR));
  char* gatewayIR[8] = {"sensor", "gatewayIR", "", "", jsonVal, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(gatewayIR[1]);
  createDiscovery(gatewayIR[0],
                  subjectIRtoMQTT, gatewayIR[1], (char*)getUniqueId(gatewayIR[1], gatewayIR[2]).c_str(),
                  will_Topic, gatewayIR[3], gatewayIR[4],
                  gatewayIR[5], gatewayIR[6], gatewayIR[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef Zgateway2G
  // Sensor to display 2G received value
  Log.trace(F("gateway2GDiscovery" CR));
  char* gateway2G[8] = {"sensor", "gateway2G", "", "", jsonMsg, "", "", ""};
  //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

  Log.trace(F("CreateDiscoverySensor" CR));
  //trc(gateway2G[1]);
  createDiscovery(gateway2G[0],
                  subject2GtoMQTT, gateway2G[1], (char*)getUniqueId(gateway2G[1], gateway2G[2]).c_str(),
                  will_Topic, gateway2G[3], gateway2G[4],
                  gateway2G[5], gateway2G[6], gateway2G[7],
                  0, "", "", true, "",
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
#  endif

#  ifdef ZgatewayBT
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "BT: Interval between scans", (char*)getUniqueId("interval", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.interval }}", //set availability_topic,device_class,value_template,
                  "", "", "ms", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("sensor", //set Type
                  subjectSYStoMQTT, "BT: Connnect every X scan(s)", (char*)getUniqueId("scanbcnct", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "{{ value_json.scanbcnct }}", //set availability_topic,device_class,value_template,
                  "", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  "", "", true, "", //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("switch", //set Type
                  will_Topic, "BT: Force scan", (char*)getUniqueId("force_scan", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "", "", //set availability_topic,device_class,value_template,
                  "{\"interval\":0}", "", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", false // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("switch", //set Type
                  "", "BT: Publish only sensors", (char*)getUniqueId("only_sensors", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "", //set availability_topic,device_class,value_template,
                  "{\"onlysensors\":true}", "{\"onlysensors\":false}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", true // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("switch", //set Type
                  "", "BT: Publish HASS presence", (char*)getUniqueId("hasspresence", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "", //set availability_topic,device_class,value_template,
                  "{\"hasspresence\":true}", "{\"hasspresence\":false}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", true // device name, device manufacturer, device model, device mac, retain
  );
#    ifdef ESP32
  createDiscovery("switch", //set Type
                  "", "SYS: Low Power Mode command", (char*)getUniqueId("lowpowermode", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "", //set availability_topic,device_class,value_template,
                  "{\"lowpowermode\":2}", "{\"lowpowermode\":0}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set off_delay
                  "", "", true, subjectMQTTtoBTset, //set,payload_avalaible,payload_not avalaible,is a gateway entity, command topic
                  "", "", "", "", true // device name, device manufacturer, device model, device mac, retain
  );
  createDiscovery("switch", //set Type
                  "", "BT: Connect to devices", (char*)getUniqueId("bleconnect", "").c_str(), //set state_topic,name,uniqueId
                  "", "", "", //set availability_topic,device_class,value_template,
                  "{\"bleconnect\":true}", "{\"bleconnect\":false}", "", //set,payload_on,payload_off,unit_of_meas,
                  0, //set  off_delay
                  Gateway_AnnouncementMsg, will_Message, true, subjectMQTTtoBTset, //set,payload_avalaible,payload_not avalaible   ,is a gateway entity, command topic
                  "", "", "", "", true // device name, device manufacturer, device model, device mac, retain
  );
#    endif
#  endif
}

#endif
