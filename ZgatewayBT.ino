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

          char mac_adress[13];
          trc(F("mac_adress "));
          extract_char(token_char, mac_adress, delimiter_length ,12,true);
          
          char device_type[3];
          trc(F("device_type "));
          extract_char(token_char, device_type, delimiter_length+12 ,2,false);

          int rssi;
          trc(F("rssi "));
          extract_int(token_char, rssi, delimiter_length +12+2 ,2,false);

          char val[12];
          sprintf(val, "%d", rssi);
          String mactopic(mac_adress);
          mactopic = subjectBTtoMQTT + mactopic;
          client.publish((char *)mactopic.c_str(),val);
          
          int rest_data_length;
          trc(F("rest_data_length "));
          extract_int(token_char, rest_data_length, delimiter_length+12+2+2 ,2,false);   
          
          char rest_data[rest_data_length*2];
          trc(F("rest_data "));
          extract_char(token_char, rest_data, delimiter_length+12+2+2+2 ,rest_data_length*2,false);

          char sensor_type[5];
          trc(F("sensor_type "));
          extract_char(rest_data, sensor_type, 10 ,4,true);

          if (strcmp(sensor_type, "fe95") == 0) process_miflora_data(rest_data,mac_adress);
        }
        returnedString = ""; //init data string
      }
      softserial.print(F(QUESTION_MSG));
      return true;
  }else{   
    return false;
  }
}

void process_miflora_data(char * rest_data, char * mac_adress){
  
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
  trc(String(value));
  char val[12];
  sprintf(val, "%d", value);
  
  String mactopic(mac_adress);
  mactopic = subjectBTtoMQTT + mactopic;
  
  // following the value of digit 47 we determine the type of data we get from the sensor
  switch (rest_data[47]) {
    case '9' :
          mactopic = mactopic + "/" + "fertilisation";
    break;
    case '4' :
          mactopic = mactopic + "/" + "temperature_Cx10";
    break;
    case '7' :
          mactopic = mactopic + "/" + "lux";
     break;
    case '8' :
          mactopic = mactopic + "/" + "humidity";
     break;
    default:
    trc("can't read values");
    }
    client.publish((char *)mactopic.c_str(),val);;
    trc(String(val));
  }

void revert_hex_data(char * in, char * out, int l){
  //reverting array 2 by 2 to get the data in good order
  int i = l-2 , j = 0; 
  while ( i != -2 ) {
    if (i%2 == 0) out[j] = in[i+1];
    else  out[j] = in[i-1];
    j++;
    i--;
  }
  out[l-1] = '\0';
}

void extract_char(char * token_char, char * subset, int start ,int l, boolean reverse){
    char tmp_subset[l+1];
    memcpy( tmp_subset, &token_char[start], l );
    tmp_subset[l] = '\0';
    if (reverse) revert_hex_data(tmp_subset, subset, l+1);
    else strncpy( subset, tmp_subset , l+1);
    Serial.println(subset);
}

void extract_int(char * token_char, int & subsetint, int start ,int l, boolean reverse ){
    char tmp_subset[l+1];
    char subset[l+1];
    memcpy( tmp_subset, &token_char[start], l );
    tmp_subset[l] = '\0';
    if (reverse) revert_hex_data(tmp_subset, subset, l+1);
    else strncpy( subset, tmp_subset, l+1 );
    subsetint = (int)strtol(subset, NULL, 16);
    Serial.println(subsetint);
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
