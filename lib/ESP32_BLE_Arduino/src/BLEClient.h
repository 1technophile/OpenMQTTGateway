/*
 * BLEDevice.h
 *
 *  Created on: Mar 22, 2017
 *      Author: kolban
 */

#ifndef MAIN_BLEDEVICE_H_
#define MAIN_BLEDEVICE_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include <esp_gattc_api.h>
#include <string.h>
#include <map>
#include <string>
#include "BLERemoteService.h"
#include "BLEService.h"
#include "BLEAddress.h"

class BLERemoteService;
class BLEClientCallbacks;

/**
 * @brief A model of a %BLE client.
 */
class BLEClient {
public:
	BLEClient();
	~BLEClient();
	bool                                       connect(BLEAddress address);
	void                                       disconnect();
	BLEAddress                                 getPeerAddress();
	int                                        getRssi();
	std::map<std::string, BLERemoteService*>*  getServices();
	BLERemoteService*                          getService(const char* uuid);
	BLERemoteService*                          getService(BLEUUID uuid);
	void                                       handleGAPEvent(
		                                            esp_gap_ble_cb_event_t  event,
                                                esp_ble_gap_cb_param_t* param);
	bool                                       isConnected();
	void                                       setClientCallbacks(BLEClientCallbacks *pClientCallbacks);
	std::string                                toString();

private:
	friend class BLEDevice;
	friend class BLERemoteService;
	friend class BLERemoteCharacteristic;
	friend class BLERemoteDescriptor;

	void                                       gattClientEventHandler(
		esp_gattc_cb_event_t event,
		esp_gatt_if_t gattc_if,
		esp_ble_gattc_cb_param_t* param);

	uint16_t                                   getConnId();
	esp_gatt_if_t                              getGattcIf();
	BLEAddress    m_peerAddress = BLEAddress((uint8_t*)"\0\0\0\0\0\0");
	uint16_t      m_conn_id;
//	int           m_deviceType;
	esp_gatt_if_t m_gattc_if;
	bool          m_isConnected;

	BLEClientCallbacks* m_pClientCallbacks;
	FreeRTOS::Semaphore m_semaphoreRegEvt        = FreeRTOS::Semaphore("RegEvt");
	FreeRTOS::Semaphore m_semaphoreOpenEvt       = FreeRTOS::Semaphore("OpenEvt");
	FreeRTOS::Semaphore m_semaphoreSearchCmplEvt = FreeRTOS::Semaphore("SearchCmplEvt");
	FreeRTOS::Semaphore m_semaphoreRssiCmplEvt   = FreeRTOS::Semaphore("RssiCmplEvt");
	std::map<std::string, BLERemoteService*> m_servicesMap;
	bool m_haveServices; // Have we previously obtain the set of services.
}; // class BLEDevice


/**
 * @brief Callbacks associated with a %BLE client.
 */
class BLEClientCallbacks {
public:
	virtual ~BLEClientCallbacks() {};
	virtual void onConnect(BLEClient *pClient) = 0;
};

#endif // CONFIG_BT_ENABLED
#endif /* MAIN_BLEDEVICE_H_ */
