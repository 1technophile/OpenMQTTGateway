#pragma once

#include <gmock/gmock.h>
#include <storage/IStorage.h>

#include <string>

/**
 * @brief Mock implementation of IStorage for unit testing
 */
class MockStorage : public IStorage {
public:
  MOCK_METHOD(bool, begin, (bool readOnly), (override));
  MOCK_METHOD(const char*, getNamespace, (), (override));
  MOCK_METHOD(void, end, (), (override));
  MOCK_METHOD(bool, isKey, (const char* key), (override));
  MOCK_METHOD(const char*, getString, (const char* key, const char* defaultValue), (override));
  MOCK_METHOD(size_t, putString, (const char* key, const char* value), (override));
  MOCK_METHOD(int, remove, (const char* key), (override));

  // Stored data for simulating storage
  std::string storedData;
};
