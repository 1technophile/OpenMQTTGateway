/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   This files enables you to set your parameters for the PWMLED actuator

    Copyright: (c)

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

/*-------------------PWMLED topics & parameters----------------------*/

#if defined(ZactuatorPWMLED) && !ESP32
# error The PWM LED actuator only works on ESP32 currently. It should be possible to get working on devices that support a 16-bit PWM duty cycle though.
#endif

// PWMLED MQTT Subjects
#define subjectMQTTtoPWMLED "/commands/MQTTtoPWMLED"
#define subjectMQTTtoPWMLEDsetleds subjectMQTTtoPWMLED "/set"             //set LEDs with JSON struct {"r":0-255,"g":0-255,"b":0-255,"w0":0-255,"w0":0-255,"fade":<fade time in seconds>}
#define subjectMQTTtoPWMLEDcalibrateleds subjectMQTTtoPWMLED "/calibrate" //set LED calibration JSON struct {"gamma-r":0.5-4.0,"min-r":0-255,"max-r":0-255 etc. }

// Edit the following lines for your leds arrangement.
#define PWMLED_R_PIN 25
#define PWMLED_G_PIN 33
#define PWMLED_B_PIN 32
#define PWMLED_W0_PIN 23
#define PWMLED_W1_PIN 22
