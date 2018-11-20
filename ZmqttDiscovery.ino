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

void pubMqttDiscovery()
{
#ifdef omgStatus
  trc(F("omgStatusDiscovery"));
  createDiscovery("binary_sensor",                                      //set sensorType
                  will_Topic, DEVICENAME, getUniqueId("omg", "status"), //set state_topic,name,uniqueId
                  "", "connectivity", "",                               //set availability_topic,device_class,value_template,
                  Gateway_AnnouncementMsg, will_Message, "",            //set,payload_on,payload_off,unit_of_meas,
                  true, false, 0);                                      //set optimistic,retain, off_delay
#endif
#ifdef discBme280
  trc(F("bme280Discovery"));
  createDiscovery("sensor",
                  BME, "tempc", getUniqueId("bme", "tempc"),
                  will_Topic, "temperature", "{{ value_json.tempc }}",
                  "", "", "°C",
                  true, false, 0);
  createDiscovery("sensor",
                  BME, "tempf", getUniqueId("bme", "tempf"),
                  will_Topic, "temperature", "{{ value_json.tempf }}",
                  "", "", "°F",
                  true, false, 0);
  createDiscovery("sensor",
                  BME, "pa", getUniqueId("bme", "pa"),
                  will_Topic, "pressure", "{{ float(value_json.pa) * 0.01 }}",
                  "", "", "hPa",
                  true, false, 0);
  createDiscovery("sensor",
                  BME, "hum", getUniqueId("bme", "hum"),
                  will_Topic, "humidity", "{{ value_json.hum }}",
                  "", "", "%",
                  true, false, 0);
  createDiscovery("sensor",
                  BME, "altim", getUniqueId("bme", "altim"),
                  will_Topic, "", "{{ value_json.altim }}",
                  "", "", "m",
                  true, false, 0);
  createDiscovery("sensor",
                  BME, "altift", getUniqueId("bme", "altift"),
                  will_Topic, "", "{{ value_json.altift }}",
                  "", "", "ft",
                  true, false, 0);
#endif
}

void createDiscovery(String sensor_type,
                     String state_topic, String name, String unique_id,
                     String availability_topic, String device_class, String value_template,
                     String payload_on, String payload_off, String unit_of_meas,
                     bool optimistic, bool retain, int off_delay)
{
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer3;
  JsonObject &sensor = jsonBuffer3.createObject();
  sensor.set("stat_t", state_topic); //state_topic
  sensor.set("name", name);          //name
  sensor.set("uniq_id", unique_id);  //unique_id
  if (availability_topic != "")
  {
    sensor.set("avty_t", availability_topic); //availability_topic
  }
  if (device_class != "")
  {
    sensor.set("dev_cla", device_class); //device_class
  }
  if (value_template != "")
  {
    sensor.set("val_tpl", value_template); //value_template
  }
  if (payload_on != "")
  {
    sensor.set("pl_on", payload_on); // payload_on
  }
  if (payload_off != "")
  {
    sensor.set("pl_off", payload_off); //payload_off
  }
  if (unit_of_meas != "")
  {
    sensor.set("unit_of_meas", unit_of_meas); //unit_of_measurement
  }
  if (optimistic != true)
  {
    sensor.set("opt", optimistic); //optimistic
  }
  if (retain != false)
  {
    sensor.set("ret", retain); //retain
  }
  if (off_delay > 0)
  {
    sensor.set("off_delay", off_delay); //off_delay
  }

  char JSONmessageBuffer[JSON_MSG_BUFFER];
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonDeviceBuffer;
  JsonObject &device = jsonDeviceBuffer.createObject();
  device.set("name", DEVICENAME);
  device.set("manufacturer", DEVICEMANUFACTURER);
  device.set("sw_version", OMG_VERSION);
  JsonArray &identifiers = device.createNestedArray("identifiers");
  identifiers.add(getMacAddress());

  sensor.set("device", device); //device sensor is connected to

  String topic = (String)discovery_Topic + "/" + sensor_type + "/" + unique_id + "/config";
  pubDiscovery(topic, sensor);
}

void pubDiscovery(String topicori, JsonObject &data)
{
  char JSONmessageBuffer[JSON_MSG_BUFFER];
  trc(F("Pub json into:"));
  trc(topicori);
  data.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  trc(JSONmessageBuffer);
  client.publish((char *)topicori.c_str(), JSONmessageBuffer, true);
}

String getMacAddress()
{
  uint8_t baseMac[6];
#ifdef ESP8266
  WiFi.macAddress(baseMac);
#else
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
#endif
  char baseMacChr[13] = {0};
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  return String(baseMacChr);
}

String getUniqueId(String name, String sufix)
{
  String uniqueId = (String)getMacAddress() + name + sufix;
  return String(uniqueId);
}

#endif
