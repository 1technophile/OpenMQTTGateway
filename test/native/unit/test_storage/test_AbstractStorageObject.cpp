#include <ArduinoJson.h>
#include <config_JSONMessages.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <storage/AbstractStorageObject.h>
#include <storage/IJsonable.h>
#include <storage/IStorage.h>

#include <cstring>
#include <string>

#include "../mocks/mock_IStorage.h"
#include "../mocks/mock_arduino.h"

// A test class derived from AbstractStorageObject for testing purposes

class TestStorageObject : public AbstractStorageObject {
public:
  TestStorageObject(IStorage& storageRef, const char* rootKey)
      : AbstractStorageObject(storageRef, rootKey),
        testValue(0),
        testBool(false),
        testString("") {}

  // Implement IJsonable interface
  void from(JsonObject& data) override {
    if (data.containsKey("testValue")) {
      testValue = data["testValue"].as<int>();
    }
    if (data.containsKey("testBool")) {
      testBool = data["testBool"].as<bool>();
    }
    if (data.containsKey("testString")) {
      testString = data["testString"].as<std::string>();
    }
  }

  void to(JsonObject& data) override {
    data["testValue"] = testValue;
    data["testBool"] = testBool;
    data["testString"] = testString;
  }

  // Test members
  int testValue;
  bool testBool;
  std::string testString;
};

using namespace testing;

class AbstractStorageObjectTest : public Test {
protected:
  void SetUp() override {
    testObj = new TestStorageObject(mockStorage, "testKey");
  }

  void TearDown() override {
    delete testObj;
  }

  MockStorage mockStorage;
  TestStorageObject* testObj;
};

// Test successful save operation
TEST_F(AbstractStorageObjectTest, SaveOnStorage_Success) {
  testObj->testValue = 42;
  testObj->testBool = true;
  testObj->testString = "hello";

  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(StrEq("testKey"), _)).WillOnce(DoAll(SaveArg<1>(&mockStorage.storedData), Return(10)));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->saveOnStorage();
  ASSERT_TRUE(result);

  // Verify JSON content
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  deserializeJson(doc, mockStorage.storedData);
  EXPECT_EQ(doc["testValue"], 42);
  EXPECT_EQ(doc["testBool"], true);
  EXPECT_STREQ(doc["testString"], "hello");
}

// Test save with empty/default values
TEST_F(AbstractStorageObjectTest, SaveOnStorage_EmptyValues) {
  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(StrEq("testKey"), _))
      .WillOnce(Return(5));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->saveOnStorage();
  ASSERT_TRUE(result);
}

// Test save failure
TEST_F(AbstractStorageObjectTest, SaveOnStorage_Failure) {
  testObj->testValue = 99;

  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(StrEq("testKey"), _))
      .WillOnce(Return(0));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->saveOnStorage();
  ASSERT_FALSE(result);
}

// Test successful load operation
TEST_F(AbstractStorageObjectTest, LoadFromStorage_Success) {
  std::string jsonData = R"({"testValue":99,"testBool":true,"testString":"loaded"})";
  mockStorage.storedData = jsonData;

  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(StrEq("testKey"), _))
      .WillOnce(Return(mockStorage.storedData.c_str()));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->loadFromStorage();
  ASSERT_TRUE(result);
  ASSERT_EQ(testObj->testValue, 99);
  ASSERT_TRUE(testObj->testBool);
  ASSERT_EQ(testObj->testString, "loaded");
}

// Test load with missing key
TEST_F(AbstractStorageObjectTest, LoadFromStorage_KeyNotFound) {
  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(false));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->loadFromStorage();
  ASSERT_TRUE(result);
}

// Test load with invalid JSON
TEST_F(AbstractStorageObjectTest, LoadFromStorage_InvalidJson) {
  mockStorage.storedData = "{invalid json}";

  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(StrEq("testKey"), _))
      .WillOnce(Return(mockStorage.storedData.c_str()));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->loadFromStorage();
  ASSERT_FALSE(result);
}

