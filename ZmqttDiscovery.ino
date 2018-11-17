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

#ifdef ZmqttDiscovery

void pubMqttDiscovery() {

#ifdef omgStatus
  trc(F("omgStatusDiscovery"));
  createBinarySensorDiscovery(will_Topic, "", DEVICENAME, getUniqueId("omg", "status"), "connectivity", "", Gateway_AnnouncementMsg, will_Message, false, false, 0);
#endif
#ifdef ZsensorBME280
#ifdef discBme280
  trc(F("bme280Discovery"));
#ifdef bmeTempC
  createSensorDiscovery(BME, will_Topic, "tempc", getUniqueId("bme", "tempc"), "°C", "temperature", "{{ value_json.tempc }}");
#endif
#ifdef bmeTempF
  createSensorDiscovery(BME, will_Topic, "tempf", getUniqueId("bme", "tempf"), "°F", "temperature", "{{ value_json.tempf }}");
#endif
#ifdef bmePreMbar
  createSensorDiscovery(BME, will_Topic, "pa", getUniqueId("bme", "mbar"), "hPa", "pressure", "{{ float(value_json.pa) * 0.01 }}");
#endif
#ifdef bmeHum
  createSensorDiscovery(BME, will_Topic, "hum", getUniqueId("bme", "hum"), "%", "humidity", "{{ value_json.hum }}");
#endif
#ifdef bmeAltM
  createSensorDiscovery(BME, will_Topic, "altim", getUniqueId("bme", "altim"), "m", "", "{{ value_json.altim }}");
#endif
#ifdef bmeAltFt
  createSensorDiscovery(BME, will_Topic, "altift", getUniqueId("bme", "altift"), "ft", "", "{{ value_json.altift }}");
#endif
#endif
#endif
}

void pubDiscovery(String topicori, JsonObject& data) {
  char JSONmessageBuffer[JSON_MSG_BUFFER];
  trc(F("Pub json into:"));
  trc(topicori);
  data.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  trc(JSONmessageBuffer);
  client.publish((char *)topicori.c_str(), JSONmessageBuffer, true);
}

void createBinarySensorDiscovery(String state_topic, String availability_topic, String name, String unique_id, String device_class, String value_template, String payload_on, String payload_off, bool optimistic, bool retain, int off_delay) {
  char JSONmessageBuffer[JSON_MSG_BUFFER];
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject& device = jsonDeviceBuffer.createObject();
  device.set("name", DEVICENAME);
  device.set("manufacturer", DEVICEMANUFACTURER);
  device.set("sw_version", OMG_VERSION);
  JsonArray& identifiers = device.createNestedArray("identifiers");
  identifiers.add(getMacAddress());

  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer3;
  JsonObject& sensor = jsonBuffer3.createObject();
  sensor.set("stat_t", state_topic); //state_topic
  if (availability_topic != "") {
    sensor.set("avty_t", availability_topic); //availability_topic
  }
  sensor.set("name", name);
  sensor.set("uniq_id", unique_id); //unique_id
  if (device_class != "") {
    sensor.set("dev_cla", device_class); //device_class
  }
  if (value_template != "") {
    sensor.set("val_tpl", value_template); //value_template
  }
  sensor.set("pl_on", payload_on); // payload_on
  sensor.set("pl_off", payload_off); //payload_off
  sensor.set("opt", optimistic); //optimistic
  sensor.set("ret", retain); //retain
  if (off_delay > 0) {
    sensor.set("off_delay", off_delay); //off_delay
  }
  sensor.set("device", device);
  String topic = (String)discovery_Topic + "/binary_sensor/" + unique_id + "/config";
  pubDiscovery(topic, sensor);
}

void createSensorDiscovery(String state_topic, String availability_topic, String name, String unique_id, String unit_of_measurement, String device_class, String value_template) {
  char JSONmessageBuffer[JSON_MSG_BUFFER];
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject& device = jsonDeviceBuffer.createObject();
  device.set("name", DEVICENAME);
  device.set("manufacturer", DEVICEMANUFACTURER);
  device.set("sw_version", OMG_VERSION);
  JsonArray& identifiers = device.createNestedArray("identifiers");
  identifiers.add(getMacAddress());

  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer3;
  JsonObject& sensor = jsonBuffer3.createObject();
  sensor.set("stat_t", state_topic); //state_topic
  sensor.set("avty_t", availability_topic); //availability_topic
  sensor.set("name", name);
  sensor.set("uniq_id", unique_id); //unique_id
  sensor.set("unit_of_meas", unit_of_measurement); //unit_of_measurement
  if (device_class != "") {
    sensor.set("dev_cla", device_class); //device_class
  }
  sensor.set("val_tpl", value_template); //value_template
  sensor.set("device", device);
  String topic = (String)discovery_Topic + "/sensor/" + unique_id + "/config";
  pubDiscovery(topic, sensor);
}


String getMacAddress() {
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[13] = {0};
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  return String(baseMacChr);
}

String getUniqueId(String name, String sufix)
{
  #ifdef ESP8266
  String uniqueId = WiFi.getMacAddress() + name + sufix;
  #else
  String uniqueId = (String)getMacAddress() + name + sufix;
  #endif
  return String(uniqueId);
}

#endif
