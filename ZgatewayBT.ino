/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send IR signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received IR signal

  Copyright: (c)1technophile

IMPORTANT NOTE: On arduino connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/IRremoteInt.h so as to free pin D3 for RF RECEIVER PIN
  
Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifdef ZgatewayBT
#include <SoftwareSerial.h>

#define STRING_MSG "OK+DISC"
#define QUESTION_MSG "AT+DISI?"
#define RESPONSE_MSG "OK+DISIS"

//SoftwareSerial softserial(D7, D6);
SoftwareSerial softserial(10, 11);
unsigned long time1 = 0;

void setupBT() {
  Serial.begin(57600);
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

    String response = softserial.readString();
    Serial.println(response);
    if (response.indexOf(STRING_MSG)>=0)
    {
      response.replace(RESPONSE_MSG,"");
      int device_number = response.length()/78;
      Serial.println(device_number);
      
      for (int i=0;i<device_number;i++){
           String onedevice = response.substring(0,78);
           onedevice.replace(STRING_MSG,"");
           Serial.println(onedevice);
           String company = onedevice.substring(1,9);
           Serial.println(company);
           String uuid = onedevice.substring(10,42);
           Serial.println(uuid);
           String others = onedevice.substring(43,53);
           Serial.println(others);
           String mac = onedevice.substring(54,66);
           Serial.println(mac);
           String rssi = onedevice.substring(67,71);
           Serial.println(rssi);
           response = response.substring(78);
        }
      }
  }
  if (millis() > (time1 + 10000)) {//retriving value of adresses and rssi
       time1 = millis();
       softserial.print(F(QUESTION_MSG));
  }
}
#endif
