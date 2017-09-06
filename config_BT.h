/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the bluetooth low energy gateway (beacons detection)
  
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
/*----------------------BT topics & parameters-------------------------*/
#define subjectBTtoMQTT "home/BTtoMQTT/"
#define TimeBtw_Read 10000 //define the time between 2 scans
#define HM-10 
//#define HM-11 // uncomment this line if you use HM-11 and comment the line above

/*-------------------PIN DEFINITIONS----------------------*/
#ifdef ESP8266
  #define BT_RX 13 //D7 ESP8266 RX connect HM-10 or 11 TX
  #define BT_TX 12 //D6 ESP8266 TX connect HM-10 or 11 RX
#else
  #define BT_RX 5 //arduino RX connect HM-10 or 11 TX
  #define BT_TX 6 //arduino TX connect HM-10 or 11 RX
#endif
