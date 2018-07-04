/*
 * BLEScan.cpp
 *
 *  Created on: Jul 1, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)


#include <esp_log.h>
#include <esp_err.h>

#include <map>

#include "BLEAdvertisedDevice.h"
#include "BLEScan.h"
#include "BLEUtils.h"
#include "GeneralUtils.h"
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

static const char* LOG_TAG = "BLEScan";


/**
 * Constructor
 */
BLEScan::BLEScan() {
	m_scan_params.scan_type          = BLE_SCAN_TYPE_PASSIVE; // Default is a passive scan.
	m_scan_params.own_addr_type      = BLE_ADDR_TYPE_PUBLIC;
	m_scan_params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
	m_pAdvertisedDeviceCallbacks     = nullptr;
	m_stopped                        = true;
	m_wantDuplicates                 = false;
	setInterval(100);
	setWindow(100);
} // BLEScan


/**
 * @brief Handle GAP events related to scans.
 * @param [in] event The event type for this event.
 * @param [in] param Parameter data for this event.
 */
void BLEScan::handleGAPEvent(
	esp_gap_ble_cb_event_t  event,
	esp_ble_gap_cb_param_t* param) {

	switch(event) {

	// ESP_GAP_BLE_SCAN_RESULT_EVT
	// ---------------------------
	// scan_rst:
	// esp_gap_search_evt_t search_evt
	// esp_bd_addr_t bda
	// esp_bt_dev_type_t dev_type
	// esp_ble_addr_type_t ble_addr_type
	// esp_ble_evt_type_t ble_evt_type
	// int rssi
	// uint8_t ble_adv[ESP_BLE_ADV_DATA_LEN_MAX]
	// int flag
	// int num_resps
	// uint8_t adv_data_len
	// uint8_t scan_rsp_len
		case ESP_GAP_BLE_SCAN_RESULT_EVT: {

			switch(param->scan_rst.search_evt) {
				//
				// ESP_GAP_SEARCH_INQ_CMPL_EVT
				//
				// Event that indicates that the duration allowed for the search has completed or that we have been
				// asked to stop.
				case ESP_GAP_SEARCH_INQ_CMPL_EVT: {
					m_stopped = true;
					m_semaphoreScanEnd.give();
					break;
				} // ESP_GAP_SEARCH_INQ_CMPL_EVT

				//
				// ESP_GAP_SEARCH_INQ_RES_EVT
				//
				// Result that has arrived back from a Scan inquiry.
				case ESP_GAP_SEARCH_INQ_RES_EVT: {
					if (m_stopped) { // If we are not scanning, nothing to do with the extra results.
						break;
					}

// Examine our list of previously scanned addresses and, if we found this one already,
// ignore it.
					BLEAddress advertisedAddress(param->scan_rst.bda);
					bool found = false;

					for (int i=0; i<m_scanResults.getCount(); i++) {
						if (m_scanResults.getDevice(i).getAddress().equals(advertisedAddress)) {
							found = true;
							break;
						}
					}
					if (found && !m_wantDuplicates) {  // If we found a previous entry AND we don't want duplicates, then we are done.
						ESP_LOGD(LOG_TAG, "Ignoring %s, already seen it.", advertisedAddress.toString().c_str());
						break;
					}

					// We now construct a model of the advertised device that we have just found for the first
					// time.
					BLEAdvertisedDevice advertisedDevice;
					advertisedDevice.setAddress(advertisedAddress);
					advertisedDevice.setRSSI(param->scan_rst.rssi);
					advertisedDevice.setAdFlag(param->scan_rst.flag);
					advertisedDevice.parseAdvertisement((uint8_t*)param->scan_rst.ble_adv);
					advertisedDevice.setScan(this);

					if (m_pAdvertisedDeviceCallbacks) {
						m_pAdvertisedDeviceCallbacks->onResult(advertisedDevice);
					}

					if (!found) {   // If we have previously seen this device, don't record it again.
						m_scanResults.m_vectorAdvertisedDevices.push_back(advertisedDevice);
					}

					break;
				} // ESP_GAP_SEARCH_INQ_RES_EVT

				default: {
					break;
				}
			} // switch - search_evt


			break;
		} // ESP_GAP_BLE_SCAN_RESULT_EVT

		default: {
			break;
		} // default
	} // End switch
} // gapEventHandler


