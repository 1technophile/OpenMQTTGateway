/*
 * BLEDescriptor.cpp
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <sstream>
#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include "sdkconfig.h"
#include <esp_log.h>
#include <esp_err.h>
#include "BLEService.h"
#include "BLEDescriptor.h"
#include "GeneralUtils.h"
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

static const char* LOG_TAG = "BLEDescriptor";


#define NULL_HANDLE (0xffff)


/**
 * @brief BLEDescriptor constructor.
 */
BLEDescriptor::BLEDescriptor(const char* uuid) : BLEDescriptor(BLEUUID(uuid)) {
}	

/**
 * @brief BLEDescriptor constructor.
 */
BLEDescriptor::BLEDescriptor(BLEUUID uuid) {
	m_bleUUID            = uuid;
	m_value.attr_value   = (uint8_t *)malloc(ESP_GATT_MAX_ATTR_LEN);  // Allocate storage for the value.
	m_value.attr_len     = 0;                                         // Initial length is 0.
	m_value.attr_max_len = ESP_GATT_MAX_ATTR_LEN;                     // Maximum length of the data.
	m_handle             = NULL_HANDLE;                               // Handle is initially unknown.
	m_pCharacteristic    = nullptr;                                   // No initial characteristic.
	m_pCallback          = nullptr;                                   // No initial callback.

} // BLEDescriptor


/**
 * @brief BLEDescriptor destructor.
 */
BLEDescriptor::~BLEDescriptor() {
	free(m_value.attr_value);   // Release the storage we created in the constructor.
} // ~BLEDescriptor


/**
 * @brief Execute the creation of the descriptor with the BLE runtime in ESP.
 * @param [in] pCharacteristic The characteristic to which to register this descriptor.
 */
void BLEDescriptor::executeCreate(BLECharacteristic* pCharacteristic) {
	ESP_LOGD(LOG_TAG, ">> executeCreate(): %s", toString().c_str());

	if (m_handle != NULL_HANDLE) {
		ESP_LOGE(LOG_TAG, "Descriptor already has a handle.");
		return;
	}

	m_pCharacteristic = pCharacteristic; // Save the characteristic associated with this service.

	esp_attr_control_t control;
	control.auto_rsp = ESP_GATT_RSP_BY_APP;
	m_semaphoreCreateEvt.take("executeCreate");
	esp_err_t errRc = ::esp_ble_gatts_add_char_descr(
			pCharacteristic->getService()->getHandle(),
			getUUID().getNative(),
			(esp_gatt_perm_t)(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE),
			&m_value,
			&control);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "<< esp_ble_gatts_add_char_descr: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}

	m_semaphoreCreateEvt.wait("executeCreate");
	ESP_LOGD(LOG_TAG, "<< executeCreate");
} // executeCreate


/**
 * @brief Get the BLE handle for this descriptor.
 * @return The handle for this descriptor.
 */
uint16_t BLEDescriptor::getHandle() {
	return m_handle;
} // getHandle


/**
 * @brief Get the length of the value of this descriptor.
 * @return The length (in bytes) of the value of this descriptor.
 */
size_t BLEDescriptor::getLength() {
	return m_value.attr_len;
} // getLength


/**
 * @brief Get the UUID of the descriptor.
 */
BLEUUID BLEDescriptor::getUUID() {
	return m_bleUUID;
} // getUUID



/**
 * @brief Get the value of this descriptor.
 * @return A pointer to the value of this descriptor.
 */
uint8_t* BLEDescriptor::getValue() {
	return m_value.attr_value;
} // getValue


/**
 * @brief Handle GATT server events for the descripttor.
 * @param [in] event
 * @param [in] gatts_if
 * @param [in] param
 */
