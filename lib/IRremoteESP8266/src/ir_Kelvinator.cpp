// Copyright 2016 David Conran
//
// Code to emulate IR Kelvinator YALIF remote control unit, which should control
// at least the following Kelvinator A/C units:
// KSV26CRC, KSV26HRC, KSV35CRC, KSV35HRC, KSV53HRC, KSV62HRC, KSV70CRC,
// KSV70HRC, KSV80HRC.
//
// Note:
// * Unsupported:
//    - All Sleep modes.
//    - All Timer modes.
//    - "I Feel" button & mode.
//    - Energy Saving mode.
//    - Low Heat mode.
//    - Fahrenheit.

#include "ir_Kelvinator.h"
#include <algorithm>
#ifndef ARDUINO
#include <string>
#endif
#include "IRrecv.h"
#include "IRsend.h"
#include "IRutils.h"

// KK  KK EEEEEEE LL     VV     VV IIIII NN   NN   AAA   TTTTTTT  OOOOO  RRRRRR
// KK KK  EE      LL     VV     VV  III  NNN  NN  AAAAA    TTT   OO   OO RR   RR
// KKKK   EEEEE   LL      VV   VV   III  NN N NN AA   AA   TTT   OO   OO RRRRRR
// KK KK  EE      LL       VV VV    III  NN  NNN AAAAAAA   TTT   OO   OO RR  RR
// KK  KK EEEEEEE LLLLLLL   VVV    IIIII NN   NN AA   AA   TTT    OOOO0  RR   RR

// Constants
#define KELVINATOR_TICK                       85U
#define KELVINATOR_HDR_MARK_TICKS            106U
#define KELVINATOR_HDR_MARK      (KELVINATOR_HDR_MARK_TICKS * KELVINATOR_TICK)
#define KELVINATOR_HDR_SPACE_TICKS            53U
#define KELVINATOR_HDR_SPACE     (KELVINATOR_HDR_SPACE_TICKS * KELVINATOR_TICK)
#define KELVINATOR_BIT_MARK_TICKS              8U
#define KELVINATOR_BIT_MARK      (KELVINATOR_BIT_MARK_TICKS * KELVINATOR_TICK)
#define KELVINATOR_ONE_SPACE_TICKS            18U
#define KELVINATOR_ONE_SPACE     (KELVINATOR_ONE_SPACE_TICKS * KELVINATOR_TICK)
#define KELVINATOR_ZERO_SPACE_TICKS            6U
#define KELVINATOR_ZERO_SPACE    (KELVINATOR_ZERO_SPACE_TICKS * KELVINATOR_TICK)
#define KELVINATOR_GAP_SPACE_TICKS           235U
#define KELVINATOR_GAP_SPACE     (KELVINATOR_GAP_SPACE_TICKS * KELVINATOR_TICK)
#define KELVINATOR_CMD_FOOTER                  2U
#define KELVINATOR_CMD_FOOTER_BITS             3U

#define KELVINATOR_POWER                       8U
#define KELVINATOR_MODE_MASK                0xF8U
#define KELVINATOR_FAN_OFFSET                  4U
#define KELVINATOR_BASIC_FAN_MASK uint8_t(0xFFU ^ (3U << KELVINATOR_FAN_OFFSET))
#define KELVINATOR_FAN_MASK uint8_t(0xFFU ^ (7U << KELVINATOR_FAN_OFFSET))
#define KELVINATOR_CHECKSUM_START             10U
#define KELVINATOR_VENT_SWING_OFFSET           6U
#define KELVINATOR_VENT_SWING uint8_t(1U << KELVINATOR_VENT_SWING_OFFSET)
#define KELVINATOR_VENT_SWING_V       uint8_t(1U)
#define KELVINATOR_VENT_SWING_H  uint8_t(1U << 4)
#define KELVINATOR_SLEEP_1_AND_3 uint8_t(1U << 7)
#define KELVINATOR_QUIET_OFFSET                7U
#define KELVINATOR_QUIET uint8_t(1U << KELVINATOR_QUIET_OFFSET)
#define KELVINATOR_ION_FILTER_OFFSET           6U
#define KELVINATOR_ION_FILTER uint8_t(1U << KELVINATOR_ION_FILTER_OFFSET)
#define KELVINATOR_LIGHT_OFFSET                5U
#define KELVINATOR_LIGHT uint8_t(1U << KELVINATOR_LIGHT_OFFSET)
#define KELVINATOR_XFAN_OFFSET                 7U
#define KELVINATOR_XFAN uint8_t(1U << KELVINATOR_XFAN_OFFSET)
#define KELVINATOR_TURBO_OFFSET                4U
#define KELVINATOR_TURBO uint8_t(1U << KELVINATOR_TURBO_OFFSET)

