// Copyright 2018 Erdem U. Altinyurt
// Copyright 2019 David Conran

#include "ir_Vestel.h"
#include <algorithm>
#ifndef UNIT_TEST
#include <Arduino.h>
#else
#include <string>
#endif
#include "IRrecv.h"
#include "IRremoteESP8266.h"
#include "IRsend.h"
#include "IRutils.h"
#include "ir_Haier.h"

//                 VV     VV  EEEEEEE   SSSSS  TTTTTTTT  EEEEEEE  LL
//                 VV     VV  EE       S          TT     EE       LL
//                  VV   VV   EEEEE     SSSS      TT     EEEEE    LL
//                   VV VV    EE            S     TT     EE       LL
//                    VVV     EEEEEEE  SSSSS      TT     EEEEEEE  LLLLLLL

// Vestel added by Erdem U. Altinyurt

// Equipment it seems compatible with:
//  * Vestel AC Model BIOX CXP-9 (9K BTU)
//  * <Add models (A/C & remotes) you've gotten it working with here>

// Ref:
//   None. Totally reverse engineered.

#if SEND_VESTEL_AC
// Send a Vestel message
//
// Args:
//   data:   Contents of the message to be sent.
//   nbits:  Nr. of bits of data to be sent. Typically kVestelBits.
//
// Status: STABLE / Working.
//
void IRsend::sendVestelAc(const uint64_t data, const uint16_t nbits,
                          const uint16_t repeat) {
  if (nbits % 8 != 0) return;  // nbits is required to be a multiple of 8.

  sendGeneric(kVestelAcHdrMark, kVestelAcHdrSpace,   // Header
              kVestelAcBitMark, kVestelAcOneSpace,   // Data
              kVestelAcBitMark, kVestelAcZeroSpace,  // Data
              kVestelAcBitMark, 100000,              // Footer + repeat gap
              data, nbits, 38, false, repeat, 50);
}
#endif

// Code to emulate Vestel A/C IR remote control unit.

// Initialise the object.
IRVestelAc::IRVestelAc(uint16_t pin) : _irsend(pin) { stateReset(); }

// Reset the state of the remote to a known good state/sequence.
void IRVestelAc::stateReset() {
  // Power On, Mode Auto, Fan Auto, Temp = 25C/77F
  remote_state = 0x0F00D9001FEF201ULL;
  remote_time_state = 0x201ULL;
  use_time_state = false;
}

// Configure the pin for output.
void IRVestelAc::begin() {
  _irsend.begin();
}

#if SEND_VESTEL_AC
// Send the current desired state to the IR LED.
void IRVestelAc::send() {
  checksum();  // Ensure correct checksum before sending.
  uint64_t code_to_send;
  if (use_time_state)
    code_to_send = remote_time_state;
  else
    code_to_send = remote_state;
  _irsend.sendVestelAc(code_to_send);
}
#endif  // SEND_VESTEL_AC

// Return the internal state date of the remote.
uint64_t IRVestelAc::getRaw() {
  checksum();
  if (use_time_state) return remote_time_state;
  return remote_state;
}

// Override the internal state with the new state.
void IRVestelAc::setRaw(uint8_t* newState) {
  uint64_t upState = 0;
  for (int i = 0; i < 7; i++)
    upState |= static_cast<uint64_t>(newState[i]) << (i * 8);
  remote_state = upState;
  remote_time_state = upState;
}

void IRVestelAc::setRaw(const uint64_t newState) {
  remote_state = newState;
  remote_time_state = newState;
}

// Set the requested power state of the A/C to on.
void IRVestelAc::on() { setPower(true); }

// Set the requested power state of the A/C to off.
void IRVestelAc::off() { setPower(false); }

// Set the requested power state of the A/C.
void IRVestelAc::setPower(const bool state) {
  remote_state &= ~((uint64_t)0xF << kVestelAcPowerOffset);
  if (state)
    remote_state |= ((uint64_t)0xF << kVestelAcPowerOffset);
  else
    remote_state |= ((uint64_t)0xC << kVestelAcPowerOffset);
  use_time_state = false;
}