/**
 * @brief Should we perform an active or passive scan?
 * The default is a passive scan.  An active scan means that we will wish a scan response.
 * @param [in] active If true, we perform an active scan otherwise a passive scan.
 * @return N/A.
 */
void BLEScan::setActiveScan(bool active) {
	if (active) {
		m_scan_params.scan_type = BLE_SCAN_TYPE_ACTIVE;
	} else {
		m_scan_params.scan_type = BLE_SCAN_TYPE_PASSIVE;
	}
} // setActiveScan


/**
 * @brief Set the call backs to be invoked.
 * @param [in] pAdvertisedDeviceCallbacks Call backs to be invoked.
 * @param [in] wantDuplicates  True if we wish to be called back with duplicates.  Default is false.
 */
void BLEScan::setAdvertisedDeviceCallbacks(BLEAdvertisedDeviceCallbacks* pAdvertisedDeviceCallbacks, bool wantDuplicates) {
	m_wantDuplicates = wantDuplicates;
	m_pAdvertisedDeviceCallbacks = pAdvertisedDeviceCallbacks;
} // setAdvertisedDeviceCallbacks


/**
 * @brief Set the interval to scan.
 * @param [in] The interval in msecs.
 */
void BLEScan::setInterval(uint16_t intervalMSecs) {
	m_scan_params.scan_interval = intervalMSecs / 0.625;
} // setInterval


/**
 * @brief Set the window to actively scan.
 * @param [in] windowMSecs How long to actively scan.
 */
void BLEScan::setWindow(uint16_t windowMSecs) {
	m_scan_params.scan_window = windowMSecs / 0.625;
} // setWindow


/**
 * @brief Start scanning.
 * @param [in] duration The duration in seconds for which to scan.
 * @return N/A.
 */
BLEScanResults BLEScan::start(uint32_t duration) {
	ESP_LOGD(LOG_TAG, ">> start(duration=%d)", duration);

	m_semaphoreScanEnd.take(std::string("start"));

	m_scanResults.m_vectorAdvertisedDevices.clear();

	esp_err_t errRc = ::esp_ble_gap_set_scan_params(&m_scan_params);

	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gap_set_scan_params: err: %d, text: %s", errRc, GeneralUtils::errorToString(errRc));
		m_semaphoreScanEnd.give();
		return m_scanResults;
	}

	errRc = ::esp_ble_gap_start_scanning(duration);

	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gap_start_scanning: err: %d, text: %s", errRc, GeneralUtils::errorToString(errRc));
		m_semaphoreScanEnd.give();
		return m_scanResults;
	}

	m_stopped = false;

	m_semaphoreScanEnd.wait("start");   // Wait for the semaphore to release.

	ESP_LOGD(LOG_TAG, "<< start()");
	return m_scanResults;
} // start


/**
 * @brief Stop an in progress scan.
 * @return N/A.
 */
void BLEScan::stop() {
	ESP_LOGD(LOG_TAG, ">> stop()");

	esp_err_t errRc = ::esp_ble_gap_stop_scanning();

	m_stopped = true;

	if (errRc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "esp_ble_gap_stop_scanning: err: %d, text: %s", errRc, GeneralUtils::errorToString(errRc));
		return;
	}

	m_semaphoreScanEnd.give();

	ESP_LOGD(LOG_TAG, "<< stop()");
} // stop


/**
 * @brief Dump the scan results to the log.
 */
void BLEScanResults::dump() {
	ESP_LOGD(LOG_TAG, ">> Dump scan results:");
	for (int i=0; i<getCount(); i++) {
		ESP_LOGD(LOG_TAG, "- %s", getDevice(i).toString().c_str());
	}
} // dump


/**
 * @brief Return the count of devices found in the last scan.
 * @return The number of devices found in the last scan.
 */
int BLEScanResults::getCount() {
	return m_vectorAdvertisedDevices.size();
} // getCount


/**
 * @brief Return the specified device at the given index.
 * The index should be between 0 and getCount()-1.
 * @param [in] i The index of the device.
 * @return The device at the specified index.
 */
BLEAdvertisedDevice BLEScanResults::getDevice(uint32_t i) {
	return m_vectorAdvertisedDevices.at(i);
}


#endif /* CONFIG_BT_ENABLED */
