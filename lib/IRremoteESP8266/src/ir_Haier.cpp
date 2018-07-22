// Copyright 2018 crankyoldgit
// The specifics of reverse engineering the protocol details by kuzin2006

#include "ir_Haier.h"
#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <string>
#endif
#include "IRremoteESP8266.h"
#include "IRutils.h"

//                      HH   HH   AAA   IIIII EEEEEEE RRRRRR
//                      HH   HH  AAAAA   III  EE      RR   RR
//                      HHHHHHH AA   AA  III  EEEEE   RRRRRR
//                      HH   HH AAAAAAA  III  EE      RR  RR
//                      HH   HH AA   AA IIIII EEEEEEE RR   RR

// Supported devices:
//   * Haier HSU07-HEA03 Remote control.

// Ref:
//   https://github.com/markszabo/IRremoteESP8266/issues/404
//   https://www.dropbox.com/s/mecyib3lhdxc8c6/IR%20data%20reverse%20engineering.xlsx?dl=0

// Constants
#define HAIER_AC_HDR          3000U
#define HAIER_AC_HDR_GAP      4300U

#define HAIER_AC_BIT_MARK      520U
#define HAIER_AC_ONE_SPACE    1650U
#define HAIER_AC_ZERO_SPACE    650U
#define HAIER_AC_MIN_GAP    150000U  // Completely made up value.

#if SEND_HAIER_AC
// Send a Haier A/C message.
//
// Args:
//   data: An array of bytes containing the IR command.
//   nbytes: Nr. of bytes of data in the array. (>=HAIER_AC_STATE_LENGTH)
//   repeat: Nr. of times the message is to be repeated. (Default = 0).
//
// Status: Beta / Probably working.
//
void IRsend::sendHaierAC(unsigned char data[], uint16_t nbytes,
                         uint16_t repeat) {
  if (nbytes < HAIER_AC_STATE_LENGTH)
    return;

  for (uint16_t r = 0; r <= repeat; r++) {
    enableIROut(38000);
    mark(HAIER_AC_HDR);
    space(HAIER_AC_HDR);
    sendGeneric(HAIER_AC_HDR, HAIER_AC_HDR_GAP,
                HAIER_AC_BIT_MARK, HAIER_AC_ONE_SPACE,
                HAIER_AC_BIT_MARK, HAIER_AC_ZERO_SPACE,
                HAIER_AC_BIT_MARK, HAIER_AC_MIN_GAP,
                data, nbytes, 38, true, 0,  // Repeats handled elsewhere
                50);
  }
}
#endif  // SEND_HAIER_AC

IRHaierAC::IRHaierAC(uint16_t pin) : _irsend(pin) {
  stateReset();
}

void IRHaierAC::begin() {
  _irsend.begin();
}

#if SEND_HAIER_AC
void IRHaierAC::send() {
  checksum();
  _irsend.sendHaierAC(remote_state);
}
#endif  // SEND_HAIER_AC

void IRHaierAC::checksum() {
  remote_state[8] = sumBytes(remote_state, HAIER_AC_STATE_LENGTH - 1);
}

bool IRHaierAC::validChecksum(uint8_t state[], const uint16_t length) {
  if (length < 2)  return false;  // 1 byte of data can't have a checksum.
  return (state[length - 1] == sumBytes(state, length - 1));
}

void IRHaierAC::stateReset() {
  for (uint8_t i = 1; i < HAIER_AC_STATE_LENGTH; i++)
    remote_state[i] = 0x0;
  remote_state[0] = HAIER_AC_PREFIX;
  remote_state[2] = 0b00100000;

  setTemp(HAIER_AC_DEF_TEMP);
  setFan(HAIER_AC_FAN_AUTO);
  setMode(HAIER_AC_AUTO);
  setCommand(HAIER_AC_CMD_ON);
}

uint8_t* IRHaierAC::getRaw() {
  checksum();
  return remote_state;
}

void IRHaierAC::setRaw(uint8_t new_code[]) {
  for (uint8_t i = 0; i < HAIER_AC_STATE_LENGTH; i++) {
    remote_state[i] = new_code[i];
  }
}

