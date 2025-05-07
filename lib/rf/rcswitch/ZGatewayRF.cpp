/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
  This gateway enables to:
 - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received 433Mhz signal

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
#ifdef ZgatewayRF

#  include "ZGatewayRF.h"

#  include <ArduinoLog.h>
#  include <config_RF.h>
#  include <mqtt/MQTTPublisher.h>
#  include <rf/AbstractGatewayRF.h>
#  include <rf/ZCommonRF.h>

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

ZGatewayRF::ZGatewayRF(ZCommonRF& iZCommonRF) : AbstractGatewayRF(iZCommonRF) {
  // Constructor implementation
  // Initialize any necessary variables or configurations here
  mySwitch = RCSwitch();
}

bool ZGatewayRF::enableReceive(float rfFrequency, int rfReceiverGPIO, int rfEmitterGPIO) {
  try {
    enableRFReceive(rfFrequency, rfReceiverGPIO, rfEmitterGPIO);
    return true;
  } catch (const std::exception& e) {
    Log.error(F("[RF] Error enabling RF receive: %s" CR), e.what());
    return false;
  } catch (...) {
    Log.error(F("[RF] Unknown error enabling RF receive" CR));
    return false;
  }
};

bool ZGatewayRF::disableReceive() {
  try {
    disableRFReceive();
    return true;
  } catch (const std::exception& e) {
    Log.error(F("[RF] Error disabling RF receive: %s" CR), e.what());
    return false;
  } catch (...) {
    Log.error(F("[RF] Unknown error disabling RF receive" CR));
    return false;
  }
};

void ZGatewayRF::RFtoX() {
  if (mySwitch.available()) {
    StaticJsonDocument<JSON_MSG_BUFFER> RFdataBuffer;
    JsonObject RFdata = RFdataBuffer.to<JsonObject>();
#  ifdef ESP32
    Log.trace(F("[RF] Rcv. RF - Task running on core :%d" CR), xPortGetCoreID());
#  else
    Log.trace(F("[RF] Rcv. RF" CR));
#  endif
    uint64_t MQTTvalue = mySwitch.getReceivedValue();
    int length = mySwitch.getReceivedBitlength();
    const char* binary = dec2binWzerofill(MQTTvalue, length);

    RFdata["value"] = (uint64_t)MQTTvalue;
    RFdata["protocol"] = (int)mySwitch.getReceivedProtocol();
    RFdata["length"] = (int)mySwitch.getReceivedBitlength();
    RFdata["delay"] = (int)mySwitch.getReceivedDelay();
    RFdata["tre_state"] = bin2tristate(binary);
    RFdata["binary"] = binary;

#  if defined(ESP32) || defined(ESP8266)
    unsigned int* raw = mySwitch.getReceivedRawdata();
    std::string rawDump;
    for (unsigned int i = 0; i < length * 2; i++) {
      if (i != 0)
        rawDump += ",";
      rawDump += std::to_string(raw[i]);
    }
    RFdata["raw"] = rawDump;
#  endif

#  ifdef ZradioCC1101 // set Receive off and Transmitt on
    RFdata["frequency"] = RFConfig.frequency;
#  endif

    mySwitch.resetAvailable();

    if (!isAduplicateSignal(MQTTvalue) && MQTTvalue != 0) { // conditions to avoid duplications of RF -->MQTT
#  if defined(ZmqttDiscovery) && defined(RF_on_HAS_as_DeviceTrigger)
      if (SYSConfig.discovery)
        announceGatewayTriggerTypeToHASS(MQTTvalue);
#  endif
      RFdata["origin"] = subjectRFtoMQTT;
      rfHandler.getPublisher().enqueueJsonObject(RFdata);
      // Casting "receivedSignal[o].value" to (unsigned long) because ArduinoLog doesn't support uint64_t for ESP's
      Log.trace(F("[RF] Store val: %u" CR), (unsigned long)MQTTvalue);
      storeSignalValue(MQTTvalue);
      if (repeatRFwMQTT) {
        Log.trace(F("[RF] Pub RF for rpt" CR));
        RFdata["origin"] = subjectMQTTtoRF;
        rfHandler.getPublisher().enqueueJsonObject(RFdata);
      }
    } else {
      Log.trace(F("[RF] RF signal received but already managed" CR));
    }
  }
  // else {
  //    No RF signal received
  // }
}

