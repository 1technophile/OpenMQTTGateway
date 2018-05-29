/******************************************************************************
I2C_DeltaAltitude.ino
BME280 Arduino and Teensy example
Marshall Taylor @ SparkFun Electronics
May 20, 2015
https://github.com/sparkfun/SparkFun_BME280_Arduino_Library

This sketch configures a BME280 to measure height changes, with a button
to zero relative altitude.

Hardware connections:
BME280 -> Arduino
GND -> GND
3.3 -> 3.3
SDA -> A4
SCL -> A5

Button (Normally open) between A4 and A5.

To use, run the sketch then press and hold the button until the data on the
serial monitor shows zero.  Then, move the sensor to a new altitude and watch
how the output changes!

Note: For most accurate results keep the sensor orientation and temperature the
same between measurements.

Resources:
Uses Wire.h for I2C operation
Uses SPI.h for SPI operation
Included CircularBuffer class for averaging

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
#include "CircularBuffer.h"

#include "Wire.h"
#include "SPI.h"

//Global sensor object
BME280 mySensor;
uint8_t buttonGroundPin = A0;
uint8_t buttonSensePin = A2;

//This is a fixed-size buffer used for averaging data, a software LP filter
//Depending on the settings, the response can be quite slow.
CircularBuffer myBuffer(50);

float reference = 0;

void setup()
{
	//Configure the button IO
	pinMode( buttonSensePin, INPUT_PULLUP );
	pinMode( buttonGroundPin, OUTPUT );
	digitalWrite( buttonGroundPin, 0 );
	
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
	mySensor.settings.runMode = 3; //Normal mode
	
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
	mySensor.settings.filter = 4; //Lots of HW filter
	
	//tempOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.tempOverSample = 5;

	//pressOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
    mySensor.settings.pressOverSample = 5;
	
	//humidOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.humidOverSample = 1;
	
	Serial.begin(57600);
	Serial.print("Program Started\n");
	Serial.print("Starting BME280... result of .begin(): 0x");
	delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
	//Calling .begin() causes the settings to be loaded
	Serial.println(mySensor.begin(), HEX);

	Serial.println();

}

void loop()
{
	//Get the local temperature!  Do this for calibration
	mySensor.readTempC();
	
	//Check button.  No debounce, so hold for operation
	if( digitalRead( buttonSensePin ) == 0 )
	{
		//Set reference altitude.
		reference = mySensor.readFloatAltitudeFeet();
	}
	
	float lastAlt = mySensor.readFloatAltitudeFeet() - reference;
	myBuffer.pushElement(lastAlt);
	float tempAlt = myBuffer.averageLast(15); //Do an average of the latest samples
	
	Serial.println();
	Serial.print("Temperature: ");
	Serial.print(mySensor.readTempC(), 2);
	Serial.println(" degrees C");
	Serial.print("Last sample: ");
	if(lastAlt >= 0)
	{
		Serial.print(" ");
	}
	Serial.print(lastAlt, 2);
	Serial.println(" ft");	
	Serial.print("Last 15 samples averaged: ");
	if(tempAlt >= 0)
	{
		Serial.print(" ");
	}
	Serial.print(tempAlt);
	Serial.println(" ft");
	delay(200);

}
