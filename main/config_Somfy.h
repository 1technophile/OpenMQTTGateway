#ifndef config_Somfy_h
#define config_Somfy_h

#include <Arduino.h>
#include <ArduinoJson.h>

extern void setupSomfy();
extern void MQTTtoSomfy(char* topicOri, JsonObject& RFdata);
/*----------------------------USER PARAMETERS-----------------------------*/
#define EEPROM_ADDRESS_START 0
#define SOMFY_REMOTE_NUM     1

// Do not change the order of the remotes, because based on the index the rolling codes are stored
const uint32_t somfyRemotes[SOMFY_REMOTE_NUM] = {0x5184c8};

/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectMQTTtoSomfy "/commands/MQTTtoSomfy"

/*-------------------INTERNAL DEFINITIONS----------------------*/
#define CC1101_FREQUENCY_SOMFY 433.42

#endif
