/*
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

   This is the Climate Addon:
   - Measures Temperature and Pressure
   - Generates Values for: Temperature in degrees C and F, Pressure in Pa, Altitude in Meter and Feet
   - Required Hardware Module: Bosch BMP390
   - Required Library: Adafruit BMP3XX

   Connection Schemata:
   --------------------

   BMP390 ------> ESP8266
   =====================================================
   Vcc ----------------> 5V/3.3V    (5V or 3.3V depends on the BMP390 board variant)
   GND ----------------> GND
   SCL ----------------> D1
   SDA ----------------> D2

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

#ifdef ZsensorBMP390
#  include <stdint.h>

#  include "Adafruit_BMP3XX.h"
#  include "TheengsCommon.h"
#  include "Wire.h" // Library for communication with I2C / TWI devices
#  include "config_BMP390.h"

//Time used to wait for an interval before resending measured values
static unsigned long timebmp390 = 0;
static uint8_t BMP390_i2c_addr = BMP390_I2C_ADDR1; // Default I2C address for BMP390 is 0x77, alternate is 0x76
static Adafruit_BMP3XX bmp;

static inline uint8_t mapOversample(int v) {
  // Values:
  // -------------------------
  //  0, skipped
  //  1 through 5, oversampling *2, *4, *8, *16, *32 respectively (BMP3XX does not expose 1x)
  switch (v) {
    case 0: return BMP3_NO_OVERSAMPLING;
    case 1: return BMP3_OVERSAMPLING_2X;
    case 2: return BMP3_OVERSAMPLING_4X;
    case 3: return BMP3_OVERSAMPLING_8X;
    case 4: return BMP3_OVERSAMPLING_16X;
    case 5: return BMP3_OVERSAMPLING_32X;
    default: return BMP3_OVERSAMPLING_4X;
  }
}

void setupZsensorBMP390() {
  // Allow custom pins on ESP Platforms
  Wire.begin(BMP390_PIN_SDA, BMP390_PIN_SCL);

  THEENGS_LOG_NOTICE(F("Setup BMP390 on address: %X" CR), BMP390_i2c_addr);

  delay(10); // Gives the Sensor enough time to turn on (The BMP390 requires 2ms to start up)

  bool ok = bmp.begin_I2C(BMP390_I2C_ADDR1);
  if (ok) {
    BMP390_i2c_addr = BMP390_I2C_ADDR1;
  } else {
    ok = bmp.begin_I2C(BMP390_I2C_ADDR2);
    if (ok) {
      BMP390_i2c_addr = BMP390_I2C_ADDR2;
    }
  }

  if (!ok) {
    THEENGS_LOG_NOTICE(F("Bosch BMP390 failed (not found at 0x76/0x77)" CR));
    return;
  }

  //***Operation settings*****************************//

  // tempOverSample - Values:
  // ------------------------
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  bmp.setTemperatureOversampling(mapOversample(BMP390TemperatureOversample));

  // pressOverSample - Values:
  // -------------------------
  //  0, skipped
  //  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
  bmp.setPressureOversampling(mapOversample(BMP390PressureOversample));

  // Filter can be off or number of FIR coefficients - Values:
  // ---------------------------------------------------------
  //  0, filter off
  //  1, coefficients = 2
  //  2, coefficients = 4
  //  3, coefficients = 8
  //  4, coefficients = 16
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);

  // Output Data Rate - Values:
  // --------------------------
  //  BMP3_ODR_xx_HZ (see Adafruit_BMP3XX.h)
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  THEENGS_LOG_NOTICE(F("Bosch BMP390 successfully initialized: %X" CR), BMP390_i2c_addr);
}

void MeasureTempAndPressureBMP390() {
  if (millis() > (timebmp390 + TimeBetweenReadingbmp390)) {
    timebmp390 = millis();
    static float persisted_bmp_tempc;
    static float persisted_bmp_tempf;
    static float persisted_bmp_pa;
    static float persisted_bmp_altim;
    static float persisted_bmp_altift;

    bool read_ok = bmp.performReading();
    float BmpTempC = NAN;
    float BmpTempF = NAN;
    float BmpPa = NAN;
    float BmpAltiM = NAN;
    float BmpAltiFt = NAN;

    if (read_ok) {
      BmpTempC = bmp.temperature + (float)BMP390Correction;
      BmpTempF = (BmpTempC * 9.0f / 5.0f) + 32.0f;
      BmpPa = bmp.pressure;
      BmpAltiM = bmp.readAltitude((float)BMP390_SEALEVEL_HPA);
      BmpAltiFt = BmpAltiM * 3.280839895f;
    }

    // Check if reads failed and exit early (to try again).
    if (!read_ok || isnan(BmpTempC) || isnan(BmpTempF) || isnan(BmpPa) || isnan(BmpAltiM) || isnan(BmpAltiFt)) {
      THEENGS_LOG_ERROR(F("Failed to read from BMP390!" CR));
    } else {
      THEENGS_LOG_TRACE(F("Creating BMP390 buffer" CR));
      StaticJsonDocument<JSON_MSG_BUFFER> BMP390dataBuffer;
      JsonObject BMP390data = BMP390dataBuffer.to<JsonObject>();
      // Generate Temperature in degrees C
      if (BmpTempC != persisted_bmp_tempc || bmp390_always) {
        BMP390data["tempc"] = (float)BmpTempC;
      } else {
        THEENGS_LOG_TRACE(F("Same Degrees C don't send it" CR));
      }

      // Generate Temperature in degrees F
      if (BmpTempF != persisted_bmp_tempf || bmp390_always) {
        BMP390data["tempf"] = (float)BmpTempF;
      } else {
        THEENGS_LOG_TRACE(F("Same Degrees F don't send it" CR));
      }

      // Generate Pressure in Pa
      if (BmpPa != persisted_bmp_pa || bmp390_always) {
        BMP390data["pa"] = (float)BmpPa;
      } else {
        THEENGS_LOG_TRACE(F("Same Pressure don't send it" CR));
      }

      // Generate Altitude in Meter
      if (BmpAltiM != persisted_bmp_altim || bmp390_always) {
        THEENGS_LOG_TRACE(F("Sending Altitude Meter to MQTT" CR));
        BMP390data["altim"] = (float)BmpAltiM;
      } else {
        THEENGS_LOG_TRACE(F("Same Altitude Meter don't send it" CR));
      }

      // Generate Altitude in Feet
      if (BmpAltiFt != persisted_bmp_altift || bmp390_always) {
        BMP390data["altift"] = (float)BmpAltiFt;
      } else {
        THEENGS_LOG_TRACE(F("Same Altitude Feet don't send it" CR));
      }
      BMP390data["origin"] = BMP390TOPIC;
      enqueueJsonObject(BMP390data);
    }

    persisted_bmp_tempc = BmpTempC;
    persisted_bmp_tempf = BmpTempF;
    persisted_bmp_pa = BmpPa;
    persisted_bmp_altim = BmpAltiM;
    persisted_bmp_altift = BmpAltiFt;
  }
}

#endif
