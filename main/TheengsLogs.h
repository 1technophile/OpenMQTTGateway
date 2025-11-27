/**
 * @file TheengsLogs.h
 * @brief Cross-platform native logging for OpenMQTTGateway
 * 
 * Provides unified logging interface:
 * - Arduino (ESP32/ESP8266): Uses ArduinoLog library
 * - Native/test: Uses fprintf to stderr (compatible with GoogleTest)
 * 
 * Features:
 * - Full F() macro compatibility (flash string support)
 * - Compatible with existing THEENGS_LOG_* macros across 50+ files
 * - Platform-optimized implementations for each target
 */

#pragma once

// ============================================================================
// PLATFORM DETECTION
// ============================================================================

// Detect if we're on Arduino platforms (ESP32/ESP8266/AVR)
#if defined(ARDUINO) && !defined(UNIT_TEST) && !defined(UNIT_TEST_NATIVE)
#  define THEENGS_PLATFORM_ARDUINO 1
#else
#  define THEENGS_PLATFORM_ARDUINO 0
#endif

// Detect specific ESP platforms for optimization
#if defined(ESP32)
#  define THEENGS_PLATFORM_ESP32 1
#else
#  define THEENGS_PLATFORM_ESP32 0
#endif

#if defined(ESP8266)
#  define THEENGS_PLATFORM_ESP8266 1
#else
#  define THEENGS_PLATFORM_ESP8266 0
#endif

// ============================================================================
// PLATFORM-SPECIFIC HEADERS & MACROS
// ============================================================================

#if THEENGS_PLATFORM_ARDUINO
#  include <Arduino.h>
// ESP32 and others: Use ArduinoLog library
// Declare the global Log object (defined in main.cpp)
#  include <ArduinoLog.h>
extern Logging Log;
#endif

// Define CR (Carriage Return) for all platforms
#ifndef CR
#  define CR "\n"
#endif

// ============================================================================
// LOG LEVEL DEFINITIONS
// ============================================================================

#ifndef LOG_LEVEL_SILENT
#  define LOG_LEVEL_SILENT 0
#endif
#ifndef LOG_LEVEL_FATAL
#  define LOG_LEVEL_FATAL 1
#endif
#ifndef LOG_LEVEL_ERROR
#  define LOG_LEVEL_ERROR 2
#endif
#ifndef LOG_LEVEL_WARNING
#  define LOG_LEVEL_WARNING 3
#endif
#ifndef LOG_LEVEL_NOTICE
#  define LOG_LEVEL_NOTICE 4
#endif
#ifndef LOG_LEVEL_TRACE
#  define LOG_LEVEL_TRACE 5
#endif
#ifndef LOG_LEVEL_VERBOSE
#  define LOG_LEVEL_VERBOSE 6
#endif

// Set default log level if not specified
#ifndef LOG_LEVEL
#  define LOG_LEVEL LOG_LEVEL_NOTICE
#endif

// ============================================================================
// PLATFORM-SPECIFIC LOGGING IMPLEMENTATIONS
// ============================================================================

#if THEENGS_PLATFORM_ESP8266
// The issue: ESP8266's F() macro (FPSTR(PSTR())) doesn't work with variadic templates
// This is a known limitation of the ESP8266 Arduino core preprocessor
// Use Serial.printf_P() directly instead of ArduinoLog for ESP8266 if needed
// Disable the use of F() on ESP8266 to avoid confusion
#  define F(x) (x)
#endif

#if THEENGS_PLATFORM_ARDUINO
// ESP32 and other Arduino platforms: Use ArduinoLog library
// ArduinoLog supports F() strings on these platforms

