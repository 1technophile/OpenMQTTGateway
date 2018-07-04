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
  - Chris Broekema
  - Lars Englund
  - Fredrik Lindqvist
  - Philippe LUC
  - Patrick Wilhelm
  - Georgi Yanev
  - zerinrc
  - ChiefZ
  - 8666

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
#include <ArduinoJson.h>

// Modules config inclusion
#if defined(ZgatewayRF) || defined(ZgatewayRF2)
  #include "config_RF.h"
#endif
#ifdef ZgatewayRF315
  #include "config_RF315.h"
#endif
#ifdef ZgatewaySRFB
  #include "config_SRFB.h"
#endif
#ifdef ZgatewayBT
  #include "config_BT.h"
#endif
#ifdef ZgatewayIR
  #include "config_IR.h"
#endif
#ifdef Zgateway2G
  #include "config_2G.h"
#endif
#ifdef ZactuatorONOFF
  #include "config_ONOFF.h"
#endif
#ifdef ZsensorINA226
  #include "config_INA226.h"
#endif
#ifdef ZsensorHCSR501
  #include "config_HCSR501.h"
#endif
#ifdef ZsensorADC
  #include "config_ADC.h"
#endif
#ifdef ZsensorBH1750
  #include "config_BH1750.h"
#endif
#ifdef ZsensorTSL2561
  #include "config_TSL2561.h"
#endif
#ifdef ZsensorBME280
  #include "config_BME280.h"
#endif
#ifdef ZsensorDHT
  #include "config_DHT.h"
#endif
#ifdef ZgatewayRFM69
  #include "config_RFM69.h"
#endif

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

boolean connectedOnce = false; //indicate if we have been connected once to MQTT

#ifdef ESP32
  #include <WiFi.h>
  #include <ArduinoOTA.h>
  WiFiClient eClient;
  #ifdef MDNS_SD
    #include <ESPmDNS.h>
  #endif
#elif defined(ESP8266)
  #include <FS.h> 
  #include <ESP8266WiFi.h>
  #include <ArduinoOTA.h>
  #include <DNSServer.h>
  #include <ESP8266WebServer.h>
  #include <WiFiManager.h>  
  WiFiClient eClient;
  #ifdef MDNS_SD
    #include <ESP8266mDNS.h>
  #endif
#else
  #include <Ethernet.h>
  EthernetClient eClient;
#endif

// client link to pubsub mqtt
PubSubClient client(eClient);

//MQTT last attemps reconnection date
unsigned long lastReconnectAttempt = 0;

//timers for LED indicators
unsigned long timer_led_receive = 0;
unsigned long timer_led_send = 0;
unsigned long timer_led_error = 0;

//Time used to wait for an interval before checking system measures
unsigned long timer_sys_measures = 0;

//Wifi manager parameters
//flag for saving data
bool shouldSaveConfig = true;
//do we have been connected once to mqtt

//callback notifying us of the need to save config
void saveConfigCallback () {
  trc("Should save config");
  shouldSaveConfig = true;
}

