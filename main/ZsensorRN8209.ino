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

float voltage = 0;
float current = 0;
float power = 0;

unsigned long PublishingTimerRN8209 = 0;

void rn8209_loop(void* mode) {
  while (1) {
    uint32_t temp_voltage = 0;
    uint8_t retv = rn8209c_read_voltage(&temp_voltage);
    uint8_t ret = rn8209c_read_emu_status();
    uint8_t retc = 1;
    uint8_t retp = 1;
    static float previousPower = 0;
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
      if (retc == 0) {
        current = current / 10000.0;
        overLimitCurrent(current);
      }
    }
    StaticJsonDocument<128> doc;
    JsonObject data = doc.to<JsonObject>();
    if (retc == 0) {
      data["current"] = current;
    }
    if (retv == 0) {
      voltage = (float)temp_voltage / 1000.0;
      data["volt"] = voltage;
    }
    if (retp == 0) {
      power = power / 10000.0;
      data["power"] = power;
    }
    unsigned long now = millis();
    if ((now > (PublishingTimerRN8209 + TimeBetweenPublishingRN8209) ||
         !PublishingTimerRN8209 ||
         (abs(power - previousPower) > previousPower * PreviousPowerThreshold) && abs(power - previousPower) > MinPowerThreshold) &&
        !ProcessLock) {
      PublishingTimerRN8209 = now;
      previousPower = power;
      if (data) pub(subjectRN8209toMQTT, data);
    }
    delay(TimeBetweenReadingRN8209);
  }
}

void setupRN8209() {
  STU_8209C cal = {0};
  cal.Ku = RN8209_KU;
  cal.Kia = RN8209_KIA;
  cal.EC = RN8209_EC;
  set_user_param(cal);
  init_8209c_interface();
  xTaskCreate(rn8209_loop, "rn8209_loop", RN8209_TASK_STACK_SIZE, NULL, 10, NULL);
  Log.trace(F("ZsensorRN8209 setup done " CR));
}

#endif // ZsensorRN8209