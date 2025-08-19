/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data based on ESPilight library
 - publish MQTT data to a different topic related to received 433Mhz signal based on ESPilight library

    Copyright: (c)Florian ROBERT
    Pilight gateway made by steadramon, improvments with the help of puuu

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
#  include "TheengsCommon.h"

#  if defined(ESP32)
#    include <SPIFFS.h>
#  else
#    include <FS.h>
#  endif

#  include "config_RF.h"

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

#  include <ESPiLight.h>
ESPiLight rf(RF_EMITTER_GPIO); // use -1 to disable transmitter
extern int currentReceiver;
#  ifdef Pilight_rawEnabled
// raw output support
bool pilightRawEnabled = 0;
#  endif

void disableCurrentReceiver();
void initCC1101();
void enableActiveReceiver();

void pilightCallback(const String& protocol, const String& message, int status,
                     size_t repeats, const String& deviceID) {
  if (status == VALID) {
    THEENGS_LOG_TRACE(F("Creating RF PiLight buffer" CR));
    StaticJsonDocument<JSON_MSG_BUFFER> RFPiLightdataBuffer;
    JsonObject RFPiLightdata = RFPiLightdataBuffer.to<JsonObject>();
    StaticJsonDocument<JSON_MSG_BUFFER> jsonBuffer2;
    JsonObject msg = jsonBuffer2.to<JsonObject>();

    if (message.length() > 0) {
      auto error = deserializeJson(jsonBuffer2, message);
      if (error) {
        THEENGS_LOG_ERROR(F("deserializeJson() failed: %s" CR), error.c_str());
        return;
      }
      RFPiLightdata["message"] = msg;
    }

    if (protocol.length() > 0) {
      RFPiLightdata["protocol"] = protocol;
    }

    if (deviceID.length() > 0) {
      // If deviceID is non-empty, use it directly for value
      RFPiLightdata["value"] = deviceID;
    } else if (!msg.isNull()) {
      // deviceID returned from Pilight is only extracted from id field
      // but some device may use another name as unique identifier
      char* choices[] = {"key", "unit", "device_id", "systemcode", "unitcode", "programcode"};

      for (uint8_t i = 0; i < 6; i++) {
        if (msg.containsKey(choices[i])) {
          // Set value directly from msg; supports both strings and integers
          RFPiLightdata["value"] = msg[choices[i]];
          break;
        }
      }
    }

    RFPiLightdata["repeats"] = (int)repeats;
    RFPiLightdata["status"] = (int)status;
    RFPiLightdata["origin"] = subjectPilighttoMQTT;
    enqueueJsonObject(RFPiLightdata);
    if (repeatPilightwMQTT) {
      THEENGS_LOG_TRACE(F("Pub Pilight for rpt" CR));
      RFPiLightdata["origin"] = subjectMQTTtoPilight;
      enqueueJsonObject(RFPiLightdata);
    }
  }
}

#  ifdef Pilight_rawEnabled
void pilightRawCallback(const uint16_t* pulses, size_t length) {
  uint16_t pulse;

  if (!pilightRawEnabled) {
    THEENGS_LOG_TRACE(F("Pilight RAW not enabled" CR));
    return;
  }

  StaticJsonDocument<JSON_MSG_BUFFER> RFPiLightdataBuffer;
  JsonObject RFPiLightdata = RFPiLightdataBuffer.to<JsonObject>();

  RFPiLightdata["format"] = "RAW";
  RFPiLightdata["rawlen"] = length;
  RFPiLightdata["pulsesString"] = rf.pulseTrainToString(pulses, length); // c=pulse_array_key;p=pulse_types

  // Enqueue data
  RFPiLightdata["origin"] = subjectPilighttoMQTT;
  enqueueJsonObject(RFPiLightdata);
}
#  endif

void savePilightConfig() {
  THEENGS_LOG_TRACE(F("saving Pilight config" CR));
  DynamicJsonDocument json(4096);
  deserializeJson(json, rf.enabledProtocols());

  File configFile = SPIFFS.open("/pilight.json", "w");
  if (!configFile) {
    THEENGS_LOG_ERROR(F("failed to open config file for writing" CR));
  }

  serializeJsonPretty(json, Serial);
  serializeJson(json, configFile);
  configFile.close();
}

void loadPilightConfig() {
  THEENGS_LOG_TRACE(F("reading Pilight config file" CR));
  File configFile = SPIFFS.open("/pilight.json", "r");
  if (configFile) {
    THEENGS_LOG_TRACE(F("opened Pilight config file" CR));
    DynamicJsonDocument json(configFile.size() * 4);
    auto error = deserializeJson(json, configFile);
    if (error) {
      THEENGS_LOG_ERROR(F("deserialize config failed: %s, buffer capacity: %u" CR), error.c_str(), json.capacity());
    }
    serializeJson(json, Serial);
    if (!json.isNull()) {
      String rflimit;
      serializeJson(json, rflimit);
      rf.limitProtocols(rflimit);
    } else {
      THEENGS_LOG_WARNING(F("failed to load json config" CR));
    }
    configFile.close();
  }
}

void PilighttoX() {
  rf.loop();
}

void XtoPilight(const char* topicOri, JsonObject& Pilightdata) {
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
      THEENGS_LOG_NOTICE(F("PiLight protocols enabled: %s" CR), rf.enabledProtocols().c_str());
      success = true;
    }
    if (Pilightdata.containsKey("available")) {
      THEENGS_LOG_NOTICE(F("PiLight protocols available: %s" CR), rf.availableProtocols().c_str());
      success = true;
    }
