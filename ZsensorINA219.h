/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   INA219 sensor 
  
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
#ifndef ZSENSOR_INA219_H
#define ZSENSOR_INA219_H

#include "Zsensor.h"
#include "User_config.h"

#include <Wire.h>
#include <Adafruit_INA219.h>

#define subjectVoltINA219    MQTT_ROOT Gateway_Name "/INA219/Volt_V"
#define subjectCurrentINA219 MQTT_ROOT Gateway_Name "/INA219/Current_mA"        
#define subjectPowerINA219   MQTT_ROOT Gateway_Name "/INA219/Power_W" 
#define TimeBetweenReadingINA219 30000 // time between 2 INA219 readings

class ZsensorINA219 : public Zsensor {
 public:
  void init();
  void measure(PubSubClient& client);
};

  

#endif
