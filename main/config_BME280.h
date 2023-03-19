/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker
   Send and receiving command by MQTT

   This files enables you to set parameters for the BME280 or BMP280 sensors.

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

   BME280/BMP280 ------> Arduino Uno ----------> ESP8266
   =====================================================
   Vcc ----------------> 5V/3.3V     ----------> 5V/3.3V    (5V or 3.3V depends on the BME280/BMP280 board variant)
   GND ----------------> GND         ----------> GND
   SCL ----------------> Pin A5      ----------> D1
   SDA ----------------> Pin A4      ----------> D2

*/
#ifndef config_BME280_h
#define config_BME280_h

extern void setupZsensorBME280();
extern void MeasureTempHumAndPressure();

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
#ifndef BME280_PIN_SDA
#  define BME280_PIN_SDA SDA
#endif
#ifndef BME280_PIN_SCL
#  define BME280_PIN_SCL SCL
#endif

// Oversampling for BME280/BMP280 devices

#ifndef BME280TemperatureOversample
// BME280TemperatureOversample - Values:
// ------------------------
//  0, skipped
//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
#  define BME280TemperatureOversample 1
#endif

#ifndef BME280PressureOversample
// BME280PressureOversample - Values:
// -------------------------
//  0, skipped
//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
#  define BME280PressureOversample 1
#endif

#ifndef BME280HumidityOversample
// BME280HumidityOversample - Values:
// -------------------------
//  0, skipped
//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
#  define BME280HumidityOversample 1
#endif

// Temperature correction for BME280/BMP280 devices

#ifndef BME280Correction
// BME280Correction - Correction in Celsius of temperature reported by BME280/BMP280 sensor. Both Celsius and Fahrenheit temperatures are adjusted.
// -------------------------
// Value is a float
// ie Compiler Directive '-DBME280Correction=-3.4'
#  define BME280Correction 0
#endif

#endif
