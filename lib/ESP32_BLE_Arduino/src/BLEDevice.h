/*
 * BLEDevice.h
 *
 *  Created on: Mar 16, 2017
 *      Author: kolban
 */

#ifndef MAIN_BLEDevice_H_
#define MAIN_BLEDevice_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_gap_ble_api.h> // ESP32 BLE
#include <esp_gattc_api.h>   // ESP32 BLE
#include <map>               // Part of C++ STL
#include <string>
#include <esp_bt.h>

#include "BLEServer.h"
#include "BLEClient.h"
#include "BLEUtils.h"
#include "BLEScan.h"
#include "BLEAddress.h"

/**
 * @brief %BLE functions.
 */
class BLEDevice {
public:

	static BLEClient*  createClient();    // Create a new BLE client.
	static BLEServer*  createServer();    // Cretae a new BLE server.
	static BLEAddress  getAddress();      // Retrieve our own local BD address.
	static BLEScan*    getScan();         // Get the scan object
	static std::string getValue(BLEAddress bdAddress, BLEUUID serviceUUID, BLEUUID characteristicUUID);	  // Get the value of a characteristic of a service on a server.
	static void        init(std::string deviceName);   // Initialize the local BLE environment.
	static void        setPower(esp_power_level_t powerLevel);  // Set our power level.
	static void        setValue(BLEAddress bdAddress, BLEUUID serviceUUID, BLEUUID characteristicUUID, std::string value);   // Set the value of a characteristic on a service on a server.
	static std::string toString();        // Return a string representation of our device.
	static void        whiteListAdd(BLEAddress address);    // Add an entry to the BLE white list.
	static void        whiteListRemove(BLEAddress address); // Remove an entry from the BLE white list.
	static void		   setEncryptionLevel(esp_ble_sec_act_t level);
	static void		   setSecurityCallbacks(BLESecurityCallbacks* pCallbacks);
	static esp_err_t   setMTU(uint16_t mtu);
	static uint16_t	   getMTU();
	static bool        getInitialized(); // Returns the state of the device, is it initialized or not?

private:
	static BLEServer *m_pServer;
	static BLEScan   *m_pScan;
	static BLEClient *m_pClient;
	static esp_ble_sec_act_t 	m_securityLevel;
	static BLESecurityCallbacks* m_securityCallbacks;
	static uint16_t		m_localMTU;

	static esp_gatt_if_t getGattcIF();

	static void gattClientEventHandler(
		esp_gattc_cb_event_t      event,
		esp_gatt_if_t             gattc_if,
		esp_ble_gattc_cb_param_t* param);

	static void gattServerEventHandler(
	   esp_gatts_cb_event_t      event,
	   esp_gatt_if_t             gatts_if,
	   esp_ble_gatts_cb_param_t* param);

	static void gapEventHandler(
		esp_gap_ble_cb_event_t  event,
		esp_ble_gap_cb_param_t* param);

}; // class BLE

#endif // CONFIG_BT_ENABLED
#endif /* MAIN_BLEDevice_H_ */
