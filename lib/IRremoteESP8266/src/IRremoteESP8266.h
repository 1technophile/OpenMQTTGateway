 /***************************************************
 * IRremote for ESP8266
 *
 * Based on the IRremote library for Arduino by Ken Shirriff
 * Version 0.11 August, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
 *
 * Edited by Mitra to add new controller SANYO
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 *
 * JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
 * LG added by Darryl Smith (based on the JVC protocol)
 * Whynter A/C ARC-110WD added by Francesco Meschia
 * Coolix A/C / heatpump added by (send) bakrus & (decode) crankyoldgit
 * Denon: sendDenon, decodeDenon added by Massimiliano Pinto
          (from https://github.com/z3t0/Arduino-IRremote/blob/master/ir_Denon.cpp)
 * Kelvinator A/C and Sherwood added by crankyoldgit
 * Mitsubishi (TV) sending added by crankyoldgit
 * Pronto code sending added by crankyoldgit
 * Mitsubishi & Toshiba A/C added by crankyoldgit
 *     (derived from https://github.com/r45635/HVAC-IR-Control)
 * DISH decode by marcosamarinho
 * Gree Heatpump sending added by Ville Skyttä (scop)
 *     (derived from https://github.com/ToniA/arduino-heatpumpir/blob/master/GreeHeatpumpIR.cpp)
 * Updated by markszabo (https://github.com/markszabo/IRremoteESP8266) for sending IR code on ESP8266
 * Updated by Sebastien Warin (http://sebastien.warin.fr) for receiving IR code on ESP8266
 *
 * Updated by sillyfrog for Daikin, adopted from
 * (https://github.com/mharizanov/Daikin-AC-remote-control-over-the-Internet/)
 * Fujitsu A/C code added by jonnygraham
 * Trotec AC code by stufisher
 * Carrier AC code by crankyoldgit
 *
 *  GPL license, all text above must be included in any redistribution
 ****************************************************/

#ifndef IRREMOTEESP8266_H_
#define IRREMOTEESP8266_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#ifdef UNIT_TEST
#include <iostream>
#endif

// Library Version
#define _IRREMOTEESP8266_VERSION_ "2.4.2"
// Supported IR protocols
// Each protocol you include costs memory and, during decode, costs time
// Disable (set to false) all the protocols you do not need/want!
// The Air Conditioner protocols are the most expensive memory-wise.
//
#define DECODE_HASH          true  // Semi-unique code for unknown messages

#define SEND_RAW             true

#define DECODE_NEC           true
#define SEND_NEC             true

#define DECODE_SHERWOOD      true  // Doesn't exist. Actually is DECODE_NEC
#define SEND_SHERWOOD        true

#define DECODE_RC5           true
#define SEND_RC5             true

#define DECODE_RC6           true
#define SEND_RC6             true

#define DECODE_RCMM          true
#define SEND_RCMM            true

#define DECODE_SONY          true
#define SEND_SONY            true

#define DECODE_PANASONIC     true
#define SEND_PANASONIC       true

#define DECODE_JVC           true
#define SEND_JVC             true

#define DECODE_SAMSUNG       true
#define SEND_SAMSUNG         true

#define DECODE_WHYNTER       true
#define SEND_WHYNTER         true

#define DECODE_AIWA_RC_T501  true
#define SEND_AIWA_RC_T501    true

#define DECODE_LG            true
#define SEND_LG              true

#define DECODE_SANYO         true
#define SEND_SANYO           true

#define DECODE_MITSUBISHI    true
#define SEND_MITSUBISHI      true

#define DECODE_MITSUBISHI2   true
#define SEND_MITSUBISHI2     true

#define DECODE_DISH          true
#define SEND_DISH            true

#define DECODE_SHARP         true
#define SEND_SHARP           true

#define DECODE_DENON         true
#define SEND_DENON           true

#define DECODE_KELVINATOR    true
#define SEND_KELVINATOR      true

#define DECODE_MITSUBISHI_AC false  // Not written.
#define SEND_MITSUBISHI_AC   true

#define DECODE_FUJITSU_AC    true
#define SEND_FUJITSU_AC      true

#define DECODE_DAIKIN        true
#define SEND_DAIKIN          true

#define DECODE_COOLIX        true
#define SEND_COOLIX          true

#define DECODE_GLOBALCACHE   false  // Not written.
#define SEND_GLOBALCACHE     true

#define DECODE_GREE          true
#define SEND_GREE            true

#define DECODE_PRONTO        false  // Not written.
#define SEND_PRONTO          true

