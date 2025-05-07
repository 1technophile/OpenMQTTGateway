#ifndef IGATEWAYRF_H
#define IGATEWAYRF_H

#include <main_utils.h>

class ZCommonRF;

class AbstractGatewayRF {
public:
  AbstractGatewayRF(ZCommonRF& iZCommonRF) : rfHandler(iZCommonRF) {};

  virtual ~AbstractGatewayRF() {};

  virtual bool enableReceive(float rfFrequency, int rfReceiverGPIO, int rfEmitterGPIO) = 0;

  virtual bool disableReceive() = 0;

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
  virtual void RFtoX() = 0;

#if simpleReceiving
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
  virtual void XtoRF(const char* topicOri, const char* datacallback) = 0;
#endif

#if jsonReceiving
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
  virtual void XtoRF(const char* topicOri, JsonObject& RFdata) = 0;
#endif

protected:
  ZCommonRF& rfHandler;
};

#endif // IGATEWAYRF_H