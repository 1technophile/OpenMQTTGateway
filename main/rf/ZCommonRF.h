/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a wifi or ethernet gateway between your BLE/433mhz/infrared IR signal and an MQTT broker 
   Send and receiving command by MQTT
  
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
#ifndef ZCOMMONRF_H
#define ZCOMMONRF_H

#if defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRTL_433) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)

#  ifdef ESP32
#    include <Preferences.h>
#  endif

#  ifdef ZradioCC1101
#    include <ELECHOUSE_CC1101_SRC_DRV.h>
#  endif

#  include <main_utils.h>
#  include <rf/AbstractGatewayRF.h>

#  include <unordered_map>

// Forward declaration of RFConfig
class RFConfig;
class MQTTPublisher;

/**
 * @brief The ZCommonRF class provides a common interface for RF communication.
 *
 * This class is responsible for managing RF configurations, enabling and disabling
 * RF receivers, and handling MQTT communication related to RF settings. It supports
 * multiple RF gateways and provides methods to initialize, configure, and retrieve
 * the state of RF receivers.
 *
 * Key functionalities include:
 * - Initializing and loading RF configurations.
 * - Validating RF frequencies and receivers.
 * - Enabling and disabling active RF receivers.
 * - Publishing RF state measurements.
 * - Configuring RF settings from JSON objects.
 * - Handling MQTT commands for RF configuration.
 * - Managing multiple RF gateways.
 *
 * The class supports conditional compilation for different platforms (e.g., ESP32)
 * and RF modules (e.g., CC1101). It uses the ArduinoJson library for JSON handling
 * and the MQTTPublisher class for MQTT communication.
 */
class ZCommonRF {
public:
  /**
   * @brief Constructs a new ZCommonRF object with the given MQTTPublisher reference.
   * 
   * This constructor initializes a new ZCommonRF object with the provided MQTTPublisher
   * reference. The MQTTPublisher object is used to publish messages to an MQTT broker.
   * 
   * @param publicher A reference to an MQTTPublisher object.
   * @param iConfig A reference to the RF configuration object.
   */
  ZCommonRF(MQTTPublisher& publicher, RFConfig& iConfig);

  /**
   * @brief Initializes and loads the common RF configuration.
   * 
   * This function calls the initialization function for the RF configuration
   * and then loads the configuration settings.
   */
  void setupCommonRF();

  /**
 * @brief Checks if the given frequency is within the valid ranges for CC1101.
 *
 * This function verifies if the provided frequency in MHz falls within the 
 * valid frequency ranges for the CC1101 transceiver. The valid ranges are:
 * - 300 MHz to 348 MHz
 * - 387 MHz to 464 MHz
 * - 779 MHz to 928 MHz
 *
 * @param mhz The frequency in MHz to be checked.
 * @return true if the frequency is within the valid ranges, false otherwise.
 */
  bool validFrequency(float mhz);

  /**
 * @brief Disables the currently active receiver based on the value of `currentReceiver`.
 *
 * This function logs the current receiver being disabled and then disables the receiver
 * by calling the appropriate function based on the value of `currentReceiver`.
 *
 * Supported receivers:
 * - ACTIVE_NONE: No receiver is active, nothing is done.
 * - ACTIVE_PILIGHT: Calls `disablePilightReceive()` if `ZgatewayPilight` is defined.
 * - ACTIVE_RF: Calls `disableRFReceive()` if `ZgatewayRF` is defined.
 * - ACTIVE_RTL: Calls `disableRTLreceive()` if `ZgatewayRTL_433` is defined.
 * - ACTIVE_RF2: Calls `disableRF2Receive()` if `ZgatewayRF2` is defined.
 *
 * If `currentReceiver` does not match any of the supported receivers, an error is logged.
 */
  void disableCurrentReceiver();

