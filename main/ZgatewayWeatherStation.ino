/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
  - publish MQTT data, which are received from RF433Mhz signal (based on WeatherStationDataRx)

    Library for read weather data from Venus W174/W132, Auriol H13726, Hama EWS 1500, Meteoscan W155/W160

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

#ifdef ZgatewayWeatherStation
#  include <WeatherStationDataRx.h>
WeatherStationDataRx wsdr(RF_WS_RECEIVER_GPIO, true);

void PairedDeviceAdded(byte newID) {
#  if defined(ESP8266) || defined(ESP32)
  Serial.printf("ZgatewayWeatherStation: New device paired %d\r\n", newID);
#  else
  Serial.print("ZgatewayWeatherStation: New device paired ");
  Serial.println(newID, DEC);
#  endif
  const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(2);
  StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
  JsonObject& RFdata = jsonBuffer.createObject();
  RFdata.set("sensor", newID);
  RFdata.set("action", "paired");
  pub(subjectRFtoMQTT, RFdata);
  wsdr.pair(NULL, PairedDeviceAdded);
}

void setupWeatherStation() {
  Log.notice(F("RF_WS_RECEIVER_GPIO %d" CR), RF_WS_RECEIVER_GPIO);
  wsdr.begin();
  wsdr.pair(NULL, PairedDeviceAdded);
  Log.trace(F("ZgatewayWeatherStation setup done " CR));
}

void sendWindSpeedData(byte id, float wind_speed, byte battery_status) {
  unsigned long MQTTvalue = 10000 + round(wind_speed);
  if (!isAduplicate(MQTTvalue)) { // conditions to avoid duplications of RF -->MQTT
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(3);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
    JsonObject& RFdata = jsonBuffer.createObject();
    RFdata.set("sensor", id);
    RFdata.set("wind_speed", wind_speed);
    RFdata.set("battery", bitRead(battery_status, 0) == 0 ? "OK" : "Low");
    pub(subjectRFtoMQTT, RFdata);
    Log.trace(F("Store wind speed val: %lu" CR), MQTTvalue);
    storeValue(MQTTvalue);
  }
}

void sendRainData(byte id, float rain_volume, byte battery_status) {
  unsigned long MQTTvalue = 11000 + round(rain_volume * 10.0);
  if (!isAduplicate(MQTTvalue)) { // conditions to avoid duplications of RF -->MQTT
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(3);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
    JsonObject& RFdata = jsonBuffer.createObject();
    RFdata.set("sensor", id);
    RFdata.set("rain_volume", rain_volume);
    RFdata.set("battery", bitRead(battery_status, 1) == 0 ? "OK" : "Low");
    pub(subjectRFtoMQTT, RFdata);
    Log.trace(F("Store rain_volume: %lu" CR), MQTTvalue);
    storeValue(MQTTvalue);
  }
}

void sendWindData(byte id, int wind_direction, float wind_gust, byte battery_status) {
  unsigned long MQTTvalue = 20000 + round(wind_gust * 10.0) + wind_direction;
  if (!isAduplicate(MQTTvalue)) { // conditions to avoid duplications of RF -->MQTT
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(4);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
    JsonObject& RFdata = jsonBuffer.createObject();
    RFdata.set("sensor", id);
    RFdata.set("wind_direction", wind_direction);
    RFdata.set("wind_gust", wind_gust);
    RFdata.set("battery", bitRead(battery_status, 0) == 0 ? "OK" : "Low");
    pub(subjectRFtoMQTT, RFdata);
    Log.trace(F("Store wind data val: %lu" CR), MQTTvalue);
    storeValue(MQTTvalue);
  }
}

void sendTemperatureData(byte id, float temperature, int humidity, byte battery_status) {
  unsigned long MQTTvalue = 40000 + abs(round(temperature * 100.0)) + humidity;
  if (!isAduplicate(MQTTvalue)) { // conditions to avoid duplications of RF -->MQTT
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(4);
    StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
    JsonObject& RFdata = jsonBuffer.createObject();
    RFdata.set("sensor", id);
    RFdata.set("temperature", temperature);
    RFdata.set("humidity", humidity);
    RFdata.set("battery", bitRead(battery_status, 0) == 0 ? "OK" : "Low");
    pub(subjectRFtoMQTT, RFdata);
    Log.trace(F("Store temp val: %lu" CR), MQTTvalue);
    storeValue(MQTTvalue);
  }
}

void ZgatewayWeatherStationtoMQTT() {
  char newData = wsdr.readData();
  switch (newData) {
    case 'T':
      Log.trace(F("Temperature" CR));
      sendTemperatureData(wsdr.sensorID(), wsdr.readTemperature(), wsdr.readHumidity(), wsdr.batteryStatus());
      break;

    case 'S':
      Log.trace(F("Wind speed" CR));
      sendWindSpeedData(wsdr.sensorID(), wsdr.readWindSpeed(), wsdr.batteryStatus());
      break;

    case 'G':
      Log.trace(F("Wind direction" CR));
      sendWindData(wsdr.sensorID(), wsdr.readWindDirection(), wsdr.readWindGust(), wsdr.batteryStatus());
      break;

    case 'R':
      Log.trace(F("Rain volume" CR));
      sendRainData(wsdr.sensorID(), wsdr.readRainVolume(), wsdr.batteryStatus());
      break;

    default:
      break;
  }
}

#endif
