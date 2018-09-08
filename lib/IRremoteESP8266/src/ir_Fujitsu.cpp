// Copyright 2017 Jonny Graham, David Conran
#include "ir_Fujitsu.h"
#include <algorithm>
#ifndef ARDUINO
#include <string>
#endif
#include "IRsend.h"
#include "IRutils.h"


// Fujitsu A/C support added by Jonny Graham & David Conran

// Equipment it seems compatible with:
//  * Fujitsu ASYG30LFCA with remote AR-RAH2E
//  * Fujitsu AST9RSGCW with remote AR-DB1
//  * <Add models (A/C & remotes) you've gotten it working with here>

// Ref:
// These values are based on averages of measurements
#define FUJITSU_AC_HDR_MARK    3324U
#define FUJITSU_AC_HDR_SPACE   1574U
#define FUJITSU_AC_BIT_MARK    448U
#define FUJITSU_AC_ONE_SPACE   1182U
#define FUJITSU_AC_ZERO_SPACE  390U
#define FUJITSU_AC_MIN_GAP     8100U

#if SEND_FUJITSU_AC
// Send a Fujitsu A/C message.
//
// Args:
//   data: An array of bytes containing the IR command.
//   nbytes: Nr. of bytes of data in the array. Typically one of:
//           FUJITSU_AC_STATE_LENGTH
//           FUJITSU_AC_STATE_LENGTH - 1
//           FUJITSU_AC_STATE_LENGTH_SHORT
//           FUJITSU_AC_STATE_LENGTH_SHORT - 1
//   repeat: Nr. of times the message is to be repeated.
//          (Default = FUJITSU_AC_MIN_REPEAT).
//
// Status: BETA / Appears to be working.
//
void IRsend::sendFujitsuAC(unsigned char data[], uint16_t nbytes,
                           uint16_t repeat) {
  sendGeneric(FUJITSU_AC_HDR_MARK, FUJITSU_AC_HDR_SPACE,
              FUJITSU_AC_BIT_MARK, FUJITSU_AC_ONE_SPACE,
              FUJITSU_AC_BIT_MARK, FUJITSU_AC_ZERO_SPACE,
              FUJITSU_AC_BIT_MARK, FUJITSU_AC_MIN_GAP,
              data, nbytes, 38, false, repeat, 50);
}
#endif  // SEND_FUJITSU_AC

// Code to emulate Fujitsu A/C IR remote control unit.

// Initialise the object.
IRFujitsuAC::IRFujitsuAC(uint16_t pin, fujitsu_ac_remote_model_t model)
    : _irsend(pin) {
  setModel(model);
  stateReset();
}

void IRFujitsuAC::setModel(fujitsu_ac_remote_model_t model) {
  _model = model;
  switch (model) {
    case ARDB1:
      _state_length = FUJITSU_AC_STATE_LENGTH - 1;
      _state_length_short = FUJITSU_AC_STATE_LENGTH_SHORT - 1;
      break;
    default:
      _state_length = FUJITSU_AC_STATE_LENGTH;
      _state_length_short = FUJITSU_AC_STATE_LENGTH_SHORT;
  }
}

// Reset the state of the remote to a known good state/sequence.
void IRFujitsuAC::stateReset() {
  _temp = 24;
  _fanSpeed = FUJITSU_AC_FAN_HIGH;
  _mode = FUJITSU_AC_MODE_COOL;
  _swingMode = FUJITSU_AC_SWING_BOTH;
  _cmd = FUJITSU_AC_CMD_TURN_ON;
  buildState();
}

// Configure the pin for output.
void IRFujitsuAC::begin() {
  _irsend.begin();
}

#if SEND_FUJITSU_AC
// Send the current desired state to the IR LED.
void IRFujitsuAC::send() {
  getRaw();
  _irsend.sendFujitsuAC(remote_state, getStateLength());
}
#endif  // SEND_FUJITSU_AC