  /**
 * @brief Enables the active receiver based on the configuration.
 *
 * This function checks the active receiver configuration and initializes
 * the corresponding receiver. It supports multiple receiver types, including
 * Pilight, RF, RTL_433, and RF2. If no valid receiver is selected, it logs an error.
 *
 * The function performs the following steps:
 * - Logs the active receiver configuration.
 * - Switches based on the active receiver type:
 *   - If ZgatewayPilight is defined and the active receiver is ACTIVE_PILIGHT,
 *     it initializes the CC1101 module and enables Pilight receive mode.
 *   - If ZgatewayRF is defined and the active receiver is ACTIVE_RF,
 *     it initializes the CC1101 module and enables RF receive mode.
 *   - If ZgatewayRTL_433 is defined and the active receiver is ACTIVE_RTL,
 *     it initializes the CC1101 module and enables RTL_433 receive mode.
 *   - If ZgatewayRF2 is defined and the active receiver is ACTIVE_RF2,
 *     it initializes the CC1101 module and enables RF2 receive mode.
 * - If the active receiver is ACTIVE_RECERROR, it logs an error indicating
 *   that no receiver is selected.
 * - If the active receiver is not supported, it logs an error with the
 *   unsupported receiver type.
 */
  void enableActiveReceiver();

  /**
 * @brief Publishes the state of the RTL_433 receiver.
 *
 * This function collects various metrics related to the RTL_433 receiver and 
 * serializes them into a JSON object. The JSON object is then enqueued for 
 * further processing and also returned as a string.
 *
 * @return A JSON string representing the state of the RTL_433 receiver.
 *
 * The JSON object contains the following fields:
 * - "active": Indicates if the receiver is active.
 * - "frequency": The frequency of the receiver (if ZradioCC1101 or ZradioSX127x is defined).
 * - "rssithreshold": The RSSI threshold (if ZgatewayRTL_433 is defined and the active receiver is RTL).
 * - "rssi": The current RSSI value (if ZgatewayRTL_433 is defined and the active receiver is RTL).
 * - "avgrssi": The average RSSI value (if ZgatewayRTL_433 is defined and the active receiver is RTL).
 * - "count": The message count (if ZgatewayRTL_433 is defined and the active receiver is RTL).
 * - "rtl433_stack": The high water mark of the rtl_433_Decoder stack (if ZgatewayRTL_433 is defined and the active receiver is RTL).
 * - "ookthreshold": The OOK threshold (if ZradioSX127x is defined and the active receiver is RTL).
 * - "origin": The origin of the message.
 */
  String stateRFMeasures();

  /**
   * @brief Handles the MQTT to RF configuration set command.
   *
   * This function processes the incoming MQTT message to set the RF configuration.
   * It follows a specific order of operations based on the provided JSON data:
   * 1. Executes `init=true` and `load=true` commands (if both are present, INIT prevails over LOAD).
   * 2. Applies parameters included in the JSON data.
   * 3. Executes `erase=true` and `save=true` commands (if both are present, ERASE prevails over SAVE).
   *
   * @param topicOri The original MQTT topic.
   * @param RFdata The JSON object containing the RF configuration data.
   */
  void XtoRFset(const char* topicOri, JsonObject& RFdata);

  /**
   * @brief Adds a gateway RF object to the system.
   * 
   * This function registers a new IGatewayRF object, allowing it to be used
   * within the system for RF communication.
   * 
   * @param gatewayRFId A unique identifier for the IGatewayRF object.
   * @param gatewayRF A pointer to an IGatewayRF object that will be added.
   */
  void addGatewayRF(int gatewayRFId, AbstractGatewayRF* gatewayRF);

  /**
   * @brief Retrieves the gateway RF object corresponding to the given ID.
   * 
   * @param gatewayRFId The ID of the gateway RF to retrieve.
   * @return AbstractGatewayRF* Pointer to the gateway RF object.
   */
  AbstractGatewayRF* getGatewayRF(int gatewayRFId);

