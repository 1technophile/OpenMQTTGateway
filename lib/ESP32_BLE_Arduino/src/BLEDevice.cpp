/*
 * BLE.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <bt.h>                // ESP32 BLE
#include <esp_bt_main.h>       // ESP32 BLE
#include <esp_gap_ble_api.h>   // ESP32 BLE
#include <esp_gatts_api.h>     // ESP32 BLE
#include <esp_gattc_api.h>     // ESP32 BLE
#include <esp_err.h>           // ESP32 ESP-IDF
#include <esp_log.h>           // ESP32 ESP-IDF
#include <map>                 // Part of C++ Standard library
#include <sstream>             // Part of C++ Standard library
#include <iomanip>             // Part of C++ Standard library

#include "BLEDevice.h"
#include "BLEClient.h"
#include "BLEUtils.h"
#include "GeneralUtils.h"
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

static const char* LOG_TAG = "BLEDevice";


/**
 * Singletons for the BLEDevice.
 */
BLEServer* BLEDevice::m_pServer = nullptr;
BLEScan*   BLEDevice::m_pScan   = nullptr;
BLEClient* BLEDevice::m_pClient = nullptr;
bool initialized = false;
/**
 * @brief Create a new instance of a client.
 * @return A new instance of the client.
 */
BLEClient* BLEDevice::createClient() {
	m_pClient = new BLEClient();
	return m_pClient;
} // createClient


/**
 * @brief Create a new instance of a server.
 * @return A new instance of the server.
 */
BLEServer* BLEDevice::createServer() {
	ESP_LOGD(LOG_TAG, ">> createServer");
	m_pServer = new BLEServer();
	m_pServer->createApp(0);
	ESP_LOGD(LOG_TAG, "<< createServer");
	return m_pServer;
} // createServer


/**
 * @brief Handle GATT server events.
 *
 * @param [in] event The event that has been newly received.
 * @param [in] gatts_if The connection to the GATT interface.
 * @param [in] param Parameters for the event.
 */
void BLEDevice::gattServerEventHandler(
   esp_gatts_cb_event_t      event,
   esp_gatt_if_t             gatts_if,
   esp_ble_gatts_cb_param_t* param
) {
	ESP_LOGD(LOG_TAG, "gattServerEventHandler [esp_gatt_if: %d] ... %s",
		gatts_if,
		BLEUtils::gattServerEventTypeToString(event).c_str());

	BLEUtils::dumpGattServerEvent(event, gatts_if, param);

	if (BLEDevice::m_pServer != nullptr) {
		BLEDevice::m_pServer->handleGATTServerEvent(event, gatts_if, param);
	}
} // gattServerEventHandler


/**
 * @brief Handle GATT client events.
 *
 * Handler for the GATT client events.
 *
 * @param [in] event
 * @param [in] gattc_if
 * @param [in] param
 */
void BLEDevice::gattClientEventHandler(
	esp_gattc_cb_event_t      event,
	esp_gatt_if_t             gattc_if,
	esp_ble_gattc_cb_param_t* param) {

	ESP_LOGD(LOG_TAG, "gattClientEventHandler [esp_gatt_if: %d] ... %s",
		gattc_if, BLEUtils::gattClientEventTypeToString(event).c_str());
	BLEUtils::dumpGattClientEvent(event, gattc_if, param);
/*
	switch(event) {
		default: {
			break;
		}
	} // switch
	*/

	// If we have a client registered, call it.
	if (BLEDevice::m_pClient != nullptr) {
		BLEDevice::m_pClient->gattClientEventHandler(event, gattc_if, param);
	}

} // gattClientEventHandler


/**
 * @brief Handle GAP events.
 */
