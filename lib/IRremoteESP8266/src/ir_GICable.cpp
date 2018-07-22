// Copyright 2018 David Conran

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <algorithm>
#include "IRrecv.h"
#include "IRsend.h"
#include "IRutils.h"

//              GGGG      IIIII       CCCCC    AAA   BBBBB   LL      EEEEEEE
//             GG  GG      III       CC    C  AAAAA  BB   B  LL      EE
//            GG           III       CC      AA   AA BBBBBB  LL      EEEEE
//            GG   GG ...  III  ...  CC    C AAAAAAA BB   BB LL      EE
//             GGGGGG ... IIIII ...   CCCCC  AA   AA BBBBBB  LLLLLLL EEEEEEE
//
// Ref:
//   https://github.com/cyborg5/IRLib2/blob/master/IRLibProtocols/IRLib_P09_GICable.h
//   https://github.com/markszabo/IRremoteESP8266/issues/447

// Constants
#define GICABLE_HDR_MARK                9000U
#define GICABLE_HDR_SPACE               4400U
#define GICABLE_BIT_MARK                 550U
#define GICABLE_ONE_SPACE               4400U
#define GICABLE_ZERO_SPACE              2200U
#define GICABLE_RPT_SPACE               2200U
#define GICABLE_MIN_COMMAND_LENGTH     99600U
#define GICABLE_MIN_GAP (GICABLE_MIN_COMMAND_LENGTH - \
    (GICABLE_HDR_MARK + GICABLE_HDR_SPACE + \
     GICABLE_BITS * (GICABLE_BIT_MARK + GICABLE_ONE_SPACE) + GICABLE_BIT_MARK))


#if SEND_GICABLE
// Send a raw G.I. Cable formatted message.
//
// Args:
//   data:   The message to be sent.
//   nbits:  The number of bits of the message to be sent.
//           Typically GICABLE_BITS.
//   repeat: The number of times the command is to be repeated.
//
// Status: Alpha / Untested.
//
// Ref:
void IRsend::sendGICable(uint64_t data, uint16_t nbits, uint16_t repeat) {
  sendGeneric(GICABLE_HDR_MARK, GICABLE_HDR_SPACE,
              GICABLE_BIT_MARK, GICABLE_ONE_SPACE,
              GICABLE_BIT_MARK, GICABLE_ZERO_SPACE,
              GICABLE_BIT_MARK, GICABLE_MIN_GAP, GICABLE_MIN_COMMAND_LENGTH,
              data, nbits, 39, true, 0,  // Repeats are handled later.
              50);
  // Message repeat sequence.
  if (repeat)
    sendGeneric(GICABLE_HDR_MARK, GICABLE_RPT_SPACE,
                0, 0, 0, 0,  // No actual data sent.
                GICABLE_BIT_MARK, GICABLE_MIN_GAP, GICABLE_MIN_COMMAND_LENGTH,
                0, 0,  // No data to be sent.
                39, true, repeat - 1, 50);
}
#endif  // SEND_GICABLE

#if DECODE_GICABLE
// Decode the supplied G.I. Cable message.
//
// Args:
//   results: Ptr to the data to decode and where to store the decode result.
//   nbits:   The number of data bits to expect. Typically GICABLE_BITS.
//   strict:  Flag indicating if we should perform strict matching.
// Returns:
//   boolean: True if it can decode it, false if it can't.
//
// Status: Alpha / Not tested against a real device.
bool IRrecv::decodeGICable(decode_results *results, uint16_t nbits,
                           bool strict) {
  if (results->rawlen < 2 * (nbits + HEADER + FOOTER) - 1)
    return false;  // Can't possibly be a valid GICABLE message.
  if (strict && nbits != GICABLE_BITS)
    return false;  // Not strictly an GICABLE message.

  uint64_t data = 0;
  uint16_t offset = OFFSET_START;

  // Header
  if (!matchMark(results->rawbuf[offset++], GICABLE_HDR_MARK)) return false;
  if (!matchSpace(results->rawbuf[offset++], GICABLE_HDR_SPACE)) return false;

  // Data
  match_result_t data_result = matchData(&(results->rawbuf[offset]), nbits,
                                         GICABLE_BIT_MARK,
                                         GICABLE_ONE_SPACE,
                                         GICABLE_BIT_MARK,
                                         GICABLE_ZERO_SPACE);
  if (data_result.success == false) return false;
  data = data_result.data;
  offset += data_result.used;

  // Footer
  if (!matchMark(results->rawbuf[offset++], GICABLE_BIT_MARK)) return false;
  if (offset < results->rawlen &&
      !matchAtLeast(results->rawbuf[offset++], GICABLE_MIN_GAP))
    return false;

  // Compliance
  if (strict) {
    // We expect a repeat frame.
    if (!matchMark(results->rawbuf[offset++], GICABLE_HDR_MARK)) return false;
    if (!matchSpace(results->rawbuf[offset++], GICABLE_RPT_SPACE)) return false;
    if (!matchMark(results->rawbuf[offset++], GICABLE_BIT_MARK)) return false;
  }

  // Success
  results->bits = nbits;
  results->value = data;
  results->decode_type = GICABLE;
  results->command = 0;
  results->address = 0;
  return true;
}
#endif  // DECODE_GICABLE
