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
#include <RCSwitch.h> // library for controling Radio frequency switch

// array to store previous received RFs, IRs codes and their timestamps
unsigned long ReceivedSignal[10][2] ={{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};
/*------------------------------------------------------------------------*/

//adding this to bypass the problem of the arduino builder issue 50
void callback(char*topic, byte* payload,unsigned int length);

RCSwitch mySwitch = RCSwitch();

#ifdef ESP8266
  #include <IRremoteESP8266.h>
  #include <ESP8266WiFi.h>
  IRrecv irrecv(IR_RECEIVER_PIN);
  IRsend irsend(IR_EMITTER_PIN);
  WiFiClient eClient;
#else
  #include <IRremote.h>
  #include <Ethernet.h>
  EthernetClient eClient;
  IRrecv irrecv(IR_RECEIVER_PIN);
  IRsend irsend; //connect IR emitter pin to D9 on arduino, you need to comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library IRremote.h so as to free pin D3 for RF RECEIVER PIN
#endif

decode_results results;

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
      trc(F("connected with authentication"));
    #else
      if (client.connect(Gateway_Name, will_Topic, will_QoS, will_Retain, will_Message)) {
      trc(F("connected without authentication"));
    #endif
    // Once connected, publish an announcement...
      client.publish(will_Topic,Gateway_AnnouncementMsg);
      //Subscribing to topic
      if (client.subscribe(subjectMQTTtoX)) {
        trc(F("subscription OK to"));
        trc(subjectMQTTtoX);
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

  //IR init parameters
#ifdef ESP8266
  irsend.begin();
#endif

  irrecv.enableIRIn(); // Start the receiver

  //RF init parameters
  mySwitch.enableTransmit(RF_EMITTER_PIN);
  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); 
  mySwitch.enableReceive(RF_RECEIVER_PIN); 

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
  IPAddress dns_adress(dns);
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.config(ip_adress,gateway_adress,subnet_adress); //Uncomment this line if you want to use advanced network config
  trc("OpenMQTTGateway ip adress: ");   
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
  //Ethernet.begin(mac, ip, dns, gateway, subnet);
  trc("My IP address is: ");   
  trc(String(Ethernet.localIP()));
  
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
    #ifdef ZaddonDHT
      MeasureTempAndHum(); //Addon to measure the temperature with a DHT
    #endif
    // Receive loop, if data received by RF433 or IR send it by MQTT
    if (mySwitch.available() || irrecv.decode(&results)) {
      boolean result = sendValuebyMQTT();
      if(result)
        trc(F("value successfully sent by MQTT"));
    }
  }

  delay(100);
}

boolean sendValuebyMQTT(){
// Topic on which we will send data
trc(F("Receiving 433Mhz or IR signal"));
unsigned long MQTTvalue = 0;
  if (mySwitch.available()||irrecv.decode(&results)){
      trc(F("Signal detected"));
      String subject;
      String valueAdvanced ="";
      if (mySwitch.available()){
        MQTTvalue = mySwitch.getReceivedValue();
        valueAdvanced = "Value " + String(MQTTvalue)+" Bit " + String(mySwitch.getReceivedBitlength()) + " Delay " + String(mySwitch.getReceivedDelay()) + " Protocol " + String(mySwitch.getReceivedProtocol());
        mySwitch.resetAvailable();
        subject = subject433toMQTT;
      }else if (irrecv.decode(&results)){
        MQTTvalue=results.value;
        irrecv.resume(); // Receive the next value
        subject = subjectIRtoMQTT;
      }
      if (!isAduplicate(MQTTvalue) && MQTTvalue!=0) {// conditions to avoid duplications of RF -->MQTT
          if (valueAdvanced != ""){
            trc(F("Sending advanced signal to MQTT"));
            client.publish(subject433toMQTTAdvanced,(char *)valueAdvanced.c_str());
          }
          trc(F("Sending signal to MQTT"));
          String value = String(MQTTvalue);
          trc(value);
          boolean result = client.publish((char *)subject.c_str(),(char *)value.c_str());
          if (result)storeValue(MQTTvalue);
          return result;
      } 
  return false;
  }      
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
    trc(F("send this code :"));
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
      trc(F("don't send the received code"));
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

  // RF DATA ANALYSIS
  //We look into the subject to see if a special RF protocol is defined 
  int valuePRT = 0;
  int valuePLSL  = 0;
  int pos = topic.lastIndexOf(RFprotocolKey);       
  if (pos != -1){
    pos = pos + +strlen(RFprotocolKey);
    valuePRT = (topic.substring(pos,pos + 1)).toInt();
    trc(F("RF Protocol number:"));
    trc(String(valuePRT));
  }
  //We look into the subject to see if a special RF pulselength is defined 
  int pos2 = topic.lastIndexOf(RFpulselengthKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RFpulselengthKey);
    valuePLSL = (topic.substring(pos2,pos2 + 3)).toInt();
    trc(F("RF Pulse Length:"));
    trc(String(valuePLSL));
  }
  
  if ((topic == subjectMQTTto433) && (valuePRT == 0) && (valuePLSL  == 0)){
    trc(F("Sending data by RF, default parameters"));
    mySwitch.setProtocol(1,350);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    boolean result = client.publish(subjectGTWRFtoMQTT, datacallback);
    if (result)trc(F("Acknowedgement of reception published"));
    
  } else if ((valuePRT != 0) || (valuePLSL  != 0)){
    trc(F("Sending data by RF, user defined parameters"));
    if (valuePRT == 0) valuePRT = 1;
    if (valuePLSL == 0) valuePLSL = 350;
    trc(String(valuePRT));
    trc(String(valuePLSL));
    mySwitch.setProtocol(valuePRT,valuePLSL);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    boolean result = client.publish(subjectGTWRFtoMQTT, datacallback);
    if (result)trc(F("Acknowedgement of reception published"));
  }
  
  // IR DATA ANALYSIS    
  //send received MQTT value by IR signal (example of signal sent data = 1086296175)
  boolean signalSent = false;
  #ifdef ESP8266 // send coolix not available for arduino IRRemote library
  if (topic.lastIndexOf("IR_COOLIX") != -1 ){
    irsend.sendCOOLIX(data, 24);
    signalSent = true;
  }
  #endif
  if (topic.lastIndexOf("IR_NEC")!= -1 || topic == subjectMQTTtoIR){
    irsend.sendNEC(data, 32);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Whynter")!=-1){
    irsend.sendWhynter(data, 32);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_LG")!=-1){
    irsend.sendLG(data, 28);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Sony")!=-1){
    irsend.sendSony(data, 12);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_DISH")!=-1){
    irsend.sendDISH(data, 16);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_RC5")!=-1){
    irsend.sendRC5(data, 12);
    signalSent = true;
  }
  if (topic.lastIndexOf("IR_Sharp")!=-1){
    irsend.sendSharpRaw(data, 15);
    signalSent = true;
  }
   if (topic.lastIndexOf("IR_SAMSUNG")!=-1){
   irsend.sendSAMSUNG(data, 32);
    signalSent = true;
  }
  if (signalSent){
    boolean result = client.publish(subjectGTWIRtoMQTT, datacallback);
    if (result)trc(F("Acknowedgement of reception published"));
  }
   irrecv.enableIRIn(); // ReStart the IR receiver (if not restarted it is not able to receive data)
}

//trace
void trc(String msg){
  if (TRACE) {
  Serial.println(msg);
  }
}
