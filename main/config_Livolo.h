#ifndef config_Livolo_h
#define config_Livolo_h

#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef ZgatewayLivolo
extern void setupLivolo();
extern void MQTTtoLivolo(char * topicOri, char * datacallback);
extern void MQTTtoLivolo(char * topicOri, JsonObject& RFdata);

/*-------------------Livolo topics & parameters----------------------*/
//433Mhz Livolo MQTT Subjects and keys
#define subjectMQTTtoLivolo  Base_Topic Gateway_Name "/commands/MQTTtoLivolo"

#ifndef LIVOLO_EMITTER_PIN
    #ifdef ESP8266
        #define LIVOLO_EMITTER_PIN 3 // RX on nodemcu if it doesn't work with 3, try with 4 (D2) // put 5 with rf bridge direct mod
    #elif ESP32
        #define LIVOLO_EMITTER_PIN 12 // D12 on DOIT ESP32
    #elif __AVR_ATmega2560__
        #define LIVOLO_EMITTER_PIN 4
    #else
        //IMPORTANT NOTE: On arduino UNO connect IR emitter pin to D9 , comment #define IR_USE_TIMER2 and uncomment #define IR_USE_TIMER1 on library <library>IRremote/boarddefs.h so as to free pin D3 for RF RECEIVER PIN
        //RF PIN definition
        #define LIVOLO_EMITTER_PIN 4 //4 = D4 on arduino
    #endif
#endif

#endif
#endif