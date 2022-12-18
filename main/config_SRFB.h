/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT
 
   This files enables to set your parameter for the radiofrequency gateway on sonoff rf bridge (ZgatewaySRFB)
   This implementation is based on Xose Pérez work ESPURNA (https://bitbucket.org/xoseperez/espurna)

    Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>
  
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
#ifndef config_SRFB_h
#define config_SRFB_h

extern void setupSRFB();
extern bool SRFBtoMQTT();
extern void MQTTtoSRFB(char* topicOri, char* datacallback);
extern void MQTTtoSRFB(char* topicOri, JsonObject& RFdata);
/*-------------------RF topics & parameters----------------------*/
//433Mhz MQTT Subjects and keys
#define subjectMQTTtoSRFB      "/commands/MQTTtoSRFB"
#define subjectMQTTtoSRFBRaw   "/commands/MQTTtoSRFB/Raw"
#define subjectSRFBtoMQTT      "/SRFBtoMQTT"
#define subjectGTWSRFBtoMQTT   "/SRFBtoMQTT"
#define SRFBRptKey             "RPT_"
#define SRFBmaxipulselengthKey "Thigh_"
#define SRFBminipulselengthKey "Tlow_"
#define SRFBsyncKey            "Tsyn_"

#define repeatSRFBwMQTT false // do we repeat a received signal by using MQTT with Sonoff RF Bridge

// -----------------------------------------------------------------------------
// RFBRIDGE
// -----------------------------------------------------------------------------

#define RF_SEND_TIMES    4 // How many times to send the message
#define RF_SEND_DELAY    500 // Interval between sending in ms
#define RF_RECEIVE_DELAY 500 // Interval between receiving in ms (avoid debouncing)

// -----------------------------------------------------------------------------
// DEFINITIONS
// -----------------------------------------------------------------------------

#define RF_MESSAGE_SIZE 9
#define RF_CODE_START   0xAA
#define RF_CODE_ACK     0xA0
#define RF_CODE_RFIN    0xA4
#define RF_CODE_RFOUT   0xA5
#define RF_CODE_STOP    0x55

#endif
