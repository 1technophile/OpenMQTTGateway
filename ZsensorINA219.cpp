/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    INA219 reading Addon
  
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
// Pinout
// INA219 - Uno - Mega - NODEMCU
// SCL    - A5  - 21 - D1
// SDA    - A4  - 20 - D2
//

#include "ZsensorINA219.h"


Adafruit_INA219 ina219;
unsigned long timeINA219 = 0;


void ZsensorINA219::init() {

  Serial.println("Initializing INA219");
  ina219.begin();
  ina219.setCalibration_32V_1A();
  // ina219.setCalibration_16V_400mA();

  Serial.println("INA219 initialized");
}

void ZsensorINA219::measure(PubSubClient& client) {

  if (millis() > (timeINA219 + TimeBetweenReadingINA219)) {

    Serial.println("measuring INA219");
    
    float shuntvoltage = ina219.getShuntVoltage_mV();
    float busvoltage = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();
    float loadvoltage = busvoltage + (shuntvoltage / 1000);

    Serial.println("publishing INA219");
    
    this->publish(client, subjectVoltINA219, loadvoltage);
    this->publish(client, subjectCurrentINA219, String(current_mA).c_str());
    this->publish(client, subjectPowerINA219, String(loadvoltage * current_mA / 1000).c_str());
    
    timeINA219 = millis();
  }
}


