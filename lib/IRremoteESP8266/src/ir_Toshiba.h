// Copyright 2017 David Conran
#ifndef IR_TOSHIBA_H_
#define IR_TOSHIBA_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRsend.h"

//     TTTTTTT  OOOOO   SSSSS  HH   HH IIIII BBBBB     AAA
//       TTT   OO   OO SS      HH   HH  III  BB   B   AAAAA
//       TTT   OO   OO  SSSSS  HHHHHHH  III  BBBBBB  AA   AA
//       TTT   OO   OO      SS HH   HH  III  BB   BB AAAAAAA
//       TTT    OOOO0   SSSSS  HH   HH IIIII BBBBBB  AA   AA

// Toshiba A/C support added by David Conran

// Constants
#define TOSHIBA_AC_AUTO              0U
#define TOSHIBA_AC_COOL              1U
#define TOSHIBA_AC_DRY               2U
#define TOSHIBA_AC_HEAT              3U
#define TOSHIBA_AC_POWER             4U
#define TOSHIBA_AC_FAN_AUTO          0U
#define TOSHIBA_AC_FAN_MAX           5U
#define TOSHIBA_AC_MIN_TEMP         17U  // 17C
#define TOSHIBA_AC_MAX_TEMP         30U  // 30C

class IRToshibaAC {
 public:
  explicit IRToshibaAC(uint16_t pin);

  void stateReset();
#if SEND_TOSHIBA_AC
  void send();
#endif  // SEND_TOSHIBA_AC
  void begin();
  void on();
  void off();
  void setPower(bool state);
  bool getPower();
  void setTemp(uint8_t temp);
  uint8_t getTemp();
  void setFan(uint8_t fan);
  uint8_t getFan();
  void setMode(uint8_t mode);
  uint8_t getMode(bool useRaw = false);
  void setRaw(uint8_t newState[]);
  uint8_t* getRaw();
  static bool validChecksum(const uint8_t state[],
                            const uint16_t length = TOSHIBA_AC_STATE_LENGTH);
#ifdef ARDUINO
  String toString();
#else
  std::string toString();
#endif
#ifndef UNIT_TEST

 private:
#endif
  uint8_t remote_state[TOSHIBA_AC_STATE_LENGTH];
  void checksum(const uint16_t length = TOSHIBA_AC_STATE_LENGTH);
  static uint8_t calcChecksum(const uint8_t state[],
                              const uint16_t length = TOSHIBA_AC_STATE_LENGTH);
  uint8_t mode_state;
  IRsend _irsend;
};

#endif  // IR_TOSHIBA_H_
