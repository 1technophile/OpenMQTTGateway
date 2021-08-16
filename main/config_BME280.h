/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the BME280 sensor

    Copyright: (c) Hans-Juergen Dinges  
    
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

   Connection Schemata:
   --------------------

   BH1750 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> Vu (5V)
   GND ---------> GND ------------------> GND
   SCL ---------> Pin A5 ---------------> D1
   SDA ---------> Pin A4 ---------------> D2
   ADD ---------> N/C (Not Connected) --> N/C (Not Connected)

*/
#ifndef config_BME280_h
#define config_BME280_h

extern void setupBME280();
extern void BME280toMQTT();

#ifndef bme280_always
#  define bme280_always true // if false when the current value of the parameter is the same as previous one don't send it by MQTT
#endif
#ifndef TimeBetweenReadingbme280
#  define TimeBetweenReadingbme280 30000
#endif

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#ifndef BMETOPIC
#  define BMETOPIC "/CLIMAtoMQTT/bme"
#endif

//Time used to wait for an interval before resending measured values
unsigned long timebme280 = 0;
int BME280_i2c_addr = 0x76; // Bosch BME280 I2C Address

// Only supported for ESP
int BME280_PIN_SDA = SDA; // PIN SDA
int BME280_PIN_SCL = SCL; // PIN SCL

// Temperature correction for BME280 devices

#ifndef BME280Correction

// BME280Correction - Correction in celcius of temperature reported by bme280 sensor.  Both Celcius and Farenheit temperatures are adjusted.
// -------------------------
// Value is a float
// ie Compiler Directive '-DBME280Correction=-3.4'

#  define BME280Correction 0
#endif

#endif
