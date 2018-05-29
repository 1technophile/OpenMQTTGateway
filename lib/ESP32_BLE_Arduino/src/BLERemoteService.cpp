/*
 * BLERemoteService.cpp
 *
 *  Created on: Jul 8, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <sstream>
#include "BLERemoteService.h"
#include "BLEUtils.h"
#include "GeneralUtils.h"
#include <esp_log.h>
#include <esp_err.h>
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

static const char* LOG_TAG = "BLERemoteService";

BLERemoteService::BLERemoteService(
		esp_gatt_id_t srvcId,
		BLEClient*    pClient,
		uint16_t      startHandle,
		uint16_t      endHandle
	) {

	ESP_LOGD(LOG_TAG, ">> BLERemoteService()");
	m_srvcId  = srvcId;
	m_pClient = pClient;
	m_uuid    = BLEUUID(m_srvcId);
	m_haveCharacteristics = false;
	m_startHandle = startHandle;
	m_endHandle = endHandle;

	ESP_LOGD(LOG_TAG, "<< BLERemoteService()");
}


BLERemoteService::~BLERemoteService() {
	removeCharacteristics();
}

/*
static bool compareSrvcId(esp_gatt_srvc_id_t id1, esp_gatt_srvc_id_t id2) {
	if (id1.id.inst_id != id2.id.inst_id) {
		return false;
	}
	if (!BLEUUID(id1.id.uuid).equals(BLEUUID(id2.id.uuid))) {
		return false;
	}
	return true;
} // compareSrvcId
*/

/**
 * @brief Handle GATT Client events
 */
void BLERemoteService::gattClientEventHandler(
	esp_gattc_cb_event_t      event,
	esp_gatt_if_t             gattc_if,
	esp_ble_gattc_cb_param_t *evtParam) {
	switch(event) {
		//
		// ESP_GATTC_GET_CHAR_EVT
		//
		// get_char:
		// - esp_gatt_status_t    status
		// - uin1t6_t             conn_id
		// - esp_gatt_srvc_id_t   srvc_id
		// - esp_gatt_id_t        char_id
		// - esp_gatt_char_prop_t char_prop
		//
	/*
		case ESP_GATTC_GET_CHAR_EVT: {
			// Is this event for this service?  If yes, then the local srvc_id and the event srvc_id will be
			// the same.
			if (compareSrvcId(m_srvcId, evtParam->get_char.srvc_id) == false) {
				break;
			}

			// If the status is NOT OK then we have a problem and continue.
			if (evtParam->get_char.status != ESP_GATT_OK) {
				m_semaphoreGetCharEvt.give();
				break;
			}

			// This is an indication that we now have the characteristic details for a characteristic owned
			// by this service so remember it.
			m_characteristicMap.insert(std::pair<std::string, BLERemoteCharacteristic*>(
					BLEUUID(evtParam->get_char.char_id.uuid).toString(),
					new BLERemoteCharacteristic(evtParam->get_char.char_id, evtParam->get_char.char_prop, this)	));


			// Now that we have received a characteristic, lets ask for the next one.
			esp_err_t errRc = ::esp_ble_gattc_get_characteristic(
					m_pClient->getGattcIf(),
					m_pClient->getConnId(),
					&m_srvcId,
					&evtParam->get_char.char_id);
			if (errRc != ESP_OK) {
				ESP_LOGE(LOG_TAG, "esp_ble_gattc_get_characteristic: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
				break;
			}

			//m_semaphoreGetCharEvt.give();
			break;
		} // ESP_GATTC_GET_CHAR_EVT
*/
		default: {
			break;
		}
	} // switch

	// Send the event to each of the characteristics owned by this service.
	for (auto &myPair : m_characteristicMap) {
	   myPair.first->gattClientEventHandler(event, gattc_if, evtParam);
	}
} // gattClientEventHandler


/**
 * @brief Get the remote characteristic object for the characteristic UUID.
 * @param [in] uuid Remote characteristic uuid.
 * @return Reference to the remote characteristic object.
 */
