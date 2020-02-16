/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your MikroMarz electricity meter and a MQTT broker
   Send and receiving command by MQTT

   https://www.mikromarz.com/www-mikromarz-cz/eshop/51-1-Elektromery/179-2-3-fazove-elektromery/5/690-3-fazovy-2-tarifni-elektromer-SE1-PM2

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
#ifndef config_ElectricityMeter_h
#define config_ElectricityMeter_h

#include <Arduino.h>
#include <ArduinoJson.h>

extern void setupElectricityMeter();
extern void ZgatewayElectricityMetertoMQTT();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectToMQTT "/EMtoMQTT"


/*-------------------PIN DEFINITIONS----------------------*/
#ifndef RF_WS_RECEIVER_PIN
    #ifdef ESP8266
        #define RF_WS_RECEIVER_PIN 0 // D3 on nodemcu // put 4 with rf bridge direct mod
    #elif ESP32
        #define RF_WS_RECEIVER_PIN 27 // D27 on DOIT ESP32
    #endif
#endif

#endif