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

/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR NETWORK PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#define mqtt_server "192.168.1.17"
//#define mqtt_user "your_username" // not compulsory only if your broker needs authentication
#define mqtt_password "your_password" // not compulsory only if your broker needs authentication
#define mqtt_port 1883
#define Gateway_Name "OpenMQTTGateway"
#define will_Topic "home/OpenMQTTGateway/LWT"
#define will_QoS 0
#define will_Retain true
#define will_Message "Offline"
#define Gateway_AnnouncementMsg "Online"

/*-------------DEFINE YOUR NETWORK PARAMETERS BELOW----------------*/
// Update these with values suitable for your network.
#ifdef ESP8266 // for nodemcu, weemos and esp8266
  #define wifi_ssid "wifi ssid"
  #define wifi_password "wifi password"
#else // for arduino + W5100
  const byte mac[] = {  0xDE, 0xED, 0xBA, 0xFE, 0x54, 0x95 }; //W5100 ethernet shield mac adress
#endif

const byte ip[] = { 192, 168, 1, 99 }; //ip adress
// Advanced network config (optional) if you want to use these parameters uncomment line 158, 172 and comment line 171  of OpenMQTTGateway.ino
const byte gateway[] = { 192, 168, 1, 1 }; //ip adress
const byte Dns[] = { 192, 168, 1, 1 }; //ip adress
const byte subnet[] = { 255, 255, 255, 0 }; //ip adress

/*-------------DEFINE YOUR OTA PARAMETERS BELOW----------------*/
#define ota_hostname "OTAHOSTNAME"
#define ota_password "OTAPASSWORD"
#define ota_port 8266

/*-------------DEFINE THE MODULES YOU WANT BELOW----------------*/
//Addons and module management, comment the line if you don't use
//#define ZsensorDHT
#define ZgatewayRF
#define ZgatewayIR
#define ZgatewayBT
/*----------------------------OTHER PARAMETERS-----------------------------*/
/*-------------------CHANGING THEM IS NOT COMPULSORY-----------------------*/
//variables to avoid duplicates for RF
#define time_avoid_duplicate 3000 // if you want to avoid duplicate mqtt message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates

/*--------------MQTT general topics-----------------*/
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#define subjectMQTTtoX "home/commands/#"
#define subjectMultiGTWKey "toMQTT"

/*-------------------RF topics----------------------*/
//433Mhz MQTT Subjects and keys
#define subjectMQTTtoRF "home/commands/MQTTto433"
#define subjectRFtoMQTT "home/433toMQTT"
#define subjectGTWRFtoMQTT "home/433toMQTT"
#define subjectRFtoMQTTprotocol "home/433toMQTT/protocol"
#define subjectRFtoMQTTbits "home/433toMQTT/bits"
#define subjectRFtoMQTTlength "home/433toMQTT/length"
#define RFprotocolKey "433_" // protocol will be defined if a subject contains RFprotocolKey followed by a value of 1 digit
#define RFbitsKey "RFBITS_" // bits  will be defined if a subject contains RFbitsKey followed by a value of 2 digits
#define repeatRFwMQTT false // do we repeat a received signal by using mqtt
/*
RF supported protocols
433_1
433_2
433_3
433_4
433_5
433_6
*/
#define RFpulselengthKey "PLSL_" // pulselength will be defined if a subject contains RFprotocolKey followed by a value of 3 digits
// subject monitored to listen traffic processed by other gateways to store data and avoid ntuple
#define subjectMultiGTWRF "+/433toMQTT"

/*-------------------IR topics----------------------*/
//IR MQTT Subjects
#define subjectGTWIRtoMQTT "home/IRtoMQTT"
#define subjectIRtoMQTT "home/IRtoMQTT"
#define subjectMQTTtoIR "home/commands/MQTTtoIR"
#define subjectIRtoMQTTprotocol "home/IRtoMQTT/protocol"
#define subjectIRtoMQTTbits "home/IRtoMQTT/bits"
#define subjectIRtoMQTTRaw "home/IRtoMQTT/raw"
// subject monitored to listen traffic processed by other gateways to store data and avoid ntuple
#define subjectMultiGTWIR "+/IRtoMQTT"
#define IRbitsKey "IRBITS_" // bits  will be defined if a subject contains IRbitsKey followed by a value of 2 digits
#define IRRptKey "RPT_" // repeats  will be defined if a subject contains IRRptKey followed by a value of 1 digit
#define repeatIRwMQTT false // do we repeat a received signal by using mqtt

#define pubIRunknownPrtcl false // key to avoid mqtt publication of unknown IR protocol (set to true if you want to publish unknown protocol)
#define PanasonicAddress      0x4004     // Panasonic address (Pre data) 

#ifdef ESP8266 //IR supported protocols on ESP8266, all supported per default
  #define IR_GC
  #define IR_Raw
  #define IR_COOLIX
  #define IR_Whynter
  #define IR_LG
  #define IR_Sony
  #define IR_DISH
  #define IR_RC5
  #define IR_Sharp
  #define IR_SAMSUNG
  #define IR_PANASONIC
  #define IR_RCMM
#else //IR supported protocols on arduino uncomment if you want to send with this protocol, NEC protocol is available per default
  //#define IR_COOLIX
  //#define IR_Whynter
  //#define IR_LG
  //#define IR_Sony
  //#define IR_DISH
  //#define IR_RC5
  //#define IR_Sharp
  //#define IR_SAMSUNG
  //#define IR_Raw
  //#define IR_PANASONIC
#endif

/*----------------------BT topics-------------------------*/
#define subjectBTtoMQTT "home/BTtoMQTT/"

/*-------------------PIN DEFINITIONS----------------------*/
#define IR_RECEIVER_PIN 2 // put 2 = D4 on nodemcu, 2 = D2 on arduino
#define RF_EMITTER_PIN 4 //put 4 = D2 on nodemcu, 4 = D4 on arduino

#ifdef ESP8266
  #define IR_EMITTER_PIN 14 // 14 = D5 on nodemcu #define only usefull for ESP8266
  //RF PIN definition
  #define RF_RECEIVER_PIN 5 //  5 = D1 on nodemcu
  #define BT_RX D7 //ESP8266 RX connect HM-10 TX
  #define BT_TX D6 //ESP8266 TX connect HM-10 RX
#else
  //IMPORTANT NOTE: On arduino UNO connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/IRremoteInt.h so as to free pin D3 for RF RECEIVER PIN
  //RF PIN definition
  #define RF_RECEIVER_PIN 1 //  1 = D3 on arduino
  #define BT_RX 5 //arduino RX connect HM-10 TX
  #define BT_TX 6 //arduino TX connect HM-10 RX
#endif
//RF number of signal repetition
#define RF_EMITTER_REPEAT 20

/*-------------------ACTIVATE TRACES----------------------*/
#define TRACE 1  // 0= trace off 1 = trace on

