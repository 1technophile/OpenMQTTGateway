// Copyright 2017 Ville Skyttä (scop)
// Copyright 2017, 2018 David Conran
//
// Code to emulate Gree protocol compatible HVAC devices.
// Should be compatible with:
// * Heat pumps carrying the "Ultimate" brand name.
// * EKOKAI air conditioners.
//

#include "ir_Gree.h"
#include <algorithm>
#ifndef ARDUINO
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRrecv.h"
#include "IRsend.h"
#include "IRutils.h"
#include "ir_Kelvinator.h"

//                      GGGG  RRRRRR  EEEEEEE EEEEEEE
//                     GG  GG RR   RR EE      EE
//                    GG      RRRRRR  EEEEE   EEEEE
//                    GG   GG RR  RR  EE      EE
//                     GGGGGG RR   RR EEEEEEE EEEEEEE

// Constants
// Ref: https://github.com/ToniA/arduino-heatpumpir/blob/master/GreeHeatpumpIR.h
#define GREE_HDR_MARK           9000U
#define GREE_HDR_SPACE          4000U
#define GREE_BIT_MARK            620U
#define GREE_ONE_SPACE          1600U
#define GREE_ZERO_SPACE          540U
#define GREE_MSG_SPACE         19000U
#define GREE_BLOCK_FOOTER      0b010U
#define GREE_BLOCK_FOOTER_BITS     3U

#if SEND_GREE
// Send a Gree Heat Pump message.
//
// Args:
//   data: An array of bytes containing the IR command.
//   nbytes: Nr. of bytes of data in the array. (>=GREE_STATE_LENGTH)
//   repeat: Nr. of times the message is to be repeated. (Default = 0).
//
// Status: ALPHA / Untested.
//
// Ref:
//   https://github.com/ToniA/arduino-heatpumpir/blob/master/GreeHeatpumpIR.cpp
void IRsend::sendGree(unsigned char data[], uint16_t nbytes, uint16_t repeat) {
  if (nbytes < GREE_STATE_LENGTH)
    return;  // Not enough bytes to send a proper message.

  for (uint16_t r = 0; r <= repeat; r++) {
    // Block #1
    sendGeneric(GREE_HDR_MARK, GREE_HDR_SPACE,
                GREE_BIT_MARK, GREE_ONE_SPACE,
                GREE_BIT_MARK, GREE_ZERO_SPACE,
                0, 0,  // No Footer.
                data, 4, 38, false, 0, 50);
    // Footer #1
    sendGeneric(0, 0,  // No Header
                GREE_BIT_MARK, GREE_ONE_SPACE,
                GREE_BIT_MARK, GREE_ZERO_SPACE,
                GREE_BIT_MARK, GREE_MSG_SPACE,
                0b010, 3, 38, true, 0, false);

    // Block #2
    sendGeneric(0, 0,  // No Header for Block #2
                GREE_BIT_MARK, GREE_ONE_SPACE,
                GREE_BIT_MARK, GREE_ZERO_SPACE,
                GREE_BIT_MARK, GREE_MSG_SPACE,
                data + 4, nbytes - 4, 38, false, 0, 50);
  }
}

// Send a Gree Heat Pump message.
//
// Args:
//   data: The raw message to be sent.
//   nbits: Nr. of bits of data in the message. (Default is GREE_BITS)
//   repeat: Nr. of times the message is to be repeated. (Default = 0).
//
// Status: ALPHA / Untested.
//
// Ref:
//   https://github.com/ToniA/arduino-heatpumpir/blob/master/GreeHeatpumpIR.cpp
void IRsend::sendGree(uint64_t data, uint16_t nbits, uint16_t repeat) {
  if (nbits != GREE_BITS)
    return;  // Wrong nr. of bits to send a proper message.
  // Set IR carrier frequency
  enableIROut(38);

  for (uint16_t r = 0; r <= repeat; r++) {
    // Header
    mark(GREE_HDR_MARK);
    space(GREE_HDR_SPACE);

    // Data
    for (int16_t i = 8; i <= nbits; i += 8) {
      sendData(GREE_BIT_MARK, GREE_ONE_SPACE, GREE_BIT_MARK, GREE_ZERO_SPACE,
               (data >> (nbits - i)) & 0xFF, 8, false);
      if (i == nbits / 2) {
        // Send the mid-message Footer.
        sendData(GREE_BIT_MARK, GREE_ONE_SPACE, GREE_BIT_MARK, GREE_ZERO_SPACE,
                 0b010, 3);
        mark(GREE_BIT_MARK);
        space(GREE_MSG_SPACE);
      }
    }
    // Footer
    mark(GREE_BIT_MARK);
    space(GREE_MSG_SPACE);
  }
}
#endif  // SEND_GREE

