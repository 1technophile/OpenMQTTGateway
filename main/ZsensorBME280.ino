/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This is the Climate Addon:
   - Measures Temperature, Humidity and Pressure
   - Generates Values for: Temperature in degrees C and F, Humidity in %, Pressure in Pa, Altitude in Meter and Feet
   - Required Hardware Module: Bosch BME280
   - Required Library: SparkFun BME280 Library v1.1.0 by Marshall Taylor

   Connection Schemata:
   --------------------

   bme280 ------> Arduino Uno ----------> ESP8266
   ==============================================
   Vcc ---------> 5V -------------------> Vu (5V)
   GND ---------> GND ------------------> GND
   SCL ---------> Pin A5 ---------------> D1
   SDA ---------> Pin A4 ---------------> D2
 
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
*/
#include "User_config.h"

#ifdef ZsensorBME280
#  include <stdint.h>

#  include "SparkFunBME280.h"
#  include "Wire.h" // Library for communication with I2C / TWI devices

//Global sensor object
BME280 mySensor;

void setupZsensorBME280() {
  mySensor.settings.commInterface = I2C_MODE;
  mySensor.settings.I2CAddress = BME280_i2c_addr;
  Log.notice(F("Setup BME280 on adress: %H" CR), BME280_i2c_addr);
  //***Operation settings*****************************//

  // runMode Setting - Values:
  // -------------------------
  //  0, Sleep mode
  //  1 or 2, Forced mode
  //  3, Normal mode
  mySensor.settings.runMode = 3; //Normal mode

  // tStandby Setting - Values:
  // --------------------------
  //  0, 0.5ms
  //  1, 62.5ms
  //  2, 125ms
  //  3, 250ms
  //  4, 500ms
  //  5, 1000ms
  //  6, 10ms
  //  7, 20ms
  mySensor.settings.tStandby = 1;

  // Filter can be off or number of FIR coefficients - Values:
  // ---------------------------------------------------------
  //  0, filter off
  //  1, coefficients = 2
  //  2, coefficients = 4
  //  3, coefficients = 8
  //  4, coefficients = 16
  mySensor.settings.filter = 4;

  // tempOverSample - Values:
  // ------------------------
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.tempOverSample = 1;

  // pressOverSample - Values:
  // -------------------------
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.pressOverSample = 1;

  // humidOverSample - Values:
  // -------------------------
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  mySensor.settings.humidOverSample = 1;

  delay(10); // Gives the Sensor enough time to turn on (The BME280 requires 2ms to start up)
  Log.notice(F("Bosch BME280 Initialized - Result of .begin(): 0x %h" CR), mySensor.begin());
}

void MeasureTempHumAndPressure() {
  if (millis() > (timebme280 + TimeBetweenReadingbme280)) {
    timebme280 = millis();
    static float persisted_bme_tempc;
    static float persisted_bme_tempf;
    static float persisted_bme_hum;
    static float persisted_bme_pa;
    static float persisted_bme_altim;
    static float persisted_bme_altift;

    float BmeTempC = mySensor.readTempC();
    float BmeTempF = mySensor.readTempF();
    float BmeHum = mySensor.readFloatHumidity();
    float BmePa = mySensor.readFloatPressure();
    float BmeAltiM = mySensor.readFloatAltitudeMeters();
    float BmeAltiFt = mySensor.readFloatAltitudeFeet();

    // Check if reads failed and exit early (to try again).
    if (isnan(BmeTempC) || isnan(BmeTempF) || isnan(BmeHum) || isnan(BmePa) || isnan(BmeAltiM) || isnan(BmeAltiFt)) {
      Log.error(F("Failed to read from BME280!" CR));
    } else {
      Log.trace(F("Creating BME280 buffer" CR));
      StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject& BME280data = jsonBuffer.createObject();
      // Generate Temperature in degrees C
      if (BmeTempC != persisted_bme_tempc || bme280_always) {
        BME280data.set("tempc", (float)BmeTempC);
      } else {
        Log.trace(F("Same Degrees C don't send it" CR));
      }

      // Generate Temperature in degrees F
      if (BmeTempF != persisted_bme_tempf || bme280_always) {
        BME280data.set("tempf", (float)BmeTempF);
      } else {
        Log.trace(F("Same Degrees F don't send it" CR));
      }

      // Generate Humidity in percent
      if (BmeHum != persisted_bme_hum || bme280_always) {
        BME280data.set("hum", (float)BmeHum);
      } else {
        Log.trace(F("Same Humidity don't send it" CR));
      }

      // Generate Pressure in Pa
      if (BmePa != persisted_bme_pa || bme280_always) {
        BME280data.set("pa", (float)BmePa);
      } else {
        Log.trace(F("Same Pressure don't send it" CR));
      }

      // Generate Altitude in Meter
      if (BmeAltiM != persisted_bme_altim || bme280_always) {
        Log.trace(F("Sending Altitude Meter to MQTT" CR));
        BME280data.set("altim", (float)BmeAltiM);
      } else {
        Log.trace(F("Same Altitude Meter don't send it" CR));
      }

      // Generate Altitude in Feet
      if (BmeAltiFt != persisted_bme_altift || bme280_always) {
        BME280data.set("altift", (float)BmeAltiFt);
      } else {
        Log.trace(F("Same Altitude Feet don't send it" CR));
      }
      if (BME280data.size() > 0)
        pub(BMETOPIC, BME280data);
    }

    persisted_bme_tempc = BmeTempC;
    persisted_bme_tempf = BmeTempF;
    persisted_bme_hum = BmeHum;
    persisted_bme_pa = BmePa;
    persisted_bme_altim = BmeAltiM;
    persisted_bme_altift = BmeAltiFt;
  }
}

#endif
