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
  - crankyoldgit
  - Spudtater
  
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

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT & NETWORK PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#define mqtt_server "192.168.1.17"
#define mqtt_user "your_username" // not compulsory only if your broker needs authentication
#define mqtt_password "your_password" // not compulsory only if your broker needs authentication
#define mqtt_port 1883
#define Gateway_Name "OpenMQTTGateway"
#define will_Topic "home/OpenMQTTGateway/LWT"
#define will_QoS 0
#define will_Retain true
#define will_Message "Offline"
#define Gateway_AnnouncementMsg "Online"

// Update these with values suitable for your network.
#ifdef ESP8266 // for nodemcu, weemos and esp8266
  #define wifi_ssid "wifi ssid"
  #define wifi_password "wifi password"
#else // for arduino + W5100
  const byte mac[] = {  0xDE, 0xED, 0xBA, 0xFE, 0x54, 0x95 }; //W5100 ethernet shield mac adress
  const byte ip[] = { 192, 168, 1, 95 }; //W5100 ethernet shield ip adress
#endif

//Addons management, comment the line if you don't use
//#define ZaddonDHT true

/*----------------------------OTHER PARAMETERS-----------------------------*/
/*-------------------CHANGING THEM IS NOT COMPULSORY-----------------------*/
//variables to avoid duplicates for RF
#define time_avoid_duplicate 3000 // if you want to avoid duplicate mqtt message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates

//MQTT definitions
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#define subjectMQTTtoX "home/commands/#"

//433Mhz MQTT Subjects and keys
#define subjectMQTTto433 "home/commands/MQTTto433"
#define subject433toMQTT "home/433toMQTT"
#define subjectGTWRFtoMQTT "home/433toMQTT"
#define subject433toMQTTAdvanced "home/433toMQTTAdvanced"
#define RFprotocolKey "433_" // protocol will be defined if a subject contains RFprotocolKey followed by a value of 1 digit
#define RFpulselengthKey "PLSL_" // pulselength will be defined if a subject contains RFprotocolKey followed by a value of 3 digits
//IR MQTT Subjects
#define subjectGTWIRtoMQTT "home/sensors/ir"
#define subjectIRtoMQTT "home/sensors/ir"
#define subjectMQTTtoIR "home/commands/MQTTtoIR"
/*
RF supported protocols
433_1
433_2
433_3
433_4
433_5
433_6
IR supported protocols
IR_NEC
IR_LG
IR_Sony
IR_DISH
IR_Sharp
IR_SAMSUNG
IR_COOLIX
IR_Whynter
*/

//IR PIN definition
#define IR_RECEIVER_PIN 2 // put 2 = D4 on nodemcu, 2 = D2 on arduino
#define RF_EMITTER_PIN 4 //put 4 = D2 on nodemcu, 4 = D4 on arduino

#ifdef ESP8266
  #define IR_EMITTER_PIN 14 // 14 = D5 on nodemcu #define only usefull for ESP8266
  //RF PIN definition
  #define RF_RECEIVER_PIN 5 //  5 = D1 on nodemcu
#else
  //IMPORTANT NOTE: On arduino UNO connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/IRremoteInt.h so as to free pin D3 for RF RECEIVER PIN
  //RF PIN definition
  #define RF_RECEIVER_PIN 1 //  1 = D3 on arduino
#endif
//RF number of signal repetition
#define RF_EMITTER_REPEAT 20

//Do we want to see trace for debugging purposes
#define TRACE 1  // 0= trace off 1 = trace on
