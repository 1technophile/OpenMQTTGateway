/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the radiofrequency gateways (ZgatewayRF and ZgatewayRF2) with RCswitch and newremoteswitch library
  
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

/*-------------------RF topics & parameters----------------------*/
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
//RF number of signal repetition
#define RF_EMITTER_REPEAT 20

//variables to avoid duplicates for RF
#define time_avoid_duplicate 3000 // if you want to avoid duplicate mqtt message received set this to > 0, the value is the time in milliseconds during which we don't publish duplicates

/*-------------------RF2 topics & parameters----------------------*/
//433Mhz newremoteswitch MQTT Subjects and keys
#define subjectMQTTtoRF2 "home/commands/MQTTtoRF2"
#define RF2codeKey "CODE_" // code will be defined if a subject contains RF2codeKeyrotocolKey followed by a value of 7 digits
#define RF2periodKey "PERIOD_" // period  will be defined if a subject contains RF2periodKey followed by a value of 3 digits
#define RF2unitKey "UNIT_"  // number of your unit value  will be defined if a subject contains RF2unitKey followed by a value of 1 digit

/*-------------------PIN DEFINITIONS----------------------*/
#ifdef I2C_Wiring // With Support for I2C Modules
  #define RF_EMITTER_PIN 15 //put 15 = D8 on nodemcu
  #ifdef ESP8266
    //RF PIN definition
    #define RF_RECEIVER_PIN 0 //  0 = D3 on nodemcu
  #else
    //IMPORTANT NOTE: On arduino UNO connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/IRremoteInt.h so as to free pin D3 for RF RECEIVER PIN
    //RF PIN definition
    #define RF_RECEIVER_PIN 1 //  1 = D3 on arduino
  #endif
#endif

#ifdef Classic_Wiring // Without Support for I2C Modules
  #define RF_EMITTER_PIN 4 //put 4 = D2 on nodemcu, 4 = D4 on arduino
  #ifdef ESP8266
    //RF PIN definition
    #define RF_RECEIVER_PIN 5 //  5 = D1 on nodemcu
  #else
    //IMPORTANT NOTE: On arduino UNO connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/IRremoteInt.h so as to free pin D3 for RF RECEIVER PIN
    //RF PIN definition
    #define RF_RECEIVER_PIN 1 //  1 = D3 on arduino
  #endif
#endif

#ifdef RFM69_Wiring // Without Support for I2C Modules and HM10 or 11
  #ifdef ESP8266
    #define RF_RECEIVER_PIN 0 //  0 = D3 on nodemcu
    #define RF_EMITTER_PIN D2 //put D2 on nodemcu
  #else
    //IMPORTANT NOTE: On arduino UNO connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/IRremoteInt.h so as to free pin D3 for RF RECEIVER PIN
    //RF PIN definition
    #define RF_RECEIVER_PIN 1 //  1 = D3 on arduino
    #define RF_EMITTER_PIN 4 //4 = D4 on arduino
  #endif
#endif