#if SEND_KELVINATOR
// Send a Kelvinator A/C message.
//
// Args:
//   data: An array of bytes containing the IR command.
//   nbytes: Nr. of bytes of data in the array. (>=KELVINATOR_STATE_LENGTH)
//   repeat: Nr. of times the message is to be repeated. (Default = 0).
//
// Status: STABLE / Known working.
//
void IRsend::sendKelvinator(unsigned char data[], uint16_t nbytes,
                            uint16_t repeat) {
  if (nbytes < KELVINATOR_STATE_LENGTH)
    return;  // Not enough bytes to send a proper message.

  for (uint16_t r = 0; r <= repeat; r++) {
    // Command Block #1 (4 bytes)
    sendGeneric(KELVINATOR_HDR_MARK, KELVINATOR_HDR_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE,
                0, 0,  // No Footer yet.
                data, 4, 38, false, 0, 50);
    // Send Footer for the command block (3 bits (B010))
    sendGeneric(0, 0,  // No Header
                KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_GAP_SPACE,
                KELVINATOR_CMD_FOOTER, KELVINATOR_CMD_FOOTER_BITS,
                38, false, 0, 50);
    // Data Block #1 (4 bytes)
    sendGeneric(0, 0,  // No header
                KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_GAP_SPACE * 2,
                data + 4, 4, 38, false, 0, 50);
    // Command Block #2 (4 bytes)
    sendGeneric(KELVINATOR_HDR_MARK, KELVINATOR_HDR_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE,
                0, 0,  // No Footer yet.
                data + 8, 4, 38, false, 0, 50);
    // Send Footer for the command block (3 bits (B010))
    sendGeneric(0, 0,  // No Header
                KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_GAP_SPACE,
                KELVINATOR_CMD_FOOTER, KELVINATOR_CMD_FOOTER_BITS,
                38, false, 0, 50);
    // Data Block #2 (4 bytes)
    sendGeneric(0, 0,  // No header
                KELVINATOR_BIT_MARK, KELVINATOR_ONE_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_ZERO_SPACE,
                KELVINATOR_BIT_MARK, KELVINATOR_GAP_SPACE * 2,
                data + 12, 4, 38, false, 0, 50);
  }
}
#endif  // SEND_KELVINATOR

IRKelvinatorAC::IRKelvinatorAC(uint16_t pin) : _irsend(pin) {
  stateReset();
}

void IRKelvinatorAC::stateReset() {
  for (uint8_t i = 0; i < KELVINATOR_STATE_LENGTH; i++)
    remote_state[i] = 0x0;
  remote_state[3] = 0x50;
  remote_state[11] = 0x70;
}

void IRKelvinatorAC::begin() {
  _irsend.begin();
}

void IRKelvinatorAC::fixup() {
  // X-Fan mode is only valid in COOL or DRY modes.
  if (getMode() != KELVINATOR_COOL && getMode() != KELVINATOR_DRY)
    setXFan(false);
  checksum();  // Calculate the checksums
}

#if SEND_KELVINATOR
void IRKelvinatorAC::send() {
  fixup();   // Ensure correct settings before sending.
  _irsend.sendKelvinator(remote_state);
}
#endif  // SEND_KELVINATOR