void IRFujitsuAC::buildState() {
  remote_state[0] = 0x14;
  remote_state[1] = 0x63;
  remote_state[2] = 0x00;
  remote_state[3] = 0x10;
  remote_state[4] = 0x10;
  bool fullCmd = false;
  switch (_cmd) {
    case FUJITSU_AC_CMD_TURN_OFF:
      remote_state[5] = 0x02;
      break;
    case FUJITSU_AC_CMD_STEP_HORIZ:
      remote_state[5] = 0x79;
      break;
    case FUJITSU_AC_CMD_STEP_VERT:
      remote_state[5] = 0x6C;
      break;
    default:
      switch (_model) {
        case ARRAH2E:
          remote_state[5] = 0xFE;
          break;
        case ARDB1:
          remote_state[5] = 0xFC;
          break;
      }
      fullCmd = true;
      break;
  }
  if (fullCmd) {  // long codes
    uint8_t tempByte = _temp - FUJITSU_AC_MIN_TEMP;
    // Nr. of bytes in the message after this byte.
    remote_state[6] = _state_length - 7;

    remote_state[7] = 0x30;
    remote_state[8] = (_cmd == FUJITSU_AC_CMD_TURN_ON) | (tempByte << 4);
    remote_state[9] = _mode | 0 << 4;  // timer off
    remote_state[10] = _fanSpeed | _swingMode << 4;
    remote_state[11] = 0;  // timerOff values
    remote_state[12] = 0;  // timerOff/On values
    remote_state[13] = 0;  // timerOn values
    if (_model == ARRAH2E)
      remote_state[14] = 0x20;
    else
      remote_state[14] = 0x00;

    uint8_t checksum = 0;
    uint8_t checksum_complement = 0;
    if (_model == ARRAH2E) {
      checksum = sumBytes(remote_state + _state_length_short,
                          _state_length - _state_length_short - 1);
    } else if (_model == ARDB1) {
      checksum = sumBytes(remote_state, _state_length - 1);
      checksum_complement = 0x9B;
    }
    // and negate the checksum and store it in the last byte.
    remote_state[_state_length - 1] = checksum_complement - checksum;
  } else {  // short codes
    if (_model == ARRAH2E)
      // The last byte is the inverse of penultimate byte
      remote_state[_state_length_short - 1] = ~remote_state[_state_length_short
                                                            - 2];
    // Zero the rest of the state.
    for (uint8_t i = _state_length_short;
         i < FUJITSU_AC_STATE_LENGTH;
         i++)
      remote_state[i] = 0;
  }
}

uint8_t IRFujitsuAC::getStateLength() {
  buildState();  // Force an update of the internal state.
  if ((_model == ARRAH2E && remote_state[5] != 0xFE) ||
      (_model == ARDB1 && remote_state[5] != 0xFC))
    return _state_length_short;
  else
    return _state_length;
}

// Return a pointer to the internal state date of the remote.
uint8_t* IRFujitsuAC::getRaw() {
  buildState();
  return remote_state;
}

void IRFujitsuAC::buildFromState(const uint16_t length) {
  switch (length) {
    case FUJITSU_AC_STATE_LENGTH - 1:
    case FUJITSU_AC_STATE_LENGTH_SHORT - 1:
      setModel(ARDB1);
      break;
    default:
      setModel(ARRAH2E);
  }
  switch (remote_state[6]) {
    case 8:
      setModel(ARDB1);
      break;
    case 9:
      setModel(ARRAH2E);
      break;
  }
  setTemp((remote_state[8] >> 4) + FUJITSU_AC_MIN_TEMP);
  if (remote_state[8] & 0x1)
    setCmd(FUJITSU_AC_CMD_TURN_ON);
  else
    setCmd(FUJITSU_AC_CMD_STAY_ON);
  setMode(remote_state[9] & 0b111);
  setFanSpeed(remote_state[10] & 0b111);
  setSwing(remote_state[10] >> 4);
  switch (remote_state[5]) {
    case FUJITSU_AC_CMD_TURN_OFF:
    case FUJITSU_AC_CMD_STEP_HORIZ:
    case FUJITSU_AC_CMD_STEP_VERT:
      setCmd(remote_state[5]);
      break;
  }
}

