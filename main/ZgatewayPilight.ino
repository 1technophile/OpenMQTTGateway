/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data based on ESPilight library
 - publish MQTT data to a different topic related to received 433Mhz signal based on ESPilight library

    Copyright: (c)Florian ROBERT
    Pilight Gateway made by steadramon, improvments with the help of puuu

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

#ifdef ZgatewayPilight

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

#  include <ESPiLight.h>
ESPiLight rf(RF_EMITTER_GPIO); // use -1 to disable transmitter

#  ifdef Pilight_rawEnabled
// raw output support
bool pilightRawEnabled = 0;
#  endif

void pilightCallback(const String& protocol, const String& message, int status,
                     size_t repeats, const String& deviceID) {
  if (status == VALID) {
    Log.trace(F("Creating RF PiLight buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
    JsonObject RFPiLightdata = jsonBuffer.to<JsonObject>();
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer2;
    JsonObject msg = jsonBuffer2.to<JsonObject>();
    auto error = deserializeJson(jsonBuffer2, message);
    if (error) {
      Log.error(F("deserializeJson() failed: %s" CR), error.c_str());
      return;
    }
    RFPiLightdata["message"] = msg;
    RFPiLightdata["protocol"] = (const char*)protocol.c_str();
    RFPiLightdata["length"] = (const char*)deviceID.c_str();

    const char* device_id = deviceID.c_str();
    if (!strlen(device_id)) {
      // deviceID returned from Pilight is only extracted from id field
      // but some device may use another name as unique identifier
      char* choices[] = {"key", "unit", "device_id", "systemcode", "unitcode", "programcode"};

      for (uint8_t i = 0; i < 6; i++) {
        if (msg[choices[i]]) {
          device_id = (const char*)msg[choices[i]];
          break;
        }
      }
    }

    RFPiLightdata["value"] = device_id;
    RFPiLightdata["repeats"] = (int)repeats;
    RFPiLightdata["status"] = (int)status;
    pub(subjectPilighttoMQTT, RFPiLightdata);
    if (repeatPilightwMQTT) {
      Log.trace(F("Pub Pilight for rpt" CR));
      pub(subjectMQTTtoPilight, RFPiLightdata);
    }
  }
}

#  ifdef Pilight_rawEnabled
void pilightRawCallback(const uint16_t* pulses, size_t length) {
  uint16_t pulse;

  if (!pilightRawEnabled) {
    Log.trace(F("Pilight RAW not enabled" CR));
    return;
  }

  Log.trace(F("Creating RF PiLight buffer" CR));
  StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject RFPiLightdata = jsonBuffer.to<JsonObject>();

  RFPiLightdata["format"] = "RAW";
  RFPiLightdata["rawlen"] = length;
  RFPiLightdata["pulsesString"] = rf.pulseTrainToString(pulses, length); // c=pulse_array_key;p=pulse_types

  // publish data
  pub(subjectPilighttoMQTT, RFPiLightdata);
}
#  endif

void setupPilight() {
#  ifdef ZradioCC1101 //receiving with CC1101
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);
  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY);
#  endif
  rf.setCallback(pilightCallback);
  rf.initReceiver(RF_RECEIVER_GPIO);
  pinMode(RF_EMITTER_GPIO, OUTPUT); // Set this here, because if this is the RX pin it was reset to INPUT by Serial.end();
  Log.notice(F("RF_EMITTER_GPIO: %d " CR), RF_EMITTER_GPIO);
  Log.notice(F("RF_RECEIVER_GPIO: %d " CR), RF_RECEIVER_GPIO);
  Log.trace(F("ZgatewayPilight command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoPilight);
  Log.trace(F("ZgatewayPilight setup done " CR));
}

void savePilightConfig() {
  Log.trace(F("saving Pilight config" CR));
  DynamicJsonDocument json(4096);
  deserializeJson(json, rf.enabledProtocols());

  File configFile = SPIFFS.open("/pilight.json", "w");
  if (!configFile) {
    Log.error(F("failed to open config file for writing" CR));
  }

  serializeJsonPretty(json, Serial);
  serializeJson(json, configFile);
  configFile.close();
}

void loadPilightConfig() {
  Log.trace(F("reading Pilight config file" CR));
  File configFile = SPIFFS.open("/pilight.json", "r");
  if (configFile) {
    Log.trace(F("opened Pilight config file" CR));
    DynamicJsonDocument json(configFile.size() * 4);
    auto error = deserializeJson(json, configFile);
    if (error) {
      Log.error(F("deserialize config failed: %s, buffer capacity: %u" CR), error.c_str(), json.capacity());
    }
    serializeJson(json, Serial);
    if (!json.isNull()) {
      String rflimit;
      serializeJson(json, rflimit);
      rf.limitProtocols(rflimit);
    } else {
      Log.warning(F("failed to load json config" CR));
    }
    configFile.close();
  }
}

void PilighttoMQTT() {
  rf.loop();
}

void MQTTtoPilight(char* topicOri, JsonObject& Pilightdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoPilightProtocol)) {
    bool success = false;
    if (Pilightdata.containsKey("reset")) {
      rf.limitProtocols(rf.availableProtocols());
      savePilightConfig();
      success = true;
    }
    if (Pilightdata.containsKey("limit")) {
      String output;
      serializeJson(Pilightdata["limit"], output);
      rf.limitProtocols(output);
      savePilightConfig();
      success = true;
    }
    if (Pilightdata.containsKey("enabled")) {
      Log.notice(F("PiLight protocols enabled: %s" CR), rf.enabledProtocols().c_str());
      success = true;
    }
    if (Pilightdata.containsKey("available")) {
      Log.notice(F("PiLight protocols available: %s" CR), rf.availableProtocols().c_str());
      success = true;
    }
#  ifdef Pilight_rawEnabled
    if (Pilightdata.containsKey("rawEnabled")) {
      Log.notice(F("Setting PiLight raw output enabled: %s" CR), Pilightdata["rawEnabled"]);
      pilightRawEnabled = (bool)Pilightdata["rawEnabled"];
      disablePilightReceive();
      delay(1);
      enablePilightReceive();
      success = true;
    }
#  endif

    if (success) {
      pub(subjectGTWPilighttoMQTT, Pilightdata); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    } else {
      pub(subjectGTWPilighttoMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("MQTTtoPilightProtocol Fail json" CR));
    }
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoPilight)) {
    const char* message = Pilightdata["message"];
    const char* protocol = Pilightdata["protocol"];
    const char* raw = Pilightdata["raw"];
    float tempMhz = Pilightdata["mhz"];
    bool success = false;
    if (raw) {
      uint16_t codes[MAXPULSESTREAMLENGTH];
      int repeats = rf.stringToRepeats(raw);
      if (repeats < 0) {
        switch (repeats) {
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_R:
            Log.trace(F("'r' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_END:
            Log.trace(F("';' or '@' not found in data string" CR));
            break;
        }
        repeats = 10;
      }
      int msgLength = rf.stringToPulseTrain(raw, codes, MAXPULSESTREAMLENGTH);
      if (msgLength > 0) {
#  ifdef ZradioCC1101
        disableActiveReceiver();
        ELECHOUSE_cc1101.Init();
        pinMode(RF_EMITTER_GPIO, OUTPUT);
        ELECHOUSE_cc1101.SetTx(receiveMhz); // set Transmit on
        rf.disableReceiver();
#  endif
        rf.sendPulseTrain(codes, msgLength, repeats);
        Log.notice(F("MQTTtoPilight raw ok" CR));
        success = true;
      } else {
        Log.trace(F("MQTTtoPilight raw KO" CR));
        switch (msgLength) {
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_C:
            Log.trace(F("'c' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_P:
            Log.trace(F("'p' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_END:
            Log.trace(F("';' or '@' not found in data string" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_TYPE:
            Log.trace(F("pulse type not defined" CR));
            break;
        }
        Log.error(F("Invalid JSON: raw data malformed" CR));
      }
    }
    if (message && protocol) {
      Log.trace(F("MQTTtoPilight msg & protocol ok" CR));
#  ifdef ZradioCC1101
      disableActiveReceiver();
      ELECHOUSE_cc1101.Init();
      pinMode(RF_EMITTER_GPIO, OUTPUT);
      ELECHOUSE_cc1101.SetTx(receiveMhz); // set Transmit on
      rf.disableReceiver();
#  endif
      int msgLength = rf.send(protocol, message);
      if (msgLength > 0) {
        Log.trace(F("Adv data MQTTtoPilight push state via PilighttoMQTT" CR));
        pub(subjectGTWPilighttoMQTT, Pilightdata);
        success = true;
      } else {
        switch (msgLength) {
          case ESPiLight::ERROR_UNAVAILABLE_PROTOCOL:
            Log.error(F("protocol is not available" CR));
            break;
          case ESPiLight::ERROR_INVALID_PILIGHT_MSG:
            Log.error(F("message is invalid" CR));
            break;
          case ESPiLight::ERROR_INVALID_JSON:
            Log.error(F("message is not a proper json object" CR));
            break;
          case ESPiLight::ERROR_NO_OUTPUT_PIN:
            Log.error(F("no transmitter pin" CR));
            break;
          default:
            Log.error(F("Invalid JSON: can't read message/protocol" CR));
        }
      }
    }
    if (Pilightdata.containsKey("active")) {
      Log.trace(F("PiLight active:" CR));
      activeReceiver = ACTIVE_PILIGHT; // Enable PILIGHT Gateway
      success = true;
    }
#  ifdef ZradioCC1101
    if (Pilightdata.containsKey("mhz") && validFrequency(tempMhz)) {
      receiveMhz = tempMhz;
      Log.notice(F("PiLight Receive mhz: %F" CR), receiveMhz);
      success = true;
    }
#  endif
    if (success) {
      pub(subjectGTWPilighttoMQTT, Pilightdata); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
    } else {
      pub(subjectGTWPilighttoMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      Log.error(F("MQTTtoPilight Fail json" CR));
    }
    enableActiveReceiver(false);
  }
}

extern void disablePilightReceive() {
  Log.trace(F("disablePilightReceive" CR));
  rf.initReceiver(-1);
  rf.disableReceiver();
};

extern void enablePilightReceive() {
#  ifdef ZradioCC1101
  Log.notice(F("Switching to Pilight Receiver: %F" CR), receiveMhz);
#  else
  Log.notice(F("Switching to Pilight Receiver" CR));
#  endif
#  ifdef ZgatewayRF
  disableRFReceive();
#  endif
#  ifdef ZgatewayRF2
  disableRF2Receive();
#  endif
#  ifdef ZgatewayRTL_433
  disableRTLreceive();
#  endif

#  ifdef ZradioCC1101
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.SetRx(receiveMhz); // set Receive on
#  endif
  rf.setCallback(pilightCallback);
#  ifdef Pilight_rawEnabled
  if (pilightRawEnabled) {
    rf.setPulseTrainCallBack(pilightRawCallback);
  }
#  endif
  rf.initReceiver(RF_RECEIVER_GPIO);
  pinMode(RF_EMITTER_GPIO, OUTPUT); // Set this here, because if this is the RX pin it was reset to INPUT by Serial.end();
  rf.enableReceiver();
  loadPilightConfig();
};
#endif