uint8_t* IRKelvinatorAC::getRaw() {
  fixup();   // Ensure correct settings before sending.
  return remote_state;
}

void IRKelvinatorAC::setRaw(uint8_t new_code[]) {
  for (uint8_t i = 0; i < KELVINATOR_STATE_LENGTH; i++) {
    remote_state[i] = new_code[i];
  }
}

uint8_t IRKelvinatorAC::calcBlockChecksum(const uint8_t *block,
                                          const uint16_t length) {
  uint8_t sum = KELVINATOR_CHECKSUM_START;
  // Sum the lower half of the first 4 bytes of this block.
  for (uint8_t i = 0; i < 4 && i < length - 1; i++, block++)
    sum += (*block & 0x0FU);
    // then sum the upper half of the next 3 bytes.
  for (uint8_t i = 4; i < length - 1; i++, block++)
    sum += (*block >> 4);
  // Trim it down to fit into the 4 bits allowed. i.e. Mod 16.
  return sum & 0x0FU;
}

// Many Bothans died to bring us this information.
void IRKelvinatorAC::checksum(const uint16_t length) {
  // For each command + options block.
  for (uint16_t offset = 0; offset + 7 < length; offset += 8) {
    uint8_t sum = calcBlockChecksum(remote_state + offset);
    remote_state[7 + offset] = (sum << 4) | (remote_state[7 + offset] & 0xFU);
  }
}

// Verify the checksum is valid for a given state.
// Args:
//   state:  The array to verify the checksum of.
//   length: The size of the state.
// Returns:
//   A boolean.
bool IRKelvinatorAC::validChecksum(const uint8_t state[],
                                   const uint16_t length) {
  for (uint16_t offset = 0; offset + 7 < length; offset += 8) {
    // Top 4 bits of the last byte in the block is the block's checksum.
    if (state[offset + 7] >> 4 != calcBlockChecksum(state + offset))
      return false;
  }
  return true;
}

void IRKelvinatorAC::on() {
  remote_state[0] |= KELVINATOR_POWER;
  remote_state[8] = remote_state[0];  // Duplicate to the 2nd command chunk.
}

void IRKelvinatorAC::off() {
  remote_state[0] &= ~KELVINATOR_POWER;
  remote_state[8] = remote_state[0];  // Duplicate to the 2nd command chunk.
}

void IRKelvinatorAC::setPower(bool state) {
  if (state)
    on();
  else
    off();
}

bool IRKelvinatorAC::getPower() {
  return ((remote_state[0] & KELVINATOR_POWER) != 0);
}

// Set the temp. in deg C
void IRKelvinatorAC::setTemp(uint8_t temp) {
  temp = std::max((uint8_t) KELVINATOR_MIN_TEMP, temp);
  temp = std::min((uint8_t) KELVINATOR_MAX_TEMP, temp);
  remote_state[1] = (remote_state[1] & 0xF0U) | (temp - KELVINATOR_MIN_TEMP);
  remote_state[9] = remote_state[1];  // Duplicate to the 2nd command chunk.
}

// Return the set temp. in deg C
uint8_t IRKelvinatorAC::getTemp() {
  return ((remote_state[1] & 0xFU) + KELVINATOR_MIN_TEMP);
}

// Set the speed of the fan, 0-5, 0 is auto, 1-5 is the speed
void IRKelvinatorAC::setFan(uint8_t fan) {
  fan = std::min((uint8_t) KELVINATOR_FAN_MAX, fan);  // Bounds check

  // Only change things if we need to.
  if (fan != getFan()) {
    // Set the basic fan values.
    uint8_t fan_basic = std::min((uint8_t) KELVINATOR_BASIC_FAN_MAX, fan);
    remote_state[0] = (remote_state[0] & KELVINATOR_BASIC_FAN_MASK) |
        (fan_basic << KELVINATOR_FAN_OFFSET);
    remote_state[8] = remote_state[0];  // Duplicate to the 2nd command chunk.
    // Set the advanced(?) fan value.
    remote_state[14] = (remote_state[14] & KELVINATOR_FAN_MASK) |
        (fan << KELVINATOR_FAN_OFFSET);
    setTurbo(false);  // Turbo mode is turned off if we change the fan settings.
  }
}

