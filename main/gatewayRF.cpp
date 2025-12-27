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
#include "User_config.h"

#ifdef ZgatewayRF
#  include "TheengsCommon.h"
#  include "config_RF.h"

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
extern void initCC1101();
#  endif

#  include <RCSwitch.h> // library for controling Radio frequency switch

void disableCurrentReceiver();
void enableActiveReceiver();

RCSwitch mySwitch = RCSwitch();

/**
 * @brief Converts a binary string to a tristate string.
 *
 * This function takes a binary string as input and converts it to a tristate string.
 * The tristate string is composed of '0', '1', and 'F' characters, where:
 * - '0' represents "00" in the binary string
 * - '1' represents "11" in the binary string
 * - 'F' represents "01" in the binary string
 *
 * If the input binary string contains any other combination, the function returns "-".
 *
 * @note CONVERSION function from https://github.com/sui77/rc-switch/tree/master/examples/ReceiveDemo_Advanced
 *
 * @param bin The input binary string.
 * @return A pointer to the tristate string.
 */
static const char* bin2tristate(const char* bin) {
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

/**
 * @brief Converts a decimal number to a binary string with zero fill.
 *
 * This function takes an unsigned long decimal number and converts it to a binary string
 * representation, ensuring that the resulting string is zero-padded to the specified bit length.
 *
 * @param Dec The decimal number to be converted.
 * @param bitLength The length of the resulting binary string, including leading zeros.
 * @return A pointer to a static character array containing the binary string representation.
 *
 * @note The returned string is stored in a static buffer, so it will be overwritten by subsequent
 * calls to this function. The buffer size is fixed at 64 characters.
 *
 * @note CONVERSION function from https://github.com/sui77/rc-switch/tree/master/examples/ReceiveDemo_Advanced
 *
 */
static char* dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
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
/**
 * @brief Announces RF signal data to Home Assistant via MQTT for device trigger configuration.
 *
 * This function creates and publishes a Home Assistant configuration message
 * for an RF signal received by the gateway. It constructs the necessary
 * MQTT topic and payload to announce the RF signal as a device trigger
 * in Home Assistant, allowing it to be used as an automation trigger.
 *
 * @param MQTTvalue The RF signal value to be published to MQTT.
 */
void announceGatewayTriggerTypeToHASS(uint64_t MQTTvalue) {
  char val[11];
  sprintf(val, "%lu", MQTTvalue);
  String iSignal = String(val);
#    if valueAsATopic
  String discovery_topic = String(subjectRFtoMQTT) + "/" + iSignal;
#    else
  String discovery_topic = String(subjectRFtoMQTT);
#    endif
  THEENGS_LOG_TRACE(F("[RF] Entity Discovered, create HA Discovery CFG" CR));
  announceGatewayTrigger(
      discovery_topic.c_str(), // topic
      "Received", // type
      String("RF-" + iSignal).c_str(), // subtype
      iSignal.c_str(), //signal id
      "{{trigger.value.raw}}" // value template
  );
}
#  endif

/**
 * @brief Processes received RF signals and converts them to JSON format for further handling.
 *
 * This function checks if an RF signal is available, extracts relevant data from the signal,
 * and stores it in a JSON object. It also handles duplicate signal detection and optionally
 * publishes the signal data for MQTT discovery and repetition.
 *
 * @note This function is designed to work with both ESP32 and ESP8266 platforms.
 *
 * @details The function performs the following steps:
 * - Checks if an RF signal is available.
 * - Logs the reception of the RF signal.
 * - Extracts the value, protocol, length, delay, tristate, and binary representation of the signal.
 * - For ESP32 and ESP8266, extracts the raw data of the signal.
 * - If the ZradioCC1101 is defined, includes the frequency in the JSON object.
 * - Resets the availability status of the RF signal.
 * - Checks for duplicate signals and processes the signal if it is not a duplicate.
 * - Optionally publishes the signal data for MQTT discovery and repetition.
 *
 * @param None
 * @return void
 */
void RFtoX() {
  if (mySwitch.available()) {
    StaticJsonDocument<JSON_MSG_BUFFER> RFdataBuffer;
    JsonObject RFdata = RFdataBuffer.to<JsonObject>();
#  ifdef ESP32
    THEENGS_LOG_TRACE(F("[RF] Rcv. RF - Task running on core :%d" CR), xPortGetCoreID());
#  else
    THEENGS_LOG_TRACE(F("[RF] Rcv. RF" CR));
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
    RFdata["frequency"] = iRFConfig.getFrequency();
#  endif

    mySwitch.resetAvailable();

    if (MQTTvalue != 0 && !isAduplicateSignal(MQTTvalue)) { // conditions to avoid duplications of RF -->MQTT
#  if defined(ZmqttDiscovery) && defined(RF_on_HAS_as_DeviceTrigger)
      if (SYSConfig.discovery)
        announceGatewayTriggerTypeToHASS(MQTTvalue);
#  endif
      RFdata["origin"] = subjectRFtoMQTT;
      enqueueJsonObject(RFdata);
      // Casting "receivedSignal[o].value" to (unsigned long) because ArduinoLog doesn't support uint64_t for ESP's
      THEENGS_LOG_TRACE(F("[RF] Store val: %u" CR), (unsigned long)MQTTvalue);
      storeSignalValue(MQTTvalue);
      if (repeatRFwMQTT) {
        THEENGS_LOG_TRACE(F("[RF] Pub RF for rpt" CR));
        RFdata["origin"] = subjectMQTTtoRF;
        enqueueJsonObject(RFdata);
      }
    } else {
      THEENGS_LOG_TRACE(F("[RF] RF signal received but already managed" CR));
    }
  }
  // else {
  //    No RF signal received
  // }
}

#  if simpleReceiving // FALSE MEAN you don't want to use old way reception analysis
/**
 * @brief Transmits RF signals based on the provided MQTT topic and data.
 *
 * This function processes the MQTT topic and data to determine the RF protocol,
 * pulse length, and number of bits to use for transmission. It then transmits
 * the RF signal using the specified parameters. If no specific parameters are
 * provided, default values are used.
 *
 * @param topicOri The original MQTT topic string.
 * @param datacallback The data to be transmitted, provided as a string.
 *
 * The function performs the following steps:
 * 1. Disables the current RF receiver and enables the transmitter if ZradioCC1101 is defined.
 * 2. Converts the data string to a 64-bit unsigned integer.
 * 3. Analyzes the topic string to extract RF protocol, pulse length, and bit count.
 * 4. Transmits the RF signal using the extracted or default parameters.
 * 5. Publishes an acknowledgment to the GTWRF topic.
 * 6. Re-enables the RF receiver and disables the transmitter if ZradioCC1101 is defined.
 */
void XtoRF(const char* topicOri, const char* datacallback) {
#    ifdef ZradioCC1101 // set Receive off and Transmitt on
  disableCurrentReceiver();
  ELECHOUSE_cc1101.SetTx(iRFConfig.getFrequency());
  THEENGS_LOG_NOTICE(F("[RF] Transmit frequency: %F" CR), iRFConfig.getFrequency());
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
    THEENGS_LOG_TRACE(F("[RF] MQTTtoRF dflt" CR));
    mySwitch.setProtocol(1, 350);
    mySwitch.send(data, 24);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWRFtoMQTT, datacallback);
  } else if ((valuePRT != 0) || (valuePLSL != 0) || (valueBITS != 0)) {
    THEENGS_LOG_TRACE(F("[RF] MQTTtoRF usr par." CR));
    if (valuePRT == 0)
      valuePRT = 1;
    if (valuePLSL == 0)
      valuePLSL = 350;
    if (valueBITS == 0)
      valueBITS = 24;
    THEENGS_LOG_NOTICE(F("[RF] Protocol: %d, Pulse Lgth: %d, Bits nb: %d" CR), valuePRT, valuePLSL, valueBITS);
    mySwitch.setProtocol(valuePRT, valuePLSL);
    mySwitch.send(data, valueBITS);
    // Acknowledgement to the GTWRF topic
    pub(subjectGTWRFtoMQTT, datacallback); // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
  }
