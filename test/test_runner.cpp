#include <gtest/gtest.h>
// uncomment line below if you plan to use GMock
// #include <gmock/gmock.h>

// TEST(...)
// TEST_F(...)

TEST(TestEnvironment, TestDummy) {
  ASSERT_TRUE(true);
  ASSERT_FALSE(false);
  ASSERT_EQ(1, 1);
  ASSERT_NE(1, 2);
  ASSERT_LT(1, 2);
  ASSERT_LE(1, 1);
  ASSERT_GT(2, 1);
  ASSERT_GE(1, 1);
}

#if defined(ARDUINO)
#  include <Arduino.h>

void setup() {
  // should be the same value as for the `test_speed` option in "platformio.ini"
  // default value is test_speed=115200
  Serial.begin(115200);

  ::testing::InitGoogleTest();
  // if you plan to use GMock, replace the line above with
  // ::testing::InitGoogleMock();
}

void loop() {
  // Run tests
  if (RUN_ALL_TESTS())
    ;

  // sleep for 1 sec
  delay(1000);
}

#else
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  // if you plan to use GMock, replace the line above with
  // ::testing::InitGoogleMock(&argc, argv);

  if (RUN_ALL_TESTS())
    ;

  // Always return zero-code and allow PlatformIO to parse results
  return 0;
}
#endif