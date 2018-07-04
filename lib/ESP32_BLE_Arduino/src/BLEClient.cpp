/*
 * BLEDevice.cpp
 *
 *  Created on: Mar 22, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include "BLEClient.h"
#include "BLEUtils.h"
#include "BLEService.h"
#include "GeneralUtils.h"
#include <string>
#include <sstream>
#include <unordered_set>
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

/*
 * Design
 * ------
 * When we perform a searchService() requests, we are asking the BLE server to return each of the services
 * that it exposes.  For each service, we received an ESP_GATTC_SEARCH_RES_EVT event which contains details
 * of the exposed service including its UUID.
 *
 * The objects we will invent for a BLEClient will be as follows:
 * * BLERemoteService - A model of a remote service.
 * * BLERemoteCharacteristic - A model of a remote characteristic
 * * BLERemoteDescriptor - A model of a remote descriptor.
 *
 * Since there is a hierarchical relationship here, we will have the idea that from a BLERemoteService will own
 * zero or more remote characteristics and a BLERemoteCharacteristic will own zero or more remote BLEDescriptors.
 *
 * We will assume that a BLERemoteService contains a map that maps BLEUUIDs to the set of owned characteristics
 * and that a BLECharacteristic contains a map that maps BLEUUIDs to the set of owned descriptors.
 *
 *
 */
static const char* LOG_TAG = "BLEClient";

BLEClient::BLEClient() {
	m_pClientCallbacks = nullptr;
	m_conn_id          = 0;
	m_gattc_if         = 0;
	m_haveServices     = false;
	m_isConnected      = false;  // Initially, we are flagged as not connected.
} // BLEClient


/**
 * @brief Destructor.
 */
BLEClient::~BLEClient() {
	// We may have allocated service references associated with this client.  Before we are finished
	// with the client, we must release resources.
	for (auto &myPair : m_servicesMap) {
	   delete myPair.second;
	}
	m_servicesMap.clear();
} // ~BLEClient


/**
 * @brief Clear any existing services.
 *
 */
void BLEClient::clearServices() {
	ESP_LOGD(LOG_TAG, ">> clearServices");
	// Delete all the services.
	for (auto &myPair : m_servicesMap) {
	   delete myPair.second;
	}
	m_servicesMap.clear();
	ESP_LOGD(LOG_TAG, "<< clearServices");
} // clearServices


/**
 * @brief Connect to the partner (BLE Server).
 * @param [in] address The address of the partner.
 * @return True on success.
 */
bool BLEClient::connect(BLEAddress address) {
	ESP_LOGD(LOG_TAG, ">> connect(%s)", address.toString().c_str());

// We need the connection handle that we get from registering the application.  We register the app
// and then block on its completion.  When the event has arrived, we will have the handle.
	m_semaphoreRegEvt.take("connect");

	clearServices(); // Delete any services that may exist.

	esp_err_t errRc = ::esp_ble_gattc_app_register(0);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gattc_app_register: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return false;
	}

	m_semaphoreRegEvt.wait("connect");

	m_peerAddress = address;

	// Perform the open connection request against the target BLE Server.
	m_semaphoreOpenEvt.take("connect");
	errRc = ::esp_ble_gattc_open(
		getGattcIf(),
		*getPeerAddress().getNative(), // address
		1                              // direct connection
	);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gattc_open: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return false;
	}

	uint32_t rc = m_semaphoreOpenEvt.wait("connect");   // Wait for the connection to complete.
	ESP_LOGD(LOG_TAG, "<< connect(), rc=%d", rc==ESP_GATT_OK);
	return rc == ESP_GATT_OK;
} // connect


/**
 * @brief Disconnect from the peer.
 * @return N/A.
 */
void BLEClient::disconnect() {
	ESP_LOGD(LOG_TAG, ">> disconnect()");
	esp_err_t errRc = ::esp_ble_gattc_close(getGattcIf(), getConnId());
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gattc_close: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}
	esp_ble_gattc_app_unregister(getGattcIf());
	m_peerAddress = BLEAddress("00:00:00:00:00:00");
	ESP_LOGD(LOG_TAG, "<< disconnect()");
} // disconnect


/**
 * @brief Handle GATT Client events
 */