IRGreeAC::IRGreeAC(uint16_t pin) : _irsend(pin) {
  stateReset();
}

void IRGreeAC::stateReset() {
  // This resets to a known-good state to Power Off, Fan Auto, Mode Auto, 25C.
  for (uint8_t i = 0; i < GREE_STATE_LENGTH; i++)
    remote_state[i] = 0x0;
  remote_state[1] = 0x09;
  remote_state[2] = 0x20;
  remote_state[3] = 0x50;
  remote_state[5] = 0x20;
  remote_state[7] = 0x50;
}

void IRGreeAC::fixup() {
  checksum();  // Calculate the checksums
}

void IRGreeAC::begin() {
  _irsend.begin();
}

#if SEND_GREE
void IRGreeAC::send() {
  fixup();   // Ensure correct settings before sending.
  _irsend.sendGree(remote_state);
}
#endif  // SEND_GREE

uint8_t* IRGreeAC::getRaw() {
  fixup();   // Ensure correct settings before sending.
  return remote_state;
}

void IRGreeAC::setRaw(uint8_t new_code[]) {
  for (uint8_t i = 0; i < GREE_STATE_LENGTH; i++) {
    remote_state[i] = new_code[i];
  }
}

void IRGreeAC::checksum(const uint16_t length) {
  // Gree uses the same checksum alg. as Kelvinator's block checksum.
  uint8_t sum = IRKelvinatorAC::calcBlockChecksum(remote_state, length);
  remote_state[length - 1] = (sum << 4) | (remote_state[length - 1] & 0xFU);
}

// Verify the checksum is valid for a given state.
// Args:
//   state:  The array to verify the checksum of.
//   length: The size of the state.
// Returns:
//   A boolean.
bool IRGreeAC::validChecksum(const uint8_t state[], const uint16_t length) {
  // Top 4 bits of the last byte in the state is the state's checksum.
  if (state[length - 1] >> 4 == IRKelvinatorAC::calcBlockChecksum(state,
                                                                  length))
    return true;
  else
    return false;
}

void IRGreeAC::on() {
  remote_state[0] |= GREE_POWER1_MASK;
  remote_state[2] |= GREE_POWER2_MASK;
}

void IRGreeAC::off() {
  remote_state[0] &= ~GREE_POWER1_MASK;
  remote_state[2] &= ~GREE_POWER2_MASK;
}

void IRGreeAC::setPower(const bool state) {
  if (state)
    on();
  else
    off();
}

bool IRGreeAC::getPower() {
  return (remote_state[0] & GREE_POWER1_MASK) &&
         (remote_state[2] & GREE_POWER2_MASK);
}

// Set the temp. in deg C
void IRGreeAC::setTemp(const uint8_t temp) {
  uint8_t new_temp = std::max((uint8_t) GREE_MIN_TEMP, temp);
  new_temp = std::min((uint8_t) GREE_MAX_TEMP, new_temp);
  if (getMode() == GREE_AUTO) new_temp = 25;
  remote_state[1] = (remote_state[1] & 0xF0U) | (new_temp - GREE_MIN_TEMP);
}

// Return the set temp. in deg C
uint8_t IRGreeAC::getTemp() {
  return ((remote_state[1] & 0xFU) + GREE_MIN_TEMP);
}

// Set the speed of the fan, 0-3, 0 is auto, 1-3 is the speed
void IRGreeAC::setFan(const uint8_t speed) {
  uint8_t fan = std::min((uint8_t) GREE_FAN_MAX, speed);  // Bounds check

  if (getMode() == GREE_DRY) fan = 1;  // DRY mode is always locked to fan 1.
  // Set the basic fan values.
  remote_state[0] &= ~GREE_FAN_MASK;
  remote_state[0] |= (fan << 4);
}

uint8_t IRGreeAC::getFan() {
  return ((remote_state[0] & GREE_FAN_MASK) >> 4);
}

