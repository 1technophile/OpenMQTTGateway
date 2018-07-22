// Copyright 2018 crankyoldgit
// The specifics of reverse engineering the protocol details by kuzin2006

#ifndef IR_HAIER_H_
#define IR_HAIER_H_

#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRsend.h"

//                      HH   HH   AAA   IIIII EEEEEEE RRRRRR
//                      HH   HH  AAAAA   III  EE      RR   RR
//                      HHHHHHH AA   AA  III  EEEEE   RRRRRR
//                      HH   HH AAAAAAA  III  EE      RR  RR
//                      HH   HH AA   AA IIIII EEEEEEE RR   RR

// Ref:
//   https://github.com/markszabo/IRremoteESP8266/issues/404
//   https://www.dropbox.com/s/mecyib3lhdxc8c6/IR%20data%20reverse%20engineering.xlsx?dl=0

// Constants
// Byte 0
#define HAIER_AC_PREFIX      0b10100101

// Byte 1
#define HAIER_AC_MIN_TEMP      16
#define HAIER_AC_MAX_TEMP      30
#define HAIER_AC_DEF_TEMP      25

#define HAIER_AC_CMD_OFF          0b00000000
#define HAIER_AC_CMD_ON           0b00000001
#define HAIER_AC_CMD_MODE         0b00000010
#define HAIER_AC_CMD_FAN          0b00000011
#define HAIER_AC_CMD_TEMP_UP      0b00000110
#define HAIER_AC_CMD_TEMP_DOWN    0b00000111
#define HAIER_AC_CMD_SLEEP        0b00001000
#define HAIER_AC_CMD_TIMER_SET    0b00001001
#define HAIER_AC_CMD_TIMER_CANCEL 0b00001010
#define HAIER_AC_CMD_HEALTH       0b00001100
#define HAIER_AC_CMD_SWING        0b00001101

// Byte 2
#define HAIER_AC_SWING_OFF        0b00000000
#define HAIER_AC_SWING_UP         0b00000001
#define HAIER_AC_SWING_DOWN       0b00000010
#define HAIER_AC_SWING_CHG        0b00000011

// Byte 6
#define HAIER_AC_AUTO           0
#define HAIER_AC_COOL           1
#define HAIER_AC_DRY            2
#define HAIER_AC_HEAT           3
#define HAIER_AC_FAN            4

#define HAIER_AC_FAN_AUTO       0
#define HAIER_AC_FAN_LOW        1
#define HAIER_AC_FAN_MED        2
#define HAIER_AC_FAN_HIGH       3

#define HAIER_AC_MAX_TIME      (23 * 60 + 59)


class IRHaierAC {
 public:
  explicit IRHaierAC(uint16_t pin);

#if SEND_HAIER_AC
  void send();
#endif  // SEND_HAIER_AC
  void begin();

  void setCommand(const uint8_t command);
  uint8_t getCommand();

  void setTemp(const uint8_t temp);
  uint8_t getTemp();

  void setFan(const uint8_t speed);
  uint8_t getFan();

  uint8_t getMode();
  void setMode(const uint8_t mode);

  bool getSleep();
  void setSleep(const bool state);
  bool getHealth();
  void setHealth(const bool state);

  int16_t getOnTimer();
  void setOnTimer(const uint16_t mins);
  int16_t getOffTimer();
  void setOffTimer(const uint16_t mins);
  void cancelTimers();

  uint16_t getCurrTime();
  void setCurrTime(const uint16_t mins);

  uint8_t getSwing();
  void setSwing(const uint8_t state);

  uint8_t* getRaw();
  void setRaw(uint8_t new_code[]);
  static bool validChecksum(uint8_t state[],
                            const uint16_t length = HAIER_AC_STATE_LENGTH);
  #ifdef ARDUINO
    String toString();
    static String timeToString(const uint16_t nr_mins);
  #else
    std::string toString();
    static std::string timeToString(const uint16_t nr_mins);
  #endif

 private:
  uint8_t remote_state[HAIER_AC_STATE_LENGTH];
  void stateReset();
  void checksum();
  static uint16_t getTime(const uint8_t ptr[]);
  static void setTime(uint8_t ptr[], const uint16_t nr_mins);
  IRsend _irsend;
};

#endif  // IR_HAIER_H_
