#include "User_config.h"

#ifdef ZsensorSHTC3
#  include <SparkFun_SHTC3.h>

#  include "TheengsCommon.h"

SHTC3 mySHTC3;

//Time used to wait for an interval before resending temp and hum
unsigned long timedht = 0;
void errorDecoder(SHTC3_Status_TypeDef message);

void errorDecoder(SHTC3_Status_TypeDef message) // The errorDecoder function prints "SHTC3_Status_TypeDef" resultsin a human-friendly way
{
  switch (message) {
    case SHTC3_Status_Nominal:
      THEENGS_LOG_NOTICE(F("Nominal"));
      break;
    case SHTC3_Status_Error:
      THEENGS_LOG_ERROR(F("Error"));
      break;
    case SHTC3_Status_CRC_Fail:
      THEENGS_LOG_ERROR(F("CRC Fail"));
      break;
    default:
      THEENGS_LOG_ERROR(F("Unknown return code"));
      break;
  }
}

void setupSHTC3() {
  Wire.begin();
  errorDecoder(mySHTC3.begin()); // To start the sensor you must call "begin()", the default settings use Wire (default Arduino I2C port)
}

void MeasureTempAndHum() {
  if (millis() > (timedht + TimeBetweenReadingSHTC3)) { //retrieving value of temperature and humidity of the box from SHTC3 every xUL
    timedht = millis();
    static float persistedh;
    static float persistedt;
    SHTC3_Status_TypeDef result = mySHTC3.update();
    if (mySHTC3.lastStatus == SHTC3_Status_Nominal) {
      // Read temperature as Celsius (the default)
      float t = mySHTC3.toDegC();
      float h = mySHTC3.toPercent();
      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t)) {
        THEENGS_LOG_ERROR(F("Failed to read from SHTC3 sensor!" CR));
      } else {
        THEENGS_LOG_TRACE(F("Creating SHTC3 buffer" CR));
        StaticJsonDocument<JSON_MSG_BUFFER> SHTC3dataBuffer;
        JsonObject SHTC3data = SHTC3dataBuffer.to<JsonObject>();
        if (h != persistedh || shtc3_always) {
          SHTC3data["hum"] = (float)h;
        } else {
          THEENGS_LOG_TRACE(F("Same hum don't send it" CR));
        }
        if (t != persistedt || shtc3_always) {
          SHTC3data["tempc"] = (float)t;
          SHTC3data["tempf"] = mySHTC3.toDegF();
        } else {
          THEENGS_LOG_TRACE(F("Same temp don't send it" CR));
        }
        SHTC3data["origin"] = SHTC3TOPIC;
        enqueueJsonObject(SHTC3data);
      }
      persistedh = h;
      persistedt = t;
    } else {
      errorDecoder(mySHTC3.lastStatus);
      THEENGS_LOG_ERROR(F("Failed to read from SHTC3 sensor!" CR));
    }
  }
}
#endif
