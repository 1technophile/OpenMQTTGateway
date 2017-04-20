/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This program enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal
 - receive MQTT data from a topic and send IR signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received IR signal

  Copyright: (c)1technophile

  Contributors:
  - 1technophile
  - crankyoldgit
  - Spudtater
  - rickybrent
  - ekim from Home assistant forum
  - ronvl from Home assistant forum

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
#include "User_config.h"
#include <PubSubClient.h>

// array to store previous received RFs, IRs codes and their timestamps
unsigned long ReceivedSignal[10][2] ={{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};
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
int lastReconnectAttempt = 0;

boolean reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    trc(F("Attempting MQTT connection...")); //F function enable to decrease sram usage
    #ifdef mqtt_user
      if (client.connect(Gateway_Name, mqtt_user, mqtt_password, will_Topic, will_QoS, will_Retain, will_Message)) { // if an mqtt user is defined we connect to the broker with authentication
    #else
      if (client.connect(Gateway_Name, will_Topic, will_QoS, will_Retain, will_Message)) {
    #endif
      trc(F("connected to MQTT broker"));
    // Once connected, publish an announcement...
      client.publish(will_Topic,Gateway_AnnouncementMsg);
      //Subscribing to topic
      if (client.subscribe(subjectMQTTtoX) && client.subscribe(subjectMultiGTWRF)&& client.subscribe(subjectMultiGTWIR)) {
        trc(F("subscription OK to the subjects defined"));
      }
      } else {
      trc(F("failed, rc="));
      trc(String(client.state()));
      trc(F(" try again in 5 seconds"));
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
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      trc(F("client mqtt not connected, trying to connect"));
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
        trc(F("RF  successfully sent by MQTT"));
      #endif
      #ifdef ZgatewayIR
        boolean resultIR = IRtoMQTT();
        if(resultIR)
        trc(F("IR successfully sent by MQTT"));
      #endif
  }
  delay(100);
}

void storeValue(long MQTTvalue){
    long now = millis();
    // find oldest value of the buffer
    int o = getMin();
    trc(F("Minimum index: "));
    trc(String(o));
    // replace it by the new one
    ReceivedSignal[o][0] = MQTTvalue;
    ReceivedSignal[o][1] = now;
    trc(F("store this code :"));
    trc(String(ReceivedSignal[o][0])+"/"+String(ReceivedSignal[o][1]));
    trc(F("Col: value/timestamp"));
    for (int i = 0; i < 10; i++)
    {
      trc(String(i) + ":" + String(ReceivedSignal[i][0])+"/"+String(ReceivedSignal[i][1]));
    }
}

int getMin(){
  int minimum = ReceivedSignal[0][1];
  int minindex=0;
  for (int i = 0; i < 10; i++)
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
for (int i=0; i<10;i++){
 if (ReceivedSignal[i][0] == value){
      long now = millis();
      if (now - ReceivedSignal[i][1] < time_avoid_duplicate){ // change
      trc(F("--------------don't send the received code--------------"));
      return true;
    }
  }
}
return false;
}

void receivingMQTT(char * topicOri, char * datacallback) {
  String topic = topicOri;

  trc(F("Receiving data by MQTT"));
  trc(topic);  
  trc(F("Callback value"));
  trc(String(datacallback));
  unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
  trc(F("Converted value to unsigned long"));
  trc(String(data));

  // Storing data received
  int pos0 = topic.lastIndexOf(subjectMultiGTWKey);
  if (pos0 != -1){
    trc(F("Storing signal"));
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
