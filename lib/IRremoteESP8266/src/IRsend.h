// Copyright 2009 Ken Shirriff
// Copyright 2015 Mark Szabo
// Copyright 2017 David Conran
#ifndef IRSEND_H_
#define IRSEND_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include "IRremoteESP8266.h"

// Originally from https://github.com/shirriff/Arduino-IRremote/
// Updated by markszabo (https://github.com/markszabo/IRremoteESP8266) for
// sending IR code on ESP8266

#if TEST || UNIT_TEST
#define VIRTUAL virtual
#else
#define VIRTUAL
#endif

// Constants
// Offset (in microseconds) to use in Period time calculations to account for
// code excution time in producing the software PWM signal.
// Value was calculated on Wemos D1 mini using v2.4.1 with v2.4.0 ESP core
#define PERIOD_OFFSET -5
#define DUTY_DEFAULT 50
#define DUTY_MAX 100  // Percentage
// delayMicroseconds() is only accurate to 16383us.
// Ref: https://www.arduino.cc/en/Reference/delayMicroseconds
#define MAX_ACCURATE_USEC_DELAY 16383U

// Classes
class IRsend {
 public:
  explicit IRsend(uint16_t IRsendPin, bool inverted = false,
                  bool use_modulation = true);
  void begin();
  void enableIROut(uint32_t freq, uint8_t duty = DUTY_DEFAULT);
  VIRTUAL void _delayMicroseconds(uint32_t usec);
  VIRTUAL uint16_t mark(uint16_t usec);
  VIRTUAL void space(uint32_t usec);
  int8_t calibrate(uint16_t hz = 38000U);
  void sendRaw(uint16_t buf[], uint16_t len, uint16_t hz);
  void sendData(uint16_t onemark, uint32_t onespace, uint16_t zeromark,
                uint32_t zerospace, uint64_t data, uint16_t nbits,
                bool MSBfirst = true);
  void sendGeneric(const uint16_t headermark, const uint32_t headerspace,
                   const uint16_t onemark, const uint32_t onespace,
                   const uint16_t zeromark, const uint32_t zerospace,
                   const uint16_t footermark, const uint32_t gap,
                   const uint64_t data, const uint16_t nbits,
                   const uint16_t frequency, const bool MSBfirst,
                   const uint16_t repeat, const uint8_t dutycycle);
  void sendGeneric(const uint16_t headermark, const uint32_t headerspace,
                   const uint16_t onemark, const uint32_t onespace,
                   const uint16_t zeromark, const uint32_t zerospace,
                   const uint16_t footermark, const uint32_t gap,
                   const uint32_t mesgtime,
                   const uint64_t data, const uint16_t nbits,
                   const uint16_t frequency, const bool MSBfirst,
                   const uint16_t repeat, const uint8_t dutycycle);
  void sendGeneric(const uint16_t headermark, const uint32_t headerspace,
                   const uint16_t onemark, const uint32_t onespace,
                   const uint16_t zeromark, const uint32_t zerospace,
                   const uint16_t footermark, const uint32_t gap,
                   const uint8_t *dataptr, const uint16_t nbytes,
                   const uint16_t frequency, const bool MSBfirst,
                   const uint16_t repeat, const uint8_t dutycycle);
void send(uint16_t type, uint64_t data, uint16_t nbits);
#if (SEND_NEC || SEND_SHERWOOD || SEND_AIWA_RC_T501 || SEND_SANYO)
  void sendNEC(uint64_t data, uint16_t nbits = NEC_BITS, uint16_t repeat = 0);
  uint32_t encodeNEC(uint16_t address, uint16_t command);
#endif
#if SEND_SONY
  // sendSony() should typically be called with repeat=2 as Sony devices
  // expect the code to be sent at least 3 times. (code + 2 repeats = 3 codes)
  // Legacy use of this procedure was to only send a single code so call it with
  // repeat=0 for backward compatibility. As of v2.0 it defaults to sending
  // a Sony command that will be accepted be a device.
  void sendSony(uint64_t data, uint16_t nbits = SONY_20_BITS,
                uint16_t repeat = SONY_MIN_REPEAT);
  uint32_t encodeSony(uint16_t nbits, uint16_t command, uint16_t address,
                      uint16_t extended = 0);
#endif
#if SEND_SHERWOOD
  void sendSherwood(uint64_t data, uint16_t nbits = SHERWOOD_BITS,
                    uint16_t repeat = SHERWOOD_MIN_REPEAT);
#endif
#if SEND_SAMSUNG
  void sendSAMSUNG(uint64_t data, uint16_t nbits = SAMSUNG_BITS,
                   uint16_t repeat = 0);
  uint32_t encodeSAMSUNG(uint8_t customer, uint8_t command);
#endif
#if SEND_LG
  void sendLG(uint64_t data, uint16_t nbits = LG_BITS, uint16_t repeat = 0);
  uint32_t encodeLG(uint16_t address, uint16_t command);
#endif
#if (SEND_SHARP || SEND_DENON)
  uint32_t encodeSharp(uint16_t address, uint16_t command,
                       uint16_t expansion = 1, uint16_t check = 0,
                       bool MSBfirst = false);
  void sendSharp(uint16_t address, uint16_t command,
                 uint16_t nbits = SHARP_BITS, uint16_t repeat = 0);
  void sendSharpRaw(uint64_t data, uint16_t nbits = SHARP_BITS,
                    uint16_t repeat = 0);
#endif
#if SEND_JVC
  void sendJVC(uint64_t data, uint16_t nbits = JVC_BITS, uint16_t repeat = 0);
  uint16_t encodeJVC(uint8_t address, uint8_t command);
#endif
#if SEND_DENON
  void sendDenon(uint64_t data, uint16_t nbits = DENON_BITS,
                 uint16_t repeat = 0);
#endif
#if SEND_SANYO
  uint64_t encodeSanyoLC7461(uint16_t address, uint8_t command);
  void sendSanyoLC7461(uint64_t data, uint16_t nbits = SANYO_LC7461_BITS,
                       uint16_t repeat = 0);
#endif
#if SEND_DISH
  // sendDISH() should typically be called with repeat=3 as DISH devices
  // expect the code to be sent at least 4 times. (code + 3 repeats = 4 codes)
  // Legacy use of this procedure was only to send a single code
  // so use repeat=0 for backward compatibility.
  void sendDISH(uint64_t data, uint16_t nbits = DISH_BITS,
                uint16_t repeat = DISH_MIN_REPEAT);
#endif
#if (SEND_PANASONIC || SEND_DENON)
  void sendPanasonic64(uint64_t data, uint16_t nbits = PANASONIC_BITS,
                       uint16_t repeat = 0);
  void sendPanasonic(uint16_t address, uint32_t data,
                     uint16_t nbits = PANASONIC_BITS, uint16_t repeat = 0);
  uint64_t encodePanasonic(uint16_t manufacturer, uint8_t device,
                           uint8_t subdevice, uint8_t function);
#endif
#if SEND_RC5
  void sendRC5(uint64_t data, uint16_t nbits = RC5X_BITS, uint16_t repeat = 0);
  uint16_t encodeRC5(uint8_t address, uint8_t command,
                     bool key_released = false);
  uint16_t encodeRC5X(uint8_t address, uint8_t command,
                      bool key_released = false);
  uint64_t toggleRC5(uint64_t data);
#endif
#if SEND_RC6
  void sendRC6(uint64_t data, uint16_t nbits = RC6_MODE0_BITS,
               uint16_t repeat = 0);
  uint64_t encodeRC6(uint32_t address, uint8_t command,
                     uint16_t mode = RC6_MODE0_BITS);
  uint64_t toggleRC6(uint64_t data, uint16_t nbits = RC6_MODE0_BITS);
#endif
#if SEND_RCMM
  void sendRCMM(uint64_t data, uint16_t nbits = RCMM_BITS, uint16_t repeat = 0);
#endif
#if SEND_COOLIX
  void sendCOOLIX(uint64_t data, uint16_t nbits = COOLIX_BITS,
                  uint16_t repeat = 0);
#endif
#if SEND_WHYNTER
  void sendWhynter(uint64_t data, uint16_t nbits = WHYNTER_BITS,
                   uint16_t repeat = 0);
#endif
#if SEND_MITSUBISHI
  void sendMitsubishi(uint64_t data, uint16_t nbits = MITSUBISHI_BITS,
                      uint16_t repeat = MITSUBISHI_MIN_REPEAT);
#endif
#if SEND_MITSUBISHI2
  void sendMitsubishi2(uint64_t data, uint16_t nbits = MITSUBISHI_BITS,
                       uint16_t repeat = MITSUBISHI_MIN_REPEAT);
#endif
#if SEND_MITSUBISHI_AC
  void sendMitsubishiAC(unsigned char data[],
                        uint16_t nbytes = MITSUBISHI_AC_STATE_LENGTH,
                        uint16_t repeat = MITSUBISHI_AC_MIN_REPEAT);
#endif
#if SEND_FUJITSU_AC
  void sendFujitsuAC(unsigned char data[],
                     uint16_t nbytes,
                     uint16_t repeat = FUJITSU_AC_MIN_REPEAT);
#endif
#if SEND_GLOBALCACHE
  void sendGC(uint16_t buf[], uint16_t len);
#endif
#if SEND_KELVINATOR
  void sendKelvinator(unsigned char data[],
                      uint16_t nbytes = KELVINATOR_STATE_LENGTH,
                      uint16_t repeat = 0);
#endif
#if SEND_DAIKIN
  void sendDaikin(unsigned char data[],
                  uint16_t nbytes = DAIKIN_COMMAND_LENGTH,
                  uint16_t repeat = 0);
  void sendDaikinGapHeader();
#endif
#if SEND_AIWA_RC_T501
  void sendAiwaRCT501(uint64_t data, uint16_t nbits = AIWA_RC_T501_BITS,
                      uint16_t repeat = AIWA_RC_T501_MIN_REPEAT);
#endif
#if SEND_GREE
  void sendGree(uint64_t data, uint16_t nbits = GREE_BITS, uint16_t repeat = 0);
  void sendGree(uint8_t data[], uint16_t nbytes = GREE_STATE_LENGTH,
                uint16_t repeat = 0);
#endif
#if SEND_PRONTO
  void sendPronto(uint16_t data[], uint16_t len, uint16_t repeat = 0);
#endif
#if SEND_ARGO
  void sendArgo(unsigned char data[],
                uint16_t nbytes = ARGO_COMMAND_LENGTH,
                uint16_t repeat = 0);
#endif
#if SEND_TROTEC
  void sendTrotec(unsigned char data[],
                  uint16_t nbytes = TROTEC_COMMAND_LENGTH,
                  uint16_t repeat = 0);
#endif
#if SEND_NIKAI
  void sendNikai(uint64_t data, uint16_t nbits = NIKAI_BITS,
                 uint16_t repeat = 0);
#endif
#if SEND_TOSHIBA_AC
  void sendToshibaAC(unsigned char data[],
                     uint16_t nbytes = TOSHIBA_AC_STATE_LENGTH,
                     uint16_t repeat = TOSHIBA_AC_MIN_REPEAT);
#endif
#if SEND_MIDEA
  void sendMidea(uint64_t data, uint16_t nbits = MIDEA_BITS,
                 uint16_t repeat = MIDEA_MIN_REPEAT);
#endif
#if SEND_MAGIQUEST
  void sendMagiQuest(uint64_t data, uint16_t nbits = MAGIQUEST_BITS,
                     uint16_t repeat = 0);
  uint64_t encodeMagiQuest(uint32_t wand_id, uint16_t magnitude);
#endif
#if SEND_LASERTAG
  void sendLasertag(uint64_t data, uint16_t nbits = LASERTAG_BITS,
                    uint16_t repeat = LASERTAG_MIN_REPEAT);
#endif
#if SEND_CARRIER_AC
  void sendCarrierAC(uint64_t data, uint16_t nbits = CARRIER_AC_BITS,
                     uint16_t repeat = CARRIER_AC_MIN_REPEAT);
#endif
#if SEND_HAIER_AC
  void sendHaierAC(unsigned char data[],
                   uint16_t nbytes = HAIER_AC_STATE_LENGTH,
                   uint16_t repeat = 0);
#endif
#if SEND_HITACHI_AC
  void sendHitachiAC(unsigned char data[],
                     uint16_t nbytes = HITACHI_AC_STATE_LENGTH,
                     uint16_t repeat = 0);
#endif
#if SEND_HITACHI_AC1
  void sendHitachiAC1(unsigned char data[],
                      uint16_t nbytes = HITACHI_AC1_STATE_LENGTH,
                      uint16_t repeat = 0);
#endif
#if SEND_HITACHI_AC2
  void sendHitachiAC2(unsigned char data[],
                      uint16_t nbytes = HITACHI_AC2_STATE_LENGTH,
                      uint16_t repeat = 0);
#endif
#if SEND_GICABLE
  void sendGICable(uint64_t data, uint16_t nbits = GICABLE_BITS,
                   uint16_t repeat = GICABLE_MIN_REPEAT);
#endif

 protected:
#ifdef UNIT_TEST
#ifndef HIGH
#define HIGH 0x1
#endif
#ifndef LOW
#define LOW 0x0
#endif
#endif  // UNIT_TEST
  uint8_t outputOn;
  uint8_t outputOff;
  VIRTUAL void ledOff();
  VIRTUAL void ledOn();

 private:
  uint16_t onTimePeriod;
  uint16_t offTimePeriod;
  uint16_t IRpin;
  int8_t periodOffset;
  uint8_t _dutycycle;
  bool modulation;
  uint32_t calcUSecPeriod(uint32_t hz, bool use_offset = true);
};

#endif  // IRSEND_H_
