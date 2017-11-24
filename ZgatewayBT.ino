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
#define RESPONSE_MSG "OK+DISIS"
#define RESP_END_MSG "OK+DISCE"
#define SETUP_MSG "OK+RESET"

SoftwareSerial softserial(BT_RX, BT_TX);

String returnedString = "";
unsigned long timebt = 0;

// this struct define which parts of the hexadecimal chain we extract and what to do with these parts
struct decompose d[6] = {{"mac",16,12,true},{"typ",28,2,false},{"rsi",30,2,false},{"rdl",32,2,false},{"sty",44,4,true},{"rda",34,60,false}};

void setupBT() {
  softserial.begin(9600);
  softserial.print(F("AT+ROLE1"));
  delay(100);
  softserial.print(F("AT+IMME1"));
  delay(100);
  softserial.print(F("AT+RESET"));
  delay(100);
}

#ifndef ZgatewayBT_stable // published for test only, if you want to test mi flora integration uncomment ZgatewayBT_stable in config_BT
#define QUESTION_MSG "AT+DISA?"
boolean BTtoMQTT() {

  //extract serial data from module in hexa format
  while (softserial.available() > 0) {
      int a = softserial.read();
      if (a < 16) {
        returnedString = returnedString + "0";
      } 
        returnedString = returnedString + String(a,HEX);  
  }

  if (millis() > (timebt + TimeBtw_Read)) {//retriving data
      timebt = millis();
      #ifdef ESP8266
        yield();
      #endif
      if (returnedString != "") {
        size_t pos = 0;
        while ((pos = returnedString.lastIndexOf(delimiter)) != std::string::npos) {
          #ifdef ESP8266
            yield();
          #endif
          String token = returnedString.substring(pos);
          returnedString.remove(pos,returnedString.length() );
          char token_char[token.length()+1];
          token.toCharArray(token_char, token.length()+1);

          for(int i =0; i<6;i++)
          {
            extract_char(token_char,d[i].extract,d[i].start, d[i].len ,d[i].reverse, false);
            if (i == 3) d[5].len = (int)strtol(d[i].extract, NULL, 16) * 2; // extracting the length of the rest data
          }

          if((strlen(d[0].extract)) == 12) // if a mac adress is detected we publish it
          {
              strupp(d[0].extract);
              String mactopic(d[0].extract);
              trc(mactopic);
              mactopic = subjectBTtoMQTT + mactopic;
              int rssi = (int)strtol(d[2].extract, NULL, 16) - 256;
              char val[12];
              sprintf(val, "%d", rssi);
              client.publish((char *)mactopic.c_str(),val);
              if (strcmp(d[4].extract, "fe95") == 0) 
              boolean result = process_miflora_data(d[5].extract,d[0].extract);
              
              return true;
          }
        }
        returnedString = ""; //init data string
        return false;
      }
      softserial.print(F(QUESTION_MSG));
      return false;
  }else{   
    return false;
  }
}

void strupp(char* beg)
{
    while (*beg = std::toupper(*beg))
       ++beg;
}

boolean process_miflora_data(char * rest_data, char * mac_adress){
  
  char tmp_rest_data_length[1];
  memcpy( tmp_rest_data_length, &rest_data[51], 1 );
  int data_length = ((int)strtol(tmp_rest_data_length, NULL, 16)*2)+1;
  char rev_data[data_length];
  char data[data_length];
  memcpy( rev_data, &rest_data[52], data_length );
  rev_data[data_length] = '\0';
  
  // reverse data order
  revert_hex_data(rev_data, data, data_length);
  int value = strtol(data, NULL, 16);
  char val[12];
  sprintf(val, "%d", value);
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;

          
  // following the value of digit 47 we determine the type of data we get from the sensor
  switch (rest_data[47]) {
    case '9' :
          mactopic = mactopic + "/" + "fer";
    break;
    case '4' :
          mactopic = mactopic + "/" + "tem";
          dtostrf(value/10,4,1,val); // override temperature value in case we have, indeed temp has to be divided by 10
    break;
    case '7' :
          mactopic = mactopic + "/" + "lux";
     break;
    case '8' :
          mactopic = mactopic + "/" + "hum";
     break;
    default:
    trc("can't read values");
    return false;
    }
    client.publish((char *)mactopic.c_str(),val);;
    trc(String(val));
    return true;
  }

#endif

#ifdef ZgatewayBT_stable
#define QUESTION_MSG "AT+DISI?"
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
#endif