#    ifdef ZradioCC1101 // set Receive on and Transmitt off
  ELECHOUSE_cc1101.SetRx(iRFConfig.getFrequency());
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
 * - "frequency": The transmission frequency for CC1101 (optional, default is iRFConfig.getFrequency()).
 *
 * The function logs the transmission details and acknowledges the sending by publishing the value to an acknowledgement topic.
 * It also restores the default repeat transmit value after sending the signal.
 */
void XtoRF(const char* topicOri, JsonObject& RFdata) {
  if (cmpToMainTopic(topicOri, subjectMQTTtoRF)) {
    THEENGS_LOG_TRACE(F("[RF] MQTTtoRF json" CR));
    uint64_t data = RFdata["value"];
    if (data != 0) {
      int valuePRT = RFdata["protocol"] | 1;
      int valuePLSL = RFdata["delay"] | 350;
      int valueBITS = RFdata["length"] | 24;
      int valueRPT = RFdata["repeat"] | RF_EMITTER_REPEAT;
      THEENGS_LOG_NOTICE(F("[RF] Protocol:%d, Pulse Lgth: %d, Bits nb: %d" CR), valuePRT, valuePLSL, valueBITS);
      disableCurrentReceiver();
#    ifdef ZradioCC1101
      initCC1101();
      int txPower = RFdata["txpower"] | RF_CC1101_TXPOWER;
      ELECHOUSE_cc1101.setPA((int)txPower);
      THEENGS_LOG_NOTICE(F("[RF] CC1101 TX Power: %d" CR), txPower);
      float txFrequency = RFdata["frequency"] | iRFConfig.getFrequency();
      ELECHOUSE_cc1101.SetTx(txFrequency);
      THEENGS_LOG_NOTICE(F("[RF] Transmit frequency: %F" CR), txFrequency);
#    endif
      mySwitch.enableTransmit(RF_EMITTER_GPIO);
      mySwitch.setRepeatTransmit(valueRPT);
      mySwitch.setProtocol(valuePRT, valuePLSL);
      mySwitch.send(data, valueBITS);
      THEENGS_LOG_NOTICE(F("[RF] MQTTtoRF OK" CR));
      // we acknowledge the sending by publishing the value to an acknowledgement topic, for the moment even if it is a signal repetition we acknowledge also
      RFdata["origin"] = subjectGTWRFtoMQTT;
      enqueueJsonObject(RFdata);
      mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT); // Restore the default value
    }

    enableActiveReceiver();
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
 * @note THIS SEEMS LIKE A DEAD CODE. THE FUNCTION IS NOT CALLED ANYWHERE.
 */
void disableRFReceive() {
  THEENGS_LOG_TRACE(F("[RF] disable RFReceive %d" CR), RF_RECEIVER_GPIO);
  mySwitch.disableReceive();
}

/**
 * @brief Enables the RF receiver and optionally the RF transmitter.
 *
 * This function initializes the RF receiver on the specified GPIO pin and, if not disabled,
 * initializes the RF transmitter on the specified GPIO pin. It also sets the RF frequency
 * and logs the configuration details.
 *
 * @param rfFrequency The frequency for the RF communication in MHz. Default is iRFConfig.getFrequency().
 * @param rfReceiverGPIO The GPIO pin number for the RF receiver. Default is RF_RECEIVER_GPIO.
 * @param rfEmitterGPIO The GPIO pin number for the RF transmitter. Default is RF_EMITTER_GPIO.
 *
 * @note If RF_DISABLE_TRANSMIT is defined, the RF transmitter will be disabled.
 */
void enableRFReceive(
    float rfFrequency = iRFConfig.getFrequency(),
    int rfReceiverGPIO = RF_RECEIVER_GPIO,
    int rfEmitterGPIO = RF_EMITTER_GPIO) {
  THEENGS_LOG_NOTICE(F("[RF] Enable RF Receiver: %F Mhz, RF_EMITTER_GPIO: %d, RF_RECEIVER_GPIO: %d" CR), rfFrequency, rfEmitterGPIO, rfReceiverGPIO);

#  ifdef RF_DISABLE_TRANSMIT
  mySwitch.disableTransmit();
#  else
  mySwitch.enableTransmit(rfEmitterGPIO);
#  endif

  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT);
  mySwitch.enableReceive(rfReceiverGPIO);

  THEENGS_LOG_TRACE(F("[RF] Setup command topic: %s%s%s\n Setup done" CR), (const char*)mqtt_topic, (const char*)gateway_name, (const char*)subjectMQTTtoRF);
}

#endif
