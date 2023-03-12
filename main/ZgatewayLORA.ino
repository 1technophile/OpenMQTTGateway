/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR/BLE signal  and a MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send LORA signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received LORA signal

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
#include "User_config.h"

#ifdef ZgatewayLORA

#  include <LoRa.h>
#  include <SPI.h>
#  include <Wire.h>

#  define WIPHONE_MESSAGE_MAGIC   0x6c6d
#  define WIPHONE_MESSAGE_MIN_LEN sizeof(wiphone_message) - WIPHONE_MAX_MESSAGE_LEN
#  define WIPHONE_MAX_MESSAGE_LEN 230

typedef struct __attribute__((packed)) {
  // WiPhone uses RadioHead library which has additional (unused) headers
  uint8_t rh_to;
  uint8_t rh_from;
  uint8_t rh_id;
  uint8_t rh_flags;
  uint16_t magic;
  uint32_t to;
  uint32_t from;
  char message[WIPHONE_MAX_MESSAGE_LEN];
} wiphone_message;

enum LORA_ID_NUM {
  UNKNOWN_DEVICE = -1,
  WIPHONE,
};
typedef enum LORA_ID_NUM LORA_ID_NUM;

/*
Try and determine device given the payload
 */
uint8_t _determineDevice(byte* packet, int packetSize) {
  // Check WiPhone header
  if (packetSize >= WIPHONE_MESSAGE_MIN_LEN && ((wiphone_message*)packet)->magic == WIPHONE_MESSAGE_MAGIC)
    return WIPHONE;

  // No matches
  return UNKNOWN_DEVICE;
}

/*
Try and determine device given the JSON type
 */
uint8_t _determineDevice(JsonObject& LORAdata) {
  const char* protocol_name = LORAdata["type"];

  // No type provided
  if (!protocol_name)
    return UNKNOWN_DEVICE;

  if (strcmp(protocol_name, "WiPhone") == 0)
    return WIPHONE;

  // No matches
  return UNKNOWN_DEVICE;
}

/*
Create JSON information from WiPhone packet
 */
boolean _WiPhoneToMQTT(byte* packet, JsonObject& LORAdata) {
  // Decode the LoRa packet and send over MQTT
  wiphone_message* msg = (wiphone_message*)packet;

  // Set the header information
  char from[9] = {0};
  char to[9] = {0};
  snprintf(from, 9, "%06X", msg->from);
  snprintf(to, 9, "%06X", msg->to);

  // From and To are the last 3 octets from the WiPhone's ESP32 chip ID
  // Special case is 0x000000: "broadcast"
  LORAdata["from"] = from;
  LORAdata["to"] = to;

  LORAdata["message"] = msg->message;
  LORAdata["type"] = "WiPhone";
  return true;
}

/*
Create WiPhone packet from JSON
 */
boolean _MQTTtoWiPhone(JsonObject& LORAdata) {
  // Prepare a LoRa packet to send to the WiPhone
  wiphone_message wiphonemsg;
  wiphonemsg.rh_to = 0xff;
  wiphonemsg.rh_from = 0xff;
  wiphonemsg.rh_id = 0x00;
  wiphonemsg.rh_flags = 0x00;

  wiphonemsg.magic = WIPHONE_MESSAGE_MAGIC;
  wiphonemsg.from = strtol(LORAdata["from"], NULL, 16);
  wiphonemsg.to = strtol(LORAdata["to"], NULL, 16);
  const char* message = LORAdata["message"];
  strlcpy(wiphonemsg.message, message, WIPHONE_MAX_MESSAGE_LEN);
  LoRa.write((uint8_t*)&wiphonemsg, strlen(message) + WIPHONE_MESSAGE_MIN_LEN + 1);
  return true;
}

void setupLORA() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);

  if (!LoRa.begin(LORA_BAND)) {
    Log.error(F("ZgatewayLORA setup failed!" CR));
    while (1)
      ;
  }
  LoRa.receive();
  Log.notice(F("LORA_SCK: %d" CR), LORA_SCK);
  Log.notice(F("LORA_MISO: %d" CR), LORA_MISO);
  Log.notice(F("LORA_MOSI: %d" CR), LORA_MOSI);
  Log.notice(F("LORA_SS: %d" CR), LORA_SS);
  Log.notice(F("LORA_RST: %d" CR), LORA_RST);
  Log.notice(F("LORA_DI0: %d" CR), LORA_DI0);
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
  LoRa.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
  LoRa.setCodingRate4(LORA_CODING_RATE);
  LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  Log.trace(F("ZgatewayLORA setup done" CR));
}

