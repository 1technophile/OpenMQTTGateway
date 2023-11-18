/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

   This files enables to set your parameter for the infrared gateway

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
#ifndef config_IR_h
#define config_IR_h

extern void setupIR();
extern void IRtoMQTT();
extern void MQTTtoIR(char* topicOri, char* datacallback);
extern void MQTTtoIR(char* topicOri, JsonObject& RFdata);
/*-------------------IR topics & parameters----------------------*/
//IR MQTT Subjects
#define subjectGTWIRtoMQTT     "/IRtoMQTT"
#define subjectIRtoMQTT        "/IRtoMQTT"
#define subjectMQTTtoIR        "/commands/MQTTtoIR"
#define subjectForwardMQTTtoIR "home/gateway2/commands/MQTTtoIR"

// subject monitored to listen traffic processed by other gateways to store data and avoid ntuple
#define subjectMultiGTWIR "+/+/IRtoMQTT"
#define IRbitsKey         "IRBITS_" // bits  will be defined if a subject contains IRbitsKey followed by a value of 2 digits
#define IRRptKey          "RPT_" // repeats  will be defined if a subject contains IRRptKey followed by a value of 1 digit
#define repeatIRwMQTT     false // do we repeat a received signal by using MQTT, we send a command signal to subjectForwardMQTTtoIR
#define repeatIRwNumber   0 // default repeat of the signal
//#define RawDirectForward false // direct repeat of IR signal with raw data
#define RawFrequency 38 // raw frequency sending
//#define DumpMode true // uncomment so as to see big dumps of IR codes

#define pubIRunknownPrtcl false // key to avoid MQTT publication of unknown IR protocol (set to true if you want to publish unknown protocol)
#define PanasonicAddress  0x4004 // Panasonic address (Pre data)

#if defined(ESP8266) || defined(ESP32) //IR supported protocols on ESP8266, all supported per default
#  define IR_GC
#  define IR_RAW
#  define IR_COOLIX
#  define IR_WHYNTER
#  define IR_LG
#  define IR_SONY
#  define IR_DISH
#  define IR_RC5
#  define IR_RC6
#  define IR_SHARP
#  define IR_SAMSUNG
#  define IR_PANASONIC
#  define IR_RCMM
#  define IR_MITSUBISHI
#  define IR_GICABLE
#  define IR_MITSUBISHI2
#  define IR_LASERTAG
#  define IR_CARRIER_AC
#  define IR_MIDEA
#  define IR_NIKAI
#  define IR_SHERWOOD
#  define IR_DENON
#  define IR_AIWA_RC_T501
#  define IR_JVC
#  define IR_SANYO
#  define IR_DAIKIN
#  define IR_KELVINATOR
#  define IR_MITSUBISHI_AC
#  define IR_SANYO_LC7461
#  define IR_GREE
#  define IR_ARGO
#  define IR_TROTEC
#  define IR_TOSHIBA_AC
#  define IR_FUJITSU_AC
#  define IR_MAGIQUEST
#  define IR_HAIER_AC
#  define IR_HITACHI_AC
#  define IR_HITACHI_AC1
#  define IR_HITACHI_AC2
#  define IR_GICABLE
#  define IR_HAIER_AC_YRW02
#  define IR_WHIRLPOOL_AC
#  define IR_SAMSUNG_AC
#  define IR_LUTRON
#  define IR_ELECTRA_AC
#  define IR_PANASONIC_AC
#  define IR_PIONEER
#  define IR_LG2
#  define IR_MWM
#  define IR_DAIKIN2
#  define IR_VESTEL_AC
#  define IR_SAMSUNG36
#  define IR_TCL112AC
#  define IR_TECO
#  define IR_LEGOPF
#  define IR_MITSUBISHI_HEAVY_88
#  define IR_MITSUBISHI_HEAVY_152
#  define IR_DAIKIN216
#  define IR_SHARP_AC
#  define IR_GOODWEATHER
#  define IR_INAX
#  define IR_DAIKIN160
#  define IR_NEOCLIMA
#  define IR_DAIKIN176
#  define IR_DAIKIN128
#  define IR_AMCOR
#  define IR_DAIKIN152
#  define IR_MITSUBISHI136
#  define IR_MITSUBISHI112
#  define IR_HITACHI_AC424
#  define IR_SONY_38K
#  define IR_EPSON
#  define IR_SYMPHONY
#  define IR_HITACHI_AC3
#  define IR_DAIKIN64
#  define IR_AIRWELL
#  define IR_DELONGHI_AC
#  define IR_DOSHISHA
#  define IR_MULTIBRACKETS
#  define IR_CARRIER_AC40
#  define IR_CARRIER_AC64
#  define IR_HITACHI_AC344
#  define IR_CORONA_AC
#  define IR_MIDEA24
#  define IR_ZEPEAL
#  define IR_SANYO_AC
#  define IR_VOLTAS
#  define IR_METZ
#  define IR_TRANSCOLD
#  define IR_TECHNIBEL_AC
#  define IR_MIRAGE
#  define IR_ELITESCREENS
#  define IR_PANASONIC_AC32
#  define IR_MILESTAG2
#  define IR_ECOCLIM
#  define IR_XMP
#  define IR_KELON168
#  define IR_TEKNOPOINT
#  define IR_HAIER_AC176
#  define IR_BOSE
#  define IR_SANYO_AC88
#  define IR_TROTEC_3550
#  define IR_ARRIS
#  define IR_RHOSS
#  define IR_AIRTON
#  define IR_COOLIX48
#  define IR_HITACHI_AC264
#  define IR_HITACHI_AC296
#elif __AVR_ATmega2560__
#  define IR_COOLIX
#  define IR_Whynter
#  define IR_LG
#  define IR_Sony
#  define IR_DISH
#  define IR_RC5
#  define IR_Sharp
#  define IR_SAMSUNG
#  define IR_Raw
#  define IR_PANASONIC
#else //IR supported protocols on arduino uncomment if you want to send with this protocol, NEC protocol is available per default
//#define IR_COOLIX
//#define IR_Whynter
//#define IR_LG
//#define IR_Sony
//#define IR_DISH
//#define IR_RC5
//#define IR_Sharp
//#define IR_SAMSUNG
//#define IR_Raw
//#define IR_PANASONIC
#endif

#ifndef IR_EMITTER_INVERTED
#  if defined(ESP8266) || defined(ESP32)
#    define IR_EMITTER_INVERTED false //set to true if you want to reverse the LED signal for the emitter
#  endif
#endif

/*-------------------PIN DEFINITIONS----------------------*/
#ifndef IR_RECEIVER_GPIO
#  ifdef ESP8266
#    define IR_RECEIVER_GPIO 2 //D4 /replace by 4 with sonoff rf bridge
#  elif ESP32
#    define IR_RECEIVER_GPIO 26
#  elif __AVR_ATmega2560__
#    define IR_RECEIVER_GPIO 2 // 2 = D2 on arduino mega
#  else
#    define IR_RECEIVER_GPIO 0 // 0 = D2 on arduino UNO
#  endif
#endif

#ifndef IR_EMITTER_GPIO
#  ifdef ESP8266
#    define IR_EMITTER_GPIO 16 //D0/ replace by 0 (D3) if you use IR LOLIN controller shield /replace by 5 with sonoff rf bridge
#  elif ESP32
#    define IR_EMITTER_GPIO 14
#  elif __AVR_ATmega2560__
#    define IR_EMITTER_GPIO 9
#  else
#    define IR_EMITTER_GPIO 9
#  endif
#endif

#endif