void BLEDevice::gapEventHandler(
	esp_gap_ble_cb_event_t event,
	esp_ble_gap_cb_param_t *param) {

	BLEUtils::dumpGapEvent(event, param);

	switch(event) {
		case ESP_GAP_BLE_SEC_REQ_EVT: {
			esp_err_t errRc = ::esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
			if (errRc != ESP_OK) {
				ESP_LOGE(LOG_TAG, "esp_ble_gap_security_rsp: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			}
			break;
		}

		default: {
			break;
		}
	} // switch

	if (BLEDevice::m_pServer != nullptr) {
		BLEDevice::m_pServer->handleGAPEvent(event, param);
	}

	if (BLEDevice::m_pClient != nullptr) {
		BLEDevice::m_pClient->handleGAPEvent(event, param);
	}

	if (BLEDevice::m_pScan != nullptr) {
		BLEDevice::getScan()->handleGAPEvent(event, param);
	}
} // gapEventHandler


/**
 * @brief Retrieve the Scan object that we use for scanning.
 * @return The scanning object reference.  This is a singleton object.  The caller should not
 * try and release/delete it.
 */
BLEScan* BLEDevice::getScan() {
	//ESP_LOGD(LOG_TAG, ">> getScan");
	if (m_pScan == nullptr) {
		m_pScan = new BLEScan();
		//ESP_LOGD(LOG_TAG, " - creating a new scan object");
	}
	//ESP_LOGD(LOG_TAG, "<< getScan: Returning object at 0x%x", (uint32_t)m_pScan);
	return m_pScan;
} // getScan


/**
 * @brief Initialize the %BLE environment.
 * @param deviceName The device name of the device.
 */
void BLEDevice::init(std::string deviceName) {
	if(!initialized){
		initialized = true;
		esp_err_t errRc = ::nvs_flash_init();
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "nvs_flash_init: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

	  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	  errRc = esp_bt_controller_init(&bt_cfg);
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_bt_controller_init: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}
#ifndef CLASSIC_BT_ENABLED
	//	esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);  //FIXME waiting for response from esp-idf issue
		errRc = esp_bt_controller_enable(ESP_BT_MODE_BLE);
		//errRc = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_bt_controller_enable: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}
#else
		errRc = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_bt_controller_enable: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}
#endif
		errRc = esp_bluedroid_init();
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_bluedroid_init: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = esp_bluedroid_enable();
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_bluedroid_enable: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = esp_ble_gap_register_callback(BLEDevice::gapEventHandler);
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_ble_gap_register_callback: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = esp_ble_gattc_register_callback(BLEDevice::gattClientEventHandler);
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_ble_gattc_register_callback: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = esp_ble_gatts_register_callback(BLEDevice::gattServerEventHandler);
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_ble_gatts_register_callback: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		}

		errRc = ::esp_ble_gap_set_device_name(deviceName.c_str());
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_ble_gap_set_device_name: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		};

		esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
		errRc = ::esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
		if (errRc != ESP_OK) {
			ESP_LOGE(LOG_TAG, "esp_ble_gap_set_security_param: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
			return;
		};
	}
	vTaskDelay(200/portTICK_PERIOD_MS); // Delay for 200 msecs as a workaround to an apparent Arduino environment issue.
} // init


/**
 * @brief Set the transmission power.
 * The power level can be one of:
 * * ESP_PWR_LVL_N14
 * * ESP_PWR_LVL_N11
 * * ESP_PWR_LVL_N8
 * * ESP_PWR_LVL_N5
 * * ESP_PWR_LVL_N2
 * * ESP_PWR_LVL_P1
 * * ESP_PWR_LVL_P4
 * * ESP_PWR_LVL_P7
 * @param [in] powerLevel.
 */
/* STATIC */ void BLEDevice::setPower(esp_power_level_t powerLevel) {
	ESP_LOGD(LOG_TAG, ">> setPower: %d", powerLevel);
	esp_err_t errRc = ::esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, powerLevel);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_tx_power_set: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
	};
	ESP_LOGD(LOG_TAG, "<< setPower");
} // setPower

#endif // CONFIG_BT_ENABLED
