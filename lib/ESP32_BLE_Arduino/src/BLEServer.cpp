/*
 * BLEServer.cpp
 *
 *  Created on: Apr 16, 2017
 *      Author: kolban
 */

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
//#include <esp_gatts_api.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEService.h"
#include "BLEUtils.h"
#include <string.h>
#include <string>
#include <gatt_api.h>
#include <unordered_set>
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

static const char* LOG_TAG = "BLEServer";


/**
 * @brief Construct a %BLE Server
 *
 * This class is not designed to be individually instantiated.  Instead one should create a server by asking
 * the BLEDevice class.
 */
BLEServer::BLEServer() {
	m_appId            = -1;
	m_gatts_if         = -1;
	m_connectedCount   = 0;
	m_connId           = -1;
	m_pServerCallbacks = nullptr;

	//createApp(0);
} // BLEServer


void BLEServer::createApp(uint16_t appId) {
	m_appId = appId;
	registerApp();
} // createApp


/**
 * @brief Create a %BLE Service.
 *
 * With a %BLE server, we can host one or more services.  Invoking this function causes the creation of a definition
 * of a new service.  Every service must have a unique UUID.
 * @param [in] uuid The UUID of the new service.
 * @return A reference to the new service object.
 */
BLEService* BLEServer::createService(const char* uuid) {
	return createService(BLEUUID(uuid));
}


/**
 * @brief Create a %BLE Service.
 *
 * With a %BLE server, we can host one or more services.  Invoking this function causes the creation of a definition
 * of a new service.  Every service must have a unique UUID.
 * @param [in] uuid The UUID of the new service.
 * @param [in] numHandles The maximum number of handles associated with this service.
 * @return A reference to the new service object.
 */
BLEService* BLEServer::createService(BLEUUID uuid, uint32_t numHandles) {
	ESP_LOGD(LOG_TAG, ">> createService - %s", uuid.toString().c_str());
	m_semaphoreCreateEvt.take("createService");

	// Check that a service with the supplied UUID does not already exist.
	if (m_serviceMap.getByUUID(uuid) != nullptr) {
		ESP_LOGE(LOG_TAG, "<< Attempt to create a new service with uuid %s but a service with that UUID already exists.",
			uuid.toString().c_str());
		m_semaphoreCreateEvt.give();
		return nullptr;
	}

	BLEService* pService = new BLEService(uuid, numHandles);
	m_serviceMap.setByUUID(uuid, pService); // Save a reference to this service being on this server.
	pService->executeCreate(this);          // Perform the API calls to actually create the service.

	m_semaphoreCreateEvt.wait("createService");

	ESP_LOGD(LOG_TAG, "<< createService");
	return pService;
} // createService


/**
 * @brief Retrieve the advertising object that can be used to advertise the existence of the server.
 *
 * @return An advertising object.
 */
BLEAdvertising* BLEServer::getAdvertising() {
	return &m_bleAdvertising;
}

uint16_t BLEServer::getConnId() {
	return m_connId;
}


/**
 * @brief Return the number of connected clients.
 * @return The number of connected clients.
 */
uint32_t BLEServer::getConnectedCount() {
	return m_connectedCount;
} // getConnectedCount


uint16_t BLEServer::getGattsIf() {
	return m_gatts_if;
}

/**
 * @brief Handle a received GAP event.
 *
 * @param [in] event
 * @param [in] param
 */
void BLEServer::handleGAPEvent(
		esp_gap_ble_cb_event_t  event,
		esp_ble_gap_cb_param_t* param) {
	ESP_LOGD(LOG_TAG, "BLEServer ... handling GAP event!");
	switch(event) {
		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: {
			/*
			esp_ble_adv_params_t adv_params;
			adv_params.adv_int_min       = 0x20;
			adv_params.adv_int_max       = 0x40;
			adv_params.adv_type          = ADV_TYPE_IND;
			adv_params.own_addr_type     = BLE_ADDR_TYPE_PUBLIC;
			adv_params.channel_map       = ADV_CHNL_ALL;
			adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
			ESP_LOGD(tag, "Starting advertising");
			esp_err_t errRc = ::esp_ble_gap_start_advertising(&adv_params);
			if (errRc != ESP_OK) {
				ESP_LOGE(tag, "esp_ble_gap_start_advertising: rc=%d %s", errRc, espToString(errRc));
				return;
			}
			*/
			break;
		}

		default:
			break;
	}
} // handleGAPEvent



/**
 * @brief Handle a GATT Server Event.
 *
 * @param [in] event
 * @param [in] gatts_if
 * @param [in] param
 *
 */
