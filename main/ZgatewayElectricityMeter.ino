/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your MikroMarz electricity meter and a MQTT broker
   Send and receiving command by MQTT

   https://www.mikromarz.com/www-mikromarz-cz/eshop/51-1-Elektromery/179-2-3-fazove-elektromery/5/690-3-fazovy-2-tarifni-elektromer-SE1-PM2

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

#ifdef ESP8266
  #define PRINTER Serial1
#elif ESP32
  #define PRINTER Serial
#endif

#include "User_config.h"

#ifdef ZgatewayElectricityMeter
#include <MikromarzMeter.h>
MikromarzMeter mm = MikromarzMeter(SE1_PM2);

void setupElectricityMeter()
{
  mm.setup();
  #ifdef ESP8266
    PRINTER.begin(115200, SERIAL_8N1, SERIAL_FULL, 1);
  #elif ESP32
    PRINTER.begin(115200);
  #endif
 PRINTER.println(F("ZgatewayElectricityMeter setup done "));
}


void ZgatewayElectricityMetertoMQTT()
{  
  if (mm.readData()) {
    const int JSON_MSG_CALC_BUFFER = JSON_OBJECT_SIZE(4);    
    tarif tarif_status = mm.getTarif();
    for (byte i=1; i<4; i++) {
      StaticJsonBuffer<JSON_MSG_CALC_BUFFER> jsonBuffer;
      JsonObject &RFdata = jsonBuffer.createObject();
      PRINTER.printf("Power %d: %ld W\n", i, (long)mm.getPower(i));
      PRINTER.printf("Energy %d (%s): %ld kW/h\n", i, 
                     (tarif_status == TARIF_HIGHT ? "high" : "low"),
                     (long)mm.getEnergy(i, tarif_status));
      RFdata.set("phase", i);
      RFdata.set("power", (long)mm.getPower(i));
      RFdata.set("energy", (long)mm.getEnergy(i, tarif_status));
      RFdata.set("tarif", (tarif_status == TARIF_HIGHT ? "high" : "low"));
      pub(subjectToMQTT, RFdata);
    }
  }
  delay(1000);
}
#endif
