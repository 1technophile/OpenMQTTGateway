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
#ifndef config_CLOUD_h
#define config_CLOUD_h

/*------------------- Optional Compiler Directives ----------------------*/

#ifndef CLOUDENABLED
#  define CLOUDENABLED false
#endif

#ifndef DEVICETOKEN
#  define DEVICETOKEN ""
#endif

#ifndef CLOUDGATEWAY
#  define CLOUDGATEWAY "https://cloudbeta.openmqttgateway.com/"
#endif

/*------------------- End of Compiler Directives ----------------------*/

#define CLOUD_DEVICE_TOKEN_LENGTH 86

extern void setupCloud();
extern void CloudLoop();
extern void pubOmgCloud(const char*, const char*, bool);
extern void MQTTtoCLOUD(char*, JsonObject&);
extern String stateCLOUDStatus();
extern bool isCloudEnabled();
extern bool isCloudDeviceTokenSupplied();
extern bool setCloudEnabled(bool);
extern bool setCloudDeviceToken(String);

bool cloudActive;

#define RECONNECT_DELAY 35

#define subjectMQTTtoCLOUDset "/commands/MQTTtoCLOUD/config"
#define subjectCLOUDtoMQTT    "/CLOUDtoMQTT"

#define pubCloud(...)         \
  if (cloudActive) {          \
    pubOmgCloud(__VA_ARGS__); \
  }

#endif