bool IRFujitsuAC::setRaw(const uint8_t newState[], const uint16_t length) {
  if (length > FUJITSU_AC_STATE_LENGTH)  return false;
  for (uint16_t i = 0; i < FUJITSU_AC_STATE_LENGTH; i++) {
    if (i < length)
      remote_state[i] = newState[i];
    else
      remote_state[i] = 0;
  }
  buildFromState(length);
  return true;
}

// Set the requested power state of the A/C to off.
void IRFujitsuAC::off() {
  _cmd = FUJITSU_AC_CMD_TURN_OFF;
}

void IRFujitsuAC::stepHoriz() {
  switch (_model) {
    case ARDB1:  break;  // This remote doesn't have a horizontal option.
    default:
      _cmd = FUJITSU_AC_CMD_STEP_HORIZ;
  }
}

void IRFujitsuAC::stepVert() {
  _cmd = FUJITSU_AC_CMD_STEP_VERT;
}

// Set the requested command of the A/C.
void IRFujitsuAC::setCmd(uint8_t cmd) {
  switch (cmd) {
    case FUJITSU_AC_CMD_TURN_OFF:
    case FUJITSU_AC_CMD_TURN_ON:
    case FUJITSU_AC_CMD_STAY_ON:
    case FUJITSU_AC_CMD_STEP_VERT:
      _cmd = cmd;
      break;
    case FUJITSU_AC_CMD_STEP_HORIZ:
      if (_model != ARDB1)  // AR-DB1 remote doesn't have step horizontal.
        _cmd = cmd;
    default:
      _cmd = FUJITSU_AC_CMD_STAY_ON;
      break;
  }
}

uint8_t IRFujitsuAC::getCmd() {
  return _cmd;
}

bool IRFujitsuAC::getPower() {
  return _cmd != FUJITSU_AC_CMD_TURN_OFF;
}

// Set the temp. in deg C
void IRFujitsuAC::setTemp(uint8_t temp) {
  temp = std::max((uint8_t) FUJITSU_AC_MIN_TEMP, temp);
  temp = std::min((uint8_t) FUJITSU_AC_MAX_TEMP, temp);
  _temp = temp;
}

uint8_t IRFujitsuAC::getTemp() {
  return _temp;
}

// Set the speed of the fan
void IRFujitsuAC::setFanSpeed(uint8_t fanSpeed) {
  if (fanSpeed > FUJITSU_AC_FAN_QUIET)
    fanSpeed = FUJITSU_AC_FAN_HIGH;  // Set the fan to maximum if out of range.
  _fanSpeed = fanSpeed;
}
uint8_t IRFujitsuAC::getFanSpeed() {
  return _fanSpeed;
}

// Set the requested climate operation mode of the a/c unit.
void IRFujitsuAC::setMode(uint8_t mode) {
  if (mode > FUJITSU_AC_MODE_HEAT)
    mode = FUJITSU_AC_MODE_HEAT;  // Set the mode to maximum if out of range.
  _mode = mode;
}

uint8_t IRFujitsuAC::getMode() {
  return _mode;
}
// Set the requested swing operation mode of the a/c unit.
void IRFujitsuAC::setSwing(uint8_t swingMode) {
  switch (_model) {
    case ARDB1:
      // Set the mode to max if out of range
      if (swingMode > FUJITSU_AC_SWING_VERT)
        swingMode = FUJITSU_AC_SWING_VERT;
      break;
    case ARRAH2E:
    default:
      // Set the mode to max if out of range
      if (swingMode > FUJITSU_AC_SWING_BOTH)
        swingMode = FUJITSU_AC_SWING_BOTH;
  }
  _swingMode = swingMode;
}

uint8_t IRFujitsuAC::getSwing() {
  return _swingMode;
}