#  if simpleReceiving // FALSE MEAN you don't want to use old way reception analysis
void ZGatewayRF::XtoRF(const char* topicOri, const char* datacallback) {
#    ifdef ZradioCC1101 // set Receive off and Transmitt on
  disableCurrentReceiver();
  ELECHOUSE_cc1101.SetTx(RFConfig.frequency);
  Log.notice(F("[RF] Transmit frequency: %F" CR), RFConfig.frequency);
#    endif
  mySwitch.disableReceive();
  mySwitch.enableTransmit(RF_EMITTER_GPIO);
  uint64_t data = strtoull(datacallback, NULL, 10); // we will not be able to pass values > 4294967295 on Arduino boards

  // RF DATA ANALYSIS
  //We look into the subject to see if a special RF protocol is defined
  String topic = topicOri;
  int valuePRT = 0;
  int valuePLSL = 0;
  int valueBITS = 0;
  int pos = topic.lastIndexOf(RFprotocolKey);
  if (pos != -1) {
    pos = pos + +strlen(RFprotocolKey);
    valuePRT = (topic.substring(pos, pos + 1)).toInt();
  }
  //We look into the subject to see if a special RF pulselength is defined
  int pos2 = topic.lastIndexOf(RFpulselengthKey);
  if (pos2 != -1) {
    pos2 = pos2 + strlen(RFpulselengthKey);
    valuePLSL = (topic.substring(pos2, pos2 + 3)).toInt();
  }
  int pos3 = topic.lastIndexOf(RFbitsKey);
  if (pos3 != -1) {
    pos3 = pos3 + strlen(RFbitsKey);
    valueBITS = (topic.substring(pos3, pos3 + 2)).toInt();
  }

  if ((cmpToMainTopic(topicOri, subjectMQTTtoRF)) && (valuePRT == 0) && (valuePLSL == 0) && (valueBITS == 0)) {
    Log.trace(F("[RF] MQTTtoRF dflt" CR));
    mySwitch.setProtocol(1, 350);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWRFtoMQTT, datacallback);
  } else if ((valuePRT != 0) || (valuePLSL != 0) || (valueBITS != 0)) {
    Log.trace(F("[RF] MQTTtoRF usr par." CR));
    if (valuePRT == 0)
      valuePRT = 1;
    if (valuePLSL == 0)
      valuePLSL = 350;
    if (valueBITS == 0)
      valueBITS = 24;
    Log.notice(F("[RF] Protocol: %d, Pulse Lgth: %d, Bits nb: %d" CR), valuePRT, valuePLSL, valueBITS);
    mySwitch.setProtocol(valuePRT, valuePLSL);
    mySwitch.send(data, valueBITS);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWRFtoMQTT, datacallback); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
  }
#    ifdef ZradioCC1101 // set Receive on and Transmitt off
  ELECHOUSE_cc1101.SetRx(RFConfig.frequency);
  mySwitch.disableTransmit();
  mySwitch.enableReceive(RF_RECEIVER_GPIO);
#    endif
}
#  endif

#  if jsonReceiving // FALSE MEAN you don't want to use Json  reception analysis
/**
 * @brief Handles the conversion of MQTT messages to RF signals.
 *
 * This function decodes a JSON object received via MQTT and transmits the corresponding RF signal.
 * It supports different RF protocols and configurations.
 *
 * @param topicOri The original MQTT topic.
 * @param RFdata The JSON object containing RF data to be transmitted.
 *
 * The JSON object should contain the following fields:
 * - "value": The RF signal value to be transmitted (required).
 * - "protocol": The RF protocol to be used (optional, default is 1).
 * - "delay": The pulse length in microseconds (optional, default is 350).
 * - "length": The number of bits in the RF signal (optional, default is 24).
 * - "repeat": The number of times the RF signal should be repeated (optional, default is RF_EMITTER_REPEAT).
 * - "txpower": The transmission power for CC1101 (optional, default is RF_CC1101_TXPOWER).
 * - "frequency": The transmission frequency for CC1101 (optional, default is RFConfig.frequency).
 *
 * The function logs the transmission details and acknowledges the sending by publishing the value to an acknowledgement topic.
 * It also restores the default repeat transmit value after sending the signal.
 */
void ZGatewayRF::XtoRF(const char* topicOri, JsonObject& RFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRF)) {
    Log.trace(F("[RF] MQTTtoRF json" CR));
    uint64_t data = RFdata["value"];
    if (data != 0) {
      int valuePRT = RFdata["protocol"] | 1;
      int valuePLSL = RFdata["delay"] | 350;
      int valueBITS = RFdata["length"] | 24;
      int valueRPT = RFdata["repeat"] | RF_EMITTER_REPEAT;
      Log.notice(F("[RF] Protocol:%d, Pulse Lgth: %d, Bits nb: %d" CR), valuePRT, valuePLSL, valueBITS);
      rfHandler.disableCurrentReceiver();
#    ifdef ZradioCC1101
      initCC1101();
      int txPower = RFdata["txpower"] | RF_CC1101_TXPOWER;
      ELECHOUSE_cc1101.setPA((int)txPower);
      Log.notice(F("[RF] CC1101 TX Power: %d" CR), txPower);
      float txFrequency = RFdata["frequency"] | RFConfig.frequency;
      ELECHOUSE_cc1101.SetTx(txFrequency);
      Log.notice(F("[RF] Transmit frequency: %F" CR), txFrequency);
#    endif
      mySwitch.enableTransmit(RF_EMITTER_GPIO);
      mySwitch.setRepeatTransmit(valueRPT);
      mySwitch.setProtocol(valuePRT, valuePLSL);
      mySwitch.send(data, valueBITS);
      Log.notice(F("[RF] MQTTtoRF OK" CR));
      // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      RFdata["origin"] = subjectGTWRFtoMQTT;
      rfHandler.getPublisher().enqueueJsonObject(RFdata);
      mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); // Restore the default value
    }

    rfHandler.enableActiveReceiver();
  }
}
#  endif

