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
  - glasruit
  - HannesDi
  - Landrash
  - larsenglund
  - ChiefZ
  - PatteWi
  - jumpalottahigh
  - zerinrc
  - philfifi
  - Spudtater
  - prahjister
  - rickybrent
  - petricaM
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
#if defined(ESP8266) || defined(ESP32)
#define array_size 12
unsigned long ReceivedSignal[array_size][2] ={{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};
#else
#define array_size 4
unsigned long ReceivedSignal[array_size][2] ={{0,0},{0,0},{0,0},{0,0}};
#endif
/*------------------------------------------------------------------------*/

//adding this to bypass the problem of the arduino builder issue 50
void callback(char*topic, byte* payload,unsigned int length);

#ifdef ESP32
  #include <WiFi.h>
  #include <ArduinoOTA.h>
  #ifdef MDNS_SD
    #include <ESPmDNS.h>
  #endif
  WiFiClient eClient;
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ArduinoOTA.h>
  #ifdef MDNS_SD
    #include <ESP8266mDNS.h>
  #endif
  WiFiClient eClient;
#else
  #include <Ethernet.h>
  EthernetClient eClient;
#endif

// client link to pubsub mqtt
PubSubClient client(eClient);

//MQTT last attemps reconnection date
unsigned long lastReconnectAttempt = 0;

boolean reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    trc(F("MQTT connection...")); //F function enable to decrease sram usage
    #ifdef mqtt_user
      if (client.connect(Gateway_Name, mqtt_user, mqtt_password, will_Topic, will_QoS, will_Retain, will_Message)) { // if an mqtt user is defined we connect to the broker with authentication
    #else
      if (client.connect(Gateway_Name, will_Topic, will_QoS, will_Retain, will_Message)) {
    #endif
      trc(F("Connected to broker"));
    // Once connected, publish an announcement...
      client.publish(will_Topic,Gateway_AnnouncementMsg,will_Retain);
      // publish version
      client.publish(version_Topic,OMG_VERSION,will_Retain);
      //Subscribing to topic
      if (client.subscribe(subjectMQTTtoX)) {
        #ifdef ZgatewayRF
          client.subscribe(subjectMultiGTWRF);
        #endif
        #ifdef ZgatewayIR
          client.subscribe(subjectMultiGTWIR);
        #endif
        trc(F("Subscription OK to the subjects"));
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
  #if defined(ESP8266) || defined(ESP32)
    //Launch serial for debugging purposes
    #if defined(ZgatewaySRFB) || defined(ESP32)
      Serial.begin(SERIAL_BAUD); // in the case of sonoff RF Bridge the link to the RF emitter/receiver is made by serial and need TX/RX
    #else
      Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY);
    #endif

    //Begining wifi connection in case of ESP8266
    setup_wifi();
    // Port defaults to 8266
    ArduinoOTA.setPort(ota_port);

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(ota_hostname);

    // No authentication by default
    ArduinoOTA.setPassword(ota_password);

    ArduinoOTA.onStart([]() {
      trc(F("Start"));
    });
    ArduinoOTA.onEnd([]() {
      trc(F("\nEnd"));
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) trc(F("Auth Failed"));
      else if (error == OTA_BEGIN_ERROR) trc(F("Begin Failed"));
      else if (error == OTA_CONNECT_ERROR) trc(F("Connect Failed"));
      else if (error == OTA_RECEIVE_ERROR) trc(F("Receive Failed"));
      else if (error == OTA_END_ERROR) trc(F("End Failed"));
    });
    ArduinoOTA.begin();

    #ifdef MDNS_SD
      connectMQTTmdns();
    #else
      client.setServer(mqtt_server, mqtt_port);
    #endif
    
  #else
    //Launch serial for debugging purposes
    Serial.begin(SERIAL_BAUD);
    //Begining ethernet connection in case of Arduino + W5100
    setup_ethernet();
  #endif
  
  client.setCallback(callback);
  
  delay(1500);

  lastReconnectAttempt = 0;

  #ifdef ZsensorBME280
    setupZsensorBME280();
  #endif
  #ifdef ZsensorBH1750
    setupZsensorBH1750();
  #endif
  #ifdef ZgatewayIR
    setupIR();
  #endif
  #ifdef ZgatewayRF
    setupRF();
  #endif
  #ifdef ZgatewayRF2
    setupRF2();
  #endif
  #ifdef ZgatewayBT
    setupBT();
  #endif
  #ifdef ZgatewayRFM69
    setupRFM69();
  #endif
  #ifdef ZsensorINA226
    setupINA226();
  #endif
  #ifdef ZsensorHCSR501
    setupHCSR501();
  #endif
}

#if defined(ESP8266) || defined(ESP32)
void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  // We start by connecting to a WiFi network
  trc(F("Connecting to "));
  trc(wifi_ssid);
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  WiFi.begin(wifi_ssid, wifi_password);
  //WiFi.config(ip_adress,gateway_adress,subnet_adress); //Uncomment this line if you want to use advanced network config
    
  trc(F("OpenMQTTGateway mac: "));
  Serial.println(WiFi.macAddress()); 
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    trc(F("."));
  }
  
  trc(F("OpenMQTTGateway ip: "));
  Serial.println(WiFi.localIP());
  
  trc(F("WiFi ok"));
}
#else
void setup_ethernet() {
  Ethernet.begin(mac, ip); //Comment and uncomment the following line if you want to use advanced network config
  //Ethernet.begin(mac, ip, Dns, gateway, subnet);
  trc(F("OpenMQTTGateway ip: "));
  Serial.println(Ethernet.localIP());
  trc(F("Ethernet ok"));
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

    #if defined(ESP8266) || defined(ESP32)
      ArduinoOTA.handle();
    #endif

    #ifdef ZsensorBME280
      MeasureTempHumAndPressure(); //Addon to measure Temperature, Humidity, Pressure and Altitude with a Bosch BME280
    #endif
    #ifdef ZsensorBH1750
      MeasureLightIntensity(); //Addon to measure Light Intensity with a BH1750
    #endif
    #ifdef ZsensorDHT
      MeasureTempAndHum(); //Addon to measure the temperature with a DHT
    #endif
    #ifdef ZsensorINA226
      MeasureINA226(); //Addon to measure the temperature with a DHT
    #endif
    #ifdef ZsensorHCSR501
      MeasureHCSR501();
    #endif
    #ifdef ZsensorADC
      MeasureADC(); //Addon to measure the analog value of analog pin
    #endif
    // Receive loop, if data received by RF433 or IR send it by MQTT
    #ifdef ZgatewayRF
      if(RFtoMQTT())
      trc(F("RFtoMQTT OK"));
    #endif
    #ifdef ZgatewayRF2
      if(RF2toMQTT())
      trc(F("RF2toMQTT OK"));
    #endif
    #ifdef ZgatewaySRFB
      if(SRFBtoMQTT())
      trc(F("SRFBtoMQTT OK"));
    #endif
    #ifdef ZgatewayIR
      if(IRtoMQTT())
      trc(F("IRtoMQTT OK"));
      delay(100);
    #endif
    #ifdef ZgatewayBT
      if(BTtoMQTT())
      trc(F("BTtoMQTT OK"));
    #endif
    #ifdef ZgatewayRFM69
      if(RFM69toMQTT())
      trc(F("RFM69toMQTT OK"));
    #endif
  }

}