boolean reconnect() {
  
  int failure_number = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
      trc(F("MQTT connection...")); //F function enable to decrease sram usage
      if (client.connect(Gateway_Name, mqtt_user, mqtt_pass, will_Topic, will_QoS, will_Retain, will_Message)) {
      trc(F("Connected to broker"));
      failure_number = 0;
      // Once connected, publish an announcement...
      client.publish(will_Topic,Gateway_AnnouncementMsg,will_Retain);
      // publish version
      client.publish(version_Topic,OMG_VERSION,will_Retain);
      //Subscribing to topic
      if (client.subscribe(subjectMQTTtoX)) {
        #ifdef ZgatewayRF
          client.subscribe(subjectMultiGTWRF);
        #endif
        #ifdef ZgatewayRF315
          client.subscribe(subjectMultiGTWRF315);
        #endif
        #ifdef ZgatewayIR
          client.subscribe(subjectMultiGTWIR);
        #endif
        trc(F("Subscription OK to the subjects"));
      }
      } else {
      failure_number ++; // we count the failure
      trc(F("failed, rc="));
      trc(client.state());
      trc(F("try again in 5s"));
      // Wait 5 seconds before retrying
      delay(5000);

      if (failure_number > maxMQTTretry){
        trc(F("failed connecting to mqtt"));
        #if defined(ESP8266) && !defined(ESPWifiManualSetup)
          if (!connectedOnce) setup_wifimanager(true); // if we didn't connected once to mqtt we reset and start in AP mode again to have a chance to change the parameters
        #elif defined(ESP32) || defined(ESPWifiManualSetup)// ESP32 case we don't use Wifi manager yet
          setup_wifi();
        #endif
      }
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
    #if defined(ESP8266) && !defined(ESPWifiManualSetup)
      setup_wifimanager(false);
    #else // ESP32 case we don't use Wifi manager yet
      setup_wifi();
    #endif
    
    trc(F("OpenMQTTGateway mac: "));
    trc(WiFi.macAddress()); 
    
    trc(F("OpenMQTTGateway ip: "));
    trc(WiFi.localIP());
    
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

  #else // In case of arduino

    //Launch serial for debugging purposes
    Serial.begin(SERIAL_BAUD);
    //Begining ethernet connection in case of Arduino + W5100
    setup_ethernet();
    //setup LED status, turn all ON for short amount then leave only the RED LED ON
    pinMode(led_receive, OUTPUT);
    pinMode(led_send, OUTPUT);
    pinMode(led_error, OUTPUT);
    digitalWrite(led_receive, LOW);
    digitalWrite(led_send, LOW);
    digitalWrite(led_error, LOW);
    delay(500);
    digitalWrite(led_receive, HIGH);
    digitalWrite(led_send, HIGH);
  #endif

  #if defined(MDNS_SD) && defined(ESP8266)
     trc(F("Connecting to MQTT by mDNS without mqtt hostname"));
     connectMQTTmdns();
  #else
     long port;
     port = strtol(mqtt_port,NULL,10);
     trc(port);
   #ifdef mqtt_server_name // if name is defined we define the mqtt server by its name
     trc(F("Connecting to MQTT with mqtt hostname"));
     IPAddress mqtt_server_ip;
     WiFi.hostByName(mqtt_server_name, mqtt_server_ip);
     client.setServer(mqtt_server_ip, port);
     trc(mqtt_server_ip.toString());
   #else // if not by its IP adress
     trc(F("Connecting to MQTT by IP adress"));
     client.setServer(mqtt_server, port);
     trc(mqtt_server);
   #endif
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
  #ifdef ZsensorTSL2561
    setupZsensorTSL2561();
  #endif
  #ifdef ZactuatorONOFF
    setupONOFF();
  #endif
  #ifdef Zgateway2G
    setup2G();
  #endif
  #ifdef ZgatewayIR
    setupIR();
  #endif
  #ifdef ZgatewayRF
    setupRF();
  #endif
  #ifdef ZgatewayRF315
    setupRF315();
  #endif
  #ifdef ZgatewayRF2
    setupRF2();
  #endif
  #ifdef ZgatewaySRFB
    setupSRFB();
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


#if defined(ESP32) || defined(ESPWifiManualSetup)
void setup_wifi() {
  delay(10);
  int failureAttempt = 0; //DIRTY FIX ESP32 waiting for https://github.com/espressif/arduino-esp32/issues/653
  WiFi.mode(WIFI_STA);
  // We start by connecting to a WiFi network
  trc(F("Connecting to "));
  trc(wifi_ssid);
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  WiFi.begin(wifi_ssid, wifi_password);
  //WiFi.config(ip_adress,gateway_adress,subnet_adress,dns_adress); //Uncomment this line if you want to use advanced network config
    
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    trc(F("."));
    failureAttempt++; //DIRTY FIX ESP32
    if (failureAttempt > 30) setup_wifi(); //DIRTY FIX ESP32
  }
  
  trc(F("WiFi ok with manual config credentials"));
}

#elif defined(ESP8266) && !defined(ESPWifiManualSetup)
void setup_wifimanager(boolean initWM){
    #ifdef cleanFS
    //clean FS, for testing
      SPIFFS.format();
    #else
      if(initWM) SPIFFS.format();
    #endif
    //read configuration from FS json
    trc("mounting FS...");
  
    if (SPIFFS.begin()) {
      trc("mounted file system");
      if (SPIFFS.exists("/config.json")) {
        //file exists, reading and loading
        trc("reading config file");
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile) {
          trc("opened config file");
          size_t size = configFile.size();
          // Allocate a buffer to store contents of the file.
          std::unique_ptr<char[]> buf(new char[size]);
  
          configFile.readBytes(buf.get(), size);
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(buf.get());
          json.printTo(Serial);
          if (json.success()) {
            trc("\nparsed json");
  
            strcpy(mqtt_server, json["mqtt_server"]);
            strcpy(mqtt_port, json["mqtt_port"]);
            strcpy(mqtt_user, json["mqtt_user"]);
            strcpy(mqtt_pass, json["mqtt_pass"]);
  
          } else {
            trc("failed to load json config");
          }
        }
      }
    } else {
      trc("failed to mount FS");
    }
  
    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 20);
    WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, 20);
  
   //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
  
    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);
  
    //set static ip
    //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    
    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);

    #ifdef cleanFS
      //reset settings - for testing
      wifiManager.resetSettings();
    #else
      if(initWM) wifiManager.resetSettings();
    #endif
    //set minimu quality of signal so it ignores AP's under that quality
    wifiManager.setMinimumSignalQuality(MinimumWifiSignalQuality);
  
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect(Gateway_Name, WifiManager_password)) {
      trc("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
  
    //if you get here you have connected to the WiFi
    trc("connected to wifi");
  
    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  
    //save the custom parameters to FS
    if (shouldSaveConfig) {
      trc("saving config");
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["mqtt_server"] = mqtt_server;
      json["mqtt_port"] = mqtt_port;
      json["mqtt_user"] = mqtt_user;
      json["mqtt_pass"] = mqtt_pass;
    
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        trc("failed to open config file for writing");
      }
  
      json.printTo(Serial);
      json.printTo(configFile);
      configFile.close();
      //end save
    }
}


