/*
INA226.h - Header file for the Bi-directional Current/Power Monitor Arduino Library.

Version: 1.0.0
(c) 2014 Korneliusz Jarzebski
www.jarzebski.pl

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INA226_h
#define INA226_h

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define INA226_ADDRESS              (0x40)

#define INA226_REG_CONFIG           (0x00)
#define INA226_REG_SHUNTVOLTAGE     (0x01)
#define INA226_REG_BUSVOLTAGE       (0x02)
#define INA226_REG_POWER            (0x03)
#define INA226_REG_CURRENT          (0x04)
#define INA226_REG_CALIBRATION      (0x05)
#define INA226_REG_MASKENABLE       (0x06)
#define INA226_REG_ALERTLIMIT       (0x07)

#define INA226_BIT_SOL              (0x8000)
#define INA226_BIT_SUL              (0x4000)
#define INA226_BIT_BOL              (0x2000)
#define INA226_BIT_BUL              (0x1000)
#define INA226_BIT_POL              (0x0800)
#define INA226_BIT_CNVR             (0x0400)
#define INA226_BIT_AFF              (0x0010)
#define INA226_BIT_CVRF             (0x0008)
#define INA226_BIT_OVF              (0x0004)
#define INA226_BIT_APOL             (0x0002)
#define INA226_BIT_LEN              (0x0001)

typedef enum
{
    INA226_AVERAGES_1             = 0b000,
    INA226_AVERAGES_4             = 0b001,
    INA226_AVERAGES_16            = 0b010,
    INA226_AVERAGES_64            = 0b011,
    INA226_AVERAGES_128           = 0b100,
    INA226_AVERAGES_256           = 0b101,
    INA226_AVERAGES_512           = 0b110,
    INA226_AVERAGES_1024          = 0b111
} ina226_averages_t;

typedef enum
{
    INA226_BUS_CONV_TIME_140US    = 0b000,
    INA226_BUS_CONV_TIME_204US    = 0b001,
    INA226_BUS_CONV_TIME_332US    = 0b010,
    INA226_BUS_CONV_TIME_588US    = 0b011,
    INA226_BUS_CONV_TIME_1100US   = 0b100,
    INA226_BUS_CONV_TIME_2116US   = 0b101,
    INA226_BUS_CONV_TIME_4156US   = 0b110,
    INA226_BUS_CONV_TIME_8244US   = 0b111
} ina226_busConvTime_t;

typedef enum
{
    INA226_SHUNT_CONV_TIME_140US   = 0b000,
    INA226_SHUNT_CONV_TIME_204US   = 0b001,
    INA226_SHUNT_CONV_TIME_332US   = 0b010,
    INA226_SHUNT_CONV_TIME_588US   = 0b011,
    INA226_SHUNT_CONV_TIME_1100US  = 0b100,
    INA226_SHUNT_CONV_TIME_2116US  = 0b101,
    INA226_SHUNT_CONV_TIME_4156US  = 0b110,
    INA226_SHUNT_CONV_TIME_8244US  = 0b111
} ina226_shuntConvTime_t;

typedef enum
{
    INA226_MODE_POWER_DOWN      = 0b000,
    INA226_MODE_SHUNT_TRIG      = 0b001,
    INA226_MODE_BUS_TRIG        = 0b010,
    INA226_MODE_SHUNT_BUS_TRIG  = 0b011,
    INA226_MODE_ADC_OFF         = 0b100,
    INA226_MODE_SHUNT_CONT      = 0b101,
    INA226_MODE_BUS_CONT        = 0b110,
    INA226_MODE_SHUNT_BUS_CONT  = 0b111,
} ina226_mode_t;

class INA226
{
    public:

	bool begin(uint8_t address = INA226_ADDRESS);
	bool configure(ina226_averages_t avg = INA226_AVERAGES_1, ina226_busConvTime_t busConvTime = INA226_BUS_CONV_TIME_1100US, ina226_shuntConvTime_t shuntConvTime = INA226_SHUNT_CONV_TIME_1100US, ina226_mode_t mode = INA226_MODE_SHUNT_BUS_CONT);
	bool calibrate(float rShuntValue = 0.1, float iMaxExcepted = 2);

	ina226_averages_t getAverages(void);
	ina226_busConvTime_t getBusConversionTime(void);
	ina226_shuntConvTime_t getShuntConversionTime(void);
	ina226_mode_t getMode(void);

	void enableShuntOverLimitAlert(void);
	void enableShuntUnderLimitAlert(void);
	void enableBusOvertLimitAlert(void);
	void enableBusUnderLimitAlert(void);
	void enableOverPowerLimitAlert(void);
	void enableConversionReadyAlert(void);

	void setBusVoltageLimit(float voltage);
	void setShuntVoltageLimit(float voltage);
	void setPowerLimit(float watts);

	void setAlertInvertedPolarity(bool inverted);
	void setAlertLatch(bool latch);

	bool isMathOverflow(void);
	bool isAlert(void);

	float readShuntCurrent(void);
	float readShuntVoltage(void);
	float readBusPower(void);
	float readBusVoltage(void);

	float getMaxPossibleCurrent(void);
	float getMaxCurrent(void);
	float getMaxShuntVoltage(void);
	float getMaxPower(void);

    private:

	int8_t inaAddress;
	float currentLSB, powerLSB;
	float vShuntMax, vBusMax, rShunt;

	void setMaskEnable(uint16_t mask);
	uint16_t getMaskEnable(void);

	void writeRegister16(uint8_t reg, uint16_t val);
	int16_t readRegister16(uint8_t reg);
};

#endif