#  ifdef Pilight_rawEnabled
    if (Pilightdata.containsKey("rawEnabled")) {
      THEENGS_LOG_NOTICE(F("Setting PiLight raw output enabled: %T" CR), (bool)Pilightdata["rawEnabled"]);
      pilightRawEnabled = (bool)Pilightdata["rawEnabled"];
      if (currentReceiver == ACTIVE_PILIGHT) {
        disablePilightReceive();
        delay(1);
        enablePilightReceive();
        success = true;
      }
    }
#  endif

    if (success) {
      // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      Pilightdata["origin"] = subjectGTWPilighttoMQTT;
      enqueueJsonObject(Pilightdata);
    } else {
      pub(subjectGTWPilighttoMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      THEENGS_LOG_ERROR(F("MQTTtoPilightProtocol Fail json" CR));
    }
  } else if (cmpToMainTopic(topicOri, subjectMQTTtoPilight)) {
    const char* message = Pilightdata["message"];
    THEENGS_LOG_NOTICE(F("MQTTtoPilight message: %s" CR), message);
    const char* protocol = Pilightdata["protocol"];
    THEENGS_LOG_NOTICE(F("MQTTtoPilight protocol: %s" CR), protocol);
    const char* raw = Pilightdata["raw"];
    float txFrequency = Pilightdata["frequency"] | RFConfig.frequency;
    bool success = false;
    disableCurrentReceiver();
    initCC1101();
#  ifdef ZradioCC1101 // set Receive off and Transmitt on
    ELECHOUSE_cc1101.SetTx(txFrequency);
    THEENGS_LOG_NOTICE(F("Transmit frequency: %F" CR), txFrequency);
#  endif
    pinMode(RF_EMITTER_GPIO, OUTPUT);
    if (raw) {
      uint16_t codes[MAXPULSESTREAMLENGTH];
      int repeats = rf.stringToRepeats(raw);
      if (repeats < 0) {
        switch (repeats) {
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_R:
            THEENGS_LOG_TRACE(F("'r' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_END:
            THEENGS_LOG_TRACE(F("';' or '@' not found in data string" CR));
            break;
        }
        repeats = 10;
      }
      int msgLength = rf.stringToPulseTrain(raw, codes, MAXPULSESTREAMLENGTH);
      if (msgLength > 0) {
        rf.sendPulseTrain(codes, msgLength, repeats);
        THEENGS_LOG_NOTICE(F("MQTTtoPilight raw ok" CR));
        success = true;
      } else {
        THEENGS_LOG_TRACE(F("MQTTtoPilight raw KO" CR));
        switch (msgLength) {
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_C:
            THEENGS_LOG_TRACE(F("'c' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_P:
            THEENGS_LOG_TRACE(F("'p' not found in string, or has no data" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_END:
            THEENGS_LOG_TRACE(F("';' or '@' not found in data string" CR));
            break;
          case ESPiLight::ERROR_INVALID_PULSETRAIN_MSG_TYPE:
            THEENGS_LOG_TRACE(F("pulse type not defined" CR));
            break;
        }
        THEENGS_LOG_ERROR(F("Invalid JSON: raw data malformed" CR));
      }
    }
    if (message && protocol) {
      THEENGS_LOG_TRACE(F("MQTTtoPilight msg & protocol ok" CR));
      int msgLength = rf.send(protocol, message);
      if (msgLength > 0) {
        THEENGS_LOG_TRACE(F("Adv data XtoPilight push state via PilighttoMQTT" CR));
        // Acknowledgement
        pub(subjectGTWPilighttoMQTT, message);
        success = true;
      } else {
        switch (msgLength) {
          case ESPiLight::ERROR_UNAVAILABLE_PROTOCOL:
            THEENGS_LOG_ERROR(F("protocol is not available" CR));
            break;
          case ESPiLight::ERROR_INVALID_PILIGHT_MSG:
            THEENGS_LOG_ERROR(F("message is invalid" CR));
            break;
          case ESPiLight::ERROR_INVALID_JSON:
            THEENGS_LOG_ERROR(F("message is not a proper json object" CR));
            break;
          case ESPiLight::ERROR_NO_OUTPUT_PIN:
            THEENGS_LOG_ERROR(F("no transmitter pin" CR));
            break;
          default:
            THEENGS_LOG_ERROR(F("Invalid JSON: can't read message/protocol" CR));
        }
      }
    }
    if (!success) {
      pub(subjectGTWPilighttoMQTT, "{\"Status\": \"Error\"}"); // Fail feedback
      THEENGS_LOG_ERROR(F("MQTTtoPilight Fail json" CR));
    }
    enableActiveReceiver();
  }
}

extern void disablePilightReceive() {
  THEENGS_LOG_TRACE(F("disablePilightReceive" CR));
  rf.initReceiver(-1);
  rf.disableReceiver();
};

extern void enablePilightReceive() {
  THEENGS_LOG_NOTICE(F("Switching to Pilight Receiver: %F" CR), RFConfig.frequency);
  THEENGS_LOG_NOTICE(F("RF_EMITTER_GPIO: %d " CR), RF_EMITTER_GPIO);
  THEENGS_LOG_NOTICE(F("RF_RECEIVER_GPIO: %d " CR), RF_RECEIVER_GPIO);
  THEENGS_LOG_TRACE(F("gatewayPilight command topic: %s%s%s" CR), mqtt_topic, gateway_name, subjectMQTTtoPilight);

  initCC1101();

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
  THEENGS_LOG_TRACE(F("gatewayPilight setup done " CR));
};
#endif