uint8_t IRKelvinatorAC::getFan() {
  return ((remote_state[14] & ~KELVINATOR_FAN_MASK) >> KELVINATOR_FAN_OFFSET);
}

uint8_t IRKelvinatorAC::getMode() {
  return (remote_state[0] & ~KELVINATOR_MODE_MASK);
}

void IRKelvinatorAC::setMode(uint8_t mode) {
  // If we get an unexpected mode, default to AUTO.
  if (mode > KELVINATOR_HEAT) mode = KELVINATOR_AUTO;
  remote_state[0] = (remote_state[0] & KELVINATOR_MODE_MASK) | mode;
  remote_state[8] = remote_state[0];  // Duplicate to the 2nd command chunk.
  if (mode == KELVINATOR_AUTO || KELVINATOR_DRY)
    // When the remote is set to Auto or Dry, it defaults to 25C and doesn't
    // show it.
    setTemp(KELVINATOR_AUTO_TEMP);
}

void IRKelvinatorAC::setSwingVertical(bool state) {
  if (state) {
    remote_state[0] |= KELVINATOR_VENT_SWING;
    remote_state[4] |= KELVINATOR_VENT_SWING_V;
  } else {
    remote_state[4] &= ~KELVINATOR_VENT_SWING_V;
    if (!getSwingHorizontal())
      remote_state[0] &= ~KELVINATOR_VENT_SWING;
  }
  remote_state[8] = remote_state[0];  // Duplicate to the 2nd command chunk.
}

bool IRKelvinatorAC::getSwingVertical() {
  return ((remote_state[4] & KELVINATOR_VENT_SWING_V) != 0);
}

void IRKelvinatorAC::setSwingHorizontal(bool state) {
  if (state) {
    remote_state[0] |= KELVINATOR_VENT_SWING;
    remote_state[4] |= KELVINATOR_VENT_SWING_H;
  } else {
    remote_state[4] &= ~KELVINATOR_VENT_SWING_H;
    if (!getSwingVertical())
      remote_state[0] &= ~KELVINATOR_VENT_SWING;
  }
  remote_state[8] = remote_state[0];  // Duplicate to the 2nd command chunk.
}

bool IRKelvinatorAC::getSwingHorizontal() {
  return ((remote_state[4] & KELVINATOR_VENT_SWING_H) != 0);
}

void IRKelvinatorAC::setQuiet(bool state) {
  remote_state[12] &= ~KELVINATOR_QUIET;
  remote_state[12] |= (state << KELVINATOR_QUIET_OFFSET);
}

bool IRKelvinatorAC::getQuiet() {
  return ((remote_state[12] & KELVINATOR_QUIET) != 0);
}

void IRKelvinatorAC::setIonFilter(bool state) {
  remote_state[2] &= ~KELVINATOR_ION_FILTER;
  remote_state[2] |= (state << KELVINATOR_ION_FILTER_OFFSET);
  remote_state[10] = remote_state[2];  // Duplicate to the 2nd command chunk.
}

bool IRKelvinatorAC::getIonFilter() {
  return ((remote_state[2] & KELVINATOR_ION_FILTER) != 0);
}

void IRKelvinatorAC::setLight(bool state) {
  remote_state[2] &= ~KELVINATOR_LIGHT;
  remote_state[2] |= (state << KELVINATOR_LIGHT_OFFSET);
  remote_state[10] = remote_state[2];  // Duplicate to the 2nd command chunk.
}

bool IRKelvinatorAC::getLight() {
  return ((remote_state[2] & KELVINATOR_LIGHT) != 0);
}

