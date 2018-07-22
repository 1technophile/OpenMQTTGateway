// Copyright 2017 David Conran
#ifndef IR_MIDEA_H_
#define IR_MIDEA_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRsend.h"

//                  MM    MM IIIII DDDDD   EEEEEEE   AAA
//                  MMM  MMM  III  DD  DD  EE       AAAAA
//                  MM MM MM  III  DD   DD EEEEE   AA   AA
//                  MM    MM  III  DD   DD EE      AAAAAAA
//                  MM    MM IIIII DDDDDD  EEEEEEE AA   AA

// Midea added by crankyoldgit & bwze
// Ref:
//   https://docs.google.com/spreadsheets/d/1TZh4jWrx4h9zzpYUI9aYXMl1fYOiqu-xVuOOMqagxrs/edit?usp=sharing

// Constants
#define MIDEA_AC_COOL              0U  // 0b000
#define MIDEA_AC_DRY               1U  // 0b001
#define MIDEA_AC_AUTO              2U  // 0b010
#define MIDEA_AC_HEAT              3U  // 0b011
#define MIDEA_AC_FAN               4U  // 0b100
#define MIDEA_AC_POWER     1ULL << 39
#define MIDEA_AC_SLEEP     1ULL << 38
#define MIDEA_AC_FAN_AUTO          0U  // 0b000
#define MIDEA_AC_FAN_LOW           1U  // 0b001
#define MIDEA_AC_FAN_MED           2U  // 0b010
#define MIDEA_AC_FAN_HI            3U  // 0b011
#define MIDEA_AC_MIN_TEMP_F       62U  // 62F
#define MIDEA_AC_MAX_TEMP_F       86U  // 86F
#define MIDEA_AC_MIN_TEMP_C       16U  // 16C
#define MIDEA_AC_MAX_TEMP_C       30U  // 30C
#define MIDEA_AC_STATE_MASK         0x0000FFFFFFFFFFFFULL
#define MIDEA_AC_TEMP_MASK          0x0000FFFFE0FFFFFFULL
#define MIDEA_AC_FAN_MASK           0x0000FFC7FFFFFFFFULL
#define MIDEA_AC_MODE_MASK          0x0000FFF8FFFFFFFFULL
#define MIDEA_AC_CHECKSUM_MASK      0x0000FFFFFFFFFF00ULL

class IRMideaAC {
 public:
  explicit IRMideaAC(uint16_t pin);

  void stateReset();
#if SEND_MIDEA
  void send();
#endif  // SEND_MIDEA
  void begin();
  void on();
  void off();
  void setPower(const bool state);
  bool getPower();
  void setTemp(const uint8_t temp, const bool useCelsius = false);
  uint8_t getTemp(const bool useCelsius = false);
  void setFan(const uint8_t fan);
  uint8_t getFan();
  void setMode(const uint8_t mode);
  uint8_t getMode();
  void setRaw(uint64_t newState);
  uint64_t getRaw();
  static bool validChecksum(const uint64_t state);
  void setSleep(const bool state);
  bool getSleep();
#ifdef ARDUINO
  String toString();
#else
  std::string toString();
#endif
#ifndef UNIT_TEST

 private:
#endif
  uint64_t remote_state;
  void checksum();
  static uint8_t calcChecksum(const uint64_t state);
  IRsend _irsend;
};


#endif  // IR_MIDEA_H_