#else // Arduino case
void setup_ethernet() {
  Ethernet.begin(mac, ip); //Comment and uncomment the following line if you want to use advanced network config
  //Ethernet.begin(mac, ip, Dns, gateway, subnet);
  trc(F("OpenMQTTGateway ip: "));
  Serial.println(Ethernet.localIP());
  trc(F("Ethernet ok"));
}
#endif

#if defined(MDNS_SD) && defined(ESP8266)
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
    }else {
        trc(n);
        trc(" service(s) found");
        for (int i=0; i < n; ++i) {
            // Print details for each service found
            trc(i + 1);
            trc(MDNS.hostname(i));
            trc(MDNS.IP(i).toString());
            trc(MDNS.port(i));
        }
        if (n==1) {
          trc(F("One MQTT server found setting parameters"));
          client.setServer(MDNS.IP(0), int(MDNS.port(0)));
        }else{
          trc(F("Several MQTT servers found, please deactivate mDNS and set your default server"));
        }
    }
  }
#endif

void loop()
{
 
    unsigned long now = millis();
  //MQTT client connexion management
  if (!client.connected()) { // not connected

    //RED ON
    digitalWrite(led_error, LOW);

    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else { //connected
    // MQTT loop
    //RED OFF
    if (now - timer_led_receive > 300) {
      timer_led_receive = now;
      digitalWrite(led_receive, HIGH);
    }
    digitalWrite(led_error, HIGH);
    connectedOnce = true;
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
    #ifdef ZsensorTSL2561
      MeasureLightIntensityTSL2561();
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
    #ifdef ZgatewayRF
      if(RFtoMQTT()){
      trc(F("RFtoMQTT OK"));
      //GREEN ON
      digitalWrite(led_receive, LOW);
      timer_led_receive = millis();
      }
    #endif
    #ifdef ZgatewayRF315
      if(RF315toMQTT()){
      trc(F("RF315toMQTT OK"));
      //GREEN ON
      digitalWrite(led_receive, LOW);
      timer_led_receive = millis();
      }
    #endif
    #ifdef ZgatewayRF2
      if(RF2toMQTT()){
      trc(F("RF2toMQTT OK"));
      digitalWrite(led_receive, LOW);
      timer_led_receive = millis();
      }
    #endif
    #ifdef ZgatewaySRFB
      if(SRFBtoMQTT())
      trc(F("SRFBtoMQTT OK"));
    #endif
    #ifdef ZgatewayIR
      if(IRtoMQTT())      {
      trc(F("IRtoMQTT OK"));
      digitalWrite(led_receive, LOW);
      timer_led_receive = millis();
      delay(100);
      }
    #endif
    #ifdef ZgatewayBT
        #ifndef multiCore
          if(BTtoMQTT())
          trc(F("BTtoMQTT OK"));
        #endif
    #endif
    #ifdef Zgateway2G
      if(_2GtoMQTT()){
      trc(F("2GtoMQTT OK"));
      }
    #endif
    #ifdef ZgatewayRFM69
      if(RFM69toMQTT())
      trc(F("RFM69toMQTT OK"));
    #endif
    #if defined(ESP8266) || defined(ESP32)
      stateMeasures();
    #endif
  }
}

#if defined(ESP8266) || defined(ESP32)
void stateMeasures(){
    unsigned long now = millis();
    if (now > (timer_sys_measures + TimeBetweenReadingSYS)) {//retriving value of memory ram every TimeBetweenReadingSYS
      timer_sys_measures = millis();
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& SYSdata = jsonBuffer.createObject();
      trc("Uptime (s)");    
      unsigned long uptime = millis()/1000;
      trc(uptime);
      SYSdata["uptime"] = uptime;
      trc("Remaining memory");
      uint32_t freeMem;
      freeMem = ESP.getFreeHeap();
      SYSdata["freeMem"] = freeMem;
      trc(freeMem);
      trc("RSSI");
      long rssi = WiFi.RSSI();
      SYSdata["rssi"] = rssi;
      trc(rssi);
      trc("SSID");
      String SSID = WiFi.SSID();
      SYSdata["SSID"] = SSID;
      trc(SSID);
      trc("Activated modules");
      String modules = "";
      #ifdef ZgatewayRF
          modules = modules + ZgatewayRF;
      #endif
      #ifdef ZgatewayRF315
          modules = modules + ZgatewayRF315;
      #endif
      #ifdef ZsensorBME280
          modules = modules  + ZsensorBME280;
      #endif
      #ifdef ZsensorBH1750
          modules = modules  + ZsensorBH1750;
      #endif
      #ifdef ZsensorTSL2561
          modules = modules  + ZsensorTSL2561;
      #endif
      #ifdef ZactuatorONOFF
          modules = modules  + ZactuatorONOFF;
      #endif
      #ifdef Zgateway2G
          modules = modules  + Zgateway2G;
      #endif
      #ifdef ZgatewayIR
          modules = modules  + ZgatewayIR;
      #endif
      #ifdef ZgatewayRF2
          modules = modules  + ZgatewayRF2;
      #endif
      #ifdef ZgatewaySRFB
          modules = modules  + ZgatewaySRFB;
      #endif
      #ifdef ZgatewayBT
          modules = modules  + ZgatewayBT;
      #endif
      #ifdef ZgatewayRFM69
          modules = modules  + ZgatewayRFM69;
      #endif
      #ifdef ZsensorINA226
          modules = modules  + ZsensorINA226;
      #endif
      #ifdef ZsensorHCSR501
          modules = modules  + ZsensorHCSR501;
      #endif
      SYSdata["modules"] = modules;
      trc(modules);
      char JSONmessageBuffer[100];
      SYSdata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      client.publish(subjectSYStoMQTT,JSONmessageBuffer);
    }
}
#endif

void storeValue(unsigned long MQTTvalue){
    unsigned long now = millis();
    // find oldest value of the buffer
    int o = getMin();
    trc(F("Min ind: "));
    trc(o);
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
  unsigned int minimum = ReceivedSignal[0][1];
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

boolean isAduplicate(unsigned long value){
trc(F("isAduplicate?"));
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
//YELLOW ON
digitalWrite(led_send, LOW);
#ifdef ZgatewayRF
  MQTTtoRF(topicOri, datacallback);
#endif
#ifdef ZgatewayRF315
  MQTTtoRF315(topicOri, datacallback);
#endif
#ifdef ZgatewayRF2
  MQTTtoRF2(topicOri, datacallback);
#endif
#ifdef Zgateway2G
  MQTTto2G(topicOri, datacallback);
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
#ifdef ZactuatorONOFF
  MQTTtoONOFF(topicOri, datacallback);
#endif
//YELLOW OFF
digitalWrite(led_send, HIGH);
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
      sprintf(tmp_subset2, "%ld", long_value);
      strncpy( subset, tmp_subset2 , l+1);
    }else{
      if (reverse) revert_hex_data(tmp_subset, subset, l+1);
      else strncpy( subset, tmp_subset , l+1);
    }
    subset[l] = '\0';
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

bool to_bool(String const& s) { // thanks Chris Jester-Young from stackoverflow
     return s != "0";
}

//trace
void trc(String msg){
  if (TRACE) {
  Serial.println(msg);
  }
}

void trc(int msg){
  if (TRACE) {
  Serial.println(msg);
  }
}

void trc(unsigned int msg){
  if (TRACE) {
  Serial.println(msg);
  }
}

void trc(long msg){
  if (TRACE) {
  Serial.println(msg);
  }
}

void trc(unsigned long msg){
  if (TRACE) {
  Serial.println(msg);
  }
}

void trc(double msg){
  if (TRACE) {
  Serial.println(msg);
  }
}

void trc(float msg){
  if (TRACE) {
  Serial.println(msg);
  }
}
