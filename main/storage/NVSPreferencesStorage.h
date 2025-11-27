#pragma once

#ifdef ESP32
#  include <Preferences.h>
#  include <storage/IStorage.h>

/**
 * @brief Concrete implementation of IStorage using ESP32 Preferences (NVS)
 */
/**
 * @class NVSPreferencesStorage
 * @brief A class that provides storage functionality using NVS (Non-Volatile Storage) preferences.
 * 
 * This class implements the IStorage interface to provide methods for storing, retrieving,
 * and managing key-value pairs in non-volatile storage. It uses the Preferences library
 * for interacting with the underlying storage mechanism.
 */
class NVSPreferencesStorage : public IStorage {
public:
  /**
   * @brief Constructs an NVSPreferencesStorage object.
   */
  NVSPreferencesStorage();

  /**
   * @brief Constructs an NVSPreferencesStorage object with external Preferences.
   * @param preferences External Preferences instance to use
   */
  NVSPreferencesStorage(Preferences* preferences);

  /**
   * @brief Destroys the NVSPreferencesStorage object and releases any resources.
   */
  ~NVSPreferencesStorage() override;

  const char* getNamespace() override;

  /**
   * @brief Initializes the storage with a given namespace.
   * 
   * @param name The namespace to use for the storage.
   * @param readOnly If true, opens the storage in read-only mode.
   * @return True if the storage was successfully initialized, false otherwise.
   */
  bool begin(bool readOnly) override;

  /**
   * @brief Ends the storage session and releases resources.
   */
  void end() override;

  /**
   * @brief Checks if a key exists in the storage.
   * 
   * @param key The key to check for existence.
   * @return True if the key exists, false otherwise.
   */
  bool isKey(const char* key) override;

  /**
   * @brief Retrieves a string value associated with a key.
   * 
   * @param key The key to retrieve the value for.
   * @param defaultValue The default value to return if the key does not exist.
   * @return The string value associated with the key, or the default value if the key does not exist.
   */
  const char* getString(const char* key, const char* defaultValue = "") override;

  /**
   * @brief Stores a string value associated with a key.
   * 
   * @param key The key to associate the value with.
   * @param value The string value to store.
   * @return The size of the value stored, or 0 if the operation failed.
   */
  size_t putString(const char* key, const char* value) override;

  /**
   * @brief Removes a key-value pair from the storage.
   * 
   * @param key The key to remove.
   * @return 0 if the key was successfully removed, or an error code otherwise.
   */
  int remove(const char* key) override;

private:
  Preferences* preferences; ///< Pointer to Preferences object used for NVS operations
  bool ownsPreferences; ///< Flag indicating if this instance owns the Preferences object
};
#endif