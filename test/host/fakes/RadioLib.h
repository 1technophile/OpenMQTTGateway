#ifndef TEST_FAKE_RADIOLIB_H
#define TEST_FAKE_RADIOLIB_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "SPI.h"

constexpr int16_t RADIOLIB_ERR_NONE = 0;
constexpr int16_t RADIOLIB_ERR_PACKET_TOO_LONG = -4;

class Module {
public:
  Module(int, int, int, int, SPIClass&) {}
};

namespace fake_radiolib {
inline int16_t beginResult = RADIOLIB_ERR_NONE;
inline int16_t operationResult = RADIOLIB_ERR_NONE;
inline int16_t receiveResult = RADIOLIB_ERR_NONE;
inline int16_t readResult = RADIOLIB_ERR_NONE;
inline int16_t transmitResult = RADIOLIB_ERR_NONE;
inline float frequency = 0;
inline float bandwidth = 0;
inline uint8_t spreadingFactor = 0;
inline uint8_t codingRate = 0;
inline uint8_t syncWord = 0;
inline int8_t outputPower = 0;
inline uint16_t preambleLength = 0;
inline float tcxoVoltage = 0;
inline bool crc = false;
inline bool invertedIQ = false;
inline bool dio2RfSwitch = true;
inline int receiveCalls = 0;
inline std::vector<uint8_t> received;
inline std::vector<uint8_t> transmitted;
inline float rssi = -88.0f;
inline float snr = 5.25f;
inline float frequencyError = -320.0f;
inline void (*packetAction)() = nullptr;

inline void reset() {
  beginResult = RADIOLIB_ERR_NONE;
  operationResult = RADIOLIB_ERR_NONE;
  receiveResult = RADIOLIB_ERR_NONE;
  readResult = RADIOLIB_ERR_NONE;
  transmitResult = RADIOLIB_ERR_NONE;
  frequency = 0;
  bandwidth = 0;
  spreadingFactor = 0;
  codingRate = 0;
  syncWord = 0;
  outputPower = 0;
  preambleLength = 0;
  tcxoVoltage = 0;
  crc = false;
  invertedIQ = false;
  dio2RfSwitch = true;
  receiveCalls = 0;
  received.clear();
  transmitted.clear();
  packetAction = nullptr;
}

inline void deliver(std::vector<uint8_t> payload) {
  received = std::move(payload);
  if (packetAction != nullptr) packetAction();
}
} // namespace fake_radiolib

class SX1262 {
public:
  explicit SX1262(Module*) {}

  int16_t begin(float frequencyValue, float bandwidthValue, uint8_t spreadingFactorValue,
                uint8_t codingRateValue, uint8_t syncWordValue, int8_t powerValue,
                uint16_t preambleLengthValue, float tcxoVoltageValue) {
    fake_radiolib::frequency = frequencyValue;
    fake_radiolib::bandwidth = bandwidthValue;
    fake_radiolib::spreadingFactor = spreadingFactorValue;
    fake_radiolib::codingRate = codingRateValue;
    fake_radiolib::syncWord = syncWordValue;
    fake_radiolib::outputPower = powerValue;
    fake_radiolib::preambleLength = preambleLengthValue;
    fake_radiolib::tcxoVoltage = tcxoVoltageValue;
    return fake_radiolib::beginResult;
  }

  int16_t setDio2AsRfSwitch(bool enabled) {
    fake_radiolib::dio2RfSwitch = enabled;
    return fake_radiolib::operationResult;
  }

  void setPacketReceivedAction(void (*action)()) {
    fake_radiolib::packetAction = action;
  }

  int16_t standby() { return fake_radiolib::operationResult; }
  int16_t setFrequency(float value) {
    fake_radiolib::frequency = value;
    return fake_radiolib::operationResult;
  }
  int16_t setOutputPower(int8_t value) {
    fake_radiolib::outputPower = value;
    return fake_radiolib::operationResult;
  }
  int16_t setSpreadingFactor(uint8_t value) {
    fake_radiolib::spreadingFactor = value;
    return fake_radiolib::operationResult;
  }
  int16_t setBandwidth(float value) {
    fake_radiolib::bandwidth = value;
    return fake_radiolib::operationResult;
  }
  int16_t setCodingRate(uint8_t value) {
    fake_radiolib::codingRate = value;
    return fake_radiolib::operationResult;
  }
  int16_t setPreambleLength(size_t value) {
    fake_radiolib::preambleLength = static_cast<uint16_t>(value);
    return fake_radiolib::operationResult;
  }
  int16_t setSyncWord(uint8_t value) {
    fake_radiolib::syncWord = value;
    return fake_radiolib::operationResult;
  }
  int16_t setCRC(uint8_t length) {
    fake_radiolib::crc = length != 0;
    return fake_radiolib::operationResult;
  }
  int16_t invertIQ(bool enabled) {
    fake_radiolib::invertedIQ = enabled;
    return fake_radiolib::operationResult;
  }
  int16_t startReceive() {
    ++fake_radiolib::receiveCalls;
    return fake_radiolib::receiveResult;
  }
  size_t getPacketLength() const { return fake_radiolib::received.size(); }
  int16_t readData(uint8_t* output, size_t length) {
    std::copy_n(fake_radiolib::received.begin(), length, output);
    return fake_radiolib::readResult;
  }
  float getRSSI() const { return fake_radiolib::rssi; }
  float getSNR() const { return fake_radiolib::snr; }
  float getFrequencyError() const { return fake_radiolib::frequencyError; }
  int16_t transmit(const uint8_t* payload, size_t length) {
    fake_radiolib::transmitted.assign(payload, payload + length);
    return fake_radiolib::transmitResult;
  }
};

#endif
