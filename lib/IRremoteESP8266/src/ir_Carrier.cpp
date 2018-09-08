// Copyright 2018 David Conran

#include "IRrecv.h"
#include "IRsend.h"
#include "IRutils.h"

//             CCCCC    AAA   RRRRRR  RRRRRR  IIIII EEEEEEE RRRRRR
//            CC    C  AAAAA  RR   RR RR   RR  III  EE      RR   RR
//            CC      AA   AA RRRRRR  RRRRRR   III  EEEEE   RRRRRR
//            CC    C AAAAAAA RR  RR  RR  RR   III  EE      RR  RR
//             CCCCC  AA   AA RR   RR RR   RR IIIII EEEEEEE RR   RR

// Suits Carrier/Surrey HVAC models:
//   42QG5A55970 (remote)
//   619EGX0090E0 / 619EGX0120E0 / 619EGX0180E0 / 619EGX0220E0 (indoor units)
//   53NGK009/012 (inverter)

// Constants
// Ref:
//   https://github.com/markszabo/IRremoteESP8266/issues/385
#define CARRIER_AC_HDR_MARK           8532U
#define CARRIER_AC_HDR_SPACE          4228U
#define CARRIER_AC_BIT_MARK            628U
#define CARRIER_AC_ONE_SPACE          1320U
#define CARRIER_AC_ZERO_SPACE          532U
#define CARRIER_AC_GAP               20000U

#if SEND_CARRIER_AC
// Send a Carrier HVAC formatted message.
//
// Args:
//   data:   The message to be sent.
//   nbits:  The bit size of the message being sent. typically CARRIER_AC_BITS.
//   repeat: The number of times the message is to be repeated.
//
// Status: BETA / Appears to work on real devices.
//
void IRsend::sendCarrierAC(uint64_t data, uint16_t nbits, uint16_t repeat) {
  for (uint16_t r = 0; r <= repeat; r++) {
    uint64_t temp_data = data;
    // Carrier sends the data block three times. normal + inverted + normal.
    for (uint16_t i = 0; i < 3; i++) {
      sendGeneric(CARRIER_AC_HDR_MARK, CARRIER_AC_HDR_SPACE,
        CARRIER_AC_BIT_MARK, CARRIER_AC_ONE_SPACE,
        CARRIER_AC_BIT_MARK, CARRIER_AC_ZERO_SPACE,
        CARRIER_AC_BIT_MARK, CARRIER_AC_GAP,
        temp_data, nbits, 38, true, 0, 50);
      temp_data = invertBits(temp_data, nbits);
    }
  }
}
#endif

#if DECODE_CARRIER_AC
// Decode the supplied Carrier HVAC message.
// Carrier HVAC messages contain only 32 bits, but it is sent three(3) times.
// i.e. normal + inverted + normal
// Args:
//   results: Ptr to the data to decode and where to store the decode result.
//   nbits:   Nr. of bits to expect in the data portion.
//            Typically CARRIER_AC_BITS.
//   strict:  Flag to indicate if we strictly adhere to the specification.
// Returns:
//   boolean: True if it can decode it, false if it can't.
//
// Status: ALPHA / Untested.
//
bool IRrecv::decodeCarrierAC(decode_results *results, uint16_t nbits,
                           bool strict) {
  if (results->rawlen < ((2 * nbits + HEADER + FOOTER) * 3) - 1)
    return false;  // Can't possibly be a valid Carrier message.
  if (strict && nbits != CARRIER_AC_BITS)
    return false;  // We expect Carrier to be 32 bits of message.

  uint64_t data = 0;
  uint64_t prev_data = 0;
  uint16_t offset = OFFSET_START;

  for (uint8_t i = 0; i < 3; i++) {
    prev_data = data;
    // Header
    if (!matchMark(results->rawbuf[offset++], CARRIER_AC_HDR_MARK))
      return false;
    if (!matchSpace(results->rawbuf[offset++], CARRIER_AC_HDR_SPACE))
      return false;
    // Data
    match_result_t data_result = matchData(&(results->rawbuf[offset]), nbits,
                                           CARRIER_AC_BIT_MARK,
                                           CARRIER_AC_ONE_SPACE,
                                           CARRIER_AC_BIT_MARK,
                                           CARRIER_AC_ZERO_SPACE);
    if (data_result.success == false) return false;
    data = data_result.data;
    offset += data_result.used;
    // Footer
    if (!matchMark(results->rawbuf[offset++], CARRIER_AC_BIT_MARK))
      return false;
    if (offset < results->rawlen &&
      !matchAtLeast(results->rawbuf[offset++], CARRIER_AC_GAP))
      return false;
    // Compliance.
    if (strict) {
      // Check if the data is an inverted copy of the previous data.
      if (i > 0 && prev_data != invertBits(data, nbits))  return false;
    }
  }

  // Success
  results->bits = nbits;
  results->value = data;
  results->decode_type = CARRIER_AC;
  results->address = data >> 16;
  results->command = data & 0xFFFF;
  return true;
}
#endif
