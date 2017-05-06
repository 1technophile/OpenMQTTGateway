/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal/BLE  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - publish MQTT data to a different topic related to BLE devices rssi signal

  Copyright: (c)1technophile

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

Thanks to wolass https://github.com/wolass for suggesting me HM 10 and dinosd https://github.com/dinosd/BLE_PROXIMITY for inspiring me how to implement the gateway
*/
#ifdef ZgatewayBT
#include <SoftwareSerial.h>

#define STRING_MSG "OK+DISC:"
#define QUESTION_MSG "AT+DISI?"
#define RESPONSE_MSG "OK+DISIS"
#define SETUP_MSG "OK+RESET"
#define TimeBtw_Read 10000

SoftwareSerial softserial(BT_RX, BT_TX);

unsigned long time1 = 0;

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
      trc(F("Sending BT data to MQTT"));
      discResult.replace(RESPONSE_MSG,"");
      int device_number = discResult.length()/78;     
      for (int i=0;i<device_number;i++){
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
           #ifdef ESP8266
            yield();
           #endif
        }
        return true;
      }
    if (discResult.indexOf(SETUP_MSG)>=0)
    {
      trc(F("Connection OK to HM-10"));
    }
  }
  if (millis() > (time1 + TimeBtw_Read)) {//retriving value of adresses and rssi
       time1 = millis();
       softserial.print(F(QUESTION_MSG));
  }
  return false;
}
#endif
