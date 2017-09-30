/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal/BLE  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - publish MQTT data to a different topic related to BLE devices rssi signal

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

Thanks to wolass https://github.com/wolass for suggesting me HM 10 and dinosd https://github.com/dinosd/BLE_PROXIMITY for inspiring me how to implement the gateway
*/
#ifdef ZgatewayBT
#include <SoftwareSerial.h>

#define STRING_MSG "OK+DISC:"
#define QUESTION_MSG "AT+DISI?"
#define RESPONSE_MSG "OK+DISIS"
#define RESP_END_MSG "OK+DISCE"
#define SETUP_MSG "OK+RESET"

SoftwareSerial softserial(BT_RX, BT_TX);

unsigned long timebt = 0;

void setupBT() {
  softserial.begin(9600);
  softserial.print(F("AT+ROLE1"));
  delay(100);
  softserial.print(F("AT+IMME1"));
  delay(100);
  softserial.print(F("AT+RESET"));
  delay(100);
}

boolean BTtoMQTT() {
  while (softserial.available() > 0) {
     #ifdef ESP8266
      yield();
     #endif
    String discResult = softserial.readString();
    if (discResult.indexOf(STRING_MSG)>=0){
      discResult.replace(RESPONSE_MSG,"");
      discResult.replace(RESP_END_MSG,"");
      float device_number = discResult.length()/78.0;
      if (device_number == (int)device_number){ // to avoid publishing partial values we detect if the serial data has been fully read = a multiple of 78
        trc(F("Sending BT data to MQTT"));
        #ifdef ESP8266
          yield();
        #endif
        for (int i=0;i<(int)device_number;i++){
             String onedevice = discResult.substring(0,78);
             onedevice.replace(STRING_MSG,"");
             /*String company = onedevice.substring(0,8);
             String uuid = onedevice.substring(9,41);
             String others = onedevice.substring(42,52);*/
             String mac = onedevice.substring(53,65);
             String rssi = onedevice.substring(66,70);
             String mactopic = subjectBTtoMQTT + mac;
             trc(mactopic + " " + rssi);
             client.publish((char *)mactopic.c_str(),(char *)rssi.c_str());
             discResult = discResult.substring(78);
          }
          return true;
        }
      }
    if (discResult.indexOf(SETUP_MSG)>=0)
    {
      trc(F("Connection OK to HM-10"));
    }
  }
  if (millis() > (timebt + TimeBtw_Read)) {//retriving value of adresses and rssi
       timebt = millis();
       #ifdef ESP8266
        yield();
       #endif
       softserial.print(F(QUESTION_MSG));
       
  }
  return false;
}
#endif
