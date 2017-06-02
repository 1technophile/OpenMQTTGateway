/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal
 - receive MQTT data from a topic and send IR signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received IR signal
 - publish MQTT data to a different topic related to BLE devices rssi signal
 
  Copyright: (c)Florian ROBERT

  Contributors:
  - 1technophile
  - crankyoldgit
  - Spudtater
  - prahjister
  - rickybrent
  - ekim from Home assistant forum
  - ronvl from Home assistant forum

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
#include "User_config.h"
#include <PubSubClient.h>

// array to store previous received RFs, IRs codes and their timestamps
#ifdef ESP8266
#define array_size 12
unsigned long ReceivedSignal[array_size][2] ={{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};
#else
#define array_size 4
unsigned long ReceivedSignal[array_size][2] ={{0,0},{0,0},{0,0},{0,0}};
#endif
/*------------------------------------------------------------------------*/

//adding this to bypass the problem of the arduino builder issue 50
void callback(char*topic, byte* payload,unsigned int length);

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  WiFiClient eClient;
#else
  #include <Ethernet.h>
  EthernetClient eClient;
#endif

// client parameters
PubSubClient client(mqtt_server, mqtt_port, callback, eClient);

//MQTT last attemps reconnection date
unsigned long lastReconnectAttempt = 0;

boolean reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    trc(F("Attempting MQTT connection...")); //F function enable to decrease sram usage
    #ifdef mqtt_user
      if (client.connect(Gateway_Name, mqtt_user, mqtt_password, will_Topic, will_QoS, will_Retain, will_Message)) { // if an mqtt user is defined we connect to the broker with authentication
    #else
      if (client.connect(Gateway_Name, will_Topic, will_QoS, will_Retain, will_Message)) {
    #endif
      trc(F("connected to broker"));
    // Once connected, publish an announcement...
      client.publish(will_Topic,Gateway_AnnouncementMsg,will_Retain);
      //Subscribing to topic
      if (client.subscribe(subjectMQTTtoX) && client.subscribe(subjectMultiGTWRF)&& client.subscribe(subjectMultiGTWIR)) {
        trc(F("subscription OK to the subjects defined"));
      }
      } else {
      trc(F("failed, rc="));
      trc(String(client.state()));
      trc(F("try again in 5s"));
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  return client.connected();
}

// Callback function, when the gateway receive an MQTT value on the topics subscribed this function is called
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  trc(F("Hey I got a callback "));
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  // Conversion to a printable string
  p[length] = '\0';
  //launch the function to treat received data
  receivingMQTT(topic,(char *) p);
  // Free the memory
  free(p);
}

void setup()
{
  //Launch serial for debugging purposes
  Serial.begin(115200);

  #ifdef ESP8266
    //Begining wifi connection in case of ESP8266
    setup_wifi();
  #else
    //Begining ethernet connection in case of Arduino + W5100
    setup_ethernet();
  #endif
  
  delay(1500);
  
  lastReconnectAttempt = 0;
  
  #ifdef ZgatewayIR
    setupIR();
  #endif
  #ifdef ZgatewayRF
    setupRF();
  #endif
  #ifdef ZgatewayBT
    setupBT();
  #endif

}

#ifdef ESP8266
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  trc(F("Connecting to "));
  trc(wifi_ssid);
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.config(ip_adress,gateway_adress,subnet_adress); //Uncomment this line if you want to use advanced network config
  trc(F("OpenMQTTGateway ip adress: "));   
  Serial.println(WiFi.localIP());
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    trc(F("."));
  }
  trc(F("WiFi connected"));
}
#else
void setup_ethernet() {
  Ethernet.begin(mac, ip); //Comment and uncomment the following line if you want to use advanced network config
  //Ethernet.begin(mac, ip, Dns, gateway, subnet);
  trc(F("OpenMQTTGateway ip adress: "));   
  Serial.println(Ethernet.localIP());
  trc(F("Ethernet connected"));
}
#endif

void loop()
{
  //MQTT client connexion management
  if (!client.connected()) { // not connected
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else { //connected
    // MQTT loop
    client.loop();
    
    #ifdef ZsensorDHT
      MeasureTempAndHum(); //Addon to measure the temperature with a DHT
    #endif
    // Receive loop, if data received by RF433 or IR send it by MQTT
    #ifdef ZgatewayRF
      boolean resultRF = RFtoMQTT();
      if(resultRF)
      trc(F("RF sent by MQTT"));
    #endif
    #ifdef ZgatewayIR
      boolean resultIR = IRtoMQTT();
      if(resultIR)
      trc(F("IR sent by MQTT"));
      delay(100);
    #endif
    #ifdef ZgatewayBT
      boolean resultBT = BTtoMQTT();
      if(resultBT)
      trc(F("BT sent by MQTT"));
    #endif
    
  }

}

void storeValue(long MQTTvalue){
    unsigned long now = millis();
    // find oldest value of the buffer
    int o = getMin();
    trc(F("Min index: "));
    trc(String(o));
    // replace it by the new one
    ReceivedSignal[o][0] = MQTTvalue;
    ReceivedSignal[o][1] = now;
    trc(F("store code :"));
    trc(String(ReceivedSignal[o][0])+"/"+String(ReceivedSignal[o][1]));
    trc(F("Col: val/timestamp"));
    for (int i = 0; i < array_size; i++)
    {
      trc(String(i) + ":" + String(ReceivedSignal[i][0])+"/"+String(ReceivedSignal[i][1]));
    }
}

int getMin(){
  int minimum = ReceivedSignal[0][1];
  int minindex=0;
  for (int i = 0; i < array_size; i++)
  {
    if (ReceivedSignal[i][1] < minimum) {
      minimum = ReceivedSignal[i][1];
      minindex = i;
    }
  }
  return minindex;
}

boolean isAduplicate(long value){
trc(F("isAduplicate"));
// check if the value has been already sent during the last time_avoid_duplicate
for (int i = 0; i < array_size;i++){
 if (ReceivedSignal[i][0] == value){
      unsigned long now = millis();
      if (now - ReceivedSignal[i][1] < time_avoid_duplicate){ // change
      trc(F("---don't publish the code duplicate---"));
      return true;
    }
  }
}
return false;
}

void receivingMQTT(char * topicOri, char * datacallback) {

   if (strstr(topicOri, subjectMultiGTWKey) != NULL) // storing received value so as to avoid publishing this value if it has been already sent by this or another OpenMQTTGateway
   {
      trc(F("Storing signal"));
      unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
      storeValue(data);
      trc(F("Data stored"));
   }
  
#ifdef ZgatewayRF
  MQTTtoRF(topicOri, datacallback);
#endif
#ifdef ZgatewayIR
  MQTTtoIR(topicOri, datacallback);
#endif
}

//trace
void trc(String msg){
  if (TRACE) {
  Serial.println(msg);
  }
}
