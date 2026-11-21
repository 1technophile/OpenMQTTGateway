/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between Dimplex Connex electric heaters (2.4 GHz nRF24L01
   link from the Connex wall controller) and an MQTT broker.

   This file holds the parameters for the Connex gateway (ZgatewayCONNEX).

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
#ifndef config_CONNEX_h
#define config_CONNEX_h

#include "TheengsCommon.h"

extern void setupCONNEX();
extern void CONNEXtoX();
extern void XtoCONNEX(const char* topicOri, JsonObject& CONNEXdata);
extern void stateCONNEXMeasures();
extern void launchCONNEXDiscovery();

/*----------------------CONNEX topics -------------------------*/
// Gateway state, settings and the last setpoint of each zone (retained)
#define subjectGTWCONNEXtoMQTT "/CONNEXtoMQTT/gateway"
// Setpoint commands: {"zone":1,"temperature":20.5}
#define subjectMQTTtoCONNEX "/commands/MQTTtoCONNEX"
// Configuration: {"learn":true} {"pair":1} {"controllerid":"D677"} {"burst":200,"interval":3,"acks":10} {"power":"max"} {"save":true} {"erase":true} {"init":true}
#define subjectMQTTtoCONNEXset "/commands/MQTTtoCONNEXset"

/*----------------------CONNEX radio protocol -------------------------*/
// Connex wall controller link (see OpenNRF24L01 Docs/connex.md)
#define CONNEX_CHANNEL      40
#define CONNEX_ADDRESS      0x5049382716ULL // over the air: 50 49 38 27 16
#define CONNEX_PAYLOAD_SIZE 10
#define CONNEX_ZONES        4

// Setpoint is tenths of a degree, sent big-endian in frame bytes 5 (high) and 6 (low),
// e.g. 30.0 C = 01 2C. The wall controller never went above 25.5 C so it always sent
// byte 5 = 0x00; heaters were tested to follow 0 C, 26 C and 30 C.
#ifndef CONNEX_MIN_TEMP
#  define CONNEX_MIN_TEMP 0.0f // heaters accept 0 C (tested)
#endif
#ifndef CONNEX_MAX_TEMP
#  define CONNEX_MAX_TEMP 30.0f // the heater's own controls also stop at 30 C
#endif
#define CONNEX_TEMP_STEP 0.5f

// Each command is sent as a burst of identical frames (default 200 x 3 ms)
#ifndef CONNEX_BURST_COUNT
#  define CONNEX_BURST_COUNT 200
#endif
#ifndef CONNEX_BURST_INTERVAL_MS
#  define CONNEX_BURST_INTERVAL_MS 3
#endif
// End the burst early once this many frames have been acked (0 = always send the full burst).
// A single frame has been enough in testing; acks only prove a heater's radio heard the frame.
#ifndef CONNEX_BURST_ACKS
#  define CONNEX_BURST_ACKS 10
#endif
// nRF24L01 transmit power: 0 = min, 1 = low, 2 = high, 3 = max (RF24 PA levels)
#ifndef CONNEX_PA_LEVEL
#  define CONNEX_PA_LEVEL 1
#endif
// Learn mode listens this long for a wall controller frame
#define CONNEX_LEARN_TIMEOUT_MS 120000UL
// The wall controller repeats each frame; identical frames within this window are ignored
#define CONNEX_RX_DEDUP_MS 2000UL

/*-------------------PIN DEFINITIONS----------------------*/
// Default wiring from OpenNRF24L01 Docs/connex.md (IRQ is not used by this module)
#ifndef CONNEX_CE_GPIO
#  define CONNEX_CE_GPIO 25
#endif
#ifndef CONNEX_CSN_GPIO
#  define CONNEX_CSN_GPIO 5
#endif
#ifndef CONNEX_SCK_GPIO
#  define CONNEX_SCK_GPIO 18
#endif
#ifndef CONNEX_MISO_GPIO
#  define CONNEX_MISO_GPIO 19
#endif
#ifndef CONNEX_MOSI_GPIO
#  define CONNEX_MOSI_GPIO 23
#endif

#endif
