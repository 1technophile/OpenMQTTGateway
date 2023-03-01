/*  
  OpenMQTTGateway Addon  - ESP8266, ESP32 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
    RN8209 reading Addon
    
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
#ifdef ZsensorRN8209

#  include "ArduinoJson.h"
#  include "driver/uart.h"
#  include "rn8209_flash.h"
#  include "rn8209c_user.h"

extern "C" bool init_8209c_interface();

StaticJsonDocument<JSON_MSG_BUFFER> doc;

// GetCurrent function for critical operation like overcurrent protection
float getRN8209current() {
  uint8_t ret = rn8209c_read_emu_status();
  if (ret) {
    int32_t current;
    uint32_t temp_current = 0;
    rn8209c_read_current(phase_A, &temp_current);
    if (ret == 1) {
      current = temp_current;
    } else {
      current = (int32_t)temp_current * (-1);
    }
    return current / 10000.0;
  }
  return 0;
}

void rn8209_loop(void* mode) {
  if (!ProcessLock) {
    uint32_t voltage;
    int32_t current;
    int32_t power;

    while (1) {
      uint8_t retv = rn8209c_read_voltage(&voltage);
      uint8_t ret = rn8209c_read_emu_status();
      uint8_t retc = 1;
      uint8_t retp = 1;
      if (ret) {
        uint32_t temp_current = 0;
        uint32_t temp_power = 0;
        retc = rn8209c_read_current(phase_A, &temp_current);
        retp = rn8209c_read_power(phase_A, &temp_power);
        if (ret == 1) {
          current = temp_current;
          power = temp_power;
        } else {
          current = (int32_t)temp_current * (-1);
          power = (int32_t)temp_power * (-1);
        }
      }

      JsonObject data = doc.to<JsonObject>();
      if (retv == 0) data["volt"] = (float)voltage / 1000.0;
      if (retc == 0) data["current"] = (float)current / 10000.0;
      if (retp == 0) data["power"] = (float)power / 10000.0;
      if (data) pub(subjectRN8209toMQTT, data);
      delay(TimeBetweenReadingRN8209);
    }
  } else {
    Log.trace(F("RN8209 reading canceled by processLock" CR));
  }
}

void setupRN8209() {
  STU_8209C cal = {0};
  cal.Ku = RN8209_KU;
  cal.Kia = RN8209_KIA;
  cal.EC = RN8209_EC;
  set_user_param(cal);
  init_8209c_interface();
  xTaskCreate(rn8209_loop, "rn8209_loop", RN8209_TASK_STACK_SIZE, NULL, RN8209_TASK_PRIO, NULL);
  Log.trace(F("ZsensorRN8209 setup done " CR));
}

#endif // ZsensorRN8209