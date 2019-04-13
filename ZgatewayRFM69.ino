/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your RF/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
 - receive MQTT data from a topic and send RF signal corresponding to the received MQTT data with an RFM69 module
 - publish MQTT data to a different topic related to received signal from an RFM69 module

    Copyright: (c)Felix Rusu LowPowerLab.com
    Library and code by Felix Rusu - felix@lowpowerlab.com
    Modification of the code nanohab from bbx10 https://github.com/bbx10/nanohab
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
#ifdef ZgatewayRFM69

#include <RFM69.h>                //https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>
#include <EEPROM.h>

char RadioConfig[128];

// vvvvvvvvv Global Configuration vvvvvvvvvvv

struct _GLOBAL_CONFIG {
  uint32_t    checksum;
  char        rfmapname[32];
  char        encryptkey[16+1];
  uint8_t     networkid;
  uint8_t     nodeid;
  uint8_t     powerlevel; // bits 0..4 power leve, bit 7 RFM69HCW 1=true
  uint8_t     rfmfrequency;
};

#define GC_POWER_LEVEL    (pGC->powerlevel & 0x1F)
#define GC_IS_RFM69HCW  ((pGC->powerlevel & 0x80) != 0)
#define SELECTED_FREQ(f)  ((pGC->rfmfrequency==f)?"selected":"")

struct _GLOBAL_CONFIG *pGC;

// vvvvvvvvv Global Configuration vvvvvvvvvvv
uint32_t gc_checksum() {
  uint8_t *p = (uint8_t *)pGC;
  uint32_t checksum = 0;
  p += sizeof(pGC->checksum);
  for (size_t i = 0; i < (sizeof(*pGC) - 4); i++) {
    checksum += *p++;
  }
  return checksum;
}

#if defined(ESP8266) || defined(ESP32)
void eeprom_setup() {
  EEPROM.begin(4096);
  pGC = (struct _GLOBAL_CONFIG *)EEPROM.getDataPtr();
  // if checksum bad init GC else use GC values
  if (gc_checksum() != pGC->checksum) {
    trc(F("Factory reset"));
    memset(pGC, 0, sizeof(*pGC));
    strcpy_P(pGC->encryptkey, ENCRYPTKEY);
    strcpy_P(pGC->rfmapname, RFM69AP_NAME);
    pGC->networkid = NETWORKID;
    pGC->nodeid = NODEID;
    pGC->powerlevel = ((IS_RFM69HCW)?0x80:0x00) | POWER_LEVEL;
    pGC->rfmfrequency = FREQUENCY;
    pGC->checksum = gc_checksum();
    EEPROM.commit();
  }
}
#endif

RFM69 radio;

void setupRFM69(void) {
  #if defined(ESP8266) || defined(ESP32)
    eeprom_setup();
  #endif
  int freq;
  static const char PROGMEM JSONtemplate[] =
    R"({"msgType":"config","freq":%d,"rfm69hcw":%d,"netid":%d,"power":%d})";
  char payload[128];

  radio = RFM69(RFM69_CS, RFM69_IRQ, GC_IS_RFM69HCW, RFM69_IRQN);

  // Initialize radio
  if (!radio.initialize(pGC->rfmfrequency, pGC->nodeid, pGC->networkid))
  {
    trc(F("ZgatewayRFM69 initialization failed"));
    }

  if (GC_IS_RFM69HCW) {
    radio.setHighPower();    // Only for RFM69HCW & HW!
  }
  radio.setPowerLevel(GC_POWER_LEVEL); // power output ranges from 0 (5dBm) to 31 (20dBm)

  if (pGC->encryptkey[0] != '\0') radio.encrypt(pGC->encryptkey);

  trc(F("ZgatewayRFM69 Listening and transmitting at"));
  switch (pGC->rfmfrequency) {
    case RF69_433MHZ:
      freq = 433;
      break;
    case RF69_868MHZ:
      freq = 868;
      break;
    case RF69_915MHZ:
      freq = 915;
      break;
    case RF69_315MHZ:
      freq = 315;
      break;
    default:
      freq = -1;
      break;
  }
  trc(freq);

  size_t len = snprintf_P(RadioConfig, sizeof(RadioConfig), JSONtemplate,
      freq, GC_IS_RFM69HCW, pGC->networkid, GC_POWER_LEVEL);
  if (len >= sizeof(RadioConfig)) {
    trc(F("\n\n*** RFM69 config truncated ***\n"));
  }
}