void BLEClient::gattClientEventHandler(
	esp_gattc_cb_event_t      event,
	esp_gatt_if_t             gattc_if,
	esp_ble_gattc_cb_param_t* evtParam) {

	// Execute handler code based on the type of event received.
	switch(event) {

		//
		// ESP_GATTC_DISCONNECT_EVT
		//
		// disconnect:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		// - esp_bd_addr_t     remote_bda
		case ESP_GATTC_DISCONNECT_EVT: {
				// If we receive a disconnect event, set the class flag that indicates that we are
				// no longer connected.
				if (m_pClientCallbacks != nullptr) {
					m_pClientCallbacks->onDisconnect(this);
				}
				m_isConnected = false;
				break;
		} // ESP_GATTC_DISCONNECT_EVT

		//
		// ESP_GATTC_OPEN_EVT
		//
		// open:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		// - esp_bd_addr_t     remote_bda
		// - uint16_t          mtu
		//
		case ESP_GATTC_OPEN_EVT: {
			m_conn_id = evtParam->open.conn_id;
			if (m_pClientCallbacks != nullptr) {
				m_pClientCallbacks->onConnect(this);
			}
			if (evtParam->open.status == ESP_GATT_OK) {
				m_isConnected = true;   // Flag us as connected.
			}
			m_semaphoreOpenEvt.give(evtParam->open.status);
			break;
		} // ESP_GATTC_OPEN_EVT


		//
		// ESP_GATTC_REG_EVT
		//
		// reg:
		// esp_gatt_status_t status
		// uint16_t          app_id
		//
		case ESP_GATTC_REG_EVT: {
			m_gattc_if = gattc_if;
			m_semaphoreRegEvt.give();
			break;
		} // ESP_GATTC_REG_EVT


		//
		// ESP_GATTC_SEARCH_CMPL_EVT
		//
		// search_cmpl:
		// - esp_gatt_status_t status
		// - uint16_t          conn_id
		//
		case ESP_GATTC_SEARCH_CMPL_EVT: {
			m_semaphoreSearchCmplEvt.give();
			break;
		} // ESP_GATTC_SEARCH_CMPL_EVT


		//
		// ESP_GATTC_SEARCH_RES_EVT
		//
		// search_res:
		// - uint16_t      conn_id
		// - uint16_t      start_handle
		// - uint16_t      end_handle
		// - esp_gatt_id_t srvc_id
		//
		case ESP_GATTC_SEARCH_RES_EVT: {
			BLEUUID uuid = BLEUUID(evtParam->search_res.srvc_id);
			BLERemoteService* pRemoteService = new BLERemoteService(
				evtParam->search_res.srvc_id,
				this,
				evtParam->search_res.start_handle,
				evtParam->search_res.end_handle
			);
			m_servicesMap.insert(std::pair<std::string, BLERemoteService *>(uuid.toString(), pRemoteService));
			break;
		} // ESP_GATTC_SEARCH_RES_EVT


		default: {
			break;
		}
	} // Switch

	// Pass the request on to all services.
	for (auto &myPair : m_servicesMap) {
	   myPair.second->gattClientEventHandler(event, gattc_if, evtParam);
	}

} // gattClientEventHandler


uint16_t BLEClient::getConnId() {
	return m_conn_id;
} // getConnId



esp_gatt_if_t BLEClient::getGattcIf() {
	return m_gattc_if;
} // getGattcIf


/**
 * @brief Retrieve the address of the peer.
 *
 * Returns the Bluetooth device address of the %BLE peer to which this client is connected.
 */
BLEAddress BLEClient::getPeerAddress() {
	return m_peerAddress;
} // getAddress


/**
 * @brief Ask the BLE server for the RSSI value.
 * @return The RSSI value.
 */
int BLEClient::getRssi() {
	ESP_LOGD(LOG_TAG, ">> getRssi()");
	if (!isConnected()) {
		ESP_LOGD(LOG_TAG, "<< getRssi(): Not connected");
		return 0;
	}
	// We make the API call to read the RSSI value which is an asynchronous operation.  We expect to receive
	// an ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT to indicate completion.
	//
	m_semaphoreRssiCmplEvt.take("getRssi");
	esp_err_t rc = ::esp_ble_gap_read_rssi(*getPeerAddress().getNative());
	if (rc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "<< getRssi: esp_ble_gap_read_rssi: rc=%d %s", rc, GeneralUtils::errorToString(rc));
		return 0;
	}
	int rssiValue = m_semaphoreRssiCmplEvt.wait("getRssi");
	ESP_LOGD(LOG_TAG, "<< getRssi(): %d", rssiValue);
	return rssiValue;
} // getRssi


/**
 * @brief Get the service BLE Remote Service instance corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A reference to the Service or nullptr if don't know about it.
 */
BLERemoteService* BLEClient::getService(const char* uuid) {
    return getService(BLEUUID(uuid));
} // getService


/**
 * @brief Get the service object corresponding to the uuid.
 * @param [in] uuid The UUID of the service being sought.
 * @return A reference to the Service or nullptr if don't know about it.
 * @throws BLEUuidNotFound
 */
