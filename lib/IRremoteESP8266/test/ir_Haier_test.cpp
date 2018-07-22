// Copyright 2018 David Conran

#include "IRrecv.h"
#include "IRrecv_test.h"
#include "IRsend.h"
#include "IRsend_test.h"
#include "ir_Haier.h"
#include "gtest/gtest.h"

// Tests for sendHaierAC()

// Test sending typical data only.
TEST(TestSendHaierAC, SendDataOnly) {
  IRsendTest irsend(0);
  irsend.begin();
  uint8_t haier_zero[HAIER_AC_STATE_LENGTH] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  irsend.reset();
  irsend.sendHaierAC(haier_zero);
  EXPECT_EQ(
      "m3000s3000m3000s4300"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s150000", irsend.outputStr());

  uint8_t haier_test[HAIER_AC_STATE_LENGTH] = {
    0xA5, 0x01, 0x20, 0x01, 0x00, 0xC0, 0x20, 0x00, 0xA7};
  irsend.reset();
  irsend.sendHaierAC(haier_test);
  EXPECT_EQ(
      "m3000s3000m3000s4300"
      "m520s1650m520s650m520s1650m520s650m520s650m520s1650m520s650m520s1650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s1650"
      "m520s650m520s650m520s1650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s1650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s1650m520s1650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s1650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s1650m520s650m520s1650m520s650m520s650m520s1650m520s1650m520s1650"
      "m520s150000", irsend.outputStr());
}

// Test sending typical data with repeats.
TEST(TestSendHaierAC, SendWithRepeats) {
  IRsendTest irsend(0);
  irsend.begin();

  irsend.reset();
  uint8_t haier_test[HAIER_AC_STATE_LENGTH] = {
    0xA5, 0x01, 0x20, 0x01, 0x00, 0xC0, 0x20, 0x00, 0xA7};
  irsend.reset();
  irsend.sendHaierAC(haier_test, HAIER_AC_STATE_LENGTH, 2);  // two repeats.
  EXPECT_EQ(
      "m3000s3000m3000s4300"
      "m520s1650m520s650m520s1650m520s650m520s650m520s1650m520s650m520s1650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s1650"
      "m520s650m520s650m520s1650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s1650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s1650m520s1650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s1650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s1650m520s650m520s1650m520s650m520s650m520s1650m520s1650m520s1650"
      "m520s150000"
      "m3000s3000m3000s4300"
      "m520s1650m520s650m520s1650m520s650m520s650m520s1650m520s650m520s1650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s1650"
      "m520s650m520s650m520s1650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s1650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s1650m520s1650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s1650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s1650m520s650m520s1650m520s650m520s650m520s1650m520s1650m520s1650"
      "m520s150000"
      "m3000s3000m3000s4300"
      "m520s1650m520s650m520s1650m520s650m520s650m520s1650m520s650m520s1650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s1650"
      "m520s650m520s650m520s1650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s1650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s1650m520s1650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s1650m520s650m520s650m520s650m520s650m520s650"
      "m520s650m520s650m520s650m520s650m520s650m520s650m520s650m520s650"
      "m520s1650m520s650m520s1650m520s650m520s650m520s1650m520s1650m520s1650"
      "m520s150000", irsend.outputStr());
}

// Tests for IRHaierAC class.

TEST(TestHaierACClass, Command) {
  IRHaierAC haier(0);
  haier.begin();

  haier.setCommand(HAIER_AC_CMD_OFF);
  EXPECT_EQ(HAIER_AC_CMD_OFF, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_ON);
  EXPECT_EQ(HAIER_AC_CMD_ON, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_MODE);
  EXPECT_EQ(HAIER_AC_CMD_MODE, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_FAN);
  EXPECT_EQ(HAIER_AC_CMD_FAN, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_TEMP_UP);
  EXPECT_EQ(HAIER_AC_CMD_TEMP_UP, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_TEMP_DOWN);
  EXPECT_EQ(HAIER_AC_CMD_TEMP_DOWN, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_SLEEP);
  EXPECT_EQ(HAIER_AC_CMD_SLEEP, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_TIMER_SET);
  EXPECT_EQ(HAIER_AC_CMD_TIMER_SET, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_TIMER_CANCEL);
  EXPECT_EQ(HAIER_AC_CMD_TIMER_CANCEL, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_HEALTH);
  EXPECT_EQ(HAIER_AC_CMD_HEALTH, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_SWING);
  EXPECT_EQ(HAIER_AC_CMD_SWING, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_ON);
  EXPECT_EQ(HAIER_AC_CMD_ON, haier.getCommand());

  // Test unexpected values.
  haier.setCommand(0b00001110);
  EXPECT_EQ(HAIER_AC_CMD_OFF, haier.getCommand());
  haier.setCommand(0b00001111);
  EXPECT_EQ(HAIER_AC_CMD_OFF, haier.getCommand());
  haier.setCommand(0b00000100);
  EXPECT_EQ(HAIER_AC_CMD_OFF, haier.getCommand());
}

TEST(TestHaierACClass, OperatingMode) {
  IRHaierAC haier(0);
  haier.begin();

  haier.setMode(HAIER_AC_AUTO);
  EXPECT_EQ(HAIER_AC_AUTO, haier.getMode());

  haier.setMode(HAIER_AC_COOL);
  EXPECT_EQ(HAIER_AC_COOL, haier.getMode());

  haier.setMode(HAIER_AC_HEAT);
  EXPECT_EQ(HAIER_AC_HEAT, haier.getMode());

  haier.setMode(HAIER_AC_FAN);
  EXPECT_EQ(HAIER_AC_FAN, haier.getMode());

  haier.setMode(HAIER_AC_DRY);
  EXPECT_EQ(HAIER_AC_DRY, haier.getMode());

  haier.setMode(HAIER_AC_AUTO - 1);
  EXPECT_EQ(HAIER_AC_AUTO, haier.getMode());

  haier.setMode(HAIER_AC_COOL);
  EXPECT_EQ(HAIER_AC_COOL, haier.getMode());

  haier.setMode(HAIER_AC_FAN + 1);
  EXPECT_EQ(HAIER_AC_AUTO, haier.getMode());

  haier.setMode(255);
  EXPECT_EQ(HAIER_AC_AUTO, haier.getMode());
}

TEST(TestHaierACClass, Temperature) {
  IRHaierAC haier(0);
  haier.begin();

  haier.setTemp(HAIER_AC_MIN_TEMP);
  EXPECT_EQ(HAIER_AC_MIN_TEMP, haier.getTemp());

  haier.setCommand(HAIER_AC_CMD_ON);
  haier.setTemp(HAIER_AC_MIN_TEMP + 1);
  EXPECT_EQ(HAIER_AC_MIN_TEMP + 1, haier.getTemp());
  EXPECT_EQ(HAIER_AC_CMD_TEMP_UP, haier.getCommand());

  haier.setTemp(HAIER_AC_MAX_TEMP);
  EXPECT_EQ(HAIER_AC_MAX_TEMP, haier.getTemp());
  EXPECT_EQ(HAIER_AC_CMD_TEMP_UP, haier.getCommand());

  haier.setTemp(HAIER_AC_MIN_TEMP - 1);
  EXPECT_EQ(HAIER_AC_MIN_TEMP, haier.getTemp());
  EXPECT_EQ(HAIER_AC_CMD_TEMP_DOWN, haier.getCommand());

  haier.setTemp(HAIER_AC_MAX_TEMP + 1);
  EXPECT_EQ(HAIER_AC_MAX_TEMP, haier.getTemp());
  EXPECT_EQ(HAIER_AC_CMD_TEMP_UP, haier.getCommand());

  haier.setTemp(23);
  EXPECT_EQ(23, haier.getTemp());
  EXPECT_EQ(HAIER_AC_CMD_TEMP_DOWN, haier.getCommand());
  haier.setCommand(HAIER_AC_CMD_ON);
  haier.setTemp(23);
  EXPECT_EQ(23, haier.getTemp());
  EXPECT_EQ(HAIER_AC_CMD_ON, haier.getCommand());

  haier.setTemp(0);
  EXPECT_EQ(HAIER_AC_MIN_TEMP, haier.getTemp());
  EXPECT_EQ(HAIER_AC_CMD_TEMP_DOWN, haier.getCommand());

  haier.setTemp(255);
  EXPECT_EQ(HAIER_AC_MAX_TEMP, haier.getTemp());
  EXPECT_EQ(HAIER_AC_CMD_TEMP_UP, haier.getCommand());
}

TEST(TestHaierACClass, FanSpeed) {
  IRHaierAC haier(0);
  haier.begin();
  haier.setFan(HAIER_AC_FAN_LOW);
  haier.setCommand(HAIER_AC_CMD_ON);

  haier.setFan(HAIER_AC_FAN_AUTO);
  EXPECT_EQ(HAIER_AC_FAN_AUTO, haier.getFan());
  EXPECT_EQ(HAIER_AC_CMD_FAN, haier.getCommand());

  haier.setFan(HAIER_AC_FAN_LOW);
  EXPECT_EQ(HAIER_AC_FAN_LOW, haier.getFan());
  haier.setFan(HAIER_AC_FAN_MED);
  EXPECT_EQ(HAIER_AC_FAN_MED, haier.getFan());
  haier.setFan(HAIER_AC_FAN_HIGH);
  EXPECT_EQ(HAIER_AC_FAN_HIGH, haier.getFan());

  haier.setCommand(HAIER_AC_CMD_ON);
  haier.setFan(HAIER_AC_FAN_HIGH);
  EXPECT_EQ(HAIER_AC_FAN_HIGH, haier.getFan());
  EXPECT_EQ(HAIER_AC_CMD_ON, haier.getCommand());
}

TEST(TestHaierACClass, Swing) {
  IRHaierAC haier(0);
  haier.begin();
  haier.setFan(HAIER_AC_FAN_LOW);
  haier.setCommand(HAIER_AC_CMD_ON);

  haier.setSwing(HAIER_AC_SWING_OFF);
  EXPECT_EQ(HAIER_AC_SWING_OFF, haier.getSwing());

  haier.setSwing(HAIER_AC_SWING_UP);
  EXPECT_EQ(HAIER_AC_SWING_UP, haier.getSwing());
  EXPECT_EQ(HAIER_AC_CMD_SWING, haier.getCommand());

  haier.setSwing(HAIER_AC_SWING_DOWN);
  EXPECT_EQ(HAIER_AC_SWING_DOWN, haier.getSwing());
  EXPECT_EQ(HAIER_AC_CMD_SWING, haier.getCommand());

  haier.setSwing(HAIER_AC_SWING_CHG);
  EXPECT_EQ(HAIER_AC_SWING_CHG, haier.getSwing());
  EXPECT_EQ(HAIER_AC_CMD_SWING, haier.getCommand());
}

TEST(TestHaierACClass, CurrentTime) {
  IRHaierAC haier(0);
  haier.begin();
  EXPECT_EQ(0, haier.getCurrTime());

  haier.setCurrTime(1);
  EXPECT_EQ(1, haier.getCurrTime());

  haier.setCurrTime(60);
  EXPECT_EQ(60, haier.getCurrTime());

  haier.setCurrTime(61);
  EXPECT_EQ(61, haier.getCurrTime());

  haier.setCurrTime(18 * 60 + 34);  // 18:34
  EXPECT_EQ(1114, haier.getCurrTime());

  haier.setCurrTime(23 * 60 + 59);  // 23:59
  EXPECT_EQ(HAIER_AC_MAX_TIME, haier.getCurrTime());  // 23:59

  haier.setCurrTime(23 * 60 + 59 + 1);  // 24:00
  EXPECT_EQ(HAIER_AC_MAX_TIME, haier.getCurrTime());  // 23:59

  haier.setCurrTime(UINT16_MAX);
  EXPECT_EQ(HAIER_AC_MAX_TIME, haier.getCurrTime());  // 23:59
}

TEST(TestHaierACClass, Timers) {
  IRHaierAC haier(0);
  haier.begin();

  haier.setCommand(HAIER_AC_CMD_ON);

  // Off by default.
  EXPECT_GT(0, haier.getOnTimer());
  EXPECT_GT(0, haier.getOffTimer());
  EXPECT_EQ(HAIER_AC_CMD_ON, haier.getCommand());

  // On Timer.
  haier.setOnTimer(6 * 60);  // 6am
  EXPECT_EQ(6 * 60, haier.getOnTimer());  // 6am
  EXPECT_GT(0, haier.getOffTimer());
  EXPECT_EQ(HAIER_AC_CMD_TIMER_SET, haier.getCommand());

  haier.setCommand(HAIER_AC_CMD_ON);
  EXPECT_EQ(6 * 60, haier.getOnTimer());  // 6am
  EXPECT_GT(0, haier.getOffTimer());
  EXPECT_EQ(HAIER_AC_CMD_ON, haier.getCommand());

  haier.cancelTimers();
  EXPECT_GT(0, haier.getOnTimer());
  EXPECT_GT(0, haier.getOffTimer());
  EXPECT_EQ(HAIER_AC_CMD_TIMER_CANCEL, haier.getCommand());

  // Off Timer.
  haier.setOffTimer(18 * 60 + 30);  // 6:30pm
  EXPECT_GT(0, haier.getOnTimer());
  EXPECT_EQ(18 * 60 + 30, haier.getOffTimer());  // 6:30pm
  EXPECT_EQ(HAIER_AC_CMD_TIMER_SET, haier.getCommand());

  haier.setCommand(HAIER_AC_CMD_ON);
  EXPECT_GT(0, haier.getOnTimer());
  EXPECT_EQ(18 * 60 + 30, haier.getOffTimer());  // 6:30pm
  EXPECT_EQ(HAIER_AC_CMD_ON, haier.getCommand());

  haier.cancelTimers();
  EXPECT_GT(0, haier.getOnTimer());
  EXPECT_GT(0, haier.getOffTimer());
  EXPECT_EQ(HAIER_AC_CMD_TIMER_CANCEL, haier.getCommand());

  // Both Timers.
  haier.setOnTimer(6 * 60);  // 6am
  EXPECT_EQ(HAIER_AC_CMD_TIMER_SET, haier.getCommand());
  haier.setOffTimer(18 * 60 + 30);  // 6:30pm
  EXPECT_EQ(HAIER_AC_CMD_TIMER_SET, haier.getCommand());
  EXPECT_EQ(6 * 60, haier.getOnTimer());  // 6am
  EXPECT_EQ(18 * 60 + 30, haier.getOffTimer());  // 6:30pm

  haier.cancelTimers();
  EXPECT_GT(0, haier.getOnTimer());
  EXPECT_GT(0, haier.getOffTimer());
  EXPECT_EQ(HAIER_AC_CMD_TIMER_CANCEL, haier.getCommand());
}

TEST(TestHaierACClass, TimeToString) {
  EXPECT_EQ("00:00", IRHaierAC::timeToString(0));
  EXPECT_EQ("00:01", IRHaierAC::timeToString(1));
  EXPECT_EQ("00:10", IRHaierAC::timeToString(10));
  EXPECT_EQ("00:59", IRHaierAC::timeToString(59));

  EXPECT_EQ("01:00", IRHaierAC::timeToString(60));
  EXPECT_EQ("01:01", IRHaierAC::timeToString(61));
  EXPECT_EQ("01:59", IRHaierAC::timeToString(60 + 59));
  EXPECT_EQ("18:59", IRHaierAC::timeToString(18 * 60 + 59));
  EXPECT_EQ("23:59", IRHaierAC::timeToString(23 * 60 + 59));
}

TEST(TestHaierACClass, MessageConstuction) {
  IRHaierAC haier(0);

  EXPECT_EQ("Command: 1 (On), Mode: 0 (AUTO), Temp: 25C, Fan: 0 (AUTO), "
            "Swing: 0 (Off), Sleep: Off, Health: Off, "
            "Current Time: 00:00, On Timer: Off, Off Timer: Off",
            haier.toString());
  haier.setMode(HAIER_AC_COOL);
  haier.setTemp(21);
  haier.setFan(HAIER_AC_FAN_HIGH);
  EXPECT_EQ("Command: 3 (Fan), Mode: 1 (COOL), Temp: 21C, Fan: 3 (MAX), "
            "Swing: 0 (Off), Sleep: Off, Health: Off, "
            "Current Time: 00:00, On Timer: Off, Off Timer: Off",
            haier.toString());
  haier.setSwing(HAIER_AC_SWING_CHG);
  haier.setHealth(true);
  haier.setSleep(true);
  haier.setCurrTime(615);  // 10:15am
  EXPECT_EQ("Command: 8 (Sleep), Mode: 3 (HEAT), Temp: 21C, Fan: 3 (MAX), "
            "Swing: 3 (Chg), Sleep: On, Health: On, "
            "Current Time: 10:15, On Timer: Off, Off Timer: Off",
            haier.toString());
  haier.setOnTimer(800);  // 1:20pm
  haier.setOffTimer(1125);  // 6:45pm
  haier.setCommand(HAIER_AC_CMD_ON);

  EXPECT_EQ("Command: 1 (On), Mode: 2 (DRY), Temp: 21C, Fan: 2, "
            "Swing: 3 (Chg), Sleep: On, Health: On, "
            "Current Time: 10:15, On Timer: 13:20, Off Timer: 18:45",
            haier.toString());

  // Now change a few already set things.
  haier.setMode(HAIER_AC_HEAT);
  EXPECT_EQ("Command: 2 (Mode), Mode: 3 (HEAT), Temp: 21C, Fan: 2, "
            "Swing: 3 (Chg), Sleep: On, Health: On, "
            "Current Time: 10:15, On Timer: 13:52, Off Timer: 18:45",
            haier.toString());

  haier.setTemp(25);
  EXPECT_EQ("Command: 6 (Temp Up), Mode: 3 (HEAT), Temp: 25C, Fan: 2, "
            "Swing: 3 (Chg), Sleep: On, Health: On, "
            "Current Time: 10:15, On Timer: 13:52, Off Timer: 18:45",
            haier.toString());

  uint8_t expectedState[HAIER_AC_STATE_LENGTH] = {
    0xA5, 0x96, 0xEA, 0xCF, 0x32, 0x2D, 0x0D, 0x74, 0xD4};
  EXPECT_STATE_EQ(expectedState, haier.getRaw(), HAIER_AC_BITS);

  // Check that the checksum is valid.
  EXPECT_TRUE(IRHaierAC::validChecksum(haier.getRaw()));

  // Now load up some random data.
  uint8_t randomState[HAIER_AC_STATE_LENGTH] = {
      0x52, 0x49, 0x50, 0x20, 0x54, 0x61, 0x6C, 0x69, 0x61};
  EXPECT_FALSE(IRHaierAC::validChecksum(randomState));
  haier.setRaw(randomState);
  EXPECT_EQ("Command: 9 (Timer Set), Mode: 3 (HEAT), Temp: 20C, Fan: 2, "
            "Swing: 1 (Up), Sleep: On, Health: Off, "
            "Current Time: 16:32, On Timer: Off, Off Timer: Off",
            haier.toString());
  // getRaw() should correct the checksum.
  EXPECT_TRUE(IRHaierAC::validChecksum(haier.getRaw()));
}

// Tests for decodeHaierAC().

// Decode normal "synthetic" messages.
TEST(TestDecodeHaierAC, NormalDecodeWithStrict) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  irsend.begin();

  uint8_t expectedState[HAIER_AC_STATE_LENGTH] = {
      0xA5, 0x01, 0x20, 0x01, 0x00, 0xC0, 0x20, 0x00, 0xA7};
  // With the specific decoder.
  irsend.reset();
  irsend.sendHaierAC(expectedState);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decodeHaierAC(&irsend.capture, HAIER_AC_BITS, true));
  EXPECT_EQ(HAIER_AC, irsend.capture.decode_type);
  EXPECT_EQ(HAIER_AC_BITS, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_STATE_EQ(expectedState, irsend.capture.state, irsend.capture.bits);

  // With the all the decoders.
  irsend.reset();
  irsend.sendHaierAC(expectedState);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(HAIER_AC, irsend.capture.decode_type);
  EXPECT_EQ(HAIER_AC_BITS, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_STATE_EQ(expectedState, irsend.capture.state, irsend.capture.bits);
}

// Decode a "real" example message.
TEST(TestDecodeHaierAC, RealExample1) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  irsend.begin();

  irsend.reset();
  // Data from Issue #404 captured by kuzin2006
  uint16_t rawData[149] = {3030, 3044, 3030, 4304, 576, 1694, 550, 582, 552,
      1704, 552, 714, 550, 582, 550, 1706, 552, 582, 550, 1836, 552, 582, 578,
      568, 550, 582, 550, 714, 550, 582, 550, 582, 552, 582, 550, 1836, 552,
      582, 552, 580, 580, 1692, 550, 712, 552, 582, 550, 582, 552, 580, 550,
      714, 552, 582, 550, 582, 552, 582, 578, 698, 552, 580, 552, 582, 552,
      582, 552, 1836, 552, 580, 552, 582, 552, 582, 550, 714, 578, 568, 550,
      582, 550, 582, 552, 714, 550, 1706, 550, 1706, 550, 582, 550, 714, 552,
      582, 580, 566, 552, 582, 550, 714, 552, 580, 552, 580, 552, 1706, 550,
      714, 550, 582, 552, 582, 578, 568, 552, 712, 552, 582, 550, 582, 550,
      582, 550, 712, 552, 582, 550, 582, 552, 582, 578, 722, 552, 1704, 550,
      582, 550, 1706, 550, 736, 550, 582, 550, 1706, 550, 1704, 552, 1704, 578};
  uint8_t expectedState[HAIER_AC_STATE_LENGTH] = {
      0xA5, 0x01, 0x20, 0x01, 0x00, 0xC0, 0x20, 0x00, 0xA7};

  irsend.sendRaw(rawData, 149, 38000);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  ASSERT_EQ(HAIER_AC, irsend.capture.decode_type);
  EXPECT_EQ(HAIER_AC_BITS, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_STATE_EQ(expectedState, irsend.capture.state, irsend.capture.bits);

  IRHaierAC haier(0);
  haier.setRaw(irsend.capture.state);
  EXPECT_EQ("Command: 1 (On), Mode: 0 (AUTO), Temp: 16C, Fan: 0 (AUTO), "
            "Swing: 0 (Off), Sleep: Off, Health: Off, "
            "Current Time: 00:01, On Timer: Off, Off Timer: Off",
            haier.toString());
}

// Decode a "real" example message.
TEST(TestDecodeHaierAC, RealExample2) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  irsend.begin();

  irsend.reset();
  // Data from Issue #404 captured by kuzin2006
  uint16_t rawData[149] = {3028, 3046, 3028, 4304, 576, 1694, 552, 582, 550,
      1704, 552, 714, 550, 582, 552, 1704, 550, 582, 550, 1836, 552, 582, 578,
      1690, 552, 1704, 552, 712, 550, 582, 550, 1706, 550, 1706, 552, 712, 550,
      582, 552, 582, 578, 1690, 552, 714, 552, 580, 552, 582, 552, 582, 550,
      712, 552, 582, 550, 582, 550, 582, 578, 698, 552, 582, 550, 584, 550, 582,
      552, 1836, 550, 582, 550, 582, 550, 582, 550, 712, 578, 568, 550, 582,
      550, 582, 550, 714, 552, 1706, 550, 1706, 552, 580, 550, 714, 550, 582,
      580, 568, 550, 582, 550, 714, 550, 582, 550, 582, 550, 1706, 552, 712,
      550, 582, 550, 582, 580, 568, 552, 712, 550, 584, 550, 582, 550, 584, 550,
      712, 550, 582, 550, 582, 550, 582, 578, 722, 550, 582, 552, 580, 552, 582,
      550, 738, 550, 1706, 550, 1704, 552, 582, 550, 582, 578};
  uint8_t expectedState[HAIER_AC_STATE_LENGTH] = {
      0xA5, 0x66, 0x20, 0x01, 0x00, 0xC0, 0x20, 0x00, 0x0C};

  irsend.sendRaw(rawData, 149, 38000);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  ASSERT_EQ(HAIER_AC, irsend.capture.decode_type);
  EXPECT_EQ(HAIER_AC_BITS, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_STATE_EQ(expectedState, irsend.capture.state, irsend.capture.bits);

  IRHaierAC haier(0);
  haier.setRaw(irsend.capture.state);
  EXPECT_EQ("Command: 6 (Temp Up), Mode: 0 (AUTO), Temp: 22C, Fan: 0 (AUTO), "
            "Swing: 0 (Off), Sleep: Off, Health: Off, "
            "Current Time: 00:01, On Timer: Off, Off Timer: Off",
            haier.toString());
}

// Decode a "real" example message.
TEST(TestDecodeHaierAC, RealExample3) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  irsend.begin();

  irsend.reset();
  // Data from Issue #404 captured by kuzin2006
  uint16_t rawData[149] = {3030, 3044, 3030, 4302, 578, 1692, 550, 582, 550,
      1706, 550, 714, 550, 582, 552, 1706, 550, 582, 550, 1836, 552, 1706, 578,
      1690, 552, 1704, 552, 714, 550, 1706, 552, 1706, 550, 582, 550, 714, 552,
      582, 550, 582, 578, 1690, 550, 714, 552, 582, 552, 582, 550, 582, 550,
      714, 550, 584, 550, 582, 550, 582, 578, 700, 552, 1706, 550, 582, 550,
      582, 552, 1836, 550, 582, 550, 582, 552, 1706, 550, 714, 578, 568, 552,
      582, 552, 582, 550, 714, 550, 1706, 550, 1706, 550, 582, 552, 712, 552,
      582, 580, 568, 550, 582, 550, 714, 550, 582, 550, 582, 550, 1706, 550,
      714, 550, 582, 550, 582, 578, 568, 552, 712, 552, 582, 550, 582, 550, 582,
      550, 712, 550, 584, 550, 582, 552, 582, 578, 722, 552, 1704, 550, 582,
      550, 1706, 550, 1862, 550, 1706, 550, 582, 550, 1704, 552, 582, 578};
  uint8_t expectedState[HAIER_AC_STATE_LENGTH] = {
      0xA5, 0xEC, 0x20, 0x09, 0x20, 0xC0, 0x20, 0x00, 0xBA};

  irsend.sendRaw(rawData, 149, 38000);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  ASSERT_EQ(HAIER_AC, irsend.capture.decode_type);
  EXPECT_EQ(HAIER_AC_BITS, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_STATE_EQ(expectedState, irsend.capture.state, irsend.capture.bits);

  IRHaierAC haier(0);
  haier.setRaw(irsend.capture.state);
  EXPECT_EQ("Command: 12 (Health), Mode: 0 (AUTO), Temp: 30C, Fan: 0 (AUTO), "
            "Swing: 0 (Off), Sleep: Off, Health: On, "
            "Current Time: 00:09, On Timer: Off, Off Timer: Off",
            haier.toString());
}
