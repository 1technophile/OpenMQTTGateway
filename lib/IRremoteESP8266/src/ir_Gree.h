// Kelvinator A/C
//
// Copyright 2016 David Conran

#ifndef IR_GREE_H_
#define IR_GREE_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRsend.h"

//                      GGGG  RRRRRR  EEEEEEE EEEEEEE
//                     GG  GG RR   RR EE      EE
//                    GG      RRRRRR  EEEEE   EEEEE
//                    GG   GG RR  RR  EE      EE
//                     GGGGGG RR   RR EEEEEEE EEEEEEE

// Constants
#define GREE_AUTO                                  0U
#define GREE_COOL                                  1U
#define GREE_DRY                                   2U
#define GREE_FAN                                   3U
#define GREE_HEAT                                  4U

#define GREE_POWER1_MASK                  0b00001000U
#define GREE_POWER2_MASK                  0b01000000U
#define GREE_MIN_TEMP                             16U  // Celsius
#define GREE_MAX_TEMP                             30U  // Celsius
#define GREE_FAN_MAX                               3U
#define GREE_FAN_MASK                     0b00110000U
#define GREE_MODE_MASK                    0b00000111U
#define GREE_TURBO_MASK                   0b00010000U
#define GREE_LIGHT_MASK                   0b00100000U
#define GREE_XFAN_MASK                    0b10000000U
#define GREE_SLEEP_MASK                   0b10000000U

#define GREE_SWING_AUTO_MASK              0b01000000U
#define GREE_SWING_POS_MASK               0b00001111U
#define GREE_SWING_LAST_POS               0b00000000U
#define GREE_SWING_AUTO                   0b00000001U
#define GREE_SWING_UP                     0b00000010U
#define GREE_SWING_MIDDLE_UP              0b00000011U
#define GREE_SWING_MIDDLE                 0b00000100U
#define GREE_SWING_MIDDLE_DOWN            0b00000101U
#define GREE_SWING_DOWN                   0b00000110U
#define GREE_SWING_DOWN_AUTO              0b00000111U
#define GREE_SWING_MIDDLE_AUTO            0b00001001U
#define GREE_SWING_UP_AUTO                0b00001011U

// Classes
class IRGreeAC {
 public:
  explicit IRGreeAC(uint16_t pin);

  void stateReset();
#if SEND_GREE
  void send();
#endif  // SEND_GREE
  void begin();
  void on();
  void off();
  void setPower(const bool state);
  bool getPower();
  void setTemp(const uint8_t temp);
  uint8_t getTemp();
  void setFan(const uint8_t speed);
  uint8_t getFan();
  void setMode(const uint8_t new_mode);
  uint8_t getMode();
  void setLight(const bool state);
  bool getLight();
  void setXFan(const bool state);
  bool getXFan();
  void setSleep(const bool state);
  bool getSleep();
  void setTurbo(const bool state);
  bool getTurbo();
  void setSwingVertical(const bool automatic, const uint8_t position);
  bool getSwingVerticalAuto();
  uint8_t getSwingVerticalPosition();

  uint8_t* getRaw();
  void setRaw(uint8_t new_code[]);
  static bool validChecksum(const uint8_t state[],
                            const uint16_t length = GREE_STATE_LENGTH);
#ifdef ARDUINO
  String toString();
#else
  std::string toString();
#endif

 private:
  // The state of the IR remote in IR code form.
  uint8_t remote_state[GREE_STATE_LENGTH];
  void checksum(const uint16_t length = GREE_STATE_LENGTH);
  void fixup();
  IRsend _irsend;
};

#endif  // IR_GREE_H_
