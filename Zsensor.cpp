/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   Sensor base class
  
   Copyright (C)2018 Chris Broekema
   
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

#include "Zsensor.h"

Zsensor::Zsensor(){}

void Zsensor::init() {}


void Zsensor::measure() {}

void Zsensor::publish(PubSubClient& client, const char* topic, const char* payload) {

  client.publish(topic, payload);
  
}

void Zsensor::publish(PubSubClient& client, const char* topic, const String payload) {

  client.publish(topic, payload.c_str());
  
}


void Zsensor::publish(PubSubClient& client, const char* topic, const float payload) {

  client.publish(topic, String(payload).c_str());

}

void Zsensor::publish(PubSubClient& client, const char* topic, const int payload) {

  client.publish(topic, String(payload).c_str());
  
}
