/*
  Minimal Arduino core stand-in for the host tests.

  Only the String behaviour that main/webUIMessage.h relies on is modelled:
  construction from a C string, concatenation, append of a char or a C string,
  reserve(), length() and c_str(). It is backed by std::string so the tests stay
  free of the embedded toolchain.

  This header is NOT used by the firmware build, which gets the real core.
*/
#ifndef HOST_TEST_ARDUINO_H
#define HOST_TEST_ARDUINO_H

#include <stddef.h>
#include <string.h>

#include <string>

class String {
public:
  String() {}
  String(const char* text) : _value(text != NULL ? text : "") {}
  String(const std::string& text) : _value(text) {}

  String& operator+=(const String& rhs) {
    _value += rhs._value;
    return *this;
  }
  String& operator+=(const char* rhs) {
    if (rhs != NULL) _value += rhs;
    return *this;
  }
  String& operator+=(char rhs) {
    _value += rhs;
    return *this;
  }

  friend String operator+(const String& lhs, const String& rhs) {
    return String(lhs._value + rhs._value);
  }
  friend String operator+(const String& lhs, const char* rhs) {
    return String(lhs._value + (rhs != NULL ? rhs : ""));
  }
  friend String operator+(const char* lhs, const String& rhs) {
    return String(std::string(lhs != NULL ? lhs : "") + rhs._value);
  }

  bool operator==(const String& rhs) const {
    return _value == rhs._value;
  }
  bool operator==(const char* rhs) const {
    return rhs != NULL && _value == rhs;
  }
  bool operator!=(const String& rhs) const {
    return !(*this == rhs);
  }

  void reserve(size_t size) {
    _value.reserve(size);
  }
  size_t length() const {
    return _value.length();
  }
  const char* c_str() const {
    return _value.c_str();
  }

private:
  std::string _value;
};

#endif
