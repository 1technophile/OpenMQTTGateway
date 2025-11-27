#pragma once

#include <cstddef>
#include <string>

/**
 * @brief Interface for key-value storage abstraction (e.g. Preferences, SPIFFS, etc.)
 *
 * This interface allows to decouple storage logic from implementation (NVS, SPIFFS, etc.).
 *
 * All methods mirror the minimal subset used by RFConfiguration for preferences management.
 */
class IStorage {
public:
  virtual ~IStorage() = default;

  virtual const char* getNamespace() = 0;

  /**
     * @brief Open the storage namespace
     * @param readOnly True for read-only, false for read-write
     * @return true if opened successfully
     */
  virtual bool begin(bool readOnly) = 0;

  /**
     * @brief Close the storage
     */
  virtual void end() = 0;

  /**
     * @brief Check if a key exists
     * @param key Key name
     * @return true if key exists
     */
  virtual bool isKey(const char* key) = 0;

  /**
     * @brief Get a string value for a key
     * @param key Key name
     * @param defaultValue Default value if key not found
     * @return String value
     */
  virtual const char* getString(const char* key, const char* defaultValue = "") = 0;

  /**
     * @brief Put a string value for a key
     * @param key Key name
     * @param value Value to store
     * @return Number of bytes written
     */
  virtual size_t putString(const char* key, const char* value) = 0;

  /**
     * @brief Remove a key from storage
     * @param key Key name
     * @return 1 if removed, 0 if not found
     */
  virtual int remove(const char* key) = 0;
};