void IRGreeAC::setMode(const uint8_t new_mode) {
  uint8_t mode = new_mode;
  switch (mode) {
    case GREE_AUTO:
      // AUTO is locked to 25C
      setTemp(25);
      break;
    case GREE_DRY:
      // DRY always sets the fan to 1.
      setFan(1);
      break;
    case GREE_COOL:
    case GREE_FAN:
    case GREE_HEAT:
      break;
    default:
      // If we get an unexpected mode, default to AUTO.
      mode = GREE_AUTO;
  }
  remote_state[0] &= ~GREE_MODE_MASK;
  remote_state[0] |= mode;
}

uint8_t IRGreeAC::getMode() {
  return (remote_state[0] & GREE_MODE_MASK);
}

void IRGreeAC::setLight(const bool state) {
  remote_state[2] &= ~GREE_LIGHT_MASK;
  remote_state[2] |= (state << 5);
}

bool IRGreeAC::getLight() {
  return remote_state[2] & GREE_LIGHT_MASK;
}

void IRGreeAC::setXFan(const bool state) {
  remote_state[2] &= ~GREE_XFAN_MASK;
  remote_state[2] |= (state << 7);
}

bool IRGreeAC::getXFan() {
  return remote_state[2] & GREE_XFAN_MASK;
}

void IRGreeAC::setSleep(const bool state) {
  remote_state[0] &= ~GREE_SLEEP_MASK;
  remote_state[0] |= (state << 7);
}

bool IRGreeAC::getSleep() {
  return remote_state[0] & GREE_SLEEP_MASK;
}

void IRGreeAC::setTurbo(const bool state) {
  remote_state[2] &= ~GREE_TURBO_MASK;
  remote_state[2] |= (state << 4);
}

bool IRGreeAC::getTurbo() {
  return remote_state[2] & GREE_TURBO_MASK;
}

void IRGreeAC::setSwingVertical(const bool automatic, const uint8_t position) {
  remote_state[0] &= ~GREE_SWING_AUTO_MASK;
  remote_state[0] |= (automatic << 6);
  uint8_t new_position = position;
  if (!automatic) {
    switch (position) {
      case GREE_SWING_UP:
      case GREE_SWING_MIDDLE_UP:
      case GREE_SWING_MIDDLE:
      case GREE_SWING_MIDDLE_DOWN:
      case GREE_SWING_DOWN:
        break;
      default:
        new_position = GREE_SWING_LAST_POS;
    }
  } else {
    switch (position) {
      case GREE_SWING_AUTO:
      case GREE_SWING_DOWN_AUTO:
      case GREE_SWING_MIDDLE_AUTO:
      case GREE_SWING_UP_AUTO:
        break;
      default:
        new_position = GREE_SWING_AUTO;
    }
  }
  remote_state[4] &= ~GREE_SWING_POS_MASK;
  remote_state[4] |= new_position;
}

bool IRGreeAC::getSwingVerticalAuto() {
  return remote_state[0] & GREE_SWING_AUTO_MASK;
}

uint8_t IRGreeAC::getSwingVerticalPosition() {
  return remote_state[4] & GREE_SWING_POS_MASK;
}