void LORAtoMQTT() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject LORAdata = jsonBuffer.to<JsonObject>();
    Log.trace(F("Rcv. LORA" CR));
#  ifdef ESP32
    String taskMessage = "LORA Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
    //trc(taskMessage);
#  endif
    // Create packet and reserve null terminator space
    byte packet[packetSize + 1];
    boolean binary = false;
    for (int i = 0; i < packetSize; i++) {
      packet[i] = (char)LoRa.read();

      if (packet[i] < 32 || packet[i] > 127)
        binary = true;
    }
    // Terminate with a null character in case we have a string
    packet[packetSize] = 0;

    LORAdata["rssi"] = (int)LoRa.packetRssi();
    LORAdata["snr"] = (float)LoRa.packetSnr();
    LORAdata["pferror"] = (float)LoRa.packetFrequencyError();
    LORAdata["packetSize"] = (int)packetSize;

    uint8_t deviceId = _determineDevice(packet, packetSize);
    if (deviceId == WIPHONE) {
      _WiPhoneToMQTT(packet, LORAdata);
    } else if (binary) {
      // We have non-ascii data: create hex string of the data
      char hex[packetSize * 2 + 1];
      _rawToHex(packet, hex, packetSize);
      // Terminate with a null character
      hex[packetSize * 2] = 0;

      LORAdata["hex"] = hex;
    } else {
      // ascii payload
      LORAdata["message"] = packet;
    }
    pub(subjectLORAtoMQTT, LORAdata);
    if (repeatLORAwMQTT) {
      Log.trace(F("Pub LORA for rpt" CR));
      pub(subjectMQTTtoLORA, LORAdata);
    }
  }
}

#  if jsonReceiving
void MQTTtoLORA(char* topicOri, JsonObject& LORAdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORA)) {
    Log.trace(F("MQTTtoLORA json" CR));
    const char* message = LORAdata["message"];
    const char* hex = LORAdata["hex"];
    int txPower = LORAdata["txpower"] | LORA_TX_POWER;
    int spreadingFactor = LORAdata["spreadingfactor"] | LORA_SPREADING_FACTOR;
    long int frequency = LORAdata["frequency "] | LORA_BAND;
    long int signalBandwidth = LORAdata["signalbandwidth"] | LORA_SIGNAL_BANDWIDTH;
    int codingRateDenominator = LORAdata["codingrate"] | LORA_CODING_RATE;
    int preambleLength = LORAdata["preamblelength"] | LORA_PREAMBLE_LENGTH;
    byte syncWord = LORAdata["syncword"] | LORA_SYNC_WORD;
    bool crc = LORAdata["enablecrc"] | DEFAULT_CRC;
    bool invertIQ = LORAdata["invertiq"] | INVERT_IQ;
    if (message || hex) {
      LoRa.setTxPower(txPower);
      LoRa.setFrequency(frequency);
      LoRa.setSpreadingFactor(spreadingFactor);
      LoRa.setSignalBandwidth(signalBandwidth);
      LoRa.setCodingRate4(codingRateDenominator);
      LoRa.setPreambleLength(preambleLength);
      LoRa.setSyncWord(syncWord);
      if (crc)
        LoRa.enableCrc();
      else
        LoRa.disableCrc();

      if (invertIQ)
        LoRa.enableInvertIQ();
      else
        LoRa.disableInvertIQ();

      LoRa.beginPacket();
      uint8_t deviceId = _determineDevice(LORAdata);
      if (deviceId == WIPHONE) {
        _MQTTtoWiPhone(LORAdata);
      } else if (hex) {
        // We have hex data: create convert to binary
        byte raw[strlen(hex) / 2];
        _hexToRaw(hex, raw, sizeof(raw));
        LoRa.write((uint8_t*)raw, sizeof(raw));
      } else {
        // ascii payload
        LoRa.print(message);
      }

      LoRa.endPacket();
      Log.trace(F("MQTTtoLORA OK" CR));
      pub(subjectGTWLORAtoMQTT, LORAdata); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    } else {
      Log.error(F("MQTTtoLORA Fail json" CR));
    }
  }
}
#  endif
#  if simpleReceiving
void MQTTtoLORA(char* topicOri, char* LORAdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoLORA)) {
    LoRa.beginPacket();
    LoRa.print(LORAdata);
    LoRa.endPacket();
    Log.notice(F("MQTTtoLORA OK" CR));
    pub(subjectGTWLORAtoMQTT, LORAdata); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
  }
}
#  endif
#endif
