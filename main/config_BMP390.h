/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

   This files enables you to set parameters for the BMP390 sensor.

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

   BMP390 ------> ESP8266
   ==============================
   Vcc ----------------> 5V/3.3V    (5V or 3.3V depends on the BMP390 board variant)
   GND ----------------> GND
   SCL ----------------> D1
   SDA ----------------> D2

*/
#ifndef config_BMP390_h
#define config_BMP390_h

extern void setupZsensorBMP390();
extern void MeasureTempAndPressureBMP390();

#ifndef bmp390_always
#  define bmp390_always true // if false when the current value of the parameter is the same as previous one don't send it by MQTT
#endif
#ifndef TimeBetweenReadingbmp390
#  define TimeBetweenReadingbmp390 30000
#endif

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#ifndef BMP390TOPIC
#  define BMP390TOPIC "/CLIMAtoMQTT/bmp390"
#endif

// I2C address options for BMP390 boards (commonly 0x76 or 0x77)
#ifndef BMP390_I2C_ADDR1
#  define BMP390_I2C_ADDR1 0x77
#endif
#ifndef BMP390_I2C_ADDR2
#  define BMP390_I2C_ADDR2 0x76
#endif

// Only supported for ESP
#ifndef BMP390_PIN_SDA
#  define BMP390_PIN_SDA SDA
#endif
#ifndef BMP390_PIN_SCL
#  define BMP390_PIN_SCL SCL
#endif

// Oversampling for BMP390 devices

#ifndef BMP390TemperatureOversample
// BMP390TemperatureOversample - Values:
// ------------------------
//  0, skipped
//  1 through 5, oversampling *2, *4, *8, *16, *32 respectively (BMP3XX does not expose 1x)
#  define BMP390TemperatureOversample 1
#endif

#ifndef BMP390PressureOversample
// BMP390PressureOversample - Values:
// -------------------------
//  0, skipped
//  1 through 5, oversampling *2, *4, *8, *16, *32 respectively (BMP3XX does not expose 1x)
#  define BMP390PressureOversample 1
#endif

// Temperature correction for BMP390 devices

#ifndef BMP390Correction
// BMP390Correction - Correction in Celsius of temperature reported by BMP390 sensor. Both Celsius and Fahrenheit temperatures are adjusted.
// -------------------------
// Value is a float
// ie Compiler Directive '-DBMP390Correction=-3.4'
#  define BMP390Correction 0
#endif

// Sea-level pressure reference in hPa (used to compute altitude)
#ifndef BMP390_SEALEVEL_HPA
#  define BMP390_SEALEVEL_HPA 1013.25f
#endif

#endif
