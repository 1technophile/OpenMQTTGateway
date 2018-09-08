// Copyright 2013 mpflaga
// Copyright 2015 kitlaan
// Copyright 2017 Jason kendall, David Conran

#ifndef IR_MAGIQUEST_H_
#define IR_MAGIQUEST_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include "IRremoteESP8266.h"
#include "IRsend.h"

// MagiQuest packet is both Wand ID and magnitude of swish and flick
union magiquest {
  uint64_t llword;
  uint8_t    byte[8];
//  uint16_t   word[4];
  uint32_t  lword[2];
  struct {
    uint16_t magnitude;
    uint32_t wand_id;
    uint8_t  padding;
    uint8_t  scrap;
  } cmd;
};

#define MAGIQUEST_TOTAL_USEC   1150U
#define MAGIQUEST_ZERO_RATIO     30U  // usually <= ~25%
#define MAGIQUEST_ONE_RATIO      38U  // usually >= ~50%

#define MAGIQUEST_PERIOD       1150U
#define MAGIQUEST_MARK_ZERO     280U
#define MAGIQUEST_SPACE_ZERO    850U
#define MAGIQUEST_MARK_ONE      580U
#define MAGIQUEST_SPACE_ONE     600U
#define MAGIQUEST_GAP        100000UL  // A guess of the gap between messages
#endif  // IR_MAGIQUEST_H_
