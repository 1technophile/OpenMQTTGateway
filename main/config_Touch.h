/*  
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the esp32's touch buttons
  
    Copyright: (c) Florian Xhumari
    Uses work by Florian ROBERT
  
    This file is part of OpenMQTTGateway.
    
    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef config_Touch_h
#define config_Touch_h

extern void setupTouch();
extern void touchtoX();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectTouchtoMQTT "/touchToMQTT"

// Time between readings of the touch sensor. Don't make it too short, as
// reading one touch sensor takes 0.5 ms.
#if !defined(TOUCH_TIME_BETWEEN_READINGS) || (TOUCH_TIME_BETWEEN_READINGS) < 10
#  define TOUCH_TIME_BETWEEN_READINGS 30
#endif

// We take the average reading of the sensor, and consider as "touched" state
// any reading less than average * TOUCH_THRESHOLD / 100.
#if !defined(TOUCH_THRESHOLD)
#  define TOUCH_THRESHOLD 75
#endif

// Minimum duration of "touched" state to generate a touch event. By default,
// we'll wait 3 readings indicating "touched" in order to generate a touch event.
#if !defined(TOUCH_MIN_DURATION) || (TOUCH_MIN_DURATION < 10)
#  define TOUCH_MIN_DURATION 90
#endif

// After a touch was sensed, don't consider changes to "touched" status
// for this duration.
#if !defined(TOUCH_DEBOUNCE_TIME) || (TOUCH_DEBOUNCE_TIME < 100)
#  define TOUCH_DEBOUNCE_TIME 200
#endif

/****************************************
 *
 * One can define TOUCH_GPIO, or TOUCH_GPIO_0 through TOUCH_GPIO_9 to
 * include up to 10 sensors.
 * 
 * By default there's only one sensor at TOUCH_GPIO, default to
 * pin 4 (T0).
 * 
 ****************************************/

#if defined(TOUCH_GPIO) && !defined(TOUCH_GPIO_0)
#  define TOUCH_GPIO_0 (TOUCH_GPIO)
#endif

#if !defined(TOUCH_GPIO_0)
// T0 == GPIO 4
#  define TOUCH_GPIO_0 T0
#endif

#if defined(TOUCH_GPIO_1) && defined(TOUCH_GPIO_2) && defined(TOUCH_GPIO_3) && defined(TOUCH_GPIO_4) && defined(TOUCH_GPIO_5) && defined(TOUCH_GPIO_6) && defined(TOUCH_GPIO_7) && defined(TOUCH_GPIO_8) && defined(TOUCH_GPIO_9)
#  define TOUCH_SENSORS 10
#elif defined(TOUCH_GPIO_1) && defined(TOUCH_GPIO_2) && defined(TOUCH_GPIO_3) && defined(TOUCH_GPIO_4) && defined(TOUCH_GPIO_5) && defined(TOUCH_GPIO_6) && defined(TOUCH_GPIO_7) && defined(TOUCH_GPIO_8)
#  define TOUCH_SENSORS 9
#elif defined(TOUCH_GPIO_1) && defined(TOUCH_GPIO_2) && defined(TOUCH_GPIO_3) && defined(TOUCH_GPIO_4) && defined(TOUCH_GPIO_5) && defined(TOUCH_GPIO_6) && defined(TOUCH_GPIO_7)
#  define TOUCH_SENSORS 8
#elif defined(TOUCH_GPIO_1) && defined(TOUCH_GPIO_2) && defined(TOUCH_GPIO_3) && defined(TOUCH_GPIO_4) && defined(TOUCH_GPIO_5) && defined(TOUCH_GPIO_6)
#  define TOUCH_SENSORS 7
#elif defined(TOUCH_GPIO_1) && defined(TOUCH_GPIO_2) && defined(TOUCH_GPIO_3) && defined(TOUCH_GPIO_4) && defined(TOUCH_GPIO_5)
#  define TOUCH_SENSORS 6
#elif defined(TOUCH_GPIO_1) && defined(TOUCH_GPIO_2) && defined(TOUCH_GPIO_3) && defined(TOUCH_GPIO_4)
#  define TOUCH_SENSORS 5
#elif defined(TOUCH_GPIO_1) && defined(TOUCH_GPIO_2) && defined(TOUCH_GPIO_3)
#  define TOUCH_SENSORS 4
#elif defined(TOUCH_GPIO_1) && defined(TOUCH_GPIO_2)
#  define TOUCH_SENSORS 3
#elif defined(TOUCH_GPIO_1)
#  define TOUCH_SENSORS 2
#else
#  define TOUCH_SENSORS 1
#endif

#endif
