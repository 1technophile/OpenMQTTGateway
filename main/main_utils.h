/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This program enables to:
 - receive MQTT data from a topic and send signal (RF, IR, BLE, GSM)  corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received signals (RF, IR, BLE, GSM)

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
#ifndef MAIN_UTILS_H
#define MAIN_UTILS_H

#include <Arduino.h>
#define ARDUINOJSON_USE_LONG_LONG     1
#define ARDUINOJSON_ENABLE_STD_STRING 1
#include <ArduinoJson.h>

#ifndef JSON_MSG_BUFFER
#  if defined(ESP32)
#    define JSON_MSG_BUFFER 1024 // adjusted to minimum size covering largest home assistant discovery messages
#    if MQTT_SECURE_DEFAULT
#      define JSON_MSG_BUFFER_MAX 2048 // Json message buffer size increased to handle certificate changes through MQTT, used for the queue and the coming MQTT messages
#    else
#      define JSON_MSG_BUFFER_MAX 1024 // Minimum size for the cover MQTT discovery message
#    endif
#  elif defined(ESP8266)
#    define JSON_MSG_BUFFER     512 // Json message max buffer size, don't put 768 or higher it is causing unexpected behaviour on ESP8266, certificates handling with ESP8266 is not tested
#    define JSON_MSG_BUFFER_MAX 832 // Minimum size for MQTT discovery message
#  endif
#endif

bool cmpToMainTopic(const char* topicOri, const char* toAdd);

#if defined(ZgatewayRF) || defined(ZgatewayIR) || defined(ZgatewaySRFB) || defined(ZgatewayWeatherStation) || defined(ZgatewayRTL_433)
bool isAduplicateSignal(uint64_t);
void storeSignalValue(uint64_t);
#endif

#endif // MAIN_UTILS_H
