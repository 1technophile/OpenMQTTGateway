/*BME280 Arduino and Teensy example
Marshall Taylor @ SparkFun Electronics
Oct 19, 2015
https://github.com/sparkfun/SparkFun_BME280_Arduino_Library

This sketch displays the pressure in pascals and temperature
in fahrenheit.

Resources:
Uses Wire.h for I2C operation
Uses SPI.h for SPI operation

Hardware connections:
BME280 -> Arduino
GND -> GND
3.3 -> 3.3
SDA -> A4
SCL -> A5

LCD:
 * LCD RS pin to digital pin 7
 * LCD Enable pin to digital 6
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

Development environment specifics:
Arduino IDE 1.6.4
Teensy loader 1.23

This code is released under the [MIT License](http://opensource.org/licenses/MIT).
Please review the LICENSE.md file included with this example. If you have any questions 
or concerns with licensing, please contact techsupport@sparkfun.com.
Distributed as-is; no warranty is given.
******************************************************************************/


// Load the LiquidCrystal library, which will give us
// commands to interface to the LCD:

#include <LiquidCrystal.h>

// Initialize the library with the pins we're using.
// (Note that you can use different pins if needed.)
// See http://arduino.cc/en/Reference/LiquidCrystal
// for more information:


#include <stdint.h>
#include "SparkFunBME280.h"
//Library allows either I2C or SPI, so include both.
#include "Wire.h"
#include "SPI.h"

//Global sensor object
BME280 mySensor;

LiquidCrystal lcd(7,6,5,4,3,2);

void setup()
{
	lcd.begin(16, 2);
    lcd.clear();

	//***Driver settings********************************//
	//commInterface can be I2C_MODE or SPI_MODE
	//specify chipSelectPin using arduino pin names
	//specify I2C address.  Can be 0x77(default) or 0x76
	
	//For I2C, enable the following and disable the SPI section
	mySensor.settings.commInterface = I2C_MODE;
	mySensor.settings.I2CAddress = 0x77;
	
	//For SPI enable the following and dissable the I2C section
	//mySensor.settings.commInterface = SPI_MODE;
	//mySensor.settings.chipSelectPin = 10;


	//***Operation settings*****************************//
	
	//renMode can be:
	//  0, Sleep mode
	//  1 or 2, Forced mode
	//  3, Normal mode
	mySensor.settings.runMode = 3; //Forced mode
	
	//tStandby can be:
	//  0, 0.5ms
	//  1, 62.5ms
	//  2, 125ms
	//  3, 250ms
	//  4, 500ms
	//  5, 1000ms
	//  6, 10ms
	//  7, 20ms
	mySensor.settings.tStandby = 0;
	
	//filter can be off or number of FIR coefficients to use:
	//  0, filter off
	//  1, coefficients = 2
	//  2, coefficients = 4
	//  3, coefficients = 8
	//  4, coefficients = 16
	mySensor.settings.filter = 0;
	
	//tempOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.tempOverSample = 1;

	//pressOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
    mySensor.settings.pressOverSample = 1;
	
	//humidOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.humidOverSample = 1;
	
	Serial.begin(57600);
	Serial.print("Program Started\n");
	Serial.print("Starting BME280... result of .begin(): 0x");
	
	//Calling .begin() causes the settings to be loaded
	Serial.println(mySensor.begin(), HEX);
	delay(10);  //Use delay if reading directly after .begin is called.  BME280 requires 2ms to start up.

	Serial.print("Displaying ID, reset and ctrl regs\n");
	
	Serial.print("ID(0xD0): 0x");
	Serial.println(mySensor.readRegister(BME280_CHIP_ID_REG), HEX);
	Serial.print("Reset register(0xE0): 0x");
	Serial.println(mySensor.readRegister(BME280_RST_REG), HEX);
	Serial.print("ctrl_meas(0xF4): 0x");
	Serial.println(mySensor.readRegister(BME280_CTRL_MEAS_REG), HEX);
	Serial.print("ctrl_hum(0xF2): 0x");
	Serial.println(mySensor.readRegister(BME280_CTRL_HUMIDITY_REG), HEX);

	Serial.print("\n\n");

	Serial.print("Displaying all regs\n");
	uint8_t memCounter = 0x80;
	uint8_t tempReadData;
	for(int rowi = 8; rowi < 16; rowi++ )
	{
		Serial.print("0x");
		Serial.print(rowi, HEX);
		Serial.print("0:");
		for(int coli = 0; coli < 16; coli++ )
		{
			tempReadData = mySensor.readRegister(memCounter);
			Serial.print((tempReadData >> 4) & 0x0F, HEX);//Print first hex nibble
			Serial.print(tempReadData & 0x0F, HEX);//Print second hex nibble
			Serial.print(" ");
			memCounter++;
		}
		Serial.print("\n");
	}
	
	
	Serial.print("\n\n");
	
	Serial.print("Displaying concatenated calibration words\n");
	Serial.print("dig_T1, uint16: ");
	Serial.println(mySensor.calibration.dig_T1);
	Serial.print("dig_T2, int16: ");
	Serial.println(mySensor.calibration.dig_T2);
	Serial.print("dig_T3, int16: ");
	Serial.println(mySensor.calibration.dig_T3);
	
	Serial.print("dig_P1, uint16: ");
	Serial.println(mySensor.calibration.dig_P1);
	Serial.print("dig_P2, int16: ");
	Serial.println(mySensor.calibration.dig_P2);
	Serial.print("dig_P3, int16: ");
	Serial.println(mySensor.calibration.dig_P3);
	Serial.print("dig_P4, int16: ");
	Serial.println(mySensor.calibration.dig_P4);
	Serial.print("dig_P5, int16: ");
	Serial.println(mySensor.calibration.dig_P5);
	Serial.print("dig_P6, int16: ");
	Serial.println(mySensor.calibration.dig_P6);
	Serial.print("dig_P7, int16: ");
	Serial.println(mySensor.calibration.dig_P7);
	Serial.print("dig_P8, int16: ");
	Serial.println(mySensor.calibration.dig_P8);
	Serial.print("dig_P9, int16: ");
	Serial.println(mySensor.calibration.dig_P9);
	
	Serial.print("dig_H1, uint8: ");
	Serial.println(mySensor.calibration.dig_H1);
	Serial.print("dig_H2, int16: ");
	Serial.println(mySensor.calibration.dig_H2);
	Serial.print("dig_H3, uint8: ");
	Serial.println(mySensor.calibration.dig_H3);
	Serial.print("dig_H4, int16: ");
	Serial.println(mySensor.calibration.dig_H4);
	Serial.print("dig_H5, int16: ");
	Serial.println(mySensor.calibration.dig_H5);
	Serial.print("dig_H6, uint8: ");
	Serial.println(mySensor.calibration.dig_H6);
		
	Serial.println();
}

void loop()
{
	//Get the local temperature!  Do this for calibration
	mySensor.readTempC();
	
    lcd.setCursor(0,0);
	lcd.print("Press: ");
	lcd.print((uint32_t)mySensor.readFloatPressure());
	lcd.print(" Pa");

    lcd.setCursor(0,1);
	lcd.print("Humid: ");
	lcd.print((uint8_t)mySensor.readFloatHumidity());
	lcd.print(" %");
	
	
	delay(1000);

}
