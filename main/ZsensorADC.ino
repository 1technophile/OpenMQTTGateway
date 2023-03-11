/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    Analog GPIO reading Addon
  
    Copyright: (c)Florian ROBERT
    
    Contributors:
    - 1technophile
  
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

#ifdef ZsensorADC

#  if defined(ESP8266)
ADC_MODE(ADC_TOUT);
#  endif

//Time used to wait for an interval before resending adc value
unsigned long timeadc = 0;
unsigned long timeadcpub = 0;
void setupADC() {
  Log.notice(F("ADC_GPIO: %d" CR), ADC_GPIO);
}

void MeasureADC() {
  if (millis() > (timeadc + TimeBetweenReadingADC)) { //retrieving value of temperature and humidity of the box from DHT every xUL
#  if defined(ESP8266)
    yield();
    analogRead(ADC_GPIO); //the reliability of the readings can be significantly improved by discarding an initial reading and then averaging the results
#  endif
    timeadc = millis();
    static int persistedadc;
    int val = analogRead(ADC_GPIO);
    int sum_val = val;
    if (NumberOfReadingsADC > 1) { // add extra measurement for accuracy
      for (int i = 1; i < NumberOfReadingsADC; i++) {
        sum_val += analogRead(ADC_GPIO);
      }
      val = sum_val / NumberOfReadingsADC;
    }
    if (isnan(val)) {
      Log.error(F("Failed to read from ADC !" CR));
    } else {
      if (val >= persistedadc + ThresholdReadingADC || val <= persistedadc - ThresholdReadingADC || (MinTimeInSecBetweenPublishingADC > 0 && millis() > (timeadcpub + (MinTimeInSecBetweenPublishingADC * 1000UL)))) {
        timeadcpub = millis();
        Log.trace(F("Creating ADC buffer" CR));
        StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
        JsonObject ADCdata = jsonBuffer.to<JsonObject>();
        ADCdata["adc"] = (int)val;
        if (NumberOfReadingsADC > 1) {
          ADCdata["adc_reads"] = NumberOfReadingsADC;
        }
#  if defined(ADC_DIVIDER)
        float volt = 0;
#    if defined(ESP32)
        // Convert the analog reading (which goes from 0 - 4095) to a voltage (0 - 3.3V):
        volt = val * (3.3 / 4096.0);
#    elif defined(ESP8266)
        // Convert the analog reading (which goes from 0 - 1024) to a voltage (0 - 3.3V):
        volt = val * (3.3 / 1024.0);
#    else
        // Asume 5V and 10bits ADC
        volt = val * (5.0 / 1024.0);
#    endif
        volt *= ADC_DIVIDER;
        // let's give 2 decimal point
        int v = (volt * 100);
        volt = (float)v / 100.0;
        ADCdata["volt"] = (float)volt;
#  endif
#  if defined(ADC_CALIBRATED_SCALE_FACTOR) // calibration: scale factor = ref-Vcal-mV / sum-of-cal-readings @see https://skillbank.co.uk/arduino/calibrate.htm
        float voltage = sum_val * ADC_CALIBRATED_SCALE_FACTOR; // convert to voltage in millivolts
        ADCdata["adc_sum"] = (int)sum_val;
        ADCdata["mvolt_scaled"] = (int)voltage; // real scaled/calibrated value in mV
#  endif
        pub(ADCTOPIC, ADCdata);
        persistedadc = val;
      }
    }
  }
}
#endif
