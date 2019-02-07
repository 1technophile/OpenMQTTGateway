// Copyright 2018 Erdem U. Altinyurt
// (Used Midea.h as template)
#ifndef IR_VESTEL_H_
#define IR_VESTEL_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRsend.h"

//                 VV     VV  EEEEEEE   SSSSS  TTTTTTTT  EEEEEEE  LL
//                 VV     VV  EE       S          TT     EE       LL
//                  VV   VV   EEEEE     SSSS      TT     EEEEE    LL
//                   VV VV    EE            S     TT     EE       LL
//                    VVV     EEEEEEE  SSSSS      TT     EEEEEEE  LLLLLLL

// Vestel added by Erdem U. Altinyurt

// Constants
const uint16_t kVestelACHdrMark = 3110;
const uint16_t kVestelACHdrSpace = 9066;
const uint16_t kVestelACBitMark = 520;
const uint16_t kVestelACOneSpace = 1535;
const uint16_t kVestelACZeroSpace = 480;
const uint16_t kVestelACTolerance = 30;

const uint8_t kVestelACMinTempH = 16;
const uint8_t kVestelACMinTempC = 18;
const uint8_t kVestelACMaxTemp = 30;

const uint64_t kVestelACCRCMask = 0xFFFFFFFFFFF00000;

const uint8_t kVestelACAuto = 0;
const uint8_t kVestelACCool = 1;
const uint8_t kVestelACDry = 2;
const uint8_t kVestelACFan = 3;
const uint8_t kVestelACHeat = 4;

const uint8_t kVestelACFanAuto = 1;
const uint8_t kVestelACFanLow = 5;
const uint8_t kVestelACFanMed = 9;
const uint8_t kVestelACFanHigh = 0xB;
const uint8_t kVestelACFanAutoCool = 0xC;
const uint8_t kVestelACFanAutoHot = 0xD;

const uint8_t kVestelACNormal = 1;
const uint8_t kVestelACSleep = 3;
const uint8_t kVestelACTurbo = 7;
const uint8_t kVestelACIon = 4;
const uint8_t kVestelACSwing = 0xA;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked-bitfield-compat"
union VestelACState {
  // Remotes Command Stack
  struct {
    uint16_t footer : 12;  // 0x201 footer
    uint8_t CRC : 8;
    uint8_t swing : 4;               // auto 0xA, stop 0xF
    uint8_t turbo_sleep_normal : 4;  //  normal 0x1, sleep 0x3, turbo 0x7
    uint8_t zero : 8;                // 0x00
    uint8_t temp : 4;                // temp-16 degree Celcius
    uint8_t fan : 4;  // auto 0x1, low 0x5, mid 0x9, high 0xB, 0xD auto hot, 0xC
                      // auto cool
    uint8_t mode : 4;   // auto 0x0, cold 0x1, dry 0x2, fan 0x3, hot 0x4
    uint8_t ion : 4;    // on 0x4, off 0x0
    uint8_t power : 4;  // on 0xF, off 0xC
    uint8_t not_used : 8;
  } __attribute__((packed));  // avoids padding.

  uint64_t rawCode;

  // Remotes Timer Stack for programming AC unit for turn of after some time or
  // self wakeup - turn off at defined time. There are no timer functions
  // implemented.
  // Here is the stack only for decoding and reverse enginering purposes.
  struct {
    uint16_t t_footer : 12;  // 0x201 footer
    uint8_t t_CRC : 8;
    // timer regs has 0x18 hour mask & 0x3=Minute mask minutes are divided to 10
    // format. Need to multiply with 10 Examples: 0x01= xx:10 and 0x08 = 01:00,
    // 0x03= 00:30 0x08=01:00 0x10=02:00 0x44=08:40
    uint8_t t_turnOffMinute : 3;  // off minute/10
    uint8_t t_turnOffHour : 5;
    uint8_t t_turnOnMinute : 3;  // on minute/10
    uint8_t t_turnOnHour : 5;
    uint8_t t_hour : 5;  // actual time hour
    uint8_t t_on_active : 1;
    uint8_t t_off_active : 1;
    uint8_t t_timer_mode : 1;  // For timer operation, also need t_on_active bit
    uint8_t t_minute : 8;      // actual time minute
    uint16_t t_not_used : 16;  // 00
  } __attribute__((packed));   // avoids padding.
};
#pragma GCC diagnostic pop

class IRVestelAC {
 public:
  explicit IRVestelAC(uint16_t pin);

  void stateReset();
#if SEND_VESTEL_AC
  void send();
#endif  // SEND_VESTEL_AC
  void begin(void);
  void on(void);
  void off(void);
  void setPower(const bool state);
  bool getPower();
  void setAuto(const int8_t autoLevel);
  void setTimer(const uint16_t minutes);
  uint16_t getTimer(void);
  void setTime(const uint16_t minutes);
  uint16_t getTime(void);
  void setOnTimer(const uint16_t minutes);
  uint16_t getOnTimer(void);
  void setOffTimer(const uint16_t minutes);
  uint16_t getOffTimer(void);
  void setTemp(const uint8_t temp);
  uint8_t getTemp(void);
  void setFan(const uint8_t fan);
  uint8_t getFan(void);
  void setMode(const uint8_t mode);
  uint8_t getMode(void);
  void setRaw(uint8_t* newState);
  void setRaw(const uint64_t newState);
  uint64_t getRaw(void);
  static bool validChecksum(const uint64_t state);
  void setSwing(const bool state);
  bool getSwing(void);
  void setSleep(const bool state);
  bool getSleep(void);
  void setTurbo(const bool state);
  bool getTurbo(void);
  void setIon(const bool state);
  bool getIon(void);
  bool isTimeCommand(void);
#ifdef ARDUINO
  String toString();
#else
  std::string toString();
#endif

#ifndef UNIT_TEST
// private:
#endif
  VestelACState remote_state;
  VestelACState remote_time_state;
  bool use_time_state = false;
  void checksum();
  static uint8_t calcChecksum(const uint64_t state);
  IRsend _irsend;
};

#endif  // IR_VESTEL_H_
