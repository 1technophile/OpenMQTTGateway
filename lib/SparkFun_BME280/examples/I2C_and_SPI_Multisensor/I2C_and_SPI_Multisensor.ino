/******************************************************************************
I2C_and_SPI_Multisensor.ino
BME280 Arduino and Teensy example
Marshall Taylor @ SparkFun Electronics
May 20, 2015
https://github.com/sparkfun/SparkFun_BME280_Arduino_Library

This sketch configures two BME280 breakouts.  One is configured as I2C and
the other as SPI.  Data from both are displayed.

Sensor A is I2C and connected through A4 and A5 (SDA/SCL)
Sensor B is SPI and connected through pins 10, 11, 12, and 13 through a
level shifter, with pin 10 being used for chip select.

Resources:
Uses Wire.h for I2C operation
Uses SPI.h for SPI operation

Development environment specifics:
Arduino IDE 1.6.4
Teensy loader 1.23

This code is released under the [MIT License](http://opensource.org/licenses/MIT).
Please review the LICENSE.md file included with this example. If you have any questions 
or concerns with licensing, please contact techsupport@sparkfun.com.
Distributed as-is; no warranty is given.
******************************************************************************/

#include <stdint.h>
#include "SparkFunBME280.h"

#include "Wire.h"
#include "SPI.h"

//Global sensor object
BME280 mySensorA;
BME280 mySensorB;

void setup()
{
	//***Set up sensor 'A'******************************//
	//commInterface can be I2C_MODE or SPI_MODE
	mySensorA.settings.commInterface = I2C_MODE;
	mySensorA.settings.I2CAddress = 0x77;
	mySensorA.settings.runMode = 3; //  3, Normal mode
	mySensorA.settings.tStandby = 0; //  0, 0.5ms
	mySensorA.settings.filter = 0; //  0, filter off
	//tempOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensorA.settings.tempOverSample = 1;
	//pressOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
    mySensorA.settings.pressOverSample = 1;
	//humidOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensorA.settings.humidOverSample = 1;

	
	
	
	//***Set up sensor 'B'******************************//
	//commInterface can be I2C_MODE or SPI_MODE
	//specify chipSelectPin using arduino pin names
	//specify I2C address.  Can be 0x77(default) or 0x76
	mySensorB.settings.commInterface = SPI_MODE;
	mySensorB.settings.chipSelectPin = 10;
	mySensorB.settings.runMode = 3; //  3, Normal mode
	mySensorB.settings.tStandby = 0; //  0, 0.5ms
	mySensorB.settings.filter = 0; //  0, filter off
	//tempOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensorB.settings.tempOverSample = 1;
	//pressOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
    mySensorB.settings.pressOverSample = 1;
	//humidOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensorB.settings.humidOverSample = 1;
	
	
	
	//***Initialize the things**************************//
	Serial.begin(57600);
	Serial.print("Program Started\n");
	Serial.println("Starting BME280s... result of .begin():");
	delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
	//Calling .begin() causes the settings to be loaded
	Serial.print("Sensor A: 0x");
	Serial.println(mySensorA.begin(), HEX);
	Serial.print("Sensor B: 0x");
	Serial.println(mySensorB.begin(), HEX);


}

void loop()
{
	//Start with temperature, as that data is needed for accurate compensation.
	//Reading the temperature updates the compensators of the other functions
	//in the background.
	Serial.print("Temperature: ");
	Serial.print(mySensorA.readTempC(), 2);
	Serial.print(", ");
	Serial.print(mySensorB.readTempC(), 2);
	Serial.println(" degrees C");

	Serial.print("Temperature: ");
	Serial.print(mySensorA.readTempF(), 2);
	Serial.print(", ");
	Serial.print(mySensorB.readTempF(), 2);
	Serial.println(" degrees F");

	Serial.print("Pressure: ");
	Serial.print(mySensorA.readFloatPressure(), 2);
	Serial.print(", ");
	Serial.print(mySensorB.readFloatPressure(), 2);
	Serial.println(" Pa");

	Serial.print("Altitude: ");
	Serial.print(mySensorA.readFloatAltitudeMeters(), 2);
	Serial.print(", ");
	Serial.print(mySensorB.readFloatAltitudeMeters(), 2);
	Serial.println("m");

	Serial.print("Altitude: ");
	Serial.print(mySensorA.readFloatAltitudeFeet(), 2);
	Serial.print(", ");
	Serial.print(mySensorB.readFloatAltitudeFeet(), 2);
	Serial.println("ft");	

	Serial.print("%RH: ");
	Serial.print(mySensorA.readFloatHumidity(), 2);
	Serial.print(", ");
	Serial.print(mySensorB.readFloatHumidity(), 2);
	Serial.println(" %");
	
	Serial.println();
	
	delay(1000);

}