BLERemoteCharacteristic* BLERemoteService::getCharacteristic(const char* uuid) {
    return getCharacteristic(BLEUUID(uuid));
} // getCharacteristic
	
	
/**
 * @brief Get the characteristic object for the UUID.
 * @param [in] uuid Characteristic uuid.
 * @return Reference to the characteristic object.
 */
BLERemoteCharacteristic* BLERemoteService::getCharacteristic(BLEUUID uuid) {
// Design
// ------
// We wish to retrieve the characteristic given its UUID.  It is possible that we have not yet asked the
// device what characteristics it has in which case we have nothing to match against.  If we have not
// asked the device about its characteristics, then we do that now.  Once we get the results we can then
// examine the characteristics map to see if it has the characteristic we are looking for.
	if (!m_haveCharacteristics) {
		retrieveCharacteristics();
	}
	std::string v = uuid.toString();
	for (auto &myPair : m_characteristicMap) {
		if (myPair.second == v) {
			return myPair.first;
		}
	}
	return nullptr;
} // getCharacteristic


/**
 * @brief Retrieve all the characteristics for this service.
 * This function will not return until we have all the characteristics.
 * @return N/A
 */
void BLERemoteService::retrieveCharacteristics() {

	ESP_LOGD(LOG_TAG, ">> getCharacteristics() for service: %s", getUUID().toString().c_str());

	removeCharacteristics(); // Forget any previous characteristics.
	/*
	m_semaphoreGetCharEvt.take("getCharacteristics");

	esp_err_t errRc = ::esp_ble_gattc_get_characteristic(
		m_pClient->getGattcIf(),
		m_pClient->getConnId(),
		&m_srvcId,
		nullptr);

	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gattc_get_characteristic: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}

	m_semaphoreGetCharEvt.wait("getCharacteristics"); // Wait for the characteristics to become available.

	m_haveCharacteristics = true; // Remember that we have received the characteristics.
	*/
	//ESP_LOGE(LOG_TAG, "!!! NOT IMPLEMENTED !!!");
	//ESP_LOGD(LOG_TAG, "--- test code ---");
	/*
	uint16_t count;
	esp_gatt_status_t status = ::esp_ble_gattc_get_attr_count(
		getClient()->getGattcIf(),
		getClient()->getConnId(),
		ESP_GATT_DB_CHARACTERISTIC,
		m_startHandle,
		m_endHandle,
		0, // Characteristic handle ... only used for ESP_GATT_DB_DESCRIPTOR
		&count
	);
	if (status != ESP_GATT_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gattc_get_attr_count: %s", BLEUtils::gattStatusToString(status).c_str());
	} else {
		ESP_LOGD(LOG_TAG, "Number of characteristics associated with service is %d", count);
	}

	count = 1;
	esp_gattc_service_elem_t srvcElem;
	status = ::esp_ble_gattc_get_service(
		getClient()->getGattcIf(),
		getClient()->getConnId(),
		&m_srvcId.uuid, // UUID of service
		&srvcElem, // Records
		&count, // records retrieved
		0 // offset
	);
	if (status != ESP_GATT_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gattc_get_service: %s", BLEUtils::gattStatusToString(status).c_str());
	}
	else {
		ESP_LOGD(LOG_TAG, "%s", BLEUtils::gattcServiceElementToString(&srvcElem).c_str());
	}
	*/

	uint16_t offset = 0;
	esp_gattc_char_elem_t result;
	while(1) {
		uint16_t count = 1;
		esp_gatt_status_t status = ::esp_ble_gattc_get_all_char(
			getClient()->getGattcIf(),
			getClient()->getConnId(),
			m_startHandle,
			m_endHandle,
			&result,
			&count,
			offset
		);

		if (status == ESP_GATT_INVALID_OFFSET) {   // We have reached the end of the entries.
			break;
		}

		if (status != ESP_GATT_OK) {   // If we got an error, end.
			ESP_LOGE(LOG_TAG, "esp_ble_gattc_get_all_char: %s", BLEUtils::gattStatusToString(status).c_str());
			break;
		}

		if (count == 0) {   // If we failed to get any new records, end.
			break;
		}

		ESP_LOGD(LOG_TAG, "Found a characteristic: Handle: %d, UUID: %s", result.char_handle, BLEUUID(result.uuid).toString().c_str());

		// We now have a new characteristic ... let us add that to our set of known characteristics
		BLERemoteCharacteristic *pNewRemoteCharacteristic = new BLERemoteCharacteristic(
			result.char_handle,
			BLEUUID(result.uuid),
			result.properties,
			this
		);

		m_characteristicMap.insert(std::pair<BLERemoteCharacteristic*, std::string>(pNewRemoteCharacteristic, pNewRemoteCharacteristic->getUUID().toString()));

		offset++;   // Increment our count of number of descriptors found.
	} // Loop forever (until we break inside the loop).

	m_haveCharacteristics = true; // Remember that we have received the characteristics.
	ESP_LOGD(LOG_TAG, "<< getCharacteristics()");
} // getCharacteristics


