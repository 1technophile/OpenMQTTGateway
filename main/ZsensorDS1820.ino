/*  
  OpenMQTTGateway Addon  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR/BLE signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    DS1820 1-wire temperature sensor addon
  
    Copyright: (c) 2020 Lars Wessels
  
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

#ifdef ZsensorDS1820
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire owbus(DS1820_OWBUS_PIN);
DallasTemperature ds1820(&owbus);
DeviceAddress ds1820_address, ds1820_devices[OW_MAX_SENSORS];

static uint8_t ds1820_count = 0;
static uint8_t ds1820_resolution[OW_MAX_SENSORS];
static String ds1820_type[OW_MAX_SENSORS];
static String ds1820_addr[OW_MAX_SENSORS];


void setupZsensorDS1820()
{
  trc(String("DS1820: configured pin ") + DS1820_OWBUS_PIN + String(" for 1-wire bus"));
  ds1820.begin();

  // locate device(s) on 1-wire bus
  trc("DS1820: Found " + String(ds1820.getDeviceCount(), DEC) + String(" devices."));

  // determine device addresses on 1-wire bus
  if (owbus.search(ds1820_address)) 
  {
    do {
      // we only care about Dallas DS18X20 1-wire devices
      if (ds1820_address[0] == 0x10 || ds1820_address[0] == 0x28 || 
          ds1820_address[0] == 0x22 || ds1820_address[0] == 0x3B) 
      {
        ds1820_addr[ds1820_count] = String("0x");
        for (uint8_t i = 0; i < 8; i++)
        {  
          if (ds1820_address[i] < 0x10) ds1820_addr[ds1820_count] += String("0");
          ds1820_devices[ds1820_count][i] = ds1820_address[i];
          ds1820_addr[ds1820_count] += String(ds1820_address[i], HEX);
        }          
        
        // set the resolution and thus conversion timing
        if (ds1820_address[0] == 0x10) 
        {
          // DS1820/DS18S20 have no resolution configuration register,
          // both have a 9-bit temperature register. Resolutions greater 
          // than 9-bit can be calculated using the data from the temperature, 
          // and COUNT REMAIN and COUNT PER °C registers in the scratchpad.
          // The resolution of the calculation depends on the model.
          ds1820_type[ds1820_count] = String("DS1820/DS18S20"); 
        } 
        else if (ds1820_address[0] == 0x28) 
        {
          // DS18B20 and 18B22 are capable of different resolutions (9-12 bit)
          ds1820_type[ds1820_count] = String("DS18B20");
          ds1820.setResolution(ds1820_address, DS1820_RESOLUTION);
        } 
        else if (ds1820_address[0] == 0x22)
        {
          ds1820_type[ds1820_count] = String("DS1822");
          ds1820.setResolution(ds1820_address, DS1820_RESOLUTION);
        } 
        else 
        {
          ds1820_type[ds1820_count] = String("DS1825");
          ds1820.setResolution(ds1820_address, DS1820_RESOLUTION);
        }  
        ds1820_resolution[ds1820_count] = ds1820.getResolution(ds1820_address);
        trc(String("DS1820: Device ") + ds1820_count 
          + String(", Type ") + ds1820_type[ds1820_count] 
          + String(", Address ") + ds1820_addr[ds1820_count] 
          + String(", Resolution ") + ds1820_resolution[ds1820_count]);
        ds1820_count++;
      }       
    } while (owbus.search(ds1820_address));
    
  } 
  else
  {  
    trc(F("DS1820: Failed to enumerate sensors on 1-wire bus. Check your pin assignment!"));
  }

  // make requestTemperatures() non-blocking
  // we've to take that conversion is triggered some time before reading temperature values!
  ds1820.setWaitForConversion(false);
}

void MeasureDS1820Temp()
{
  static float persisted_temp[OW_MAX_SENSORS];
  static unsigned long timeDS1820 = 0;
  static boolean triggeredConversion = false;
  float current_temp[OW_MAX_SENSORS];
  
  // trigger temperature conversion some time before actually 
  // calling getTempC() to make reading temperatures non-blocking
  if (!triggeredConversion && (millis() > (timeDS1820 + (DS1820_INTERVAL_SEC*1000) - DS1820_CONV_TIME)))
  {
    trc(F("DS1820: Trigger temperature conversion..."));
    ds1820.requestTemperatures();
    triggeredConversion = true;
  }
  else if (triggeredConversion && (millis() > (timeDS1820 + (DS1820_INTERVAL_SEC*1000))))
  {
    timeDS1820 = millis();
    triggeredConversion = false;
    
    if (ds1820_count < 1)
    {
      trc(F("DS1820: Failed to identify any temperature sensors on 1-wire bus during setup!"));
    }
    else
    {
      trc(String("DS1820: Reading temperature(s) from ") + ds1820_count + String(" sensor(s)..."));
      StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
      JsonObject &DS1820data = jsonBuffer.createObject();

      for (uint8_t i=0; i < ds1820_count; i++)
      {
        current_temp[i] = round(ds1820.getTempC(ds1820_devices[i])*10)/10.0;
        if (current_temp[i] == -127) 
        {
          trc(String("DS1820: Device ") + ds1820_addr[i] + String(" currently disconnected!"));
        }
        else if (DS1820_ALWAYS || current_temp[i] != persisted_temp[i])
        {
          if (DS1820_FAHRENHEIT) {
            trc(String("DS1820: Temperature (") + ds1820_addr[i] + String("): ") 
                  + DallasTemperature::toFahrenheit(current_temp[i]) + String(" F"));
            DS1820data.set("temp", (float)DallasTemperature::toFahrenheit(current_temp[i]));
            DS1820data.set("unit", "F");
          } else {
            trc(String("DS1820: Temperature (") + ds1820_addr[i] 
                  + String("): ") + current_temp[i] + String(" C"));
            DS1820data.set("temp", (float)current_temp[i]);
            DS1820data.set("unit", "C");
          }
          if (DS1820_DETAILS) {          
            DS1820data.set("type", ds1820_type[i]);
            DS1820data.set("res", ds1820_resolution[i] + String("bit"));
            DS1820data.set("addr", ds1820_addr[i]);
          }
          pub(OW_TOPIC, DS1820data);
          delay(10);
        }
        else 
        {
          trc(String("DS1820: Temperature for device ") + ds1820_addr[i] + String(" didn't change, don't publish it."));
        }
        persisted_temp[i] = current_temp[i];
      }
    }
  }
}
#endif
