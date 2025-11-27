#ifdef ESP32

#  include <Arduino.h>
#  include <gtest/gtest.h>

#  include "../../main/storage/NVSPreferencesStorage.h"

// Basic embedded tests for NVSPreferencesStorage
TEST(NVSPreferencesStorageTest, BasicPutGetRemove) {
  NVSPreferencesStorage storage;

  // Use a test namespace to avoid colliding with real data
  ASSERT_TRUE(storage.begin("unittest", false));

  // Ensure key does not exist initially
  EXPECT_FALSE(storage.isKey("ut_key"));

  // Put string and verify returned size > 0
  std::string value = "hello_nvs";
  size_t written = storage.putString("ut_key", value);
  EXPECT_GT(written, 0);

  // Read back
  std::string read = storage.getString("ut_key", "");
  EXPECT_EQ(read, value);
  EXPECT_TRUE(storage.isKey("ut_key"));

  // Remove key and verify it's gone
  storage.remove("ut_key");
  EXPECT_FALSE(storage.isKey("ut_key"));

  // Cleanup namespace
  storage.end();
}

TEST(NVSPreferencesStorageTest, OverwriteAndPersistence) {
  NVSPreferencesStorage storage;
  ASSERT_TRUE(storage.begin("unittest", false));

  // Write initial value
  storage.putString("ut_key2", "v1");
  EXPECT_EQ(storage.getString("ut_key2", ""), "v1");

  // Overwrite with new value
  storage.putString("ut_key2", "v2");
  EXPECT_EQ(storage.getString("ut_key2", ""), "v2");

  // End and reopen in read-only mode to simulate later access
  storage.end();

  NVSPreferencesStorage storage2;
  ASSERT_TRUE(storage2.begin("unittest", true));
  EXPECT_EQ(storage2.getString("ut_key2", ""), "v2");

  // Cleanup
  storage2.remove("ut_key2");
  storage2.end();
}

#else

#  include <gtest/gtest.h>

TEST(NVSPreferencesStorageTest, SkipOnNonESP32) {
  GTEST_SKIP();
}

#endif
