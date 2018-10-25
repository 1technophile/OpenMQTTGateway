# Adafruit TSL2561 Light Sensor Driver  [![Build Status](https://travis-ci.org/adafruit/Adafruit_TSL2561.svg?branch=master)](https://travis-ci.org/adafruit/Adafruit_TSL2561)

This driver is for the Adafruit TSL2561 Breakout, and is based on Adafruit's Unified Sensor Library (Adafruit_Sensor).

<img src="https://cdn-shop.adafruit.com/970x728/439-00.jpg" height="300"/>


The driver supports manual or 'auto' gain. Adjusting the gain allows you to make the sensor more or less 'sensitive' to light (depending on if you are indoors or outdoors, for example):
```
tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
tsl.enableAutoGain(true);          /* Auto-gain ... switches automatically between 1x and 16x */
```

The driver also supports as automatic clipping detection, and will return '65536' lux when the sensor is saturated and data is unreliable. tsl.getEvent will return false in case of saturation and true in case of valid light data.

## About the TSL2561 ##

The TSL2561 is a 16-bit digital (I2C) light sensor, with adjustable gain and 'integration time'.  

Adjusting the 'integration time' essentially increases the resolution of the device, since the analog converter inside the chip has time to take more samples.  The integration time can be set as follows:
```
tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */
```

One of the big advantages of the TSL2561 is that it is capable of measuring both broadband (visible plus infrared) and infrared light thanks to two distinct sensing units on the device.  This is important in certain lighting environments to be able to read the light level reliably.

More information on the TSL2561 can be found in the datasheet: http://www.adafruit.com/datasheets/TSL2561.pdf

## What is the Adafruit Unified Sensor Library? ##

The Adafruit Unified Sensor Library (Adafruit_Sensor) provides a common interface and data type for any supported sensor.  It defines some basic information about the sensor (sensor limits, etc.), and returns standard SI units of a specific type and scale for each supported sensor type.

It provides a simple abstraction layer between your application and the actual sensor HW, allowing you to drop in any comparable sensor with only one or two lines of code to change in your project (essentially the constructor since the functions to read sensor data and get information about the sensor are defined in the base Adafruit_Sensor class).

This is imporant useful for two reasons:

1.) You can use the data right away because it's already converted to SI units that you understand and can compare, rather than meaningless values like 0..1023.

2.) Because SI units are standardised in the sensor library, you can also do quick sanity checks working with new sensors, or drop in any comparable sensor if you need better sensitivity or if a lower cost unit becomes available, etc. 

Light sensors will always report units in lux, gyroscopes will always report units in rad/s, etc. ... freeing you up to focus on the data, rather than digging through the datasheet to understand what the sensor's raw numbers really mean.

## About this Driver ##

Adafruit invests time and resources providing this open source code.  Please support Adafruit and open-source hardware by purchasing products from Adafruit!

Written by Kevin (KTOWN) Townsend for Adafruit Industries.
