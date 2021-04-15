/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your GridFree SUN-2000G inverter and a MQTT broker.
   Send inverter metrics by MQTT.

  This gateway enables to:
  - publish MQTT data, which are received by RS232 ModBus.

    Library for read metrics from GWL Power, GridFree SUN-2000G inverter <https://shop.gwl.eu/GridFree-Inverters/GridFree-AC-Inverter-with-limiter-2kW-SUN-2000G-45-90V.html>.

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
#ifndef config_GFSunInverter_h
#define config_GFSunInverter_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GfSun2000.h>

extern void setupGFSunInverter();
extern void ZgatewayGFSunInverterMQTT();

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectRFtoMQTT "/GFSuntoMQTT"
#ifndef GFSUNINVERTER_DELAY
#  define GFSUNINVERTER_DELAY 10000
#endif

// Enable to publish a whole dbus registry
// #define GFSUNINVERTER_DEVEL 1

#endif