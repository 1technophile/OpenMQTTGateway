/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the DHT11/22 sensor
  
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

/*-------------------PIN DEFINITIONS----------------------*/
#ifdef I2C_Wiring // With Support for I2C Modules
  #define DHT_RECEIVER_PIN 14 //on nodeMCU this is D5 GPIO14
#endif

#ifdef Classic_Wiring // Without Support for I2C Modules
  #define DHT_RECEIVER_PIN 0 //on nodeMCU this is D3 GPIO0
#endif

#ifdef RFM69_Wiring // Without Support for I2C Modules and HM10 or 11
  #define DHT_RECEIVER_PIN 0 //on nodeMCU this is D3 GPIO0
#endif