void BLEServer::handleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t* param) {

	ESP_LOGD(LOG_TAG, ">> handleGATTServerEvent: %s",
		BLEUtils::gattServerEventTypeToString(event).c_str());

	// Invoke the handler for every Service we have.
	m_serviceMap.handleGATTServerEvent(event, gatts_if, param);

	switch(event) {
		// ESP_GATTS_ADD_CHAR_EVT - Indicate that a characteristic was added to the service.
		// add_char:
		// - esp_gatt_status_t status
		// - uint16_t          attr_handle
		// - uint16_t          service_handle
		// - esp_bt_uuid_t     char_uuid
		//
		case ESP_GATTS_ADD_CHAR_EVT: {
			break;
		} // ESP_GATTS_ADD_CHAR_EVT


		// ESP_GATTS_CONNECT_EVT
		// connect:
		// - uint16_t      conn_id
		// - esp_bd_addr_t remote_bda
		// - bool          is_connected
		//
		case ESP_GATTS_CONNECT_EVT: {
			m_connId = param->connect.conn_id; // Save the connection id.
			if (m_pServerCallbacks != nullptr) {
				m_pServerCallbacks->onConnect(this);
			}
			m_connectedCount++;   // Increment the number of connected devices count.
			break;
		} // ESP_GATTS_CONNECT_EVT


		// ESP_GATTS_CREATE_EVT
		// Called when a new service is registered as having been created.
		//
		// create:
		// * esp_gatt_status_t  status
		// * uint16_t           service_handle
		// * esp_gatt_srvc_id_t service_id
		//
		case ESP_GATTS_CREATE_EVT: {
			BLEService* pService = m_serviceMap.getByUUID(param->create.service_id.id.uuid);
			m_serviceMap.setByHandle(param->create.service_handle, pService);
			m_semaphoreCreateEvt.give();
			break;
		} // ESP_GATTS_CREATE_EVT


		// ESP_GATTS_DISCONNECT_EVT
		//
		// disconnect
		// - uint16_t      conn_id
		// - esp_bd_addr_t remote_bda
		// - bool          is_connected
		//
		// If we receive a disconnect event then invoke the callback for disconnects (if one is present).
		// we also want to start advertising again.
		case ESP_GATTS_DISCONNECT_EVT: {
			m_connectedCount--;                          // Decrement the number of connected devices count.
			if (m_pServerCallbacks != nullptr) {         // If we have callbacks, call now.
				m_pServerCallbacks->onDisconnect(this);
			}
			startAdvertising(); //- do this with some delay from the loop()
			break;
		} // ESP_GATTS_DISCONNECT_EVT


		// ESP_GATTS_READ_EVT - A request to read the value of a characteristic has arrived.
		//
		// read:
		// - uint16_t      conn_id
		// - uint32_t      trans_id
		// - esp_bd_addr_t bda
		// - uint16_t      handle
		// - uint16_t      offset
		// - bool          is_long
		// - bool          need_rsp
		//
		case ESP_GATTS_READ_EVT: {
			break;
		} // ESP_GATTS_READ_EVT


		// ESP_GATTS_REG_EVT
		// reg:
		// - esp_gatt_status_t status
		// - uint16_t app_id
		//
		case ESP_GATTS_REG_EVT: {
			m_gatts_if = gatts_if;
			m_semaphoreRegisterAppEvt.give(); // Unlock the mutex waiting for the registration of the app.
			break;
		} // ESP_GATTS_REG_EVT


		// ESP_GATTS_WRITE_EVT - A request to write the value of a characteristic has arrived.
		//
		// write:
		// - uint16_t      conn_id
		// - uint16_t      trans_id
		// - esp_bd_addr_t bda
		// - uint16_t      handle
		// - uint16_t      offset
		// - bool          need_rsp
		// - bool          is_prep
		// - uint16_t      len
		// - uint8_t*      value
		//
		case ESP_GATTS_WRITE_EVT: {
			break;
		}

		default: {
			break;
		}
	}
	ESP_LOGD(LOG_TAG, "<< handleGATTServerEvent");
} // handleGATTServerEvent


/**
 * @brief Register the app.
 *
 * @return N/A
 */
void BLEServer::registerApp() {
	ESP_LOGD(LOG_TAG, ">> registerApp - %d", m_appId);
	m_semaphoreRegisterAppEvt.take("registerApp"); // Take the mutex, will be released by ESP_GATTS_REG_EVT event.
	::esp_ble_gatts_app_register(m_appId);
	m_semaphoreRegisterAppEvt.wait("registerApp");
	ESP_LOGD(LOG_TAG, "<< registerApp");
} // registerApp


/**
 * @brief Set the server callbacks.
 *
 * As a %BLE server operates, it will generate server level events such as a new client connecting or a previous client
 * disconnecting.  This function can be called to register a callback handler that will be invoked when these
 * events are detected.
 *
 * @param [in] pCallbacks The callbacks to be invoked.
 */
void BLEServer::setCallbacks(BLEServerCallbacks* pCallbacks) {
	m_pServerCallbacks = pCallbacks;
} // setCallbacks


/**
 * @brief Start advertising.
 *
 * Start the server advertising its existence.  This is a convenience function and is equivalent to
 * retrieving the advertising object and invoking start upon it.
 */
void BLEServer::startAdvertising() {
	ESP_LOGD(LOG_TAG, ">> startAdvertising");
	m_bleAdvertising.start();
	ESP_LOGD(LOG_TAG, "<< startAdvertising");
} // startAdvertising


void BLEServerCallbacks::onConnect(BLEServer* pServer) {
	ESP_LOGD("BLEServerCallbacks", ">> onConnect(): Default");
	ESP_LOGD("BLEServerCallbacks", "Device: %s", BLEDevice::toString().c_str());
	ESP_LOGD("BLEServerCallbacks", "<< onConnect()");
} // onConnect


void BLEServerCallbacks::onDisconnect(BLEServer* pServer) {
	ESP_LOGD("BLEServerCallbacks", ">> onDisconnect(): Default");
	ESP_LOGD("BLEServerCallbacks", "Device: %s", BLEDevice::toString().c_str());
	ESP_LOGD("BLEServerCallbacks", "<< onDisconnect()");
} // onDisconnect

#endif // CONFIG_BT_ENABLED
