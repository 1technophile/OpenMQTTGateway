/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

   This files enables to set your parameter for the Home assistant mqtt Discovery

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
#ifndef config_mqttDiscovery_h
#define config_mqttDiscovery_h

extern String getUniqueId(String name, String sufix);
extern void pubMqttDiscovery();
extern void createDiscoveryFromList(char *mac, char *sensorList[][8], int sensorCount);
extern void createDiscovery(char * sensor_type,
                    char * state_topic, char * s_name, char * unique_id,
                    char * availability_topic, char * device_class, char * value_template,
                    char * payload_on, char * payload_off, char * unit_of_meas,
                    int off_delay,
                    char * payload_available, char * payload_not_avalaible, bool child_device , char * command_topic);

#define discovery_Topic "homeassistant"

#define DEVICEMANUFACTURER "OMG_community"

/*-------------- Auto discovery macros-----------------*/
// Set the line below to true so as to have autodiscovery working with OpenHAB 
#define OpenHABDiscovery false

#if OpenHABDiscovery // OpenHAB autodiscovery value key definition (is defined command is not supported by OpenHAB)
#define jsonBatt      "{{ value_json.batt }}"
#define jsonLux       "{{ value_json.lux }}"
#define jsonPres      "{{ value_json.pres }}"
#define jsonFer       "{{ value_json.fer }}"
#define jsonMoi       "{{ value_json.moi }}"
#define jsonHum       "{{ value_json.hum }}"
#define jsonTemp      "{{ value_json.tem }}"
#define jsonStep      "{{ value_json.steps }}"
#define jsonWeight    "{{ value_json.weight }}"
#define jsonPresence  "{{ value_json.presence }}"
#define jsonAltim     "{{ value_json.altim }}"
#define jsonAltif     "{{ value_json.altift }}"
#define jsonTempf     "{{ value_json.tempf }}"
#define jsonMsg       "{{ value_json.message }}"
#define jsonVal       "{{ value_json.value }}"
#define jsonVolt      "{{ value_json.volt }}"
#define jsonCurrent   "{{ value_json.current }}"
#define jsonPower     "{{ value_json.power }}"
#define jsonGpio      "{{ value_json.gpio }}"
#define jsonFtcd      "{{ value_json.ftcd }}"
#define jsonWm2       "{{ value_json.wattsm2 }}"
#define jsonAdc       "{{ value_json.adc }}"
#define jsonPa        "{{ float(value_json.pa) * 0.01 }}"
#define jsonId        "{{ value_json.id }}"
#else // Home assistant autodiscovery value key definition
#define jsonBatt      "{{ value_json.batt | is_defined }}"
#define jsonLux       "{{ value_json.lux | is_defined }}"
#define jsonPres      "{{ value_json.pres | is_defined }}"
#define jsonFer       "{{ value_json.fer | is_defined }}"
#define jsonMoi       "{{ value_json.moi | is_defined }}"
#define jsonHum       "{{ value_json.hum | is_defined }}"
#define jsonTemp      "{{ value_json.tem | is_defined }}"
#define jsonStep      "{{ value_json.steps | is_defined }}"
#define jsonWeight    "{{ value_json.weight | is_defined }}"
#define jsonPresence  "{{ value_json.presence | is_defined }}"
#define jsonAltim     "{{ value_json.altim | is_defined }}"
#define jsonAltif     "{{ value_json.altift | is_defined }}"
#define jsonTempf     "{{ value_json.tempf | is_defined }}"
#define jsonMsg       "{{ value_json.message | is_defined }}"
#define jsonVal       "{{ value_json.value | is_defined }}"
#define jsonVolt      "{{ value_json.volt | is_defined }}"
#define jsonCurrent   "{{ value_json.current | is_defined }}"
#define jsonPower     "{{ value_json.power | is_defined }}"
#define jsonGpio      "{{ value_json.gpio | is_defined }}"
#define jsonFtcd      "{{ value_json.ftcd | is_defined }}"
#define jsonWm2       "{{ value_json.wattsm2 | is_defined }}"
#define jsonAdc       "{{ value_json.adc | is_defined }}"
#define jsonPa        "{{ float(value_json.pa) * 0.01 | is_defined }}"
#define jsonId        "{{ value_json.id | is_defined }}"
#endif

#endif