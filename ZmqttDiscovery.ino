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
                  0,                                               //set optimistic,retain, off_delay
                  Gateway_AnnouncementMsg, will_Message,true,""                //set,payload_avalaible,payload_not avalaible   ,is a child device, command topic, state on, state off
  );                       

#ifdef ZsensorBME280
  #define BMEparametersCount 6
  trc(F("bme280Discovery"));
  char * BMEsensor[BMEparametersCount][8] = {
     {"sensor", "tempc", "bme","temperature","{{ value_json.tempc }}","", "", "°C"} ,
     {"sensor", "tempf", "bme","temperature","{{ value_json.tempf }}","", "", "°F"} ,
     {"sensor", "pa", "bme","","{{ float(value_json.pa) * 0.01 }}","", "", "hPa"} ,
     {"sensor", "hum", "bme","humidity","{{ value_json.hum }}","", "", "%"} ,
     {"sensor", "altim", "bme","","{{ value_json.altim }}","", "", "m"} ,
     {"sensor", "altift", "bme","","{{ value_json.altift }}","", "", "ft"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<BMEparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(BMEsensor[i][1]);
    createDiscovery(BMEsensor[i][0],
                    BMETOPIC, BMEsensor[i][1], (char *)getUniqueId(BMEsensor[i][1], BMEsensor[i][2]).c_str(),
                    will_Topic, BMEsensor[i][3], BMEsensor[i][4],
                    BMEsensor[i][5], BMEsensor[i][6], BMEsensor[i][7],
                    0,"","",true,"");
  }
#endif

#ifdef ZsensorDHT
  #define DHTparametersCount 2
  trc(F("DHTDiscovery"));
  char * DHTsensor[DHTparametersCount][8] = {
     {"sensor", "tempc", "dht", "temperature","{{ value_json.temp }}","", "", "°C"} ,
     {"sensor", "hum", "dht", "humidity","{{ value_json.hum }}","", "", "%"}
  };
  
  for (int i=0;i<DHTparametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(DHTsensor[i][1]);
    createDiscovery(DHTsensor[i][0],
                    DHTTOPIC, DHTsensor[i][1], (char *)getUniqueId(DHTsensor[i][1], DHTsensor[i][2]).c_str(),
                    will_Topic, DHTsensor[i][3], DHTsensor[i][4],
                    DHTsensor[i][5], DHTsensor[i][6], DHTsensor[i][7],
                    0,"","",true,"");
  }
#endif

#ifdef ZsensorADC
  trc(F("ADCDiscovery"));
  char * ADCsensor[8] = {"sensor", "adc", "adc", "","{{ value_json.adc }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(ADCsensor[1]);
    createDiscovery(ADCsensor[0],
                    ADCTOPIC, ADCsensor[1], (char *)getUniqueId(ADCsensor[1], ADCsensor[2]).c_str(),
                    will_Topic, ADCsensor[3], ADCsensor[4],
                    ADCsensor[5], ADCsensor[6], ADCsensor[7],
                    0,"","",true,"");
#endif

#ifdef ZsensorBH1750
  #define BH1750parametersCount 3
  trc(F("BH1750Discovery"));
  char * BH1750sensor[BH1750parametersCount][8] = {
     {"sensor", "lux", "BH1750", "illuminance","{{ value_json.lux }}","", "", "lu"} ,
     {"sensor", "ftCd", "BH1750","","{{ value_json.ftCd }}","", "", ""} ,
     {"sensor", "wattsm2", "BH1750","","{{ value_json.wattsm2 }}","", "", "wm²"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<BH1750parametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(BH1750sensor[i][1]);
    createDiscovery(BH1750sensor[i][0],
                    subjectBH1750toMQTT, BH1750sensor[i][1], (char *)getUniqueId(BH1750sensor[i][1], BH1750sensor[i][2]).c_str(),
                    will_Topic, BH1750sensor[i][3], BH1750sensor[i][4],
                    BH1750sensor[i][5], BH1750sensor[i][6], BH1750sensor[i][7],
                    0,"","",true,"");
  }
#endif

#ifdef ZsensorTSL2561
  #define TSL2561parametersCount 3
  trc(F("TSL2561Discovery"));
  char * TSL2561sensor[TSL2561parametersCount][8] = {
     {"sensor", "lux", "TSL2561", "illuminance","{{ value_json.lux }}","", "", "lu"} ,
     {"sensor", "ftcd", "TSL2561","","{{ value_json.ftcd }}","", "", ""} ,
     {"sensor", "wattsm2", "TSL2561","","{{ value_json.wattsm2 }}","", "", "wm²"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<TSL2561parametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(TSL2561sensor[i][1]);
    createDiscovery(TSL2561sensor[i][0],
                    subjectTSL12561toMQTT, TSL2561sensor[i][1], (char *)getUniqueId(TSL2561sensor[i][1], TSL2561sensor[i][2]).c_str(),
                    will_Topic, TSL2561sensor[i][3], TSL2561sensor[i][4],
                    TSL2561sensor[i][5], TSL2561sensor[i][6], TSL2561sensor[i][7],
                    0,"","",true,"");
  }
#endif

#ifdef ZsensorHCSR501
  trc(F("HCSR501Discovery"));
  char * HCSR501sensor[8] = {"binary_sensor", "hcsr501", "hcsr501", "","","true", "false", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(HCSR501sensor[1]);
    createDiscovery(HCSR501sensor[0],
                    subjectHCSR501toMQTT, HCSR501sensor[1], (char *)getUniqueId(HCSR501sensor[1], HCSR501sensor[2]).c_str(),
                    will_Topic, HCSR501sensor[3], HCSR501sensor[4],
                    HCSR501sensor[5], HCSR501sensor[6], HCSR501sensor[7],
                    0,"","",true,"");
#endif

#ifdef ZsensorGPIOInput
  trc(F("GPIOInputDiscovery"));
  char * GPIOInputsensor[8] = {"binary_sensor", "GPIOInput", "GPIOInput", "","","true", "false", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(GPIOInputsensor[1]);
    createDiscovery(GPIOInputsensor[0],
                    subjectGPIOInputtoMQTT, GPIOInputsensor[1], (char *)getUniqueId(GPIOInputsensor[1], GPIOInputsensor[2]).c_str(),
                    will_Topic, GPIOInputsensor[3], GPIOInputsensor[4],
                    GPIOInputsensor[5], GPIOInputsensor[6], GPIOInputsensor[7],
                    0,"","",true,"");
#endif

#ifdef ZsensorINA226
  #define INA226parametersCount 3
  trc(F("INA226Discovery"));
  char * INA226sensor[INA226parametersCount][8] = {
     {"sensor", "volt", "INA226", "","{{ value_json.volt }}","", "", "V"} ,
     {"sensor", "current", "INA226","","{{ value_json.current }}","", "", "A"} ,
     {"sensor", "power", "INA226","","{{ value_json.power }}","", "", "W"}
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement
  };
  
  for (int i=0;i<INA226parametersCount;i++){
   trc(F("CreateDiscoverySensor"));
   trc(INA226sensor[i][1]);
    createDiscovery(INA226sensor[i][0],
                    subjectINA226toMQTT, INA226sensor[i][1], (char *)getUniqueId(INA226sensor[i][1], INA226sensor[i][2]).c_str(),
                    will_Topic, INA226sensor[i][3], INA226sensor[i][4],
                    INA226sensor[i][5], INA226sensor[i][6], INA226sensor[i][7],
                    0,"","",true,"");
  }
#endif

#ifdef ZactuatorONOFF
  trc(F("actuatorONOFFDiscovery"));
  char * actuatorONOFF[8] = {"switch", "actuatorONOFF", "", "","","ON", "OFF", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(actuatorONOFF[1]);
    createDiscovery(actuatorONOFF[0],
                    subjectGTWONOFFtoMQTT, actuatorONOFF[1], (char *)getUniqueId(actuatorONOFF[1], actuatorONOFF[2]).c_str(),
                    will_Topic, actuatorONOFF[3], actuatorONOFF[4],
                    actuatorONOFF[5], actuatorONOFF[6], actuatorONOFF[7],
                    0,"","",true,subjectMQTTtoONOFF);
#endif


#ifdef ZgatewayRF
  // Sensor to display RF received value 
  trc(F("gatewayRFDiscovery"));
  char * gatewayRF[8] = {"sensor", "gatewayRF", "", "","{{ value_json.value }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewayRF[1]);
    createDiscovery(gatewayRF[0],
                    subjectRFtoMQTT, gatewayRF[1], (char *)getUniqueId(gatewayRF[1], gatewayRF[2]).c_str(),
                    will_Topic, gatewayRF[3], gatewayRF[4],
                    gatewayRF[5], gatewayRF[6], gatewayRF[7],
                    0,"","",true,"");
#endif

#ifdef ZgatewayRF2
  // Sensor to display RF received value 
  trc(F("gatewayRF2Discovery"));
  char * gatewayRF2[8] = {"sensor", "gatewayRF2", "", "","{{ value_json.value }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewayRF2[1]);
    createDiscovery(gatewayRF2[0],
                    subjectRF2toMQTT, gatewayRF2[1], (char *)getUniqueId(gatewayRF2[1], gatewayRF2[2]).c_str(),
                    will_Topic, gatewayRF2[3], gatewayRF2[4],
                    gatewayRF2[5], gatewayRF2[6], gatewayRF2[7],
                    0,"","",true,"");
#endif

#ifdef ZgatewayRF315
  // Sensor to display RF received value 
  trc(F("gatewayRF315Discovery"));
  char * gatewayRF315[8] = {"sensor", "gatewayRF315", "", "","{{ value_json.value }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewayRF315[1]);
    createDiscovery(gatewayRF315[0],
                    subjectRF315toMQTT, gatewayRF315[1], (char *)getUniqueId(gatewayRF315[1], gatewayRF315[2]).c_str(),
                    will_Topic, gatewayRF315[3], gatewayRF315[4],
                    gatewayRF315[5], gatewayRF315[6], gatewayRF315[7],
                    0,"","",true,"");
#endif

#ifdef ZgatewayRFM69
  // Sensor to display RF received value 
  trc(F("gatewayRFM69Discovery"));
  char * gatewayRFM69[8] = {"sensor", "gatewayRFM69", "", "","{{ value_json.value }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewayRFM69[1]);
    createDiscovery(gatewayRFM69[0],
                    subjectRFM69toMQTT, gatewayRFM69[1], (char *)getUniqueId(gatewayRFM69[1], gatewayRFM69[2]).c_str(),
                    will_Topic, gatewayRFM69[3], gatewayRFM69[4],
                    gatewayRFM69[5], gatewayRFM69[6], gatewayRFM69[7],
                    0,"","",true,"");
#endif

#ifdef ZgatewayLORA
  // Sensor to display RF received value 
  trc(F("gatewayLORADiscovery"));
  char * gatewayLORA[8] = {"sensor", "gatewayLORA", "", "","{{ value_json.message }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewayLORA[1]);
    createDiscovery(gatewayLORA[0],
                    subjectLORAtoMQTT, gatewayLORA[1], (char *)getUniqueId(gatewayLORA[1], gatewayLORA[2]).c_str(),
                    will_Topic, gatewayLORA[3], gatewayLORA[4],
                    gatewayLORA[5], gatewayLORA[6], gatewayLORA[7],
                    0,"","",true,"");
#endif

#ifdef ZgatewaySRFB
  // Sensor to display RF received value 
  trc(F("gatewaySRFBDiscovery"));
  char * gatewaySRFB[8] = {"sensor", "gatewaySRFB", "", "","{{ value_json.value }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewaySRFB[1]);
    createDiscovery(gatewaySRFB[0],
                    subjectSRFBtoMQTT, gatewaySRFB[1], (char *)getUniqueId(gatewaySRFB[1], gatewaySRFB[2]).c_str(),
                    will_Topic, gatewaySRFB[3], gatewaySRFB[4],
                    gatewaySRFB[5], gatewaySRFB[6], gatewaySRFB[7],
                    0,"","",true,"");
#endif

#ifdef ZgatewayPilight
  // Sensor to display RF received value 
  trc(F("gatewayPilightDiscovery"));
  char * gatewayPilight[8] = {"sensor", "gatewayPilight", "", "","{{ value_json.message }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewayPilight[1]);
    createDiscovery(gatewayPilight[0],
                    subjectPilighttoMQTT, gatewayPilight[1], (char *)getUniqueId(gatewayPilight[1], gatewayPilight[2]).c_str(),
                    will_Topic, gatewayPilight[3], gatewayPilight[4],
                    gatewayPilight[5], gatewayPilight[6], gatewayPilight[7],
                    0,"","",true,"");
#endif

#ifdef ZgatewayIR
  // Sensor to display IR received value 
  trc(F("gatewayIRDiscovery"));
  char * gatewayIR[8] = {"sensor", "gatewayIR", "", "","{{ value_json.value }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewayIR[1]);
    createDiscovery(gatewayIR[0],
                    subjectIRtoMQTT, gatewayIR[1], (char *)getUniqueId(gatewayIR[1], gatewayIR[2]).c_str(),
                    will_Topic, gatewayIR[3], gatewayIR[4],
                    gatewayIR[5], gatewayIR[6], gatewayIR[7],
                    0,"","",true,"");
#endif

#ifdef Zgateway2G
  // Sensor to display 2G received value 
  trc(F("gateway2GDiscovery"));
  char * gateway2G[8] = {"sensor", "gateway2G", "", "","{{ value_json.message }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gateway2G[1]);
    createDiscovery(gateway2G[0],
                    subject2GtoMQTT, gateway2G[1], (char *)getUniqueId(gateway2G[1], gateway2G[2]).c_str(),
                    will_Topic, gateway2G[3], gateway2G[4],
                    gateway2G[5], gateway2G[6], gateway2G[7],
                    0,"","",true,"");
#endif

#ifdef ZgatewayBT
  // Sensor to display BT received value 
  trc(F("gatewayBTDiscovery"));
  char * gatewayBT[8] = {"sensor", "gatewayBT", "", "","{{ value_json.message }}","", "", ""};
     //component type,name,availability topic,device class,value template,payload on, payload off, unit of measurement

   trc(F("CreateDiscoverySensor"));
   trc(gatewayBT[1]);
    createDiscovery(gatewayBT[0],
                    subjectBTtoMQTT, gatewayBT[1], (char *)getUniqueId(gatewayBT[1], gatewayBT[2]).c_str(),
                    will_Topic, gatewayBT[3], gatewayBT[4],
                    gatewayBT[5], gatewayBT[6], gatewayBT[7],
                    0,"","",true,"");
#endif
}

void createDiscovery(char * sensor_type,
                     char * state_topic, char * s_name, char * unique_id,
                     char * availability_topic, char * device_class, char * value_template,
                     char * payload_on, char * payload_off, char * unit_of_meas,
                     int off_delay,
                     char * payload_available, char * payload_not_avalaible, boolean child_device , char * command_topic)
{
  const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(14) + JSON_OBJECT_SIZE(5)+ JSON_ARRAY_SIZE(1);
  StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
  JsonObject &sensor = jsonBuffer.createObject();
  sensor.set("stat_t", state_topic); //state_topic
  sensor.set("name", s_name);          //name
  sensor.set("uniq_id", unique_id);  //unique_id
  if (device_class[0])            sensor.set("dev_cla", device_class); //device_class
  if (value_template[0])          sensor.set("val_tpl", value_template); //value_template
  if (payload_on[0])              sensor.set("pl_on", payload_on); // payload_on
  if (payload_off[0])             sensor.set("pl_off", payload_off); //payload_off
  if (unit_of_meas[0])            sensor.set("unit_of_meas", unit_of_meas); //unit_of_measurement*/
  if (off_delay != 0)             sensor.set("off_delay", off_delay); //off_delay
  if (payload_available[0])       sensor.set("pl_avail", payload_available); // payload_on
  if (payload_not_avalaible[0])   sensor.set("pl_not_avail", payload_not_avalaible); //payload_off
  if (command_topic[0])           sensor.set("cmd_t", command_topic); //command_topic

/*if (strcmp(s_name, Gateway_Name) == 0){
  JsonArray &json_attributes = sensor.createNestedArray("json_attributes");
  json_attributes.add("uptime");
  json_attributes.add("freemem");
  json_attributes.add("rssi");
  json_attributes.add("SSID");
}*/


  if (child_device){
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(5) + JSON_ARRAY_SIZE(1);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonDeviceBuffer;
    JsonObject &device = jsonDeviceBuffer.createObject();
    device.set("name", Gateway_Name);
    device.set("manufacturer", DEVICEMANUFACTURER);
    device.set("sw_version", OMG_VERSION);
    JsonArray &identifiers = device.createNestedArray("identifiers");
    identifiers.add(getMacAddress());
    sensor.set("device", device); //device sensor is connected to
  }
  String topic = String(discovery_Topic) + "/" + String(sensor_type) + "/" + String(unique_id) + "/config";
  pub((char *)topic.c_str(), sensor);
}

String getMacAddress()
{
  uint8_t baseMac[6];
  char baseMacChr[13] = {0};
  #if defined(ESP8266)
    WiFi.macAddress(baseMac);
    sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  #elif defined(ESP32)
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  #else
    sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  #endif
  return String(baseMacChr);
}

String getUniqueId(String name, String sufix)
{
  String uniqueId = (String)getMacAddress() + name + sufix;
  return String(uniqueId);
}

#endif
