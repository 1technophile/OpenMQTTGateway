/*
 * BLEAdvertising.cpp
 *
 * This class encapsulates advertising a BLE Server.
 *  Created on: Jun 21, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include "BLEAdvertising.h"
#include <esp_log.h>
#include <esp_err.h>
#include "BLEUtils.h"
#include "GeneralUtils.h"
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

static const char* LOG_TAG = "BLEAdvertising";


/**
 * @brief Construct a default advertising object.
 *
 */
BLEAdvertising::BLEAdvertising() {
	m_advData.set_scan_rsp        = false;
	m_advData.include_name        = true;
	m_advData.include_txpower     = true;
	m_advData.min_interval        = 0x20;
	m_advData.max_interval        = 0x40;
	m_advData.appearance          = 0x00;
	m_advData.manufacturer_len    = 0;
	m_advData.p_manufacturer_data = nullptr;
	m_advData.service_data_len    = 0;
	m_advData.p_service_data      = nullptr;
	m_advData.service_uuid_len    = 0;
	m_advData.p_service_uuid      = nullptr;
	m_advData.flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

	m_advParams.adv_int_min       = 0x20;
	m_advParams.adv_int_max       = 0x40;
	m_advParams.adv_type          = ADV_TYPE_IND;
	m_advParams.own_addr_type     = BLE_ADDR_TYPE_PUBLIC;
	m_advParams.channel_map       = ADV_CHNL_ALL;
	m_advParams.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
} // BLEAdvertising


/**
 * @brief Add a service uuid to exposed list of services.
 * @param [in] serviceUUID The UUID of the service to expose.
 */
void BLEAdvertising::addServiceUUID(BLEUUID serviceUUID) {
	m_serviceUUIDs.push_back(serviceUUID);
} // addServiceUUID


/**
 * @brief Add a service uuid to exposed list of services.
 * @param [in] serviceUUID The string representation of the service to expose.
 */
void BLEAdvertising::addServiceUUID(const char* serviceUUID) {
	addServiceUUID(BLEUUID(serviceUUID));
} // addServiceUUID


/**
 * @brief Set the device appearance in the advertising data.
 * The appearance attribute is of type 0x19.  The codes for distinct appearances can be found here:
 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.gap.appearance.xml.
 * @param [in] appearance The appearance of the device in the advertising data.
 * @return N/A.
 */
void BLEAdvertising::setAppearance(uint16_t appearance) {
	m_advData.appearance = appearance;
} // setAppearance


/**
 * @brief Start advertising.
 * Start advertising.
 * @return N/A.
 */
void BLEAdvertising::start() {
	ESP_LOGD(LOG_TAG, ">> start");

	// We have a vector of service UUIDs that we wish to advertise.  In order to use the
	// ESP-IDF framework, these must be supplied in a contiguous array of their 128bit (16 byte)
	// representations.  If we have 1 or more services to advertise then we allocate enough
	// storage to host them and then copy them in one at a time into the contiguous storage.
	int numServices = m_serviceUUIDs.size();
	if (numServices > 0) {
		m_advData.service_uuid_len = 16*numServices;
		m_advData.p_service_uuid = new uint8_t[m_advData.service_uuid_len];
		uint8_t* p = m_advData.p_service_uuid;
		for (int i=0; i<numServices; i++) {
			ESP_LOGD(LOG_TAG, "- advertising service: %s", m_serviceUUIDs[i].toString().c_str());
			BLEUUID serviceUUID128 = m_serviceUUIDs[i].to128();
			memcpy(p, serviceUUID128.getNative()->uuid.uuid128, 16);
			p+=16;
		}
	} else {
		m_advData.service_uuid_len = 0;
		ESP_LOGD(LOG_TAG, "- no services advertised");
	}


	// Set the configuration for advertising.
	m_advData.set_scan_rsp = false;
	esp_err_t errRc = ::esp_ble_gap_config_adv_data(&m_advData);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "<< esp_ble_gap_config_adv_data: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}

	m_advData.set_scan_rsp = true;
	errRc = ::esp_ble_gap_config_adv_data(&m_advData);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "<< esp_ble_gap_config_adv_data (Scan response): rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}

	// If we had services to advertise then we previously allocated some storage for them.
	// Here we release that storage.
	if (m_advData.service_uuid_len > 0) {
		delete[] m_advData.p_service_uuid;
		m_advData.p_service_uuid = nullptr;
	}

	// Start advertising.
	errRc = ::esp_ble_gap_start_advertising(&m_advParams);
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "<< esp_ble_gap_start_advertising: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}
	ESP_LOGD(LOG_TAG, "<< start");
} // start


/**
 * @brief Stop advertising.
 * Stop advertising.
 * @return N/A.
 */
void BLEAdvertising::stop() {
	ESP_LOGD(LOG_TAG, ">> stop");
	esp_err_t errRc = ::esp_ble_gap_stop_advertising();
	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gap_stop_advertising: rc=%d %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}
	ESP_LOGD(LOG_TAG, "<< stop");
} // stop


#endif /* CONFIG_BT_ENABLED */