// Return the requested power state of the A/C.
bool IRVestelAc::getPower() {
  return (remote_state >> kVestelAcPowerOffset == 0xF);
}

// Set the temperature in Celsius degrees.
void IRVestelAc::setTemp(const uint8_t temp) {
  uint8_t new_temp = temp;
  new_temp = std::max(kVestelAcMinTempC, new_temp);
  // new_temp = std::max(kVestelAcMinTempH, new_temp); Check MODE
  new_temp = std::min(kVestelAcMaxTemp, new_temp);
  remote_state &= ~((uint64_t)0xF << kVestelAcTempOffset);
  remote_state |= (uint64_t)(new_temp - 16) << kVestelAcTempOffset;
  use_time_state = false;
}

// Return the set temperature.
uint8_t IRVestelAc::getTemp(void) {
  return ((remote_state >> kVestelAcTempOffset) & 0xF) + 16;
}

// Set the speed of the fan,
// 1-3 set the fan speed, 0 or anything else set it to auto.
void IRVestelAc::setFan(const uint8_t fan) {
  switch (fan) {
    case kVestelAcFanLow:
    case kVestelAcFanMed:
    case kVestelAcFanHigh:
    case kVestelAcFanAutoCool:
    case kVestelAcFanAutoHot:
    case kVestelAcFanAuto:
      remote_state &= ~((uint64_t)0xF << kVestelAcFanOffset);
      remote_state |= (uint64_t)fan << kVestelAcFanOffset;
      break;
    default:
      setFan(kVestelAcFanAuto);
  }
  use_time_state = false;
}

// Return the requested state of the unit's fan.
uint8_t IRVestelAc::getFan() {
  return (remote_state >> kVestelAcFanOffset) & 0xF;
}

// Get the requested climate operation mode of the a/c unit.
// Returns:
//   A uint8_t containing the A/C mode.
uint8_t IRVestelAc::getMode() {
  return (remote_state >> kVestelAcModeOffset) & 0xF;
}

// Set the requested climate operation mode of the a/c unit.
void IRVestelAc::setMode(const uint8_t mode) {
  // If we get an unexpected mode, default to AUTO.
  switch (mode) {
    case kVestelAcAuto:
    case kVestelAcCool:
    case kVestelAcHeat:
    case kVestelAcDry:
    case kVestelAcFan:
      remote_state &= ~((uint64_t)0xF << kVestelAcModeOffset);
      remote_state |= (uint64_t)mode << kVestelAcModeOffset;
      break;
    default:
      setMode(kVestelAcAuto);
  }
  use_time_state = false;
}

// Set Auto mode of AC.
void IRVestelAc::setAuto(const int8_t autoLevel) {
  if (autoLevel < -2 || autoLevel > 2) return;
  setMode(kVestelAcAuto);
  setFan((autoLevel < 0 ? kVestelAcFanAutoCool : kVestelAcFanAutoHot));
  if (autoLevel == 2)
    setTemp(30);
  else if (autoLevel == 1)
    setTemp(31);
  else if (autoLevel == 0)
    setTemp(25);
  else if (autoLevel == -1)
    setTemp(16);
  else if (autoLevel == -2)
    setTemp(17);
}

void IRVestelAc::setTimerActive(const bool on) {
  if (on)  // activation
    remote_time_state |= ((uint64_t)1 << kVestelAcTimerFlagOffset);
  else  // deactivate
    remote_time_state &= ~((uint64_t)1 << kVestelAcTimerFlagOffset);
  use_time_state = true;
}

bool IRVestelAc::isTimerActive(void) {
  return (remote_time_state >> kVestelAcTimerFlagOffset) & 1;
}