/**
 * @brief Retrieve a map of all the characteristics of this service.
 * @return A map of all the characteristics of this service.
 */
std::map<BLERemoteCharacteristic*, std::string>* BLERemoteService::getCharacteristics() {
	ESP_LOGD(LOG_TAG, ">> getCharacteristics() for service: %s", getUUID().toString().c_str());
	// If is possible that we have not read the characteristics associated with the service so do that
	// now.  The request to retrieve the characteristics by calling "retrieveCharacteristics" is a blocking
	// call and does not return until all the characteristics are available.
	if (!m_haveCharacteristics) {
		retrieveCharacteristics();
	}
	ESP_LOGD(LOG_TAG, "<< getCharacteristics() for service: %s", getUUID().toString().c_str());
	return &m_characteristicMap;
} // getCharacteristics


BLEClient* BLERemoteService::getClient() {
	return m_pClient;
}


uint16_t BLERemoteService::getEndHandle() {
	return m_endHandle;
}


esp_gatt_id_t* BLERemoteService::getSrvcId() {
	return &m_srvcId;
}


uint16_t BLERemoteService::getStartHandle() {
	return m_startHandle;
}

uint16_t BLERemoteService::getHandle() {
	ESP_LOGD(LOG_TAG, ">> getHandle: service: %s", getUUID().toString().c_str());
	//ESP_LOGE(LOG_TAG, "!!! getHandle:  NOT IMPLEMENTED !!!");
	ESP_LOGD(LOG_TAG, "<< getHandle: %d 0x%.2x", getStartHandle(), getStartHandle());
	return getStartHandle();
}


BLEUUID BLERemoteService::getUUID() {
	return m_uuid;
}


/**
 * @brief Delete the characteristics in the characteristics map.
 * We maintain a map called m_characteristicsMap that contains pointers to BLERemoteCharacteristic
 * object references.  Since we allocated these in this class, we are also responsible for deleteing
 * them.  This method does just that.
 * @return N/A.
 */
void BLERemoteService::removeCharacteristics() {
	for (auto &myPair : m_characteristicMap) {
	   delete myPair.first;
	   m_characteristicMap.erase(myPair.first);
	}
	m_characteristicMap.clear();   // Clear the map
} // removeCharacteristics



/**
 * @brief Create a string representation of this remote service.
 * @return A string representation of this remote service.
 */
std::string BLERemoteService::toString() {
	std::ostringstream ss;
	ss << "Service: uuid: " + m_uuid.toString();
	ss << ", start_handle: " << std::dec << m_startHandle << " 0x" << std::hex << m_startHandle <<
			", end_handle: " << std::dec << m_endHandle << " 0x" << std::hex << m_endHandle;
	for (auto &myPair : m_characteristicMap) {
		ss << "\n" << myPair.first->toString();
	   // myPair.second is the value
	}
	return ss.str();
} // toString





#endif /* CONFIG_BT_ENABLED */
