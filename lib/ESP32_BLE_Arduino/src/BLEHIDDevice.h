/*
 * BLEHIDDevice.h
 *
 *  Created on: Jan 03, 2018
 *      Author: chegewara
 */

#ifndef _BLEHIDDEVICE_H_
#define _BLEHIDDEVICE_H_

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BLECharacteristic.h"
#include "BLEService.h"
#include "BLEDescriptor.h"
#include "BLE2902.h"
#include "HIDTypes.h"

#define GENERIC_HID		960
#define HID_KEYBOARD	961
#define HID_MOUSE		962
#define HID_JOYSTICK	963
#define HID_GAMEPAD		964
#define HID_TABLET		965
#define HID_CARD_READER	966
#define HID_DIGITAL_PEN	967
#define HID_BARCODE		968

class BLEHIDDevice {
public:
	BLEHIDDevice(BLEServer*);
	virtual ~BLEHIDDevice();

	void reportMap(uint8_t* map, uint16_t);
	void startServices();

	BLEService* deviceInfo();
	BLEService* hidService();
	BLEService* batteryService();

	BLECharacteristic* 	manufacturer();
	void 	manufacturer(std::string name);
	//BLECharacteristic* 	pnp();
	void	pnp(uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version);
	//BLECharacteristic*	hidInfo();
	void	hidInfo(uint8_t country, uint8_t flags);
	//BLECharacteristic* 	batteryLevel();
	void 	setBatteryLevel(uint8_t level);


	//BLECharacteristic* 	reportMap();
	BLECharacteristic* 	hidControl();
	BLECharacteristic* 	inputReport(uint8_t reportID);
	BLECharacteristic* 	outputReport(uint8_t reportID);
	BLECharacteristic* 	featureReport(uint8_t reportID);
	BLECharacteristic* 	protocolMode();
	BLECharacteristic* 	bootInput();
	BLECharacteristic* 	bootOutput();

private:
	BLEService*			m_deviceInfoService;			//0x180a
	BLEService*			m_hidService;					//0x1812
	BLEService*			m_batteryService = 0;			//0x180f

	BLECharacteristic* 	m_manufacturerCharacteristic;	//0x2a29
	BLECharacteristic* 	m_pnpCharacteristic;			//0x2a50
	BLECharacteristic* 	m_hidInfoCharacteristic;		//0x2a4a
	BLECharacteristic* 	m_reportMapCharacteristic;		//0x2a4b
	BLECharacteristic* 	m_hidControlCharacteristic;		//0x2a4c
	BLECharacteristic* 	m_protocolModeCharacteristic;	//0x2a4e
	BLECharacteristic*	m_batteryLevelCharacteristic;	//0x2a19
};
#endif // CONFIG_BT_ENABLED
#endif /* _BLEHIDDEVICE_H_ */