// Set Timer option of AC.
// Valid time arguments are 0, 0.5, 1, 2, 3 and 5 hours (in min). 0 disables the
// timer.
void IRVestelAc::setTimer(const uint16_t minutes) {
  // Clear both On & Off timers.
  remote_time_state &= ~((uint64_t)0xFFFF << kVestelAcOffTimeOffset);
  // Set the "Off" time with the nr of minutes before we turn off.
  remote_time_state |= (uint64_t)(((minutes / 60) << 3) + (minutes % 60) / 10)
                       << kVestelAcOffTimeOffset;
  setOffTimerActive(false);
  // Yes. On Timer instead of Off timer active.
  setOnTimerActive(minutes != 0);
  setTimerActive(minutes != 0);
  use_time_state = true;
}

uint16_t IRVestelAc::getTimer(void) { return getOffTimer(); }

// Set the AC's internal clock
void IRVestelAc::setTime(const uint16_t minutes) {
  remote_time_state &= ~((uint64_t)0x1F << kVestelAcHourOffset);
  remote_time_state |= (uint64_t)((minutes / 60) & 0x1F)
                       << kVestelAcHourOffset;
  remote_time_state &= ~((uint64_t)0xFF << kVestelAcMinuteOffset);
  remote_time_state |= (uint64_t)((minutes % 60) & 0xFF)
                       << kVestelAcMinuteOffset;
  use_time_state = true;
}

uint16_t IRVestelAc::getTime(void) {
  return ((remote_time_state >> kVestelAcHourOffset) & 0x1F) * 60 +
         ((remote_time_state >> kVestelAcMinuteOffset) & 0xFF);
}

void IRVestelAc::setOnTimerActive(const bool on) {
  if (on)  // activation
    remote_time_state |= ((uint64_t)1 << kVestelAcOnTimerFlagOffset);
  else  // deactivate
    remote_time_state &= ~((uint64_t)1 << kVestelAcOnTimerFlagOffset);
  use_time_state = true;
}

bool IRVestelAc::isOnTimerActive(void) {
  return (remote_time_state >> kVestelAcOnTimerFlagOffset) & 1;
}

// Set AC's wake up time. Takes time in minute.
void IRVestelAc::setOnTimer(const uint16_t minutes) {
  remote_time_state &= ~((uint64_t)0xFF << kVestelAcOnTimeOffset);
  remote_time_state |= (uint64_t)(((minutes / 60) << 3) + (minutes % 60) / 10)
                       << kVestelAcOnTimeOffset;
  setOnTimerActive(minutes != 0);
  setTimerActive(false);
  use_time_state = true;
}

uint16_t IRVestelAc::getOnTimer(void) {
  uint8_t ontime = (remote_time_state >> kVestelAcOnTimeOffset) & 0xFF;
  return (ontime >> 3) * 60 + (ontime & 0x7) * 10;
}

void IRVestelAc::setOffTimerActive(const bool on) {
  if (on)  // activation
    remote_time_state |= ((uint64_t)1 << kVestelAcOffTimerFlagOffset);
  else  // deactivate
    remote_time_state &= ~((uint64_t)1 << kVestelAcOffTimerFlagOffset);
  use_time_state = true;
}

bool IRVestelAc::isOffTimerActive(void) {
  return (remote_time_state >> kVestelAcOffTimerFlagOffset) & 1;
}

// Set AC's turn off time. Takes time in minute.
void IRVestelAc::setOffTimer(const uint16_t minutes) {
  remote_time_state &= ~((uint64_t)0xFF << kVestelAcOffTimeOffset);
  remote_time_state |=
      (uint64_t)((((minutes / 60) << 3) + (minutes % 60) / 10) & 0xFF)
      << kVestelAcOffTimeOffset;
  setOffTimerActive(minutes != 0);
  setTimerActive(false);
  use_time_state = true;
}

uint16_t IRVestelAc::getOffTimer(void) {
  uint8_t offtime = (remote_time_state >> kVestelAcOffTimeOffset) & 0xFF;
  return (offtime >> 3) * 60 + (offtime & 0x7) * 10;
}