#define DECODE_ARGO          false  // Not written.
#define SEND_ARGO            true

#define DECODE_TROTEC        false  // Not implemented.
#define SEND_TROTEC          true

#define DECODE_NIKAI         true
#define SEND_NIKAI           true

#define DECODE_TOSHIBA_AC    true
#define SEND_TOSHIBA_AC      true

#define DECODE_MAGIQUEST     true
#define SEND_MAGIQUEST       true

#define DECODE_MIDEA         true
#define SEND_MIDEA           true

#define DECODE_LASERTAG      true
#define SEND_LASERTAG        true

#define DECODE_CARRIER_AC    true
#define SEND_CARRIER_AC      true

#define DECODE_HAIER_AC      true
#define SEND_HAIER_AC        true

#define DECODE_HITACHI_AC    true
#define SEND_HITACHI_AC      true

#define DECODE_HITACHI_AC1   true
#define SEND_HITACHI_AC1     true

#define DECODE_HITACHI_AC2   true
#define SEND_HITACHI_AC2     true

#define DECODE_GICABLE       true
#define SEND_GICABLE         true

#if (DECODE_ARGO || DECODE_DAIKIN || DECODE_FUJITSU_AC || DECODE_GREE || \
     DECODE_KELVINATOR || DECODE_MITSUBISHI_AC || DECODE_TOSHIBA_AC || \
     DECODE_TROTEC || DECODE_HAIER_AC || DECODE_HITACHI_AC || \
     DECODE_HITACHI_AC1 || DECODE_HITACHI_AC2)
#define DECODE_AC true  // We need some common infrastructure for decoding A/Cs.
#else
#define DECODE_AC false   // We don't need that infrastructure.
#endif

// Use millisecond 'delay()' calls where we can to avoid tripping the WDT.
// Note: If you plan to send IR messages in the callbacks of the AsyncWebserver
//       library, you need to set ALLOW_DELAY_CALLS to false.
//       Ref: https://github.com/markszabo/IRremoteESP8266/issues/430
#define ALLOW_DELAY_CALLS true

/*
 * Always add to the end of the list and should never remove entries
 * or change order. Projects may save the type number for later usage
 * so numbering should always stay the same.
 */
enum decode_type_t {
  UNKNOWN = -1,
  UNUSED = 0,
  RC5,
  RC6,
  NEC,
  SONY,
  PANASONIC,
  JVC,
  SAMSUNG,
  WHYNTER,
  AIWA_RC_T501,
  LG,
  SANYO,
  MITSUBISHI,
  DISH,
  SHARP,
  COOLIX,
  DAIKIN,
  DENON,
  KELVINATOR,
  SHERWOOD,
  MITSUBISHI_AC,
  RCMM,
  SANYO_LC7461,
  RC5X,
  GREE,
  PRONTO,  // Technically not a protocol, but an encoding.
  NEC_LIKE,
  ARGO,
  TROTEC,
  NIKAI,
  RAW,  // Technically not a protocol, but an encoding.
  GLOBALCACHE,  // Technically not a protocol, but an encoding.
  TOSHIBA_AC,
  FUJITSU_AC,
  MIDEA,
  MAGIQUEST,
  LASERTAG,
  CARRIER_AC,
  HAIER_AC,
  MITSUBISHI2,
  HITACHI_AC,
  HITACHI_AC1,
  HITACHI_AC2,
  GICABLE
};

