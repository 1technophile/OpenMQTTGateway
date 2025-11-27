#pragma once

/**
 * @file mock_arduino.h
 * @brief Mock Arduino framework functions for unit testing
 * 
 * This file provides mock implementations of Arduino framework functions
 * to enable unit testing without actual hardware dependencies.
 */

#ifdef UNIT_TEST

#  include <cstdint>
#  include <string>

// Arduino constants
#  define HIGH         1
#  define LOW          0
#  define INPUT        0
#  define OUTPUT       1
#  define INPUT_PULLUP 2

// Mock pin definitions
#  define LED_BUILTIN 2

// Time functions
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

// Digital I/O
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);

// Analog I/O
int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int val);

// Serial mock
class MockSerial {
public:
  void begin(unsigned long baud);
  void print(const char* str);
  void print(const std::string& str);
  void print(int value);
  void print(float value);
  void println(const char* str);
  void println(const std::string& str);
  void println(int value);
  void println(float value);
  void println();
  bool available();
  char read();
};

extern MockSerial Serial;

// String class mock (simplified)
class String {
public:
  String();
  String(const char* str);
  String(const std::string& str);
  String(int value);
  String(float value);

  const char* c_str() const;
  size_t length() const;
  String& operator+=(const String& other);
  String operator+(const String& other) const;
  bool operator==(const String& other) const;

private:
  std::string data_;
};

// Test utilities for Arduino mocks
namespace ArduinoMock {
void reset();
void setMillis(unsigned long value);
void setMicros(unsigned long value);
void setPinState(uint8_t pin, int value);
int getPinState(uint8_t pin);
void setAnalogValue(uint8_t pin, int value);
} // namespace ArduinoMock

#endif // UNIT_TEST