// Set the Sleep state of the A/C.
void IRVestelAc::setSleep(const bool state) {
  remote_state &= ~((uint64_t)0xF << kVestelAcTurboSleepOffset);
  remote_state |= (uint64_t)(state ? kVestelAcSleep : kVestelAcNormal)
                  << kVestelAcTurboSleepOffset;
  use_time_state = false;
}

// Return the Sleep state of the A/C.
bool IRVestelAc::getSleep() {
  return ((remote_state >> kVestelAcTurboSleepOffset) & 0xF) == kVestelAcSleep;
}

// Set the Turbo state of the A/C.
void IRVestelAc::setTurbo(const bool state) {
  remote_state &= ~((uint64_t)0xF << kVestelAcTurboSleepOffset);
  remote_state |= (uint64_t)(state ? kVestelAcTurbo : kVestelAcNormal)
                  << kVestelAcTurboSleepOffset;
  use_time_state = false;
}

// Return the Turbo state of the A/C.
bool IRVestelAc::getTurbo() {
  return ((remote_state >> kVestelAcTurboSleepOffset) & 0xF) == kVestelAcTurbo;
}

// Set the Ion state of the A/C.
void IRVestelAc::setIon(const bool state) {
  remote_state &= ~((uint64_t)0x1 << kVestelAcIonOffset);

  remote_state |= (uint64_t)(state ? 1 : 0) << kVestelAcIonOffset;
  use_time_state = false;
}

// Return the Ion state of the A/C.
bool IRVestelAc::getIon() { return (remote_state >> kVestelAcIonOffset) & 1; }

// Set the Swing Roaming state of the A/C.
void IRVestelAc::setSwing(const bool state) {
  remote_state &= ~((uint64_t)0xF << kVestelAcSwingOffset);

  remote_state |= (uint64_t)(state ? kVestelAcSwing : 0xF)
                  << kVestelAcSwingOffset;
  use_time_state = false;
}

// Return the Swing Roaming state of the A/C.
bool IRVestelAc::getSwing() {
  return ((remote_state >> kVestelAcSwingOffset) & 0xF) == kVestelAcSwing;
}

// Calculate the checksum for a given array.
// Args:
//   state:  The state to calculate the checksum over.
// Returns:
//   The 8 bit checksum value.
uint8_t IRVestelAc::calcChecksum(const uint64_t state) {
  // Just counts the set bits +1 on stream and take inverse after mask
  uint8_t sum = 0;
  uint64_t temp_state = state & kVestelAcCRCMask;
  for (; temp_state; temp_state >>= 1)
    if (temp_state & 1) sum++;
  sum += 2;
  sum = 0xff - sum;
  return sum;
}

// Verify the checksum is valid for a given state.
// Args:
//   state:  The state to verify the checksum of.
// Returns:
//   A boolean.
bool IRVestelAc::validChecksum(const uint64_t state) {
  return (((state >> kVestelAcChecksumOffset) & 0xFF) == calcChecksum(state));
}

// Calculate & set the checksum for the current internal state of the remote.
void IRVestelAc::checksum() {
  // Stored the checksum value in the last byte.
  remote_state &= ~((uint64_t)0xFF << kVestelAcChecksumOffset);
  remote_state |= (uint64_t)calcChecksum(remote_state)
                  << kVestelAcChecksumOffset;

  remote_time_state &= ~((uint64_t)0xFF << kVestelAcChecksumOffset);
  remote_time_state |= (uint64_t)calcChecksum(remote_time_state)
                       << kVestelAcChecksumOffset;
}

bool IRVestelAc::isTimeCommand() {
  return (remote_state >> kVestelAcPowerOffset == 0x00 || use_time_state);
}