  /**
   * @brief Retrieves the current instance of the AbstractGatewayRF.
   * 
   * This function returns a pointer to the current AbstractGatewayRF instance
   * being used by the system. The AbstractGatewayRF class is an abstract base
   * class that defines the interface for RF gateway implementations.
   * 
   * @return AbstractGatewayRF* Pointer to the current AbstractGatewayRF instance.
   */
  AbstractGatewayRF* getCurrentGatewayRF();

#  if !defined(ZgatewayRFM69) && !defined(ZactuatorSomfy)
  /**
 * @brief Checks if the given receiver is valid based on the enabled gateways.
 *
 * This function verifies if the provided receiver identifier corresponds to an
 * active and enabled gateway. The function checks against multiple possible
 * gateways, which are conditionally compiled based on the defined preprocessor
 * directives.
 *
 * @param receiver The identifier of the receiver to validate.
 * @return true if the receiver is valid and corresponds to an active gateway.
 * @return false if the receiver is not valid or not available.
 */
  bool validReceiver(int receiver);
#  endif

  /**
   * @brief Provides access to the MQTT publisher.
   *
   * This function returns a reference to the MQTTPublisher object,
   * allowing other components to publish messages to the MQTT broker.
   *
   * @return A reference to the MQTTPublisher object.
   */
  MQTTPublisher& getPublisher();

  /**
 * @brief Configures RF settings from a JSON object.
 *
 * This function updates the RF configuration based on the provided JSON object.
 * It handles various RF parameters such as frequency, active status, RSSI threshold,
 * and OOK threshold. Additionally, it can erase or save the configuration to non-volatile storage.
 *
 * @param RFdata A reference to a JsonObject containing the RF configuration data.
 *
 * The JSON object can contain the following keys:
 * - "frequency": The frequency to set for RF reception.
 * - "active": The active status of the RF receiver.
 * - "rssithreshold" (only if ZgatewayRTL_433 is defined): The RSSI threshold for RTL_433.
 * - "ookthreshold" (only if ZgatewayRTL_433 and RF_SX1276 or RF_SX1278 are defined): The OOK threshold for RTL_433.
 * - "status" (only if ZgatewayRTL_433 is defined): Requests the status of the RF receiver.
 * - "erase" (only if ESP32 is defined): A boolean indicating whether to erase the RF configuration from NVS.
 * - "save" (only if ESP32 is defined): A boolean indicating whether to save the RF configuration to NVS.
 *
 * The function logs the updated configuration and any errors encountered during the process.
 * It also handles enabling and disabling the current and active RF receivers.
 * 
 * TODO: Present  for retrocompatibitly, Need to Consider moving this to a separate header file for better organization.
 */

private:
  RFConfig& iRFConfig;

  /**
   * @brief A map that associates integer keys with pointers to AbstractGatewayRF objects.
   * 
   * This unordered map is used to store and manage gateway objects, where each gateway
   * is identified by a unique integer key. The keys are used to quickly access the 
   * corresponding AbstractGatewayRF pointers.
   * 
   * @note The map is unordered, meaning that the elements are not stored in any particular order.
   */
  std::unordered_map<int, AbstractGatewayRF*> gateways;

  /**
   * @brief Represents the current receiver in use.
   */
  int currentReceiver;

  /**
   * @brief Reference to an MQTTPublisher object.
   * 
   * This member variable holds a reference to an MQTTPublisher object
   * which is used to publish messages to an MQTT broker.
   */
  MQTTPublisher& publicher;

#  ifdef ZradioCC1101
  /**
 * @brief Initializes the CC1101 RF module.
 *
 * This function attempts to initialize the CC1101 RF module by setting the SPI pins
 * and checking the connection status. It will make up to 10 attempts to establish
 * a connection, with a truncated exponential backoff delay between attempts.
 *
 * The function performs the following steps:
 * 1. Sets the SPI pins for the CC1101 module if they are defined.
 * 2. Attempts to get the CC1101 module status.
 * 3. If the connection is successful, logs a success message, initializes the module,
 *    and sets the receive frequency.
 * 4. If the connection fails, logs an error message and waits for a delay before retrying.
 * 5. The delay between attempts starts at 16 milliseconds and doubles with each attempt,
 *    up to a maximum of 500 milliseconds.
 */
  void initCC1101();
#  endif // ZradioCC1101
};

#endif // defined(ZgatewayRF) || defined(ZgatewayPilight) || defined(ZgatewayRTL_433) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)

#endif // ZCOMMONRF_H
