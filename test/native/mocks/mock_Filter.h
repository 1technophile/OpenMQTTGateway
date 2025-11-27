#include <core/Filter.h> // Include the RFReceiver base class
#include <gmock/gmock.h> // Brings in gMock.

class MockFilter : public Filter {
public:
  MOCK_METHOD(void, from, (JsonObject & data), (override));
  MOCK_METHOD(void, to, (JsonObject & data), (override));
  MOCK_METHOD(void, to, (const char* data), ());
  MOCK_METHOD(void, add, (const char* value, bool inWhitelist), ());
  MOCK_METHOD(void, remove, (const char* value), ());
  MOCK_METHOD(bool, contains, (const char* key, const char* value, bool inWhitelist), ());
  MOCK_METHOD(bool, regex_match, (const char* key, const char* pattern, bool inWhitelist), ());
  MOCK_METHOD(bool, isEmpty, (bool inWhitelist), (const));
};