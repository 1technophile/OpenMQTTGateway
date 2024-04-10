/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the RN8209 sensor
  
    Copyright: (c)Florian ROBERT
  
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
#ifndef config_RN8209_h
#define config_RN8209_h

extern void setupRN8209();
extern void RN8209toMQTT();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectRN8209toMQTT "/RN8209toMQTT"

/*-------------CALIBRATION PARAMETERS----------------*/
#ifndef RN8209_KU
#  define RN8209_KU 18570
#endif
#ifndef RN8209_KIA
#  define RN8209_KIA 272433
#endif
#ifndef RN8209_EC
#  define RN8209_EC 28250
#endif

#ifndef TimeBetweenReadingRN8209
#  define TimeBetweenReadingRN8209 500 // time between 2 RN8209 readings in ms
#endif

#ifndef TimeBetweenPublishingRN8209
#  define TimeBetweenPublishingRN8209 60000 // time between 2 RN8209 publishing in ms
#endif
#ifndef MinCurrentThreshold
#  define MinCurrentThreshold 0.1 // (A) Minimum current change that will trigger the publishing of the RN8209 measurements
#endif
#ifndef MinVoltageThreshold
#  define MinVoltageThreshold 2 // (V) Minimum voltage change that will trigger the publishing of the RN8209 measurements
#endif
#endif