/**
 * @brief Disables the RF receiver.
 *
 * This function disables the RF receiver by calling the disableReceive method
 * on the mySwitch object. It also logs a trace message indicating that the RF
 * receiver has been disabled, along with the GPIO pin number used for the RF
 * receiver.
 *
 */
void ZGatewayRF::disableRFReceive() {
  Log.trace(F("[RF] disable RFReceive %d" CR), RF_RECEIVER_GPIO);
  mySwitch.disableReceive();
}

/**
 * @brief Enables the RF receiver and optionally the RF transmitter.
 *
 * This function initializes the RF receiver on the specified GPIO pin and, if not disabled, 
 * initializes the RF transmitter on the specified GPIO pin. It also sets the RF frequency 
 * and logs the configuration details.
 *
 * @param rfFrequency The frequency for the RF communication in MHz. Default is RFConfig.frequency.
 * @param rfReceiverGPIO The GPIO pin number for the RF receiver. Default is RF_RECEIVER_GPIO.
 * @param rfEmitterGPIO The GPIO pin number for the RF transmitter. Default is RF_EMITTER_GPIO.
 *
 * @note If RF_DISABLE_TRANSMIT is defined, the RF transmitter will be disabled.
 */
void ZGatewayRF::enableRFReceive(float rfFrequency, int rfReceiverGPIO, int rfEmitterGPIO) {
  Log.notice(F("[RF] Enable RF Receiver: %fMhz, RF_EMITTER_GPIO: %d, RF_RECEIVER_GPIO: %d" CR), rfFrequency, rfEmitterGPIO, rfReceiverGPIO);

#  ifdef RF_DISABLE_TRANSMIT
  mySwitch.disableTransmit();
#  else
  mySwitch.enableTransmit(rfEmitterGPIO);
#  endif

  mySwitch.setRepeatTransmit(rfEmitterGPIO);
  mySwitch.enableReceive(rfReceiverGPIO);

  //Log.trace(F("[RF] Setup command topic: %s%s%s\n Setup done" CR), (const char*)mqtt_topic, (const char*)subjectMQTTtoRF);
  Log.trace(F("[RF] Setup command topic: %s%s\n Setup done" CR), (const char*)subjectMQTTtoRF);
}

const char* ZGatewayRF::bin2tristate(const char* bin) {
  static char returnValue[50];
  int pos = 0;
  int pos2 = 0;
  while (bin[pos] != '\0' && bin[pos + 1] != '\0') {
    if (bin[pos] == '0' && bin[pos + 1] == '0') {
      returnValue[pos2] = '0';
    } else if (bin[pos] == '1' && bin[pos + 1] == '1') {
      returnValue[pos2] = '1';
    } else if (bin[pos] == '0' && bin[pos + 1] == '1') {
      returnValue[pos2] = 'F';
    } else {
      return "-";
    }
    pos = pos + 2;
    pos2++;
  }
  returnValue[pos2] = '\0';
  return returnValue;
}

char* ZGatewayRF::dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
  static char bin[64];
  unsigned int i = 0;

  while (Dec > 0) {
    bin[32 + i++] = ((Dec & 1) > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j < bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = bin[31 + i - (j - (bitLength - i))];
    } else {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';

  return bin;
}

#  if defined(ZmqttDiscovery) && defined(RF_on_HAS_as_DeviceTrigger)
void ZgatewayRF::announceGatewayTriggerTypeToHASS(uint64_t MQTTvalue) {
  char val[11];
  sprintf(val, "%lu", MQTTvalue);
  String iSignal = String(val);
#    if valueAsATopic
  String discovery_topic = String(subjectRFtoMQTT) + "/" + iSignal;
#    else
  String discovery_topic = String(subjectRFtoMQTT);
#    endif
  Log.trace(F("[RF] Entity Discovered, create HA Discovery CFG" CR));
  announceGatewayTrigger(
      discovery_topic.c_str(), // topic
      "Received", // type
      String("RF-" + iSignal).c_str(), // subtype
      iSignal.c_str(), //signal id
      "{{trigger.value.raw}}" // value template
  );
}
#  endif

#endif
