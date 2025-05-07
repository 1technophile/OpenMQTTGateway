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
#ifndef ZGATEWAYRF_H
#define ZGATEWAYRF_H

#ifdef ZgatewayRF

#  include <RCSwitch.h>
#  include <main_utils.h>
#  include <rf/AbstractGatewayRF.h>

class ZCommonRF;

class ZGatewayRF : public AbstractGatewayRF {
public:
  ZGatewayRF(ZCommonRF& iZCommonRF);

  bool enableReceive(float rfFrequency, int rfReceiverGPIO, int rfEmitterGPIO) override;

  bool disableReceive() override;

  void RFtoX() override;

#  if simpleReceiving
  void XtoRF(const char* topicOri, const char* datacallback) override;
#  endif

#  if jsonReceiving
  void XtoRF(const char* topicOri, JsonObject& RFdata) override;
#  endif

private:
  /**
   * @brief An instance of the RCSwitch class used to control and receive signals from RF devices.
   * 
   * The RCSwitch class provides methods to send and receive signals using RF (Radio Frequency) communication.
   * This instance, `mySwitch`, can be used to interact with RF devices such as remote controls, power sockets, and other RF-enabled devices.
   * 
   * @see https://github.com/sui77/rc-switch for more details on the RCSwitch library.
   */
  RCSwitch mySwitch;

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
  void disableRFReceive();

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
  void enableRFReceive(float rfFrequency, int rfReceiverGPIO, int rfEmitterGPIO);

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
  static const char* bin2tristate(const char* bin);

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
  static char* dec2binWzerofill(unsigned long Dec, unsigned int bitLength);

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
  void announceGatewayTriggerTypeToHASS(uint64_t MQTTvalue);
#  endif
};

#endif // ZgatewayRF

#endif // ZGATEWAYRF_H