void IRHaierAC::setCommand(uint8_t state) {
  remote_state[1] &= 0b11110000;
  switch (state) {
    case HAIER_AC_CMD_OFF:
    case HAIER_AC_CMD_ON:
    case HAIER_AC_CMD_MODE:
    case HAIER_AC_CMD_FAN:
    case HAIER_AC_CMD_TEMP_UP:
    case HAIER_AC_CMD_TEMP_DOWN:
    case HAIER_AC_CMD_SLEEP:
    case HAIER_AC_CMD_TIMER_SET:
    case HAIER_AC_CMD_TIMER_CANCEL:
    case HAIER_AC_CMD_HEALTH:
    case HAIER_AC_CMD_SWING:
      remote_state[1] |= (state & 0b00001111);
  }
}

uint8_t IRHaierAC::getCommand() {
  return remote_state[1] & (0b00001111);
}

void IRHaierAC::setFan(uint8_t speed) {
  uint8_t new_speed = HAIER_AC_FAN_AUTO;
  switch (speed) {
    case HAIER_AC_FAN_LOW:
      new_speed = 3;
      break;
    case HAIER_AC_FAN_MED:
      new_speed = 1;
      break;
    case HAIER_AC_FAN_HIGH:
      new_speed = 2;
      break;
    default:
      new_speed = HAIER_AC_FAN_AUTO;  // Default to auto for anything else.
  }

  if (speed != getFan())  setCommand(HAIER_AC_CMD_FAN);
  remote_state[5] &= 0b11111100;
  remote_state[5] |= new_speed;
}

uint8_t IRHaierAC::getFan() {
  switch (remote_state[5] & 0b00000011) {
    case 1:
      return HAIER_AC_FAN_MED;
    case 2:
      return HAIER_AC_FAN_HIGH;
    case 3:
      return HAIER_AC_FAN_LOW;
    default:
      return HAIER_AC_FAN_AUTO;
  }
}

void IRHaierAC::setMode(uint8_t mode) {
  uint8_t new_mode = mode;
  setCommand(HAIER_AC_CMD_MODE);
  if (mode > HAIER_AC_FAN)  // If out of range, default to auto mode.
    new_mode = HAIER_AC_AUTO;
  remote_state[7] &= 0b00011111;
  remote_state[7] |= (new_mode << 5);
}

uint8_t IRHaierAC::getMode() {
  return (remote_state[7] & 0b11100000) >> 5;
}

void IRHaierAC::setTemp(const uint8_t celcius ) {
  uint8_t temp = celcius;
  if (temp < HAIER_AC_MIN_TEMP)
    temp = HAIER_AC_MIN_TEMP;
  else if (temp > HAIER_AC_MAX_TEMP)
    temp = HAIER_AC_MAX_TEMP;

  uint8_t old_temp = getTemp();
  if (old_temp == temp)  return;
  if (old_temp > temp)
    setCommand(HAIER_AC_CMD_TEMP_DOWN);
  else
    setCommand(HAIER_AC_CMD_TEMP_UP);

  remote_state[1] &= 0b00001111;  // Clear the previous temp.
  remote_state[1] |= ((temp - HAIER_AC_MIN_TEMP) << 4);
}

uint8_t IRHaierAC::getTemp() {
  return ((remote_state[1] & 0b11110000) >> 4) + HAIER_AC_MIN_TEMP;
}

void IRHaierAC::setHealth(bool state) {
  setCommand(HAIER_AC_CMD_HEALTH);
  remote_state[4] &= 0b11011111;
  remote_state[4] |= (state << 5);
}

bool IRHaierAC::getHealth(void) {
  return remote_state[4] & (1 << 5);
}

void IRHaierAC::setSleep(bool state) {
  setCommand(HAIER_AC_CMD_SLEEP);
  remote_state[7] &= 0b10111111;
  remote_state[7] |= (state << 6);
}

bool IRHaierAC::getSleep(void) {
  return remote_state[7] & 0b01000000;
}

uint16_t IRHaierAC::getTime(const uint8_t ptr[]) {
  return (ptr[0] & 0b00011111) * 60 + (ptr[1] & 0b00111111);
}

int16_t IRHaierAC::getOnTimer() {
  if (remote_state[3] & 0b10000000)  // Check if the timer is turned on.
    return getTime(remote_state + 6);
  else
    return -1;
}