// Convert the internal state into a human readable string.
#ifdef ARDUINO
String IRVestelAc::toString() {
  String result = "";
#else
std::string IRVestelAc::toString() {
  std::string result = "";
#endif  // ARDUINO
  if (isTimeCommand()) {
    result += "Time: " + IRHaierAC::timeToString(getTime());

    result += ", Timer: ";
    result += isTimerActive() ? IRHaierAC::timeToString(getTimer()) : "Off";

    result += ", On Timer: ";
    result += (isOnTimerActive() && !isTimerActive())
                  ? IRHaierAC::timeToString(getOnTimer())
                  : "Off";

    result += ", Off Timer: ";
    result +=
        isOffTimerActive() ? IRHaierAC::timeToString(getOffTimer()) : "Off";
    return result;
  }
  // Not a time command, it's a normal command.
  result += "Power: ";
  result += (getPower() ? "On" : "Off");
  result += ", Mode: " + uint64ToString(getMode());
  switch (getMode()) {
    case kVestelAcAuto:
      result += " (AUTO)";
      break;
    case kVestelAcCool:
      result += " (COOL)";
      break;
    case kVestelAcHeat:
      result += " (HEAT)";
      break;
    case kVestelAcDry:
      result += " (DRY)";
      break;
    case kVestelAcFan:
      result += " (FAN)";
      break;
    default:
      result += " (UNKNOWN)";
  }
  result += ", Temp: " + uint64ToString(getTemp()) + "C";
  result += ", Fan: " + uint64ToString(getFan());
  switch (getFan()) {
    case kVestelAcFanAuto:
      result += " (AUTO)";
      break;
    case kVestelAcFanLow:
      result += " (LOW)";
      break;
    case kVestelAcFanMed:
      result += " (MEDIUM)";
      break;
    case kVestelAcFanHigh:
      result += " (HIGH)";
      break;
    case kVestelAcFanAutoCool:
      result += " (AUTO COOL)";
      break;
    case kVestelAcFanAutoHot:
      result += " (AUTO HOT)";
      break;
    default:
      result += " (UNKNOWN)";
  }
  result += ", Sleep: ";
  result += (getSleep() ? "On" : "Off");
  result += ", Turbo: ";
  result += (getTurbo() ? "On" : "Off");
  result += ", Ion: ";
  result += (getIon() ? "On" : "Off");
  result += ", Swing: ";
  result += (getSwing() ? "On" : "Off");
  return result;
}

#if DECODE_VESTEL_AC
// Decode the supplied Vestel message.
//
// Args:
//   results: Ptr to the data to decode and where to store the decode result.
//   nbits:   The number of data bits to expect. Typically kVestelBits.
//   strict:  Flag indicating if we should perform strict matching.
// Returns:
//   boolean: True if it can decode it, false if it can't.
//
// Status: Alpha / Needs testing against a real device.
//
bool IRrecv::decodeVestelAc(decode_results* results, uint16_t nbits,
                            bool strict) {
  if (nbits % 8 != 0)  // nbits has to be a multiple of nr. of bits in a byte.
    return false;

  if (strict)
    if (nbits != kVestelAcBits)
      return false;  // Not strictly a Vestel AC message.

  uint64_t data = 0;
  uint16_t offset = kStartOffset;

  if (nbits > sizeof(data) * 8)
    return false;  // We can't possibly capture a Vestel packet that big.

  // Header
  if (!matchMark(results->rawbuf[offset++], kVestelAcHdrMark)) return false;
  if (!matchSpace(results->rawbuf[offset++], kVestelAcHdrSpace)) return false;

  // Data (Normal)
  match_result_t data_result =
      matchData(&(results->rawbuf[offset]), nbits, kVestelAcBitMark,
                kVestelAcOneSpace, kVestelAcBitMark, kVestelAcZeroSpace,
                kVestelAcTolerance, kMarkExcess, false);

  if (data_result.success == false) return false;
  offset += data_result.used;
  data = data_result.data;

  // Footer
  if (!matchMark(results->rawbuf[offset++], kVestelAcBitMark)) return false;

  // Compliance
  if (strict)
    if (!IRVestelAc::validChecksum(data_result.data)) return false;

  // Success
  results->decode_type = VESTEL_AC;
  results->bits = nbits;
  results->value = data;
  results->address = 0;
  results->command = 0;

  return true;
}
#endif  // DECODE_VESTEL_AC