// Message lengths & required repeat values
#define AIWA_RC_T501_BITS           15U
#define AIWA_RC_T501_MIN_REPEAT      1U
#define COOLIX_BITS                 24U
#define CARRIER_AC_BITS             32U
#define CARRIER_AC_MIN_REPEAT        0U
// Daikin has a lot of static stuff that is discarded
#define DAIKIN_RAW_BITS            583U
#define DAIKIN_COMMAND_LENGTH       27U
#define DAIKIN_BITS                 (DAIKIN_COMMAND_LENGTH * 8)
#define DENON_BITS                  SHARP_BITS
#define DENON_48_BITS               PANASONIC_BITS
#define DENON_LEGACY_BITS           14U
#define DISH_BITS                   16U
#define DISH_MIN_REPEAT              3U
#define GICABLE_BITS                16U
#define GICABLE_MIN_REPEAT           1U
#define GREE_STATE_LENGTH            8U
#define GREE_BITS                   (GREE_STATE_LENGTH * 8)
#define HAIER_AC_STATE_LENGTH        9U
#define HAIER_AC_BITS               (HAIER_AC_STATE_LENGTH * 8)
#define HITACHI_AC_STATE_LENGTH     28U
#define HITACHI_AC_BITS             (HITACHI_AC_STATE_LENGTH * 8)
#define HITACHI_AC1_STATE_LENGTH    13U
#define HITACHI_AC1_BITS            (HITACHI_AC1_STATE_LENGTH * 8)
#define HITACHI_AC2_STATE_LENGTH    53U
#define HITACHI_AC2_BITS            (HITACHI_AC2_STATE_LENGTH * 8)
#define JVC_BITS                    16U
#define KELVINATOR_STATE_LENGTH     16U
#define KELVINATOR_BITS             (KELVINATOR_STATE_LENGTH * 8)
#define LG_BITS                     28U
#define LG32_BITS                   32U
#define MITSUBISHI_BITS             16U
// TODO(anyone): Verify that the Mitsubishi repeat is really needed.
#define MITSUBISHI_MIN_REPEAT        1U  // Based on marcosamarinho's code.
#define MITSUBISHI_AC_STATE_LENGTH  18U
#define MITSUBISHI_AC_MIN_REPEAT     1U
#define FUJITSU_AC_MIN_REPEAT        0U
#define FUJITSU_AC_STATE_LENGTH     16U
#define FUJITSU_AC_STATE_LENGTH_SHORT 7U
#define FUJITSU_AC_BITS             (FUJITSU_AC_STATE_LENGTH * 8)
#define FUJITSU_AC_MIN_BITS         ((FUJITSU_AC_STATE_LENGTH_SHORT - 1) * 8)
#define NEC_BITS                    32U
#define PANASONIC_BITS              48U
#define PANASONIC_MANUFACTURER   0x4004ULL
#define PRONTO_MIN_LENGTH            6U
#define RC5_RAW_BITS                14U
#define RC5_BITS      RC5_RAW_BITS - 2U
#define RC5X_BITS     RC5_RAW_BITS - 1U
#define RC6_MODE0_BITS              20U  // Excludes the 'start' bit.
#define RC6_36_BITS                 36U  // Excludes the 'start' bit.
#define RCMM_BITS                   24U
#define SAMSUNG_BITS                32U
#define SANYO_SA8650B_BITS          12U
#define SANYO_LC7461_ADDRESS_BITS   13U
#define SANYO_LC7461_COMMAND_BITS    8U
#define SANYO_LC7461_BITS           ((SANYO_LC7461_ADDRESS_BITS + \
                                     SANYO_LC7461_COMMAND_BITS) * 2)
#define SHARP_ADDRESS_BITS           5U
#define SHARP_COMMAND_BITS           8U
#define SHARP_BITS (SHARP_ADDRESS_BITS + SHARP_COMMAND_BITS + 2)  // 15U
#define SHERWOOD_BITS          NEC_BITS
#define SHERWOOD_MIN_REPEAT          1U
#define SONY_12_BITS                12U
#define SONY_15_BITS                15U
#define SONY_20_BITS                20U
#define SONY_MIN_BITS      SONY_12_BITS
#define SONY_MIN_REPEAT              2U
#define TOSHIBA_AC_STATE_LENGTH      9U
#define TOSHIBA_AC_BITS              (TOSHIBA_AC_STATE_LENGTH * 8)
#define TOSHIBA_AC_MIN_REPEAT        1U
#define TROTEC_COMMAND_LENGTH        9U
#define WHYNTER_BITS                32U
#define ARGO_COMMAND_LENGTH         12U
#define NIKAI_BITS                  24U
#define MAGIQUEST_BITS              56U
#define MIDEA_BITS                  48U
#define MIDEA_MIN_REPEAT             0U
#define LASERTAG_BITS               13U
#define LASERTAG_MIN_REPEAT          0U

// Turn on Debugging information by uncommenting the following line.
// #define DEBUG 1

#ifdef DEBUG
#ifdef UNIT_TEST
#define DPRINT(x) do { std::cout << x; } while (0)
#define DPRINTLN(x) do { std::cout << x << std::endl; } while (0)
#endif  // UNIT_TEST
#ifdef ARDUINO
#define DPRINT(x) do { Serial.print(x); } while (0)
#define DPRINTLN(x) do { Serial.println(x); } while (0)
#endif  // ARDUINO
#else  // DEBUG
#define DPRINT(x)
#define DPRINTLN(x)
#endif  // DEBUG

#endif  // IRREMOTEESP8266_H_
