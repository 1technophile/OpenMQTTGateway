#pragma once

#include <storage/IStorage.h>

/**
 * @brief Minimal no-op storage implementation for non-ESP32 targets.
 *
 * Provides an IStorage that compiles and safely does nothing.
 * Useful on ESP8266 where Preferences/NVS is unavailable.
 */
class NoopStorage : public IStorage {
public:
  const char* getNamespace() override { return "noop"; }
  bool begin(bool /*readOnly*/) override { return true; }
  void end() override {}
  bool isKey(const char* /*key*/) override { return false; }
  const char* getString(const char* /*key*/, const char* defaultValue = "") override { return defaultValue; }
  size_t putString(const char* /*key*/, const char* /*value*/) override { return 0; }
  int remove(const char* /*key*/) override { return 0; }
};