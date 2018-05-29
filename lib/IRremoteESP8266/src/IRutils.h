#ifndef IRUTILS_H_
#define IRUTILS_H_

// Copyright 2017 David Conran

#define __STDC_LIMIT_MACROS
#include <stdint.h>

uint64_t reverseBits(uint64_t input, uint16_t nbits);
void serialPrintUint64(uint64_t input, uint8_t base);

#endif  // IRUTILS_H_