#  define THEENGS_LOG_IMPL_VERBOSE(fmt, ...) Log.verbose(fmt, ##__VA_ARGS__)
#  define THEENGS_LOG_IMPL_TRACE(fmt, ...)   Log.trace(fmt, ##__VA_ARGS__)
#  define THEENGS_LOG_IMPL_NOTICE(fmt, ...)  Log.notice(fmt, ##__VA_ARGS__)
#  define THEENGS_LOG_IMPL_WARNING(fmt, ...) Log.warning(fmt, ##__VA_ARGS__)
#  define THEENGS_LOG_IMPL_ERROR(fmt, ...)   Log.error(fmt, ##__VA_ARGS__)
#  define THEENGS_LOG_IMPL_FATAL(fmt, ...)   Log.fatal(fmt, ##__VA_ARGS__)

#else
// Native/test platform: Use fprintf to stderr
#  ifndef F
#    define F(x) (x)
#  endif

#  ifdef __cplusplus
extern "C" {
#  endif

#  include <stdarg.h>
#  include <stdio.h>
/**
    * @brief Native platform logging implementation
    * 
    * Thread-safe logging for native/test environments.
    * Uses stderr for output with proper formatting.
    * 
    * @param level Log level string (e.g., "ERROR", "WARNING")
    * @param fmt printf-style format string
    * @param ... Variable arguments matching format string
    */
static inline void native_log(const char* level, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  // Print log level prefix
  fprintf(stderr, "[%s] ", level);

  // Print formatted message
  vfprintf(stderr, fmt, args);

  va_end(args);

  // Flush to ensure immediate output (important for debugging)
  fflush(stderr);
}

#  ifdef __cplusplus
}
#  endif

#  define THEENGS_LOG_IMPL_VERBOSE(...) native_log("VERBOSE", __VA_ARGS__)
#  define THEENGS_LOG_IMPL_TRACE(...)   native_log("TRACE", __VA_ARGS__)
#  define THEENGS_LOG_IMPL_NOTICE(...)  native_log("NOTICE", __VA_ARGS__)
#  define THEENGS_LOG_IMPL_WARNING(...) native_log("WARNING", __VA_ARGS__)
#  define THEENGS_LOG_IMPL_ERROR(...)   native_log("ERROR", __VA_ARGS__)
#  define THEENGS_LOG_IMPL_FATAL(...)   native_log("FATAL", __VA_ARGS__)

#endif

// ============================================================================
// UNIFIED PUBLIC LOGGING MACROS
// ============================================================================

/**
 * Logging macros with compile-time optimization:
 * - When LOG_LEVEL excludes a level, macro becomes ((void)0) - zero overhead
 * - Supports usage in ternary operators and expressions
 * - Compatible with F() macro on all platforms
 * - Format: THEENGS_LOG_<LEVEL>(format, ...)
 * 
 * Example usage:
 *   THEENGS_LOG_NOTICE(F("WiFi connected: %s" CR), WiFi.localIP().toString().c_str());
 *   THEENGS_LOG_ERROR(F("Sensor read failed: %d" CR), error_code);
 *   THEENGS_LOG_TRACE("Debug value: %d" CR, value);  // Without F() also works
 */

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
#  define THEENGS_LOG_VERBOSE(...) THEENGS_LOG_IMPL_VERBOSE(__VA_ARGS__)
#else
#  define THEENGS_LOG_VERBOSE(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
#  define THEENGS_LOG_TRACE(...) THEENGS_LOG_IMPL_TRACE(__VA_ARGS__)
#else
#  define THEENGS_LOG_TRACE(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_NOTICE
#  define THEENGS_LOG_NOTICE(...) THEENGS_LOG_IMPL_NOTICE(__VA_ARGS__)
#else
#  define THEENGS_LOG_NOTICE(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARNING
#  define THEENGS_LOG_WARNING(...) THEENGS_LOG_IMPL_WARNING(__VA_ARGS__)
#else
#  define THEENGS_LOG_WARNING(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#  define THEENGS_LOG_ERROR(...) THEENGS_LOG_IMPL_ERROR(__VA_ARGS__)
#else
#  define THEENGS_LOG_ERROR(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_FATAL
#  define THEENGS_LOG_FATAL(...) THEENGS_LOG_IMPL_FATAL(__VA_ARGS__)
#else
#  define THEENGS_LOG_FATAL(...) ((void)0)
#endif