// Convert the internal state into a human readable string.
#ifdef ARDUINO
String IRGreeAC::toString() {
  String result = "";
#else
std::string IRGreeAC::toString() {
  std::string result = "";
#endif  // ARDUINO
  result += "Power: ";
  if (getPower())
    result += "On";
  else
    result += "Off";
  result += ", Mode: " + uint64ToString(getMode());
  switch (getMode()) {
    case GREE_AUTO:
      result += " (AUTO)";
      break;
    case GREE_COOL:
      result += " (COOL)";
      break;
    case GREE_HEAT:
      result += " (HEAT)";
      break;
    case GREE_DRY:
      result += " (DRY)";
      break;
    case GREE_FAN:
      result += " (FAN)";
      break;
    default:
      result += " (UNKNOWN)";
  }
  result += ", Temp: " + uint64ToString(getTemp()) + "C";
  result += ", Fan: " + uint64ToString(getFan());
  switch (getFan()) {
    case 0:
      result += " (AUTO)";
      break;
    case GREE_FAN_MAX:
      result += " (MAX)";
      break;
  }
  result += ", Turbo: ";
  if (getTurbo())
    result += "On";
  else
    result += "Off";
  result += ", XFan: ";
  if (getXFan())
    result += "On";
  else
    result += "Off";
  result += ", Light: ";
  if (getLight())
    result += "On";
  else
    result += "Off";
  result += ", Sleep: ";
  if (getSleep())
    result += "On";
  else
    result += "Off";
  result += ", Swing Vertical Mode: ";
  if (getSwingVerticalAuto())
    result += "Auto";
  else
    result += "Manual";
  result += ", Swing Vertical Pos: " +
      uint64ToString(getSwingVerticalPosition());
  switch (getSwingVerticalPosition()) {
    case GREE_SWING_LAST_POS:
      result += " (Last Pos)";
      break;
    case GREE_SWING_AUTO:
      result += " (Auto)";
      break;
  }
  return result;
}

#if DECODE_GREE
// Decode the supplied Gree message.
//
// Args:
//   results: Ptr to the data to decode and where to store the decode result.
//   nbits:   The number of data bits to expect. Typically GREE_BITS.
//   strict:  Flag indicating if we should perform strict matching.
// Returns:
//   boolean: True if it can decode it, false if it can't.
//
// Status: ALPHA / Untested.
bool IRrecv::decodeGree(decode_results *results, uint16_t nbits, bool strict) {
  if (results->rawlen < 2 * (nbits + GREE_BLOCK_FOOTER_BITS) +
                        (HEADER + FOOTER + 1))
    return false;  // Can't possibly be a valid Gree message.
  if (strict && nbits != GREE_BITS)
    return false;  // Not strictly a Gree message.

  uint32_t data;
  uint16_t offset = OFFSET_START;

  // There are two blocks back-to-back in a full Gree IR message
  // sequence.
  int8_t state_pos = 0;
  match_result_t data_result;

  // Header
  if (!matchMark(results->rawbuf[offset++], GREE_HDR_MARK)) return false;
  if (!matchSpace(results->rawbuf[offset++], GREE_HDR_SPACE)) return false;
  // Data Block #1 (32 bits)
  data_result = matchData(&(results->rawbuf[offset]), 32, GREE_BIT_MARK,
                          GREE_ONE_SPACE, GREE_BIT_MARK, GREE_ZERO_SPACE);
  if (data_result.success == false) return false;
  data = data_result.data;
  offset += data_result.used;

  // Record Data Block #1 in the state.
  for (int i = state_pos + 3; i >= state_pos; i--, data >>= 8)
    results->state[i] = reverseBits(data & 0xFF, 8);
  state_pos += 4;

  // Block #1 footer (3 bits, B010)
  data_result = matchData(&(results->rawbuf[offset]), GREE_BLOCK_FOOTER_BITS,
                          GREE_BIT_MARK, GREE_ONE_SPACE, GREE_BIT_MARK,
                          GREE_ZERO_SPACE);
  if (data_result.success == false) return false;
  if (data_result.data != GREE_BLOCK_FOOTER) return false;
  offset += data_result.used;

  // Inter-block gap.
  if (!matchMark(results->rawbuf[offset++], GREE_BIT_MARK)) return false;
  if (!matchSpace(results->rawbuf[offset++],  GREE_MSG_SPACE)) return false;

  // Data Block #2 (32 bits)
  data_result = matchData(&(results->rawbuf[offset]), 32, GREE_BIT_MARK,
                          GREE_ONE_SPACE, GREE_BIT_MARK, GREE_ZERO_SPACE);
  if (data_result.success == false) return false;
  data = data_result.data;
  offset += data_result.used;

  // Record Data Block #2 in the state.
  for (int i = state_pos + 3; i >= state_pos; i--, data >>= 8)
    results->state[i] = reverseBits(data & 0xFF, 8);
  state_pos += 4;

  // Footer.
  if (!matchMark(results->rawbuf[offset++], GREE_BIT_MARK)) return false;
  if (offset <= results->rawlen &&
      !matchAtLeast(results->rawbuf[offset], GREE_MSG_SPACE))
    return false;

  // Compliance
  if (strict) {
    // Correct size/length)
    if (state_pos != GREE_STATE_LENGTH) return false;
    // Verify the message's checksum is correct.
    if (!IRGreeAC::validChecksum(results->state)) return false;
  }

  // Success
  results->decode_type = GREE;
  results->bits = state_pos * 8;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}
#endif  // DECODE_GREE