// Test load with null JSON
TEST_F(AbstractStorageObjectTest, LoadFromStorage_NullJson) {
  mockStorage.storedData = "null";

  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(StrEq("testKey"), _))
      .WillOnce(Return(mockStorage.storedData.c_str()));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->loadFromStorage();
  ASSERT_FALSE(result);
}

// Test load with empty JSON object
TEST_F(AbstractStorageObjectTest, LoadFromStorage_EmptyJson) {
  mockStorage.storedData = "{}";

  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(StrEq("testKey"), _))
      .WillOnce(Return(mockStorage.storedData.c_str()));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->loadFromStorage();
  ASSERT_TRUE(result);
  // Values should be defaults
  EXPECT_EQ(testObj->testValue, 0);
  EXPECT_FALSE(testObj->testBool);
}

// Test erase with existing key
TEST_F(AbstractStorageObjectTest, EraseStorage_KeyExists) {
  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(true));
  EXPECT_CALL(mockStorage, remove(StrEq("testKey")))
      .WillOnce(Return(1));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->eraseStorage();
  ASSERT_TRUE(result);
}

// Test erase with non-existing key
TEST_F(AbstractStorageObjectTest, EraseStorage_KeyNotFound) {
  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(false));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->eraseStorage();
  ASSERT_FALSE(result);
}

// Test full save-load cycle
TEST_F(AbstractStorageObjectTest, SaveLoadCycle) {
  testObj->testValue = 123;
  testObj->testBool = true;
  testObj->testString = "cycle_test";

  // Save
  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(StrEq("testKey"), _))
      .WillOnce(DoAll(
          SaveArg<1>(&mockStorage.storedData),
          Return(10)));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool saveResult = testObj->saveOnStorage();
  ASSERT_TRUE(saveResult);

  // Reset values
  testObj->testValue = 0;
  testObj->testBool = false;
  testObj->testString = "";

  // Load
  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(StrEq("testKey"), _))
      .WillOnce(Return(mockStorage.storedData.c_str()));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool loadResult = testObj->loadFromStorage();
  ASSERT_TRUE(loadResult);
  EXPECT_EQ(testObj->testValue, 123);
  EXPECT_TRUE(testObj->testBool);
  EXPECT_EQ(testObj->testString, "cycle_test");
}

// Test memory constraints (ESP32 specific)
TEST_F(AbstractStorageObjectTest, MemoryConstraints_LargeString) {
  // Test with string near ESP32 typical limits
  testObj->testString = std::string(200, 'x');
  testObj->testValue = 999;

  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(StrEq("testKey"), _))
      .WillOnce(DoAll(
          SaveArg<1>(&mockStorage.storedData),
          Return(10)));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool result = testObj->saveOnStorage();
  ASSERT_TRUE(result);

  // Verify serialization succeeded
  StaticJsonDocument<512> doc;
  auto error = deserializeJson(doc, mockStorage.storedData);
  EXPECT_FALSE(error);
}

// Test special characters in JSON
TEST_F(AbstractStorageObjectTest, SpecialCharacters) {
  testObj->testString = "Test\"Quote\\Backslash\nNewline";
  testObj->testValue = 42;

  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(StrEq("testKey"), _))
      .WillOnce(DoAll(
          SaveArg<1>(&mockStorage.storedData),
          Return(10)));
  EXPECT_CALL(mockStorage, end()).Times(1);

  bool saveResult = testObj->saveOnStorage();
  ASSERT_TRUE(saveResult);

  // Load back
  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(StrEq("testKey")))
      .WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(StrEq("testKey"), _))
      .WillOnce(Return(mockStorage.storedData.c_str()));
  EXPECT_CALL(mockStorage, end()).Times(1);

  testObj->testString = "";
  bool loadResult = testObj->loadFromStorage();
  ASSERT_TRUE(loadResult);
  EXPECT_EQ(testObj->testString, "Test\"Quote\\Backslash\nNewline");
}
