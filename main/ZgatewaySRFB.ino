/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
   - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data using SONOFF RF BRIDGE
   - publish MQTT data to a different topic related to received 433Mhz signal using SONOFF RF BRIDGE
 
    This implementation into OpenMQTTGateway is based on Xose Pérez work ESPURNA (https://bitbucket.org/xoseperez/espurna)

    Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>
    OpenMQTTGateway integration by Florian ROBERT
  
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

#ifdef ZgatewaySRFB

unsigned char _uartbuf[RF_MESSAGE_SIZE + 3] = {0};
unsigned char _uartpos = 0;

void setupSRFB() {
  Log.trace(F("ZgatewaySRFB setup done " CR));
  Log.trace(F("Serial Baud: %l" CR), SERIAL_BAUD);
}

void _rfbSend(byte* message) {
  Serial.println();
  Serial.write(RF_CODE_START);
  Serial.write(RF_CODE_RFOUT);
  for (unsigned char j = 0; j < RF_MESSAGE_SIZE; j++) {
    Serial.write(message[j]);
  }
  Serial.write(RF_CODE_STOP);
  Serial.flush();
  Serial.println();
}

void _rfbSend(byte* message, int times) {
  char buffer[RF_MESSAGE_SIZE];
  _rawToHex(message, buffer, RF_MESSAGE_SIZE);
  Log.notice(F("[RFBRIDGE] Sending MESSAGE" CR));

  for (int i = 0; i < times; i++) {
    if (i > 0) {
      unsigned long start = millis();
      while (millis() - start < RF_SEND_DELAY)
        delay(1);
    }
    _rfbSend(message);
  }
}

bool SRFBtoMQTT() {
  static bool receiving = false;

  while (Serial.available()) {
    yield();
    byte c = Serial.read();

    if (receiving) {
      if (c == RF_CODE_STOP) {
        _rfbDecode();
        receiving = false;
      } else {
        _uartbuf[_uartpos++] = c;
      }
    } else if (c == RF_CODE_START) {
      _uartpos = 0;
      receiving = true;
    }
  }
  return receiving;
}

void _rfbDecode() {
  static unsigned long last = 0;
  if (millis() - last < RF_RECEIVE_DELAY)
    return;
  last = millis();

  byte action = _uartbuf[0];
  char buffer[RF_MESSAGE_SIZE * 2 + 1] = {0};

  if (action == RF_CODE_RFIN) {
    _rawToHex(&_uartbuf[1], buffer, RF_MESSAGE_SIZE);

    Log.trace(F("Creating SRFB buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject SRFBdata = jsonBuffer.to<JsonObject>();
    SRFBdata["raw"] = String(buffer).substring(0, 18);

    int val_Tsyn = (int)(int)value_from_hex_data(buffer, 0, 4, false, false);
    SRFBdata["delay"] = (int)val_Tsyn;

    int val_Tlow = (int)value_from_hex_data(buffer, 4, 4, false, false);
    SRFBdata["val_Tlow"] = (int)val_Tlow;

    int val_Thigh = (int)value_from_hex_data(buffer, 8, 4, false, false);
    SRFBdata["val_Thigh"] = (int)val_Thigh;

    unsigned long MQTTvalue = (unsigned long)value_from_hex_data(buffer, 12, 8, false, false);
    SRFBdata["value"] = (unsigned long)MQTTvalue;

    if (!isAduplicateSignal(MQTTvalue) && MQTTvalue != 0) { // conditions to avoid duplications of RF -->MQTT
      Log.trace(F("Adv data SRFBtoMQTT" CR));
      pub(subjectSRFBtoMQTT, SRFBdata);
      Log.trace(F("Store val: %lu" CR), MQTTvalue);
      storeSignalValue(MQTTvalue);
      if (repeatSRFBwMQTT) {
        Log.trace(F("Publish SRFB for rpt" CR));
        pub(subjectMQTTtoSRFB, SRFBdata);
      }
    }
    _rfbAck();
  }
}

void _rfbAck() {
  Log.trace(F("[RFBRIDGE] Sending ACK\n" CR));
  Serial.println();
  Serial.write(RF_CODE_START);
  Serial.write(RF_CODE_ACK);
  Serial.write(RF_CODE_STOP);
  Serial.flush();
  Serial.println();
}

#  if simpleReceiving
void MQTTtoSRFB(char* topicOri, char* datacallback) {
  // RF DATA ANALYSIS
  String topic = topicOri;
  int valueRPT = 0;

  if (topic == subjectMQTTtoSRFB) {
    int valueMiniPLSL = 0;
    int valueMaxiPLSL = 0;
    int valueSYNC = 0;

    int pos = topic.lastIndexOf(SRFBRptKey);
    if (pos != -1) {
      pos = pos + +strlen(SRFBRptKey);
      valueRPT = (topic.substring(pos, pos + 1)).toInt();
      Log.notice(F("SRFB Repeat: %d" CR), valueRPT);
    }

    int pos2 = topic.lastIndexOf(SRFBminipulselengthKey);
    if (pos2 != -1) {
      pos2 = pos2 + strlen(SRFBminipulselengthKey);
      valueMiniPLSL = (topic.substring(pos2, pos2 + 3)).toInt();
      Log.notice(F("RF Mini Pulse Lgth: %d" CR), valueMiniPLSL);
    }

    int pos3 = topic.lastIndexOf(SRFBmaxipulselengthKey);
    if (pos3 != -1) {
      pos3 = pos3 + strlen(SRFBmaxipulselengthKey);
      valueMaxiPLSL = (topic.substring(pos3, pos3 + 2)).toInt();
      Log.notice(F("RF Maxi Pulse Lgth: %d" CR), valueMaxiPLSL);
    }

    int pos4 = topic.lastIndexOf(SRFBsyncKey);
    if (pos4 != -1) {
      pos4 = pos4 + strlen(SRFBsyncKey);
      valueSYNC = (topic.substring(pos4, pos4 + 2)).toInt();
      Log.notice(F("RF sync: %d" CR), valueSYNC);
    }

    Log.trace(F("MQTTtoSRFB prts" CR));
    if (valueRPT == 0)
      valueRPT = 1;
    if (valueMiniPLSL == 0)
      valueMiniPLSL = 320;
    if (valueMaxiPLSL == 0)
      valueMaxiPLSL = 900;
    if (valueSYNC == 0)
      valueSYNC = 9500;

    byte hex_valueMiniPLSL[2];
    hex_valueMiniPLSL[0] = (int)((valueMiniPLSL >> 8) & 0xFF);
    hex_valueMiniPLSL[1] = (int)(valueMiniPLSL & 0xFF);

    byte hex_valueMaxiPLSL[2];
    hex_valueMaxiPLSL[0] = (int)((valueMaxiPLSL >> 8) & 0xFF);
    hex_valueMaxiPLSL[1] = (int)(valueMaxiPLSL & 0xFF);

    byte hex_valueSYNC[2];
    hex_valueSYNC[0] = (int)((valueSYNC >> 8) & 0xFF);
    hex_valueSYNC[1] = (int)(valueSYNC & 0xFF);

    unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
    byte hex_data[3];
    hex_data[0] = (unsigned long)((data >> 16) & 0xFF);
    hex_data[1] = (unsigned long)((data >> 8) & 0xFF);
    hex_data[2] = (unsigned long)(data & 0xFF);

    byte message_b[RF_MESSAGE_SIZE];

    memcpy(message_b, hex_valueSYNC, 2);
    memcpy(message_b + 2, hex_valueMiniPLSL, 2);
    memcpy(message_b + 4, hex_valueMaxiPLSL, 2);
    memcpy(message_b + 6, hex_data, 3);

    _rfbSend(message_b, valueRPT);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWSRFBtoMQTT, datacallback); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
  }
  if (topic == subjectMQTTtoSRFBRaw) {
    int pos = topic.lastIndexOf(SRFBRptKey);
    if (pos != -1) {
      pos = pos + +strlen(SRFBRptKey);
      valueRPT = (topic.substring(pos, pos + 1)).toInt();
      Log.notice(F("SRFB Repeat: %d" CR), valueRPT);
    }
    if (valueRPT == 0)
      valueRPT = 1;

    byte message_b[RF_MESSAGE_SIZE];
    _hexToRaw(datacallback, message_b, RF_MESSAGE_SIZE);
    _rfbSend(message_b, valueRPT);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWSRFBtoMQTT, datacallback); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
  }
}
#  endif
#  if jsonReceiving
void MQTTtoSRFB(char* topicOri, JsonObject& SRFBdata) {
  // RF DATA ANALYSIS
  const char* raw = SRFBdata["raw"];
  int valueRPT = SRFBdata["repeat"] | 1;
  if (cmpToMainTopic(topicOri, subjectMQTTtoSRFB)) {
    Log.trace(F("MQTTtoSRFB json" CR));
    if (raw) { // send raw in priority when defined in the json
      Log.trace(F("MQTTtoSRFB raw ok" CR));
      byte message_b[RF_MESSAGE_SIZE];
      _hexToRaw(raw, message_b, RF_MESSAGE_SIZE);
      _rfbSend(message_b, valueRPT);
    } else {
      unsigned long data = SRFBdata["value"];
      if (data != 0) {
        Log.notice(F("MQTTtoSRFB data ok" CR));
        int valueMiniPLSL = SRFBdata["val_Tlow"];
        int valueMaxiPLSL = SRFBdata["val_Thigh"];
        int valueSYNC = SRFBdata["delay"];

        if (valueRPT == 0)
          valueRPT = 1;
        if (valueMiniPLSL == 0)
          valueMiniPLSL = 320;
        if (valueMaxiPLSL == 0)
          valueMaxiPLSL = 900;
        if (valueSYNC == 0)
          valueSYNC = 9500;

        Log.notice(F("SRFB Repeat: %d" CR), valueRPT);
        Log.notice(F("RF Mini Pulse Lgth: %d" CR), valueMiniPLSL);
        Log.notice(F("RF Maxi Pulse Lgth: %d" CR), valueMaxiPLSL);
        Log.notice(F("RF sync: %d" CR), valueSYNC);

        byte hex_valueMiniPLSL[2];
        hex_valueMiniPLSL[0] = (int)((valueMiniPLSL >> 8) & 0xFF);
        hex_valueMiniPLSL[1] = (int)(valueMiniPLSL & 0xFF);

        byte hex_valueMaxiPLSL[2];
        hex_valueMaxiPLSL[0] = (int)((valueMaxiPLSL >> 8) & 0xFF);
        hex_valueMaxiPLSL[1] = (int)(valueMaxiPLSL & 0xFF);

        byte hex_valueSYNC[2];
        hex_valueSYNC[0] = (int)((valueSYNC >> 8) & 0xFF);
        hex_valueSYNC[1] = (int)(valueSYNC & 0xFF);

        byte hex_data[3];
        hex_data[0] = (unsigned long)((data >> 16) & 0xFF);
        hex_data[1] = (unsigned long)((data >> 8) & 0xFF);
        hex_data[2] = (unsigned long)(data & 0xFF);

        byte message_b[RF_MESSAGE_SIZE];

        memcpy(message_b, hex_valueSYNC, 2);
        memcpy(message_b + 2, hex_valueMiniPLSL, 2);
        memcpy(message_b + 4, hex_valueMaxiPLSL, 2);
        memcpy(message_b + 6, hex_data, 3);

        Log.notice(F("MQTTtoSRFB OK" CR));
        _rfbSend(message_b, valueRPT);
        // Acknowledgement to the GTWRF topic
        pub(subjectGTWSRFBtoMQTT, SRFBdata); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      } else {
        Log.error(F("MQTTtoSRFB error decoding value" CR));
      }
    }
  }
}
#  endif
#endif
