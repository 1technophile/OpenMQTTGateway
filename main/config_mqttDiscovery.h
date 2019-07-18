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
extern void createDiscovery(char * sensor_type,
                    char * state_topic, char * s_name, char * unique_id,
                    char * availability_topic, char * device_class, char * value_template,
                    char * payload_on, char * payload_off, char * unit_of_meas,
                    int off_delay,
                    char * payload_available, char * payload_not_avalaible, bool child_device , char * command_topic);

#define discovery_Topic "homeassistant"

#define DEVICENAME "OpenMQTTGateway " Gateway_Name
#define DEVICEMANUFACTURER "OMG_community"

#endif