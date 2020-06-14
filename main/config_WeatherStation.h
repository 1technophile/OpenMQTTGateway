/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

   This files enables to set your parameter for the DHT11/22 sensor

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
#ifndef config_WeatherStation_h
#define config_WeatherStation_h

#include <Arduino.h>
#include <ArduinoJson.h>

extern void setupWeatherStation();
extern void ZgatewayWeatherStationtoMQTT();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectRFtoMQTT "/433toMQTT"

/*-------------------PIN DEFINITIONS----------------------*/
#ifndef RF_WS_RECEIVER_GPIO
#  ifdef ESP8266
#    define RF_WS_RECEIVER_GPIO 0 // D3 on nodemcu // put 4 with rf bridge direct mod
#  elif ESP32
#    define RF_WS_RECEIVER_GPIO 27 // D27 on DOIT ESP32
#  elif __AVR_ATmega2560__
#    define RF_WS_RECEIVER_GPIO 1 //1 = D3 on mega
#  else
#    define RF_WS_RECEIVER_GPIO 1 //1 = D3 on arduino
#  endif
#endif

#endif