int16_t IRHaierAC::getOffTimer() {
  if (remote_state[3] & 0b01000000)  // Check if the timer is turned on.
    return getTime(remote_state + 4);
  else
    return -1;
}

uint16_t IRHaierAC::getCurrTime() {
  return getTime(remote_state + 2);
}

void IRHaierAC::setTime(uint8_t ptr[], const uint16_t nr_mins) {
  uint16_t mins = nr_mins;
  if (nr_mins > HAIER_AC_MAX_TIME)
    mins = HAIER_AC_MAX_TIME;

    // Hours
  ptr[0] &= 0b11100000;
  ptr[0] |= (mins / 60);
  // Minutes
  ptr[1] &= 0b11000000;
  ptr[1] |= (mins % 60);
}

void IRHaierAC::setOnTimer(const uint16_t nr_mins) {
  setCommand(HAIER_AC_CMD_TIMER_SET);
  remote_state[3] |= 0b10000000;
  setTime(remote_state + 6, nr_mins);
}

void IRHaierAC::setOffTimer(const uint16_t nr_mins) {
  setCommand(HAIER_AC_CMD_TIMER_SET);
  remote_state[3] |= 0b01000000;
  setTime(remote_state + 4, nr_mins);
}

void IRHaierAC::cancelTimers() {
  setCommand(HAIER_AC_CMD_TIMER_CANCEL);
  remote_state[3] &= 0b00111111;
}

void IRHaierAC::setCurrTime(const uint16_t nr_mins) {
  setTime(remote_state + 2, nr_mins);
}

uint8_t IRHaierAC::getSwing() {
  return (remote_state[2] & 0b11000000) >> 6;
}

void IRHaierAC::setSwing(const uint8_t state) {
  if (state == getSwing())  return;  // Nothing to do.
  setCommand(HAIER_AC_CMD_SWING);
  switch (state) {
    case HAIER_AC_SWING_OFF:
    case HAIER_AC_SWING_UP:
    case HAIER_AC_SWING_DOWN:
    case HAIER_AC_SWING_CHG:
      remote_state[2] &= 0b00111111;
      remote_state[2] |= (state << 6);
      break;
  }
}

