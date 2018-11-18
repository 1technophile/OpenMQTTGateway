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

String State_topic = "";
String Name = "";
String Unique_Id = "";
String Availability_topic = "";
String Device_class = "";
String Value_template = "";
String Payload_on = "";
String Payload_off = "";
String Unit_of_meas = "";
bool Optimistic = true;
bool Retain = false;
int Off_delay = 0;

void pubMqttDiscovery()
{

#ifdef omgStatus
  trc(F("omgStatusDiscovery"));
  cleanAllAttributes();                                                    //clean all atributes
  createBasicAttrib(will_Topic, DEVICENAME, getUniqueId("omg", "status")); //set state_topic,name,uniqueId
  createDiscoveryAttrib("", "connectivity", "",                            //set availability_topic,device_class,value_template,
                        Gateway_AnnouncementMsg, will_Message, "",         //set,payload_on,payload_off,unit_of_meas,
                        true, false, 0);                                   //set optimistic,retain, off_delay
  createDiscovery("binary_sensor");                                        //set sensor_Type & send mqtt

#endif
#ifdef ZsensorBME280
#ifdef discBme280
  trc(F("bme280Discovery"));
#ifdef bmeTempC
  cleanAllAttributes();
  createBasicAttrib(BME, "tempc", getUniqueId("bme", "tempc"));
  createDiscoveryAttrib(will_Topic, "temperature", "{{ value_json.tempc }}",
                        "", "", "°C",
                        true, false, 0);
  createDiscovery("sensor");
#endif
#ifdef bmeTempF
  cleanAllAttributes();
  createBasicAttrib(BME, "tempf", getUniqueId("bme", "tempf"));
  createDiscoveryAttrib(will_Topic, "temperature", "{{ value_json.tempf }}",
                        "", "", "°F",
                        true, false, 0);
  createDiscovery("sensor");
#endif
#ifdef bmePreMbar
  cleanAllAttributes();
  createBasicAttrib(BME, "pa", getUniqueId("bme", "pa"));
  createDiscoveryAttrib(will_Topic, "pressure", "{{ float(value_json.pa) * 0.01 }}",
                        "", "", "hPa",
                        true, false, 0);
  createDiscovery("sensor");
#endif
#ifdef bmeHum
  cleanAllAttributes();
  createBasicAttrib(BME, "hum", getUniqueId("bme", "hum"));
  createDiscoveryAttrib(will_Topic, "humidity", "{{ value_json.hum }}",
                        "", "", "%",
                        true, false, 0);
  createDiscovery("sensor");
#endif
#ifdef bmeAltM
 cleanAllAttributes();
 createBasicAttrib(BME, "altim", getUniqueId("bme", "altim"));
 createDiscoveryAttrib(will_Topic, "", "{{ value_json.altim }}",
                       "", "", "m",
                       true, false, 0);
 createDiscovery("sensor");
#endif
#ifdef bmeAltFt
 cleanAllAttributes();
 createBasicAttrib(BME, "altift", getUniqueId("bme", "altift"));
 createDiscoveryAttrib(will_Topic, "", "{{ value_json.altift }}",
                       "", "", "ft",
                       true, false, 0);
 createDiscovery("sensor");
#endif
#endif
#endif
}

void createDiscoveryAttrib(String availability_topic, String device_class, String value_template,
                           String payload_on, String payload_off, String unit_of_meas,
                           bool optimistic, bool retain, int off_delay)
{
  Availability_topic = availability_topic;
  Device_class = device_class;
  Value_template = value_template;
  Payload_on = payload_on;
  Payload_off = payload_off;
  Unit_of_meas = unit_of_meas;
  Optimistic = optimistic;
  Retain = retain;
  Off_delay = off_delay;
}

void createBasicAttrib(String stateTopic, String name, String uniqueId)
{
  State_topic = stateTopic;
  Name = name;
  Unique_Id = uniqueId;
}

void cleanAllAttributes()
{
  State_topic = "";
  Name = "";
  Unique_Id = "";
  Availability_topic = "";
  Device_class = "";
  Value_template = "";
  Payload_on = "";
  Payload_off = "";
  Unit_of_meas = "";
  Optimistic = true;
  Retain = false;
  Off_delay = 0;
}

void createDiscovery(String sensor_Type)
{
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer3;
  JsonObject &sensor = jsonBuffer3.createObject();
  sensor.set("stat_t", State_topic); //state_topic
  sensor.set("name", Name);          //name
  sensor.set("uniq_id", Unique_Id);  //unique_id
  if (Availability_topic != "")
  {
    sensor.set("avty_t", Availability_topic); //availability_topic
  }
  if (Device_class != "")
  {
    sensor.set("dev_cla", Device_class); //device_class
  }
  if (Value_template != "")
  {
    sensor.set("val_tpl", Value_template); //value_template
  }
  if (Payload_on != "")
  {
    sensor.set("pl_on", Payload_on); // payload_on
  }
  if (Payload_off != "")
  {
    sensor.set("pl_off", Payload_off); //payload_off
  }
  if (Unit_of_meas != "")
  {
    sensor.set("unit_of_meas", Unit_of_meas); //unit_of_measurement
  }
  if (Optimistic != true)
  {
    sensor.set("opt", Optimistic); //optimistic
  }
  if (Retain != false)
  {
    sensor.set("ret", Retain); //retain
  }
  if (Off_delay > 0)
  {
    sensor.set("off_delay", Off_delay); //off_delay
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

  String topic = (String)discovery_Topic + "/" + sensor_Type + "/" + Unique_Id + "/config";
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