BLERemoteService* BLEClient::getService(BLEUUID uuid) {
	ESP_LOGD(LOG_TAG, ">> getService: uuid: %s", uuid.toString().c_str());
// Design
// ------
// We wish to retrieve the service given its UUID.  It is possible that we have not yet asked the
// device what services it has in which case we have nothing to match against.  If we have not
// asked the device about its services, then we do that now.  Once we get the results we can then
// examine the services map to see if it has the service we are looking for.
	if (!m_haveServices) {
		getServices();
	}
	std::string uuidStr = uuid.toString();
	for (auto &myPair : m_servicesMap) {
		if (myPair.first == uuidStr) {
			ESP_LOGD(LOG_TAG, "<< getService: found the service with uuid: %s", uuid.toString().c_str());
			return myPair.second;
		}
	} // End of each of the services.
	ESP_LOGD(LOG_TAG, "<< getService: not found");
	throw new BLEUuidNotFoundException;
} // getService


/**
 * @brief Ask the remote %BLE server for its services.
 * A %BLE Server exposes a set of services for its partners.  Here we ask the server for its set of
 * services and wait until we have received them all.
 * @return N/A
 */
std::map<std::string, BLERemoteService*>* BLEClient::getServices() {
/*
 * Design
 * ------
 * We invoke esp_ble_gattc_search_service.  This will request a list of the service exposed by the
 * peer BLE partner to be returned as events.  Each event will be an an instance of ESP_GATTC_SEARCH_RES_EVT
 * and will culminate with an ESP_GATTC_SEARCH_CMPL_EVT when all have been received.
 */
	ESP_LOGD(LOG_TAG, ">> getServices");

	clearServices(); // Clear any services that may exist.

	esp_err_t errRc = esp_ble_gattc_search_service(
		getGattcIf(),
		getConnId(),
		nullptr            // Filter UUID
	);
	m_semaphoreSearchCmplEvt.take("getServices");
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gattc_search_service: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return &m_servicesMap;
	}
	m_semaphoreSearchCmplEvt.wait("getServices");
	m_haveServices = true; // Remember that we now have services.
	ESP_LOGD(LOG_TAG, "<< getServices");
	return &m_servicesMap;
} // getServices


/**
 * @brief Get the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to read.
 * @throws BLEUuidNotFound
 */
std::string BLEClient::getValue(BLEUUID serviceUUID, BLEUUID characteristicUUID) {
	ESP_LOGD(LOG_TAG, ">> getValue: serviceUUID: %s, characteristicUUID: %s", serviceUUID.toString().c_str(), characteristicUUID.toString().c_str());
	std::string ret = getService(serviceUUID)->getCharacteristic(characteristicUUID)->readValue();
	ESP_LOGD(LOG_TAG, "<<getValue");
	return ret;
} // getValue


/**
 * @brief Handle a received GAP event.
 *
 * @param [in] event
 * @param [in] param
 */
void BLEClient::handleGAPEvent(
		esp_gap_ble_cb_event_t  event,
		esp_ble_gap_cb_param_t* param) {
	ESP_LOGD(LOG_TAG, "BLEClient ... handling GAP event!");
	switch(event) {
		//
		// ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT
		//
		// read_rssi_cmpl
		// - esp_bt_status_t status
		// - int8_t rssi
		// - esp_bd_addr_t remote_addr
		//
		case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT: {
			m_semaphoreRssiCmplEvt.give((uint32_t)param->read_rssi_cmpl.rssi);
			break;
		} // ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT

		default:
			break;
	}
} // handleGAPEvent


/**
 * @brief Are we connected to a partner?
 * @return True if we are connected and false if we are not connected.
 */
bool BLEClient::isConnected() {
	return m_isConnected;
} // isConnected




/**
 * @brief Set the callbacks that will be invoked.
 */
void BLEClient::setClientCallbacks(BLEClientCallbacks* pClientCallbacks) {
	m_pClientCallbacks = pClientCallbacks;
} // setClientCallbacks


/**
 * @brief Set the value of a specific characteristic associated with a specific service.
 * @param [in] serviceUUID The service that owns the characteristic.
 * @param [in] characteristicUUID The characteristic whose value we wish to write.
 * @throws BLEUuidNotFound
 */
void BLEClient::setValue(BLEUUID serviceUUID, BLEUUID characteristicUUID, std::string value) {
	ESP_LOGD(LOG_TAG, ">> setValue: serviceUUID: %s, characteristicUUID: %s", serviceUUID.toString().c_str(), characteristicUUID.toString().c_str());
	getService(serviceUUID)->getCharacteristic(characteristicUUID)->writeValue(value);
	ESP_LOGD(LOG_TAG, "<< setValue");
} // setValue


/**
 * @brief Return a string representation of this client.
 * @return A string representation of this client.
 */
std::string BLEClient::toString() {
	std::ostringstream ss;
	ss << "peer address: " << m_peerAddress.toString();
	ss << "\nServices:\n";
	for (auto &myPair : m_servicesMap) {
		ss << myPair.second->toString() << "\n";
	  // myPair.second is the value
	}
	return ss.str();
} // toString


#endif // CONFIG_BT_ENABLED
