/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 
 
   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT
   
   This is the Climate Addon:
   - Measures Temperature, Humidity and Pressure
   - Generates Values for: Temperature in degrees C and F, Humidity in %, Pressure in Pa, Altitude in Meter and Feet
   - Required Hardware Module: HTU21
   - Required Library: SparkFun HTU21 Library v1.1.3

   Connection Schemata:
   --------------------

   HTU21 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> Vu (5V)
   GND ---------> GND ------------------> GND
   SCL ---------> Pin A5 ---------------> D1
   SDA ---------> Pin A4 ---------------> D2
   
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
#ifndef config_HTU21_h
#define config_HTU21_h

extern void setupZsensorHTU21();
extern void MeasureTempHum();

#define htu21_always            true // if false when the current value of the parameter is the same as previous one don't send it by MQTT
#define TimeBetweenReadinghtu21 30000

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define HTUTOPIC "/CLIMAtoMQTT/htu"

#if defined(ESP32)
#  if !defined(I2C_SDA) || !defined(I2C_SCL)
#    define I2C_SDA 16
#    define I2C_SCL 0
#  endif
#endif

#endif