bool IRFujitsuAC::validChecksum(uint8_t state[], uint16_t length) {
  uint8_t sum = 0;
  uint8_t sum_complement = 0;
  uint8_t checksum = 0;
  switch (length) {
    case FUJITSU_AC_STATE_LENGTH_SHORT:  // ARRAH2E
      return state[length - 1] == (uint8_t) ~state[length - 2];
    case FUJITSU_AC_STATE_LENGTH - 1:  // ARDB1
      sum = sumBytes(state, length - 1);
      sum_complement = 0x9B;
      checksum = state[length - 1];
      break;
    case FUJITSU_AC_STATE_LENGTH:  // ARRAH2E
      sum = sumBytes(state + FUJITSU_AC_STATE_LENGTH_SHORT,
                     length - 1 - FUJITSU_AC_STATE_LENGTH_SHORT);
    default:  // Includes ARDB1 short.
      return true;  // Assume the checksum is valid for other lengths.
  }
  return checksum == (uint8_t) (sum_complement - sum);  // Does it match?
}

// Convert the internal state into a human readable string.
#ifdef ARDUINO
String IRFujitsuAC::toString() {
  String result = "";
#else
std::string IRFujitsuAC::toString() {
  std::string result = "";
#endif  // ARDUINO
  result += "Power: ";
  if (getPower())
    result += "On";
  else
    result += "Off";
  result += ", Mode: " + uint64ToString(getMode());
  switch (getMode()) {
    case FUJITSU_AC_MODE_AUTO:
      result += " (AUTO)";
      break;
    case FUJITSU_AC_MODE_COOL:
      result += " (COOL)";
      break;
    case FUJITSU_AC_MODE_HEAT:
      result += " (HEAT)";
      break;
    case FUJITSU_AC_MODE_DRY:
      result += " (DRY)";
      break;
    case FUJITSU_AC_MODE_FAN:
      result += " (FAN)";
      break;
    default:
      result += " (UNKNOWN)";
  }
  result += ", Temp: " + uint64ToString(getTemp()) + "C";
  result += ", Fan: " + uint64ToString(getFanSpeed());
  switch (getFanSpeed()) {
    case FUJITSU_AC_FAN_AUTO:
      result += " (AUTO)";
      break;
    case FUJITSU_AC_FAN_HIGH:
      result += " (HIGH)";
      break;
    case FUJITSU_AC_FAN_MED:
      result += " (MED)";
      break;
    case FUJITSU_AC_FAN_LOW:
      result += " (LOW)";
      break;
    case FUJITSU_AC_FAN_QUIET:
      result += " (QUIET)";
      break;
  }
  result += ", Swing: ";
  switch (getSwing()) {
    case FUJITSU_AC_SWING_OFF:
      result += "Off";
      break;
    case FUJITSU_AC_SWING_VERT:
      result += "Vert";
      break;
    case FUJITSU_AC_SWING_HORIZ:
      result += "Horiz";
      break;
    case FUJITSU_AC_SWING_BOTH:
      result += "Vert + Horiz";
      break;
    default:
      result += "UNKNOWN";
  }
  result += ", Command: ";
  switch (getCmd()) {
    case FUJITSU_AC_CMD_STEP_HORIZ:
      result += "Step vane horizontally";
      break;
    case FUJITSU_AC_CMD_STEP_VERT:
      result += "Step vane vertically";
      break;
    default:
    result += "N/A";
  }
  return result;
}

#if DECODE_FUJITSU_AC
// Decode a Fujitsu AC IR message if possible.
// Places successful decode information in the results pointer.
// Args:
//   results: Ptr to the data to decode and where to store the decode result.
//   nbits:   The number of data bits to expect. Typically FUJITSU_AC_BITS.
//   strict:  Flag to indicate if we strictly adhere to the specification.
// Returns:
//   boolean: True if it can decode it, false if it can't.
//
// Status:  ALPHA / Untested.
//
// Ref:
//
bool IRrecv::decodeFujitsuAC(decode_results *results, uint16_t nbits,
                             bool strict) {
  uint16_t offset = OFFSET_START;
  uint16_t dataBitsSoFar = 0;

  // Have we got enough data to successfully decode?
  if (results->rawlen < (2 * FUJITSU_AC_MIN_BITS) + HEADER + FOOTER - 1)
    return false;  // Can't possibly be a valid message.


  // Compliance
  if (strict) {
    switch (nbits) {
      case FUJITSU_AC_BITS:
      case FUJITSU_AC_BITS - 8:
      case FUJITSU_AC_MIN_BITS:
      case FUJITSU_AC_MIN_BITS + 8:
        break;
      default:
        return false;  // Must be called with the correct nr. of bits.
    }
  }

  // Header
  if (!matchMark(results->rawbuf[offset++], FUJITSU_AC_HDR_MARK))
    return false;
  if (!matchSpace(results->rawbuf[offset++], FUJITSU_AC_HDR_SPACE))
    return false;

  // Data (Fixed signature)
  match_result_t data_result = matchData(&(results->rawbuf[offset]),
                                        FUJITSU_AC_MIN_BITS - 8,
                                        FUJITSU_AC_BIT_MARK,
                                        FUJITSU_AC_ONE_SPACE,
                                        FUJITSU_AC_BIT_MARK,
                                        FUJITSU_AC_ZERO_SPACE);
  if (data_result.success == false)  return false;  // Fail
  if (reverseBits(data_result.data, FUJITSU_AC_MIN_BITS - 8) != 0x1010006314)
    return false;  // Signature failed.
  dataBitsSoFar += FUJITSU_AC_MIN_BITS - 8;
  offset += data_result.used;
  results->state[0] = 0x14;
  results->state[1] = 0x63;
  results->state[2] = 0x00;
  results->state[3] = 0x10;
  results->state[4] = 0x10;

  // Keep reading bytes until we either run out of message or state to fill.
  for (uint16_t i = 5;
      offset <= results->rawlen - 16 && i < FUJITSU_AC_STATE_LENGTH;
      i++, dataBitsSoFar += 8, offset += data_result.used) {
    data_result = matchData(&(results->rawbuf[offset]), 8,
                            FUJITSU_AC_BIT_MARK,
                            FUJITSU_AC_ONE_SPACE,
                            FUJITSU_AC_BIT_MARK,
                            FUJITSU_AC_ZERO_SPACE);
    if (data_result.success == false)  break;  // Fail
    results->state[i] = (uint8_t) reverseBits(data_result.data, 8);
  }

  // Footer
  if (offset > results->rawlen ||
      !matchMark(results->rawbuf[offset++], FUJITSU_AC_BIT_MARK)) return false;
  // The space is optional if we are out of capture.
  if (offset < results->rawlen &&
      !matchAtLeast(results->rawbuf[offset], FUJITSU_AC_MIN_GAP)) return false;

  // Compliance
  if (strict) {
    if (dataBitsSoFar != nbits)  return false;
  }

  results->decode_type = FUJITSU_AC;
  results->bits = dataBitsSoFar;

  // Compliance
  switch (dataBitsSoFar) {
    case FUJITSU_AC_MIN_BITS:
      // Check if this values indicate that this should have been a long state
      // message.
      if (results->state[5] == 0xFC)  return false;
      return true;  // Success
    case FUJITSU_AC_MIN_BITS + 8:
      // Check if this values indicate that this should have been a long state
      // message.
      if (results->state[5] == 0xFE)  return false;
      // The last byte needs to be the inverse of the penultimate byte.
      if (results->state[5] != (uint8_t) ~results->state[6])  return false;
      return true;  // Success
    case FUJITSU_AC_BITS - 8:
      // Long messages of this size require this byte be correct.
      if (results->state[5] != 0xFC)  return false;
      break;
    case FUJITSU_AC_BITS:
      // Long messages of this size require this byte be correct.
      if (results->state[5] != 0xFE)  return false;
      break;
    default:
      return false;  // Unexpected size.
  }
  if (!IRFujitsuAC::validChecksum(results->state, dataBitsSoFar / 8))
    return false;

  // Success
  return true;  // All good.
}
#endif  // DECODE_FUJITSU_AC