void storeValue(long MQTTvalue){
    unsigned long now = millis();
    // find oldest value of the buffer
    int o = getMin();
    trc(F("Min ind: "));
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
      trc(F("--don't pub. duplicate--"));
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
#ifdef ZgatewayRF2
  MQTTtoRF2(topicOri, datacallback);
#endif
#ifdef ZgatewaySRFB
  MQTTtoSRFB(topicOri, datacallback);
#endif
#ifdef ZgatewayIR
  MQTTtoIR(topicOri, datacallback);
#endif
#ifdef ZgatewayRFM69
  MQTTtoRFM69(topicOri, datacallback);
#endif
}

void extract_char(char * token_char, char * subset, int start ,int l, boolean reverse, boolean isNumber){
    char tmp_subset[l+1];
    memcpy( tmp_subset, &token_char[start], l );
    tmp_subset[l] = '\0';
    if (isNumber){
      char tmp_subset2[l+1];
      if (reverse) revert_hex_data(tmp_subset, tmp_subset2, l+1);
      else strncpy( tmp_subset2, tmp_subset , l+1);
      long long_value = strtoul(tmp_subset2, NULL, 16);
      sprintf(tmp_subset2, "%d", long_value);
      strncpy( subset, tmp_subset2 , l+1);
    }else{
      if (reverse) revert_hex_data(tmp_subset, subset, l+1);
      else strncpy( subset, tmp_subset , l+1);
    }
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

void connectMQTTmdns(){
    if (!MDNS.begin("ESP_MQTT")) {
        trc(F("Error setting up MDNS responder!"));
        while(1){
            delay(1000);
        }
    }
    trc(F("Browsing for service MQTT "));
    int n = MDNS.queryService("mqtt", "tcp");
    if (n == 0) {
        trc(F("no services found"));
    } else {
        trc(String(n));
        trc(" service(s) found");
        for (int i=0; i < n; ++i) {
            // Print details for each service found
            trc(String(i + 1));
            trc(String(MDNS.hostname(i)));
            trc(String(MDNS.IP(i)));
            trc(String(MDNS.port(i)));
        }
        if (n==1) {
          trc(F("One MQTT server found setting parameters"));
          client.setServer(MDNS.IP(0), int(MDNS.port(0)));
        }else{
          trc(F("Several MQTT servers found, please deactivate mDNS and set your default server"));
        }
    }
}

//trace
void trc(String msg){
  if (TRACE) {
  Serial.println(msg);
  }
}
