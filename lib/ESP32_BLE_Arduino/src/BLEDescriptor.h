/*
 * BLEDescriptor.h
 *
 *  Created on: Jun 22, 2017
 *      Author: kolban
 */

#ifndef COMPONENTS_CPP_UTILS_BLEDESCRIPTOR_H_
#define COMPONENTS_CPP_UTILS_BLEDESCRIPTOR_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)
#include <string>
#include "BLEUUID.h"
#include "BLECharacteristic.h"
#include <esp_gatts_api.h>
#include "FreeRTOS.h"

class BLEService;
class BLECharacteristic;
class BLEDescriptorCallbacks;

/**
 * @brief A model of a %BLE descriptor.
 */
class BLEDescriptor {
public:
	BLEDescriptor(const char* uuid);
	BLEDescriptor(BLEUUID uuid);
	virtual ~BLEDescriptor();

	uint16_t getHandle();
	size_t   getLength();
	BLEUUID  getUUID();
	uint8_t* getValue();
	void handleGATTServerEvent(
			esp_gatts_cb_event_t      event,
			esp_gatt_if_t             gatts_if,
			esp_ble_gatts_cb_param_t* param);
	void setCallbacks(BLEDescriptorCallbacks* pCallbacks);
	void setValue(uint8_t* data, size_t size);
	void setValue(std::string value);
	std::string toString();

private:
	friend class BLEDescriptorMap;
	friend class BLECharacteristic;
	BLEUUID              m_bleUUID;
	esp_attr_value_t     m_value;
	uint16_t             m_handle;
	BLECharacteristic*   m_pCharacteristic;
	BLEDescriptorCallbacks* m_pCallback;
	void executeCreate(BLECharacteristic* pCharacteristic);
	void setHandle(uint16_t handle);
	FreeRTOS::Semaphore m_semaphoreCreateEvt = FreeRTOS::Semaphore("CreateEvt");
};

/**
 * @brief Callbacks that can be associated with a %BLE descriptors to inform of events.
 *
 * When a server application creates a %BLE descriptor, we may wish to be informed when there is either
 * a read or write request to the descriptors value.  An application can register a
 * sub-classed instance of this class and will be notified when such an event happens.
 */
class BLEDescriptorCallbacks {
public:
	virtual ~BLEDescriptorCallbacks();
	virtual void onRead(BLEDescriptor* pDescriptor);
	virtual void onWrite(BLEDescriptor* pDescriptor);
};
#endif /* CONFIG_BT_ENABLED */
#endif /* COMPONENTS_CPP_UTILS_BLEDESCRIPTOR_H_ */