// Note: XFan mode is only valid in Cool or Dry mode.
void IRKelvinatorAC::setXFan(bool state) {
  remote_state[2] &= ~KELVINATOR_XFAN;
  remote_state[2] |= (state << KELVINATOR_XFAN_OFFSET);
  remote_state[10] = remote_state[2];  // Duplicate to the 2nd command chunk.
}

bool IRKelvinatorAC::getXFan() {
  return ((remote_state[2] & KELVINATOR_XFAN) != 0);
}

// Note: Turbo mode is turned off if the fan speed is changed.
void IRKelvinatorAC::setTurbo(bool state) {
  remote_state[2] &= ~KELVINATOR_TURBO;
  remote_state[2] |= (state << KELVINATOR_TURBO_OFFSET);
  remote_state[10] = remote_state[2];  // Duplicate to the 2nd command chunk.
}

bool IRKelvinatorAC::getTurbo() {
  return ((remote_state[2] & KELVINATOR_TURBO) != 0);
}

// Convert the internal state into a human readable string.
#ifdef ARDUINO
String IRKelvinatorAC::toString() {
  String result = "";
#else
std::string IRKelvinatorAC::toString() {
  std::string result = "";
#endif  // ARDUINO
  result += "Power: ";
  if (getPower())
    result += "On";
  else
    result += "Off";
  result += ", Mode: " + uint64ToString(getMode());
  switch (getMode()) {
    case KELVINATOR_AUTO:
      result += " (AUTO)";
      break;
    case KELVINATOR_COOL:
      result += " (COOL)";
      break;
    case KELVINATOR_HEAT:
      result += " (HEAT)";
      break;
    case KELVINATOR_DRY:
      result += " (DRY)";
      break;
    case KELVINATOR_FAN:
      result += " (FAN)";
      break;
    default:
      result += " (UNKNOWN)";
  }
  result += ", Temp: " + uint64ToString(getTemp()) + "C";
  result += ", Fan: " + uint64ToString(getFan());
  switch (getFan()) {
    case KELVINATOR_FAN_AUTO:
      result += " (AUTO)";
      break;
    case KELVINATOR_FAN_MAX:
      result += " (MAX)";
      break;
  }
  result += ", Turbo: ";
  if (getTurbo())
    result += "On";
  else
    result += "Off";
  result += ", Quiet: ";
  if (getQuiet())
    result += "On";
  else
    result += "Off";
  result += ", XFan: ";
  if (getXFan())
    result += "On";
  else
    result += "Off";
  result += ", IonFilter: ";
  if (getIonFilter())
    result += "On";
  else
    result += "Off";
  result += ", Light: ";
  if (getLight())
    result += "On";
  else
    result += "Off";
  result += ", Swing (Horizontal): ";
  if (getSwingHorizontal())
    result += "On";
  else
    result += "Off";
  result += ", Swing (Vertical): ";
  if (getSwingVertical())
    result += "On";
  else
    result += "Off";
  return result;
}

#if DECODE_KELVINATOR
// Decode the supplied Kelvinator message.
//
// Args:
//   results: Ptr to the data to decode and where to store the decode result.
//   nbits:   The number of data bits to expect. Typically KELVINATOR_BITS.
//   strict:  Flag indicating if we should perform strict matching.
// Returns:
//   boolean: True if it can decode it, false if it can't.
//
// Status: ALPHA / Untested.
bool IRrecv::decodeKelvinator(decode_results *results, uint16_t nbits,
                              bool strict) {
  if (results->rawlen < 2 * (nbits + KELVINATOR_CMD_FOOTER_BITS) +
                        (HEADER + FOOTER + 1) * 2 - 1)
    return false;  // Can't possibly be a valid Kelvinator message.
  if (strict && nbits != KELVINATOR_BITS)
    return false;  // Not strictly a Kelvinator message.

  uint32_t data;
  uint16_t offset = OFFSET_START;

  // There are two messages back-to-back in a full Kelvinator IR message
  // sequence.
  int8_t state_pos = 0;
  for (uint8_t s = 0; s < 2; s++) {
    match_result_t data_result;

    // Header
    if (!matchMark(results->rawbuf[offset], KELVINATOR_HDR_MARK)) return false;
    // Calculate how long the lowest tick time is based on the header mark.
    uint32_t mark_tick = results->rawbuf[offset++] * RAWTICK /
        KELVINATOR_HDR_MARK_TICKS;
    if (!matchSpace(results->rawbuf[offset], KELVINATOR_HDR_SPACE))
      return false;
    // Calculate how long the common tick time is based on the header space.
    uint32_t space_tick = results->rawbuf[offset++] * RAWTICK /
        KELVINATOR_HDR_SPACE_TICKS;

    // Data (Command) (32 bits)
    data_result = matchData(&(results->rawbuf[offset]), 32,
                            KELVINATOR_BIT_MARK_TICKS * mark_tick,
                            KELVINATOR_ONE_SPACE_TICKS * space_tick,
                            KELVINATOR_BIT_MARK_TICKS * mark_tick,
                            KELVINATOR_ZERO_SPACE_TICKS * space_tick);
    if (data_result.success == false) return false;
    data = data_result.data;
    offset += data_result.used;

    // Record command data in the state.
    for (int i = state_pos + 3; i >= state_pos; i--, data >>= 8)
      results->state[i] = reverseBits(data & 0xFF, 8);
    state_pos += 4;

    // Command data footer (3 bits, B010)
    data_result = matchData(&(results->rawbuf[offset]),
                            KELVINATOR_CMD_FOOTER_BITS,
                            KELVINATOR_BIT_MARK_TICKS * mark_tick,
                            KELVINATOR_ONE_SPACE_TICKS * space_tick,
                            KELVINATOR_BIT_MARK_TICKS * mark_tick,
                            KELVINATOR_ZERO_SPACE_TICKS * space_tick);
    if (data_result.success == false) return false;
    if (data_result.data != KELVINATOR_CMD_FOOTER) return false;
    offset += data_result.used;

    // Interdata gap.
    if (!matchMark(results->rawbuf[offset++],
                   KELVINATOR_BIT_MARK_TICKS * mark_tick))
      return false;
    if (!matchSpace(results->rawbuf[offset++],
                    KELVINATOR_GAP_SPACE_TICKS * space_tick))
      return false;

    // Data (Options) (32 bits)
    data_result = matchData(&(results->rawbuf[offset]), 32,
                            KELVINATOR_BIT_MARK_TICKS * mark_tick,
                            KELVINATOR_ONE_SPACE_TICKS * space_tick,
                            KELVINATOR_BIT_MARK_TICKS * mark_tick,
                            KELVINATOR_ZERO_SPACE_TICKS * space_tick);
    if (data_result.success == false) return false;
    data = data_result.data;
    offset += data_result.used;

    // Record option data in the state.
    for (int i = state_pos + 3; i >= state_pos; i--, data >>= 8)
      results->state[i] = reverseBits(data & 0xFF, 8);
    state_pos += 4;

    // Inter-sequence gap. (Double length gap)
    if (!matchMark(results->rawbuf[offset++],
                   KELVINATOR_BIT_MARK_TICKS * mark_tick))
      return false;
    if (s == 0) {
      if (!matchSpace(results->rawbuf[offset++],
                      KELVINATOR_GAP_SPACE_TICKS * space_tick * 2))
        return false;
    } else {
      if (offset <= results->rawlen &&
          !matchAtLeast(results->rawbuf[offset],
                        KELVINATOR_GAP_SPACE_TICKS * 2 * space_tick))
        return false;
    }
  }

  // Compliance
  if (strict) {
    // Correct size/length)
    if (state_pos != KELVINATOR_STATE_LENGTH) return false;
    // Verify the message's checksum is correct.
    if (!IRKelvinatorAC::validChecksum(results->state)) return false;
  }

  // Success
  results->decode_type = KELVINATOR;
  results->bits = state_pos * 8;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}
#endif  // DECODE_KELVINATOR
