// Coolix A/C
//
// Copyright 2018 David Conran

#ifndef IR_COOLIX_H_
#define IR_COOLIX_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRsend.h"

//             CCCCC   OOOOO   OOOOO  LL      IIIII XX    XX
//            CC    C OO   OO OO   OO LL       III   XX  XX
//            CC      OO   OO OO   OO LL       III    XXXX
//            CC    C OO   OO OO   OO LL       III   XX  XX
//             CCCCC   OOOO0   OOOO0  LLLLLLL IIIII XX    XX

// Supports:
//   RG57K7(B)/BGEF remote control for Beko BINR 070/071 split-type aircon.
// Ref:
//   https://github.com/markszabo/IRremoteESP8266/issues/484
// Kudos:
//   Hamper: For the breakdown and mapping of the bit values.

// Constants
// Modes
const uint8_t kCoolixCool = 0b00;
const uint8_t kCoolixDry = 0b01;
const uint8_t kCoolixAuto = 0b10;
const uint8_t kCoolixHeat = 0b11;
const uint8_t kCoolixFan = 4;                                 // Synthetic.
const uint32_t kCoolixModeMask = 0b000000000000000000001100;  // 0xC
const uint32_t kCoolixZoneFollowMask = 0b000010000000000000000000;  // 0x80000
// Fan Control
const uint8_t kCoolixFanMin = 0b100;
const uint8_t kCoolixFanMed = 0b010;
const uint8_t kCoolixFanMax = 0b001;
const uint8_t kCoolixFanAuto = 0b101;
const uint8_t kCoolixFanAuto0 = 0b000;
const uint8_t kCoolixFanZoneFollow = 0b110;
const uint8_t kCoolixFanFixed = 0b111;
const uint32_t kCoolixFanMask = 0b000000001110000000000000;  // 0x00E000
// Temperature
const uint8_t kCoolixTempMin = 17;  // Celsius
const uint8_t kCoolixTempMax = 30;  // Celsius
const uint8_t kCoolixTempRange = kCoolixTempMax - kCoolixTempMin + 1;
const uint8_t kCoolixFanTempCode = 0b1110;  // Part of Fan Mode.
const uint32_t kCoolixTempMask = 0b11110000;
const uint8_t kCoolixTempMap[kCoolixTempRange] = {
    0b0000,  // 17C
    0b0001,  // 18c
    0b0011,  // 19C
    0b0010,  // 20C
    0b0110,  // 21C
    0b0111,  // 22C
    0b0101,  // 23C
    0b0100,  // 24C
    0b1100,  // 25C
    0b1101,  // 26C
    0b1001,  // 27C
    0b1000,  // 28C
    0b1010,  // 29C
    0b1011   // 30C
};
const uint8_t kCoolixSensorTempMin = 16;  // Celsius
const uint8_t kCoolixSensorTempMax = 30;  // Celsius
const uint8_t kCoolixSensorTempIgnoreCode = 0b1111;
const uint32_t kCoolixSensorTempMask = 0b000000000000111100000000;  // 0xF00
// Fixed states/messages.
const uint8_t kCoolixPrefix = 0b1011;  // 0xB
const uint8_t kCoolixUnknown = 0xFF;
const uint32_t kCoolixOff = 0b101100100111101111100000;    // 0xB27BE0
const uint32_t kCoolixSwing = 0b101100100110101111100000;  // 0xB26BE0
const uint32_t kCoolixSleep = 0b101100101110000000000011;  // 0xB2E003
const uint32_t kCoolixTurbo = 0b101101011111010110100010;  // 0xB5F5A2
const uint32_t kCoolixLed = 0b101101011111010110100101;    // 0xB5F5A5
const uint32_t kCoolixClean = 0b101101011111010110101010;  // 0xB5F5AA
// On, 25C, Mode: Auto, Fan: Auto, Zone Follow: Off, Sensor Temp: Ignore.
const uint32_t kCoolixDefaultState = 0b101100101011111111001000;  // 0xB2BFC8

// Classes
class IRCoolixAC {
 public:
  explicit IRCoolixAC(uint16_t pin);

  void stateReset();
#if SEND_COOLIX
  void send(const uint16_t repeat = kCoolixDefaultRepeat);
#endif  // SEND_COOLIX
  void begin();
  void on();
  void off();
  void setPower(const bool state);
  bool getPower();
  void setTemp(const uint8_t temp);
  uint8_t getTemp();
  void setSensorTemp(const uint8_t desired);
  uint8_t getSensorTemp();
  void clearSensorTemp();
  void setFan(const uint8_t fan);
  uint8_t getFan();
  void setMode(const uint8_t mode);
  uint8_t getMode();
  void setSwing();
  bool getSwing();
  void setSleep();
  bool getSleep();
  void setTurbo();
  bool getTurbo();
  void setLed();
  bool getLed();
  void setClean();
  bool getClean();
  bool getZoneFollow();
  uint32_t getRaw();
  void setRaw(const uint32_t new_code);

#ifdef ARDUINO
  String toString();
#else
  std::string toString();
#endif

 private:
  uint32_t remote_state;  // The state of the IR remote in IR code form.
  uint32_t saved_state;   // Copy of the state if we required a special mode.
  IRsend _irsend;
  void setTempRaw(const uint8_t code);
  uint8_t getTempRaw();
  void setSensorTempRaw(const uint8_t code);
  void setZoneFollow(const bool state);
  bool isSpecialState(void);
  void updateSavedState(void);
  void recoverSavedState(void);
  uint32_t getNormalState(void);
};

#endif  // IR_COOLIX_H_
