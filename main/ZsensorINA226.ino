/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    INA226 reading Addon
  
    From the orgiginal work of Matthias Busse http://shelvin.de/ein-batteriemonitor-fuer-strom-und-spannung-mit-dem-ina226-und-dem-arduino-uno/
    MQTT add - 1technophile
    
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
// Pinout
// INA226 - Uno - Mega - NODEMCU
// SCL    - A5  - 21 - D1
// SDA    - A4  - 20 - D2
//
#include "User_config.h"

#ifdef ZsensorINA226
#include <Wire.h>

float rShunt = 0.1;           // Shunt Widerstand festlegen, hier 0.1 Ohm
const int INA226_ADDR = 0x40; // A0 und A1 auf GND > Adresse 40 Hex auf Seite 18 im Datenblatt

//Time used to wait for an interval before resending temp and hum
unsigned long timeINA226 = 0;

void setupINA226()
{
  Wire.begin();
  // Configuration Register Standard Einstellung 0x4127, hier aber 16 Werte Mitteln > 0x4427
  writeRegister(0x00, 0x4427); // 1.1ms Volt und Strom A/D-Wandlung, Shunt und VBus continous
}

void MeasureINA226()
{
  if (millis() > (timeINA226 + TimeBetweenReadingINA226))
  { //retrieving value of temperature and humidity of the box from DHT every xUL
    timeINA226 = millis();
    Log.trace(F("Creating INA226 buffer" CR));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject &INA226data = jsonBuffer.createObject();
    // Topic on which we will send data
    Log.trace(F("Retrieving electrical data" CR));
    // Bus Spannung, read-only, 16Bit, 0...40.96V max., LSB 1.25mV
    float volt = readRegister(0x02) * 0.00125;
    // Seite 24: Shunt Spannung +- 81,92mV mit 16 Bit, LSB 2,5uV
    int shuntvolt = readRegister(0x01);
    if (shuntvolt && 0x8000)
    {                         // eine negative Zahl? Dann 2er Komplement bilden
      shuntvolt = ~shuntvolt; // alle Bits invertieren
      shuntvolt += 1;         // 1 dazuzählen
      shuntvolt *= -1;        // negativ machen
    }
    float current = shuntvolt * 0.0000025 / rShunt; // * LSB / R
    float power = abs(volt * current);

    char volt_c[7];
    char current_c[7];
    char power_c[7];
    dtostrf(volt, 6, 3, volt_c);
    dtostrf(current, 6, 3, current_c);
    dtostrf(power, 6, 3, power_c);
    INA226data.set("volt", (char *)volt_c);
    INA226data.set("current", (char *)current_c);
    INA226data.set("power", (char *)power_c);
    pub(subjectINA226toMQTT, INA226data);
  }
}

static void writeRegister(byte reg, word value)
{
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}

static word readRegister(byte reg)
{
  word res = 0x0000;
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() == 0)
  {
    if (Wire.requestFrom(INA226_ADDR, 2) >= 2)
    {
      res = Wire.read() * 256;
      res += Wire.read();
    }
  }
  return res;
}

#endif
