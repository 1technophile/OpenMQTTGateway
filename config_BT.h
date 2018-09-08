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
#define subjectBTtoMQTT  Base_Topic Gateway_Name "/BTtoMQTT/"
//#define subjectBTtoMQTTrssi "/rssi" // will be the default parameter on the next release, for the moment rssi is published on mac topic
#define subjectBTtoMQTTrssi ""
#define subjectBTtoMQTTtem "/tem"
#define subjectBTtoMQTThum "/hum"
#define subjectBTtoMQTTmoi "/moi"
#define subjectBTtoMQTTfer "/fer"
#define subjectBTtoMQTTlux "/lux"
#define TimeBtw_Read 55555 //define the time between 2 scans
#define Scan_duration 10 //define the time for a scan
#define HM-10 
//#define HM-11 // uncomment this line if you use HM-11 and comment the line above
#define ZgatewayBT_v6xx // comment if you don't have a firmware version of HM10/11 >= v601
#define delimiter "4f4b2b444953413a"
#define delimiter_length 16

struct decompose
{
  char subject[4];
  int start;
  int len;
  boolean reverse;
  char extract[60];
};
     
/*-------------------PIN DEFINITIONS----------------------*/
#ifdef ESP8266
  #define BT_RX 13 //D7 ESP8266 RX connect HM-10 or 11 TX
  #define BT_TX 12 //D6 ESP8266 TX connect HM-10 or 11 RX
#elif defined(ESP32)
  #define BT_RX 18 // not tested
  #define BT_TX 19 // not tested
#else
  #define BT_RX 5 //arduino RX connect HM-10 or 11 TX
  #define BT_TX 6 //arduino TX connect HM-10 or 11 RX
#endif
