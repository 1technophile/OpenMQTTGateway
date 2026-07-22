/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

   This file enables to set your parameters for the RTL_433SX1262 gateway

   Receives Fine Offset/Ecowitt/LaCrosse weather sensors (868 MHz GFSK,
   "Group A" protocol family) directly through an SX1262 radio (RadioLib),
   for boards such as the Heltec WiFi LoRa 32 V3/V4 where the rtl_433_ESP
   library cannot be used (it does not support the SX1262 chip).

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
#ifndef config_RTL_433SX1262_h
#define config_RTL_433SX1262_h

#include "TheengsCommon.h"

extern void setupRTL_433SX1262();
extern void RTL_433SX1262Loop();

/*-------------------PIN DEFINITIONS----------------------*/
// Heltec WiFi LoRa 32 V3/V4 onboard SX1262 (V4 is pin-compatible with V3)

#ifndef RTL433_SX1262_NSS
#  define RTL433_SX1262_NSS 8
#endif
#ifndef RTL433_SX1262_SCK
#  define RTL433_SX1262_SCK 9
#endif
#ifndef RTL433_SX1262_MOSI
#  define RTL433_SX1262_MOSI 10
#endif
#ifndef RTL433_SX1262_MISO
#  define RTL433_SX1262_MISO 11
#endif
#ifndef RTL433_SX1262_RST
#  define RTL433_SX1262_RST 12
#endif
#ifndef RTL433_SX1262_BUSY
#  define RTL433_SX1262_BUSY 13
#endif
#ifndef RTL433_SX1262_DIO1
#  define RTL433_SX1262_DIO1 14
#endif

/*-------------------RADIO PARAMETERS----------------------*/
// "Group A" Fine Offset/Ecowitt/LaCrosse GFSK parameters (868.35 MHz)

#ifndef RTL433_SX1262_FREQ_MHZ
#  define RTL433_SX1262_FREQ_MHZ 868.35f
#endif
#ifndef RTL433_SX1262_FREQ_DEV_KHZ
#  define RTL433_SX1262_FREQ_DEV_KHZ 60.0f
#endif
#ifndef RTL433_SX1262_BW_KHZ
#  define RTL433_SX1262_BW_KHZ 234.3f
#endif
#ifndef RTL433_SX1262_BITRATE_BPS
#  define RTL433_SX1262_BITRATE_BPS 17241
#endif
#ifndef RTL433_SX1262_SYNCWORD
#  define RTL433_SX1262_SYNCWORD 0x2DD4
#endif
#ifndef RTL433_SX1262_PKT_LEN
#  define RTL433_SX1262_PKT_LEN 32
#endif
#ifndef RTL433_SX1262_TX_POWER
#  define RTL433_SX1262_TX_POWER 10
#endif
#ifndef RTL433_SX1262_PREAMBLE_LEN
#  define RTL433_SX1262_PREAMBLE_LEN 16
#endif
#ifndef RTL433_SX1262_TCXO_VOLTAGE
#  define RTL433_SX1262_TCXO_VOLTAGE 1.6f
#endif

#endif
