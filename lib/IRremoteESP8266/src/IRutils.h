#ifndef IRUTILS_H_
#define IRUTILS_H_

// Copyright 2017 David Conran

#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#ifndef ARDUINO
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRrecv.h"

uint64_t reverseBits(uint64_t input, uint16_t nbits);
String uint64ToString(uint64_t input, uint8_t base = 10);
String typeToString(const decode_type_t protocol,
                    const bool isRepeat = false);
void serialPrintUint64(uint64_t input, uint8_t base = 10);
String resultToSourceCode(const decode_results * const results);
String resultToTimingInfo(const decode_results * const results);
String resultToHumanReadableBasic(const decode_results * const results);
String resultToHexidecimal(const decode_results * const result);
String htmlEscape(const String unescaped);
bool hasACState(const decode_type_t protocol);
uint16_t getCorrectedRawLength(const decode_results * const results);
uint16_t * resultToRawArray(const decode_results * const decode);
uint8_t sumBytes(const uint8_t * const start, const uint16_t length,
                 const uint8_t init = 0);
uint8_t xorBytes(const uint8_t * const start, const uint16_t length,
                 const uint8_t init = 0);
uint16_t countBits(const uint8_t * const start, const uint16_t length,
                   const bool ones = true, const uint16_t init = 0);
uint16_t countBits(const uint64_t data, const uint8_t length,
                   const bool ones = true, const uint16_t init = 0);
uint64_t invertBits(const uint64_t data, const uint16_t nbits);
decode_type_t strToDecodeType(const char *str);
float celsiusToFahrenheit(const float deg);
float fahrenheitToCelsius(const float deg);
#endif  // IRUTILS_H_
