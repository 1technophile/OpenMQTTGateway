#ifndef TEST_FAKE_ARDUINO_H
#define TEST_FAKE_ARDUINO_H

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

using byte = uint8_t;

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int OUTPUT = 1;

namespace fake_arduino {
inline std::vector<std::pair<int, int>> pinModes;
inline std::vector<std::pair<int, int>> digitalWrites;
inline std::vector<unsigned long> delays;

inline void reset() {
  pinModes.clear();
  digitalWrites.clear();
  delays.clear();
}
} // namespace fake_arduino

inline void pinMode(int pin, int mode) {
  fake_arduino::pinModes.emplace_back(pin, mode);
}

inline void digitalWrite(int pin, int value) {
  fake_arduino::digitalWrites.emplace_back(pin, value);
}

inline void delay(unsigned long milliseconds) {
  fake_arduino::delays.push_back(milliseconds);
}

#endif