void BLEDescriptor::handleGATTServerEvent(
		esp_gatts_cb_event_t      event,
		esp_gatt_if_t             gatts_if,
		esp_ble_gatts_cb_param_t *param) {
	switch(event) {
		// ESP_GATTS_ADD_CHAR_DESCR_EVT
		//
		// add_char_descr:
		// - esp_gatt_status_t status
		// - uint16_t          attr_handle
		// - uint16_t          service_handle
		// - esp_bt_uuid_t     char_uuid
		case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
			/*
			ESP_LOGD(LOG_TAG, "DEBUG: m_pCharacteristic: %x", (uint32_t)m_pCharacteristic);
			ESP_LOGD(LOG_TAG, "DEBUG: m_bleUUID: %s, add_char_descr.char_uuid: %s, equals: %d",
				m_bleUUID.toString().c_str(),
				BLEUUID(param->add_char_descr.char_uuid).toString().c_str(),
				m_bleUUID.equals(BLEUUID(param->add_char_descr.char_uuid)));
			ESP_LOGD(LOG_TAG, "DEBUG: service->getHandle: %x, add_char_descr.service_handle: %x",
					m_pCharacteristic->getService()->getHandle(), param->add_char_descr.service_handle);
			ESP_LOGD(LOG_TAG, "DEBUG: service->lastCharacteristic: %x",
					(uint32_t)m_pCharacteristic->getService()->getLastCreatedCharacteristic());
					*/
			if (m_pCharacteristic != nullptr &&
					m_bleUUID.equals(BLEUUID(param->add_char_descr.char_uuid)) &&
					m_pCharacteristic->getService()->getHandle() == param->add_char_descr.service_handle &&
					m_pCharacteristic == m_pCharacteristic->getService()->getLastCreatedCharacteristic()) {
				setHandle(param->add_char_descr.attr_handle);
				m_semaphoreCreateEvt.give();
			}
			break;
		} // ESP_GATTS_ADD_CHAR_DESCR_EVT

		// ESP_GATTS_WRITE_EVT - A request to write the value of a descriptor has arrived.
		//
		// write:
		// - uint16_t conn_id
		// - uint16_t trans_id
		// - esp_bd_addr_t bda
		// - uint16_t handle
		// - uint16_t offset
		// - bool need_rsp
		// - bool is_prep
		// - uint16_t len
		// - uint8_t *value
		case ESP_GATTS_WRITE_EVT: {
			if (param->write.handle == m_handle) {
				setValue(param->write.value, param->write.len);   // Set the value of the descriptor.

				esp_gatt_rsp_t rsp;   // Build a response.
				rsp.attr_value.len    = getLength();
				rsp.attr_value.handle = m_handle;
				rsp.attr_value.offset = 0;
				rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
				memcpy(rsp.attr_value.value, getValue(), rsp.attr_value.len);
				esp_err_t errRc = ::esp_ble_gatts_send_response(
						gatts_if,
						param->write.conn_id,
						param->write.trans_id,
						ESP_GATT_OK,
						&rsp);

				if (errRc != ESP_OK) {   // Check the return code from the send of the response.
					ESP_LOGE(LOG_TAG, "esp_ble_gatts_send_response: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
				}

				if (m_pCallback != nullptr) {   // We have completed the write, if there is a user supplied callback handler, invoke it now.
					m_pCallback->onWrite(this);   // Invoke the onWrite callback handler.
				}
			}  // End of ... this is our handle.

			break;
		} // ESP_GATTS_WRITE_EVT

		// ESP_GATTS_READ_EVT - A request to read the value of a descriptor has arrived.
		//
		// read:
		// - uint16_t conn_id
		// - uint32_t trans_id
		// - esp_bd_addr_t bda
		// - uint16_t handle
		// - uint16_t offset
		// - bool is_long
		// - bool need_rsp
		//
		case ESP_GATTS_READ_EVT: {
			if (param->read.handle == m_handle) {  // If this event is for this descriptor ... process it

				if (m_pCallback != nullptr) {   // If we have a user supplied callback, invoke it now.
					m_pCallback->onRead(this);    // Invoke the onRead callback method in the callback handler.
				}

				if (param->read.need_rsp) {   // Do we need a response
					ESP_LOGD(LOG_TAG, "Sending a response (esp_ble_gatts_send_response)");
					esp_gatt_rsp_t rsp;
					rsp.attr_value.len      = getLength();
					rsp.attr_value.handle   = param->read.handle;
					rsp.attr_value.offset   = 0;
					rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
					memcpy(rsp.attr_value.value, getValue(), rsp.attr_value.len);

					esp_err_t errRc = ::esp_ble_gatts_send_response(
							gatts_if,
							param->read.conn_id,
							param->read.trans_id,
							ESP_GATT_OK,
							&rsp);

					if (errRc != ESP_OK) {   // Check the return code from the send of the response.
						ESP_LOGE(LOG_TAG, "esp_ble_gatts_send_response: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
					}
				} // End of need a response.
			} // End of this is our handle
			break;
		} // ESP_GATTS_READ_EVT

		default: {
			break;
		}
	}// switch event
} // handleGATTServerEvent


/**
 * @brief Set the callback handlers for this descriptor.
 * @param [in] pCallbacks An instance of a callback structure used to define any callbacks for the descriptor.
 */
void BLEDescriptor::setCallbacks(BLEDescriptorCallbacks* pCallback) {
	ESP_LOGD(LOG_TAG, ">> setCallbacks: 0x%x", (uint32_t)pCallback);
	m_pCallback = pCallback;
	ESP_LOGD(LOG_TAG, "<< setCallbacks");
} // setCallbacks


/**
 * @brief Set the handle of this descriptor.
 * Set the handle of this descriptor to be the supplied value.
 * @param [in] handle The handle to be associated with this descriptor.
 * @return N/A.
 */
void BLEDescriptor::setHandle(uint16_t handle) {
	ESP_LOGD(LOG_TAG, ">> setHandle(0x%.2x): Setting descriptor handle to be 0x%.2x", handle, handle);
	m_handle = handle;
	ESP_LOGD(LOG_TAG, "<< setHandle()");
} // setHandle


/**
 * @brief Set the value of the descriptor.
 * @param [in] data The data to set for the descriptor.
 * @param [in] length The length of the data in bytes.
 */
void BLEDescriptor::setValue(uint8_t* data, size_t length) {
	if (length > ESP_GATT_MAX_ATTR_LEN) {
		ESP_LOGE(LOG_TAG, "Size %d too large, must be no bigger than %d", length, ESP_GATT_MAX_ATTR_LEN);
		return;
	}
	m_value.attr_len = length;
	memcpy(m_value.attr_value, data, length);
} // setValue


/**
 * @brief Set the value of the descriptor.
 * @param [in] value The value of the descriptor in string form.
 */
void BLEDescriptor::setValue(std::string value) {
	setValue((uint8_t *)value.data(), value.length());
} // setValue

void BLEDescriptor::setAccessPermissions(esp_gatt_perm_t perm) {
	m_permissions = perm;
}

/**
 * @brief Return a string representation of the descriptor.
 * @return A string representation of the descriptor.
 */
std::string BLEDescriptor::toString() {
	std::stringstream stringstream;
	stringstream << std::hex << std::setfill('0');
	stringstream << "UUID: " << m_bleUUID.toString() + ", handle: 0x" << std::setw(2) << m_handle;
	return stringstream.str();
} // toString


BLEDescriptorCallbacks::~BLEDescriptorCallbacks() {}

/**
 * @brief Callback function to support a read request.
 * @param [in] pDescriptor The descriptor that is the source of the event.
 */
void BLEDescriptorCallbacks::onRead(BLEDescriptor* pDescriptor) {
	ESP_LOGD("BLEDescriptorCallbacks", ">> onRead: default");
	ESP_LOGD("BLEDescriptorCallbacks", "<< onRead");
} // onRead


/**
 * @brief Callback function to support a write request.
 * @param [in] pDescriptor The descriptor that is the source of the event.
 */
void BLEDescriptorCallbacks::onWrite(BLEDescriptor* pDescriptor) {
	ESP_LOGD("BLEDescriptorCallbacks", ">> onWrite: default");
	ESP_LOGD("BLEDescriptorCallbacks", "<< onWrite");
} // onWrite


#endif /* CONFIG_BT_ENABLED */
