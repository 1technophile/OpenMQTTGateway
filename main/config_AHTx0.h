/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 
 
   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker
   Send and receiving command by MQTT
   
   This is the Climate Addon:
   - Measures Temperature, Humidity
   - Generates Values for: Temperature in degrees C and F, Humidity in %
   - Required Hardware Module: AHTX0 (AHT10 & AHT20)
   - Required Library: Adafruit AHTX0 by Adafruit

   Connection Schemata:
   --------------------

   AHT10 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> 3v3 (3V)
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
#ifndef config_AHTx0_h
#define config_AHTx0_h

extern void setupZsensorAHTx0();
extern void MeasureAHTTempHum();

#define AHTx0_always            true // if false when the current value of the parameter is the same as previous one don't send it by MQTT
#define TimeBetweenReadingAHTx0 30000

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define AHTTOPIC "/CLIMAtoMQTT/aht"

#if defined(ESP32)
#  if !defined(AHT_I2C_SDA) || !defined(AHT_I2C_SCL)
#    define AHT_I2C_SDA 16
#    define AHT_I2C_SCL 0
#  endif
#endif

#endif