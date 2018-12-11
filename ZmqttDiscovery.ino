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
  trc(F("omgStatusDiscovery"));
  createDiscovery("binary_sensor",                                      //set sensorType
                  will_Topic, Gateway_Name, (char *)getUniqueId("", "").c_str(), //set state_topic,name,uniqueId
                  will_Topic, "connectivity", "",                               //set availability_topic,device_class,value_template,
                  Gateway_AnnouncementMsg, will_Message, "",                    //set,payload_on,payload_off,unit_of_meas,
                  false, true, 0,                                               //set optimistic,retain, off_delay
                  Gateway_AnnouncementMsg, will_Message,false                  //set,payload_avalaible,payload_not avalaible   ,is a child device
  );                       

#ifdef ZsensorBME280
  #define BMEparametersCount 6
  trc(F("bme280Discovery"));
  char * BMEsensor[BMEparametersCount][9] = {
     {"sensor", "tempc", "bme", "tempc","temperature","{{ value_json.tempc }}","", "", "°C"} ,
     {"sensor", "tempf", "bme", "tempf","temperature","{{ value_json.tempf }}","", "", "°F"} ,
     {"sensor", "pa", "bme", "pa","pressure","{{ float(value_json.pa) * 0.01 }}","", "", "hPa"} ,
     {"sensor", "hum", "bme", "hum","humidity","{{ value_json.hum }}","", "", "%"} ,
     {"sensor", "altim", "bme", "altim","","{{ value_json.altim }}","", "", "m"} ,
     {"sensor", "altift", "bme", "altift","","{{ value_json.altift }}","", "", "ft"}
  };
  
  for (int i=0;i<BMEparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(BMEsensor[i][1]);
    createDiscovery(BMEsensor[i][0],
                    BMETOPIC, BMEsensor[i][1], (char *)getUniqueId(BMEsensor[i][2], BMEsensor[i][3]).c_str(),
                    will_Topic, BMEsensor[i][4], BMEsensor[i][5],
                    BMEsensor[i][6], BMEsensor[i][7], BMEsensor[i][8],
                    true, false, 0,"","",true);
  }
#endif

#ifdef ZsensorDHT
  #define DHTparametersCount 2
  trc(F("DHTDiscovery"));
  char * DHTsensor[DHTparametersCount][9] = {
     {"sensor", "tempc", "dht", "tempc","temperature","{{ value_json.temp }}","", "", "°C"} ,
     {"sensor", "hum", "dht", "hum","humidity","{{ value_json.hum }}","", "", "%"}
  };
  
  for (int i=0;i<DHTparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(DHTsensor[i][1]);
    createDiscovery(DHTsensor[i][0],
                    DHTTOPIC, DHTsensor[i][1], (char *)getUniqueId(DHTsensor[i][2], DHTsensor[i][3]).c_str(),
                    will_Topic, DHTsensor[i][4], DHTsensor[i][5],
                    DHTsensor[i][6], DHTsensor[i][7], DHTsensor[i][8],
                    true, false, 0,"","",true);
  }
#endif

}

void createDiscovery(char * sensor_type,
                     char * state_topic, char * s_name, char * unique_id,
                     char * availability_topic, char * device_class, char * value_template,
                     char * payload_on, char * payload_off, char * unit_of_meas,
                     bool optimistic, bool retain, int off_delay,
                     char * payload_available, char * payload_not_avalaible, boolean child_device )
{
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject &sensor = jsonBuffer.createObject();
  sensor.set("stat_t", state_topic); //state_topic
  sensor.set("name", s_name);          //name
  sensor.set("uniq_id", unique_id);  //unique_id
  if (device_class[0])            sensor.set("dev_cla", device_class); //device_class
  if (value_template[0])          sensor.set("val_tpl", value_template); //value_template
  if (payload_on[0])              sensor.set("pl_on", payload_on); // payload_on
  if (payload_off[0])             sensor.set("pl_off", payload_off); //payload_off
  if (unit_of_meas[0])            sensor.set("unit_of_meas", unit_of_meas); //unit_of_measurement*/
  sensor.set("opt", optimistic)|false; //optimistic
  sensor.set("ret", retain)|false; //retain
  if (off_delay != 0)             sensor.set("off_delay", off_delay); //off_delay
  if (payload_available[0])       sensor.set("pl_avail", payload_available); // payload_on
  if (payload_not_avalaible[0])   sensor.set("pl_not_avail", payload_not_avalaible); //payload_off

/*if (strcmp(s_name, Gateway_Name) == 0){
  JsonArray &json_attributes = sensor.createNestedArray("json_attributes");
  json_attributes.add("uptime");
  json_attributes.add("freemem");
  json_attributes.add("rssi");
  json_attributes.add("SSID");
}*/


  if (child_device){
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonDeviceBuffer;
    JsonObject &device = jsonDeviceBuffer.createObject();
    device.set("name", Gateway_Name);
    device.set("manufacturer", DEVICEMANUFACTURER);
    device.set("sw_version", OMG_VERSION);
    device.set("identifiers",getMacAddress());
    sensor.set("device", device); //device sensor is connected to
  }
  String topic = String(discovery_Topic) + "/" + String(sensor_type) + "/" + String(unique_id) + "/config";
  pub((char *)topic.c_str(), sensor);
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
