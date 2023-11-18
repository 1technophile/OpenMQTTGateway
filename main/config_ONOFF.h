/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the ON OFF actuator
  
    Copyright: (c)Florian ROBERT
  
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
#ifndef config_ONOFF_h
#define config_ONOFF_h

extern void setupONOFF();
extern void MQTTtoONOFF(char* topicOri, char* datacallback);
extern void MQTTtoONOFF(char* topicOri, JsonObject& RFdata);
extern void stateONOFFMeasures();
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT PARAMETERS BELOW----------------*/
#define subjectMQTTtoONOFF    "/commands/MQTTtoONOFF"
#define subjectGTWONOFFtoMQTT "/ONOFFtoMQTT"
#define subjectMQTTtoONOFFset "/commands/MQTTtoONOFF/config"

#define ONKey  "setON"
#define OFFKey "setOFF"

#ifndef USE_LAST_STATE_ON_RESTART
#  define USE_LAST_STATE_ON_RESTART true // Define if we use the last state upon a power restart
#endif
#ifndef ACTUATOR_ON
#  define ACTUATOR_ON LOW // LOW or HIGH, set to the output level of the GPIO pin to turn the actuator on.
#endif
//#  define ACTUATOR_ONOFF_DEFAULT !ACTUATOR_ON // ACTUATOR_ON or !ACTUATOR_ON, set to the state desired on reset.
#ifndef ACTUATOR_TRIGGER
#  define ACTUATOR_TRIGGER false // false or true, enable to control an actuator directly from the board switch (default behavior if true), or by button if ACTUATOR_BUTTON_TRIGGER_LEVEL is also defined
#endif
#ifndef ACTUATOR_BUTTON_TRIGGER_LEVEL
//#  define ACTUATOR_BUTTON_TRIGGER_LEVEL LOW // 0 or 1, set to the level which to detect a button press to change the actuator state.
#endif
#ifndef MAX_TEMP_ACTUATOR
//#  define MAX_TEMP_ACTUATOR         70.0 // Temperature that will trigger the relay to go OFF
#endif
#ifndef MAX_CURRENT_ACTUATOR
//#  define MAX_CURRENT_ACTUATOR         15.0 // Current that will trigger the relay to go OFF
#endif

#ifndef TimeBetweenReadingIntTemp
#  define TimeBetweenReadingIntTemp 5000 // Time interval between internal temp measurement to switch off the relay if MAX_TEMP_ACTUATOR is reached
#endif
#ifndef TimeBetweenReadingCurrent
#  define TimeBetweenReadingCurrent 1000 // Time interval between internal current measurement to switch off the relay if MAX_TEMP_ACTUATOR is reached
#endif
/*-------------------PIN DEFINITIONS----------------------*/
// default pin, if not set into the MQTT json
#ifndef ACTUATOR_ONOFF_GPIO
#  ifdef ESP8266
#    define ACTUATOR_ONOFF_GPIO 15 //12 for sonoff basic relay
#  elif ESP32
#    define ACTUATOR_ONOFF_GPIO 15
#  else
#    define ACTUATOR_ONOFF_GPIO 13
#  endif
#endif

#ifdef ESP32
/*----------------CONFIGURABLE PARAMETERS-----------------*/
struct ONOFFConfig_s {
  bool ONOFFState; // Recorded actuator state
  bool useLastStateOnStart; // Do we use the recorded actuator state on start
};

// Global struct to store live ONOFF configuration data
extern ONOFFConfig_s ONOFFConfig;
#endif

#endif
