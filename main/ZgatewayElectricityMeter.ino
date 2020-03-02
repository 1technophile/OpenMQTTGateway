/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your MikroMarz electricity meter and a MQTT broker
   Send and receiving command by MQTT

   https://www.mikromarz.com/www-mikromarz-cz

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

#ifndef PRINTER
  #ifdef ESP8266
    #define PRINTER Serial1
  #elif ESP32
    #define PRINTER Serial
  #endif
#endif

#include "User_config.h"

#ifdef ZgatewayElectricityMeter
#define SLEEP_TIME 5000
#define MAX_IDLE 12 
#include <MikromarzMeter.h>
MikromarzMeter mm = MikromarzMeter(SE1_PM2);
byte counter = MAX_IDLE;

void setupElectricityMeter()
{
  mm.setup();
  PRINTER.end();
  #ifdef ESP8266
    PRINTER.begin(115200, SERIAL_8N1, SERIAL_FULL, 1);
  #elif ESP32
    PRINTER.begin(115200);
  #endif
 PRINTER.println(F("ZgatewayElectricityMeter setup done "));
 delay(SLEEP_TIME);
}


void ZgatewayElectricityMetertoMQTT()
{  
  if (mm.readData()) {
    counter = MAX_IDLE;
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(4);    
    tarif tarif_status = mm.getTarif();
    for (byte i=1; i<4; i++) {
      StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
      JsonObject &RFdata = jsonBuffer.createObject();
      RFdata.set("phase", i);
      RFdata.set("power", (long)mm.getPower(i));
      RFdata.set("energy", (long)mm.getEnergy(i, tarif_status));
      RFdata.set("tarif", (tarif_status == TARIF_HIGHT ? "high" : "low"));
      pub(subjectToMQTT, RFdata);
    }
  } else {
    counter--;
  }
  if (counter == 0) {
    ESP.restart();
  }
  delay(SLEEP_TIME);
}
#endif
