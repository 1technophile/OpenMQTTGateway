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
#include <bt.h>

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

	static BLEClient* createClient();
	static BLEServer* createServer();
	static void       dumpDevices();
	static BLEScan*   getScan();
	static void       init(std::string deviceName);

private:
	static BLEServer *m_pServer;
	static BLEScan   *m_pScan;
	static BLEClient *m_pClient;

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

public:
	static void setPower(esp_power_level_t powerLevel);

}; // class BLE

#endif // CONFIG_BT_ENABLED
#endif /* MAIN_BLEDevice_H_ */
