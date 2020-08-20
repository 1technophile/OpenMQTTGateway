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

#  ifdef ESP8266
#    include <espnow.h>
#  elif ESP32
#    include <esp_now.h>
#  endif

#  ifdef ESP8266
void ESPNOWtoMQTT(uint8_t* mac, uint8_t* data, uint8_t len) {
#  elif ESP32
void ESPNOWtoMQTT(const unsigned char* mac, const unsigned char* data, int len) {
#  endif
  char cmac[18];
  snprintf(cmac, sizeof(cmac), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& ESPNOWdata = jsonBuffer.createObject();
  Log.trace(F("Rcv. ESPNOW from %s" CR), cmac);
  ESPNOWdata.set("id", cmac);
  ESPNOWdata.set("message", *data);
  pub(subjectESPNOWtoMQTT, ESPNOWdata);
}

/** 
 * Setup ESPNOW gateway in receiving mode, the emitter mode is setup when sending MQTTtoESPNOW commands
 */
void setupESPNOW() {
  WiFi.mode(WIFI_AP_STA);
  (esp_now_init() == 0) ? Log.trace(F("ZgatewayESPNOW setup done" CR)) : Log.error(F("ZgatewayESPNOW setup failed" CR));
#  ifdef ESP32
  esp_wifi_set_ps(WIFI_PS_NONE); // non sleep done, on ESP8266 the modem is per default in non sleep mode
#  endif
  esp_now_register_recv_cb(ESPNOWtoMQTT);
#  ifdef ESP8266
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
#  endif
}
#endif