boolean RFM69toMQTT(void) {
  //check if something was received (could be an interrupt from the radio)
  if (radio.receiveDone())
  {
    trc(F("Creating RFM69 buffer"));
    StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject& RFM69data = jsonBuffer.createObject();
    uint8_t data[RF69_MAX_DATA_LEN+1]; // For the null character
    uint8_t SENDERID = radio.SENDERID;
    uint8_t DATALEN = radio.DATALEN;
    uint16_t RSSI = radio.RSSI;

    //save packet because it may be overwritten
    memcpy(data, (void *)radio.DATA, DATALEN);
    data[DATALEN] = '\0';  // Terminate the string

    // Ack as soon as possible
    //check if sender wanted an ACK
    if (radio.ACKRequested())
    {
      radio.sendACK();
    }
    //updateClients(senderId, rssi, (const char *)data);

    trc(F("Data received"));
    trc((const char *)data);

    char buff[sizeof(subjectRFM69toMQTT)+4];
    sprintf(buff, "%s/%d", subjectRFM69toMQTT, SENDERID);
    RFM69data.set("data", (char *)data);
    RFM69data.set("rssi", (int)radio.RSSI); 
    RFM69data.set("senderid", (int)radio.SENDERID);
    pub(buff,RFM69data);

    return true;

  } else {
    return false;
  }
}

#ifdef simpleReceiving
  void MQTTtoRFM69(char * topicOri, char * datacallback) {

    if(strstr(topicOri, subjectMQTTtoRFM69) != NULL){
      trc(F("MQTTtoRFM69 data analysis"));
      char data[RF69_MAX_DATA_LEN+1];
      memcpy(data, (void *)datacallback, RF69_MAX_DATA_LEN);
      data[RF69_MAX_DATA_LEN] = '\0';
    
      //We look into the subject to see if a special RF protocol is defined
      String topic = topicOri;
      int valueRCV = defaultRFM69ReceiverId; //default receiver id value
      int pos = topic.lastIndexOf(RFM69receiverKey);
      if (pos != -1){
        pos = pos + +strlen(RFM69receiverKey);
        valueRCV = (topic.substring(pos,pos + 3)).toInt();
        trc(F("RFM69 receiver ID:"));
        trc(valueRCV);
      }
      if(radio.sendWithRetry(valueRCV, data, strlen(data)),10) {
        trc(F(" OK "));
        // Acknowledgement to the GTWRF topic
        char buff[sizeof(subjectGTWRFM69toMQTT)+4];
        sprintf(buff, "%s/%d", subjectGTWRFM69toMQTT, radio.SENDERID);
        pub(buff, data);
      } else {
        trc(F("RFM69 sending failed"));
      }
    }
  }
#endif
#ifdef jsonReceiving
  void MQTTtoRFM69(char * topicOri, JsonObject& RFM69data) {
  
   if (strcmp(topicOri,subjectMQTTtoRFM69) == 0){
      const char * data = RFM69data["data"];
      trc(F("MQTTtoRFM69 json data analysis"));
      if(data){
        trc(F("MQTTtoRFM69 data ok"));
        int valueRCV = RFM69data["receiverid"]| defaultRFM69ReceiverId; //default receiver id value
        trc(F("RFM69 receiver ID:"));
        trc(valueRCV);
        if(radio.sendWithRetry(valueRCV, data, strlen(data)),10) {
          trc(F(" OK "));
          // Acknowledgement to the GTWRF topic
          pub(subjectGTWRFM69toMQTT, RFM69data);// we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
        } else {
          trc(F("MQTTtoRFM69 sending failed"));
        }
      }else{
        trc(F("MQTTtoRFM69 Fail reading from json"));
      }
    }
  }
#endif
#endif
