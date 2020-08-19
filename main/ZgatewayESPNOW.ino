/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR/BLE signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send ESPNOW signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received ESPNOW signal

    Copyright: (c)Florian ROBERT
  
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
#include "User_config.h"

#ifdef ZgatewayESPNOW

#  include <espnow.h>

/** 
 * Setup ESPNOW gateway in receiving mode, the emitter mode is setup when sending MQTTtoESPNOW commands
 */
void setupESPNOW() {
  (esp_now_init() == 0) ? Log.trace(F("ZgatewayESPNOW setup done" CR)) : Log.error(F("ZgatewayESPNOW setup failed" CR));
  esp_now_register_recv_cb(ESPNOWtoMQTT);
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
}

void ESPNOWtoMQTT(uint8_t* mac_addr, uint8_t* data, uint8_t len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& ESPNOWdata = jsonBuffer.createObject();
  Log.trace(F("Rcv. ESPNOW" CR));
  ESPNOWdata.set("message", *data);
  pub(subjectESPNOWtoMQTT, ESPNOWdata);
}

#endif
