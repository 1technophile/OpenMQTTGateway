/*
  Based on:
  https://how2electronics.com/measure-co2-tvoc-using-ccs811-gas-sensor-arduino/
  and ZsenzorBME280.ino
*/
#include "User_config.h"

#ifdef ZsensorCCS811
# include <stdint.h>

# include "Adafruit_CCS811.h"
# include "Wire.h" // Library for communication with I2C / TWI devices

Adafruit_CCS811 ccs;

void setupZsensorCCS811() {
#  if defined(ESP8266) || defined(ESP32)
  // Allow custom pins on ESP Platforms
  Wire.begin(CCS811_PIN_SDA, CCS811_PIN_SCL);
#  else
  Wire.begin();
#  endif

  Serial.begin(115200);
  Serial.println("CCS811 test..."); 
  if(!ccs.begin()){
    Serial.println("Failed to start CCS811 sensor!.");
   }
  // Don't wait for the sensor to be ready.
}
 
void MeasureCO2AndTVOC() {
  if(ccs.available()){
    if(!ccs.readData()){
      if (millis() > (timeccs811 + TimeBetweenReadingccs811)) {
        timeccs811 = millis();
        static float persisted_ccs_co2;
        static float persisted_ccs_tvoc;
    
        float CcsCO2 = ccs.geteCO2();
        float CcsTVOC = ccs.getTVOC();
    
        // Check if reads failed and exit early (to try again).
        if (isnan(CcsCO2) || isnan(CcsTVOC)) {
          Log.error(F("Failed to read from CCS811!" CR));
        } else {
          Log.trace(F("Creating CCS811 buffer" CR));
          StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
          JsonObject& CCS811data = jsonBuffer.createObject();
          // Generate CO2
          if (CcsCO2 != persisted_ccs_co2 || ccs811_always) {
            CCS811data.set("co2", (float)CcsCO2);
          } else {
            Log.trace(F("Same CO2 don't send it" CR));
          }
          // Generate TVOC
          if (CcsTVOC != persisted_ccs_tvoc || ccs811_always) {
            CCS811data.set("tvoc", (float)CcsTVOC);
          } else {
            Log.trace(F("Same TVOC don't send it" CR));
          }
          if (CCS811data.size() > 0)
            pub(CCSTOPIC, CCS811data);
        } 
      persisted_ccs_co2 = CcsCO2;
      persisted_ccs_tvoc = CcsTVOC;     
      }
    }
    else{
      Serial.println("ERROR!");
      while(1);
    }
  }
}

#endif