// Convert a Haier time into a human readable string.
#ifdef ARDUINO
String IRHaierAC::timeToString(const uint16_t nr_mins) {
  String result = "";
#else
std::string IRHaierAC::timeToString(const uint16_t nr_mins) {
  std::string result = "";
#endif  // ARDUINO

  if (nr_mins / 24 < 10)  result += "0";  // Zero pad.
  result += uint64ToString(nr_mins / 60);
  result += ":";
  if (nr_mins % 60 < 10)  result += "0";  // Zero pad.
  result += uint64ToString(nr_mins % 60);
  return result;
}

// Convert the internal state into a human readable string.
#ifdef ARDUINO
String IRHaierAC::toString() {
  String result = "";
#else
std::string IRHaierAC::toString() {
  std::string result = "";
#endif  // ARDUINO
  uint8_t cmd = getCommand();
  result += "Command: " + uint64ToString(cmd) +" (";
  switch (cmd) {
    case HAIER_AC_CMD_OFF:
      result += "Off";
      break;
    case HAIER_AC_CMD_ON:
      result += "On";
      break;
    case HAIER_AC_CMD_MODE:
      result += "Mode";
      break;
    case HAIER_AC_CMD_FAN:
      result += "Fan";
      break;
    case HAIER_AC_CMD_TEMP_UP:
      result += "Temp Up";
      break;
    case HAIER_AC_CMD_TEMP_DOWN:
      result += "Temp Down";
      break;
    case HAIER_AC_CMD_SLEEP:
      result += "Sleep";
      break;
    case HAIER_AC_CMD_TIMER_SET:
      result += "Timer Set";
      break;
    case HAIER_AC_CMD_TIMER_CANCEL:
      result += "Timer Cancel";
      break;
    case HAIER_AC_CMD_HEALTH:
      result += "Health";
      break;
    case HAIER_AC_CMD_SWING:
      result += "Swing";
      break;
    default:
      result += "Unknown";
  }
  result += ")";
  result += ", Mode: " + uint64ToString(getMode());
  switch (getMode()) {
    case HAIER_AC_AUTO:
      result += " (AUTO)";
      break;
    case HAIER_AC_COOL:
      result += " (COOL)";
      break;
    case HAIER_AC_HEAT:
      result += " (HEAT)";
      break;
    case HAIER_AC_DRY:
      result += " (DRY)";
      break;
    case HAIER_AC_FAN:
      result += " (FAN)";
      break;
    default:
      result += " (UNKNOWN)";
  }
  result += ", Temp: " + uint64ToString(getTemp()) + "C";
  result += ", Fan: " + uint64ToString(getFan());
  switch (getFan()) {
    case HAIER_AC_FAN_AUTO:
      result += " (AUTO)";
      break;
    case HAIER_AC_FAN_HIGH:
      result += " (MAX)";
      break;
  }
  result += ", Swing: " + uint64ToString(getSwing()) + " (";
  switch (getSwing()) {
    case HAIER_AC_SWING_OFF:
      result += "Off";
      break;
    case HAIER_AC_SWING_UP:
      result += "Up";
      break;
    case HAIER_AC_SWING_DOWN:
      result += "Down";
      break;
    case HAIER_AC_SWING_CHG:
      result += "Chg";
      break;
    default:
      result += "Unknown";
  }
  result += ")";
  result += ", Sleep: ";
  if (getSleep())
    result += "On";
  else
    result += "Off";
  result += ", Health: ";
  if (getHealth())
    result += "On";
  else
    result += "Off";
  result += ", Current Time: " + timeToString(getCurrTime());
  result += ", On Timer: ";
  if (getOnTimer() >= 0)
    result += timeToString(getOnTimer());
  else
    result += "Off";
  result += ", Off Timer: ";
  if (getOffTimer() >= 0)
    result += timeToString(getOffTimer());
  else
    result += "Off";

  return result;
}

#if DECODE_HAIER_AC
// Decode the supplied Haier message.
//
// Args:
//   results: Ptr to the data to decode and where to store the decode result.
//   nbits:   The number of data bits to expect. Typically HAIER_AC_BITS.
//   strict:  Flag indicating if we should perform strict matching.
// Returns:
//   boolean: True if it can decode it, false if it can't.
//
// Status: BETA / Appears to be working.
//
bool IRrecv::decodeHaierAC(decode_results *results, uint16_t nbits,
                           bool strict) {
  if (nbits % 8 != 0)  // nbits has to be a multiple of nr. of bits in a byte.
    return false;

  if (strict) {
    if (nbits != HAIER_AC_BITS)
      return false;  // Not strictly a HAIER_AC message.
  }

  if (results->rawlen < (2 * nbits + HEADER) + FOOTER - 1)
    return false;  // Can't possibly be a valid HAIER_AC message.

  uint16_t offset = OFFSET_START;


  // Header
  if (!matchMark(results->rawbuf[offset++], HAIER_AC_HDR)) return false;
  if (!matchSpace(results->rawbuf[offset++], HAIER_AC_HDR)) return false;
  if (!matchMark(results->rawbuf[offset++], HAIER_AC_HDR)) return false;
  if (!matchSpace(results->rawbuf[offset++], HAIER_AC_HDR_GAP)) return false;

  // Data
  for (uint16_t i = 0; i < nbits / 8; i++) {
    match_result_t data_result = matchData(&(results->rawbuf[offset]), 8,
                                           HAIER_AC_BIT_MARK,
                                           HAIER_AC_ONE_SPACE,
                                           HAIER_AC_BIT_MARK,
                                           HAIER_AC_ZERO_SPACE);
    if (data_result.success == false) return false;
    offset += data_result.used;
    results->state[i] = (uint8_t) data_result.data;
  }

  // Footer
  if (!matchMark(results->rawbuf[offset++], HAIER_AC_BIT_MARK))  return false;
  if (offset < results->rawlen &&
      !matchAtLeast(results->rawbuf[offset++], HAIER_AC_MIN_GAP))
    return false;

  // Compliance
  if (strict) {
    if (results->state[0] != HAIER_AC_PREFIX)  return false;
    if (!IRHaierAC::validChecksum(results->state, nbits / 8))  return false;
  }

  // Success
  results->decode_type = HAIER_AC;
  results->bits = nbits;
  return true;
}
#endif  // DECODE_HAIER_AC
