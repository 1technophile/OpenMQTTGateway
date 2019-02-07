// Copyright 2019 David Conran

#include "ir_Vestel.h"
#include "IRrecv.h"
#include "IRrecv_test.h"
#include "IRsend.h"
#include "IRsend_test.h"
#include "gtest/gtest.h"

// Tests for sendVestelAC()

// Test sending typical data only.
TEST(TestSendVestelAC, SendDataOnly) {
  IRsendTest irsend(0);
  irsend.begin();

  irsend.reset();
  irsend.sendVestelAC(0x0F00D9001FEF201ULL);
  EXPECT_EQ(
      "m3110s9066"
      "m520s1535m520s480m520s480m520s480m520s480m520s480m520s480m520s480"
      "m520s480m520s1535m520s480m520s480m520s1535m520s1535m520s1535m520s1535"
      "m520s480m520s1535m520s1535m520s1535m520s1535m520s1535m520s1535m520s1535"
      "m520s1535m520s480m520s480m520s480m520s480m520s480m520s480m520s480"
      "m520s480m520s480m520s480m520s480m520s1535m520s480m520s480m520s1535"
      "m520s1535m520s480m520s1535m520s1535m520s480m520s480m520s480m520s480"
      "m520s480m520s480m520s480m520s480m520s1535m520s1535m520s1535m520s1535"
      "m520s100000",
      irsend.outputStr());
}

// Test sending typical data with repeats.
TEST(TestSendVestelAC, SendWithRepeats) {
  IRsendTest irsend(0);
  irsend.begin();

  irsend.reset();
  irsend.sendVestelAC(0x0F00D9001FEF201ULL, kVestelACBits, 2);  // two repeats.
  EXPECT_EQ(
      "m3110s9066"
      "m520s1535m520s480m520s480m520s480m520s480m520s480m520s480m520s480"
      "m520s480m520s1535m520s480m520s480m520s1535m520s1535m520s1535m520s1535"
      "m520s480m520s1535m520s1535m520s1535m520s1535m520s1535m520s1535m520s1535"
      "m520s1535m520s480m520s480m520s480m520s480m520s480m520s480m520s480"
      "m520s480m520s480m520s480m520s480m520s1535m520s480m520s480m520s1535"
      "m520s1535m520s480m520s1535m520s1535m520s480m520s480m520s480m520s480"
      "m520s480m520s480m520s480m520s480m520s1535m520s1535m520s1535m520s1535"
      "m520s100000"
      "m3110s9066"
      "m520s1535m520s480m520s480m520s480m520s480m520s480m520s480m520s480"
      "m520s480m520s1535m520s480m520s480m520s1535m520s1535m520s1535m520s1535"
      "m520s480m520s1535m520s1535m520s1535m520s1535m520s1535m520s1535m520s1535"
      "m520s1535m520s480m520s480m520s480m520s480m520s480m520s480m520s480"
      "m520s480m520s480m520s480m520s480m520s1535m520s480m520s480m520s1535"
      "m520s1535m520s480m520s1535m520s1535m520s480m520s480m520s480m520s480"
      "m520s480m520s480m520s480m520s480m520s1535m520s1535m520s1535m520s1535"
      "m520s100000"
      "m3110s9066"
      "m520s1535m520s480m520s480m520s480m520s480m520s480m520s480m520s480"
      "m520s480m520s1535m520s480m520s480m520s1535m520s1535m520s1535m520s1535"
      "m520s480m520s1535m520s1535m520s1535m520s1535m520s1535m520s1535m520s1535"
      "m520s1535m520s480m520s480m520s480m520s480m520s480m520s480m520s480"
      "m520s480m520s480m520s480m520s480m520s1535m520s480m520s480m520s1535"
      "m520s1535m520s480m520s1535m520s1535m520s480m520s480m520s480m520s480"
      "m520s480m520s480m520s480m520s480m520s1535m520s1535m520s1535m520s1535"
      "m520s100000",
      irsend.outputStr());
}

// Tests for IRVestelAC class.

TEST(TestVestelACClass, Power) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setPower(true);
  EXPECT_TRUE(ac.getPower());

  ac.setPower(false);
  EXPECT_EQ(false, ac.getPower());

  ac.setPower(true);
  EXPECT_TRUE(ac.getPower());

  ac.off();
  EXPECT_EQ(false, ac.getPower());

  ac.on();
  EXPECT_TRUE(ac.getPower());
  EXPECT_FALSE(ac.isTimeCommand());
}

TEST(TestVestelACClass, OperatingMode) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setMode(kVestelACAuto);
  EXPECT_EQ(kVestelACAuto, ac.getMode());

  ac.setMode(kVestelACCool);
  EXPECT_EQ(kVestelACCool, ac.getMode());

  ac.setMode(kVestelACHeat);
  EXPECT_EQ(kVestelACHeat, ac.getMode());

  ac.setMode(kVestelACFan);
  EXPECT_EQ(kVestelACFan, ac.getMode());

  ac.setMode(kVestelACDry);
  EXPECT_EQ(kVestelACDry, ac.getMode());

  ac.setMode(kVestelACAuto - 1);
  EXPECT_EQ(kVestelACAuto, ac.getMode());

  ac.setMode(kVestelACCool);
  EXPECT_EQ(kVestelACCool, ac.getMode());

  ac.setMode(kVestelACHeat + 1);
  EXPECT_EQ(kVestelACAuto, ac.getMode());

  ac.setMode(255);
  EXPECT_EQ(kVestelACAuto, ac.getMode());
  EXPECT_FALSE(ac.isTimeCommand());
}

TEST(TestVestelACClass, Temperature) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setTemp(kVestelACMinTempC);
  EXPECT_EQ(kVestelACMinTempC, ac.getTemp());

  ac.setTemp(kVestelACMinTempC + 1);
  EXPECT_EQ(kVestelACMinTempC + 1, ac.getTemp());

  ac.setTemp(kVestelACMaxTemp);
  EXPECT_EQ(kVestelACMaxTemp, ac.getTemp());

  ac.setTemp(kVestelACMinTempC - 1);
  EXPECT_EQ(kVestelACMinTempC, ac.getTemp());

  ac.setTemp(kVestelACMaxTemp + 1);
  EXPECT_EQ(kVestelACMaxTemp, ac.getTemp());

  ac.setTemp(23);
  EXPECT_EQ(23, ac.getTemp());

  ac.setTemp(0);
  EXPECT_EQ(kVestelACMinTempC, ac.getTemp());

  ac.setTemp(255);
  EXPECT_EQ(kVestelACMaxTemp, ac.getTemp());
  EXPECT_FALSE(ac.isTimeCommand());
}

TEST(TestVestelACClass, FanSpeed) {
  IRVestelAC ac(0);
  ac.begin();
  ac.setFan(kVestelACFanLow);

  ac.setFan(kVestelACFanAuto);
  EXPECT_EQ(kVestelACFanAuto, ac.getFan());

  ac.setFan(kVestelACFanLow);
  EXPECT_EQ(kVestelACFanLow, ac.getFan());
  ac.setFan(kVestelACFanMed);
  EXPECT_EQ(kVestelACFanMed, ac.getFan());
  ac.setFan(kVestelACFanHigh);
  EXPECT_EQ(kVestelACFanHigh, ac.getFan());

  ac.setFan(kVestelACFanHigh);
  EXPECT_EQ(kVestelACFanHigh, ac.getFan());
  EXPECT_FALSE(ac.isTimeCommand());
}

TEST(TestVestelACClass, Swing) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setSwing(true);
  EXPECT_TRUE(ac.getSwing());

  ac.setSwing(false);
  EXPECT_EQ(false, ac.getSwing());

  ac.setSwing(true);
  EXPECT_TRUE(ac.getSwing());
  EXPECT_FALSE(ac.isTimeCommand());
}

TEST(TestVestelACClass, Ion) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setIon(true);
  EXPECT_TRUE(ac.getIon());

  ac.setIon(false);
  EXPECT_EQ(false, ac.getIon());

  ac.setIon(true);
  EXPECT_TRUE(ac.getIon());
  EXPECT_FALSE(ac.isTimeCommand());
}

TEST(TestVestelACClass, Turbo) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setTurbo(true);
  EXPECT_TRUE(ac.getTurbo());

  ac.setTurbo(false);
  EXPECT_EQ(false, ac.getTurbo());

  ac.setTurbo(true);
  EXPECT_TRUE(ac.getTurbo());
  EXPECT_FALSE(ac.isTimeCommand());
}

TEST(TestVestelACClass, Sleep) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setSleep(true);
  EXPECT_TRUE(ac.getSleep());

  ac.setSleep(false);
  EXPECT_EQ(false, ac.getSleep());

  ac.setSleep(true);
  EXPECT_TRUE(ac.getSleep());
  EXPECT_FALSE(ac.isTimeCommand());
}

TEST(TestVestelACClass, Time) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setTime(0);
  EXPECT_EQ(0, ac.getTime());
  EXPECT_TRUE(ac.isTimeCommand());

  ac.setTime(1);
  EXPECT_EQ(1, ac.getTime());

  ac.setTime(1234);
  EXPECT_EQ(1234, ac.getTime());

  ac.setTime(23 * 60 + 59);
  EXPECT_EQ(23 * 60 + 59, ac.getTime());
}

TEST(TestVestelACClass, OnTimer) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setOnTimer(0);
  EXPECT_EQ(0, ac.getOnTimer());
  EXPECT_TRUE(ac.isTimeCommand());

  ac.setOnTimer(1);
  EXPECT_EQ(0, ac.getOnTimer());

  ac.setOnTimer(10);
  EXPECT_EQ(10, ac.getOnTimer());

  ac.setOnTimer(12 * 60 + 15);  // we will round down to 10 min increments.
  EXPECT_EQ(12 * 60 + 10, ac.getOnTimer());

  ac.setOnTimer(23 * 60 + 50);
  EXPECT_EQ(23 * 60 + 50, ac.getOnTimer());
}

TEST(TestVestelACClass, OffTimer) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setOffTimer(0);
  EXPECT_EQ(0, ac.getOffTimer());
  EXPECT_TRUE(ac.isTimeCommand());

  ac.setOffTimer(1);
  EXPECT_EQ(0, ac.getOffTimer());

  ac.setOffTimer(10);
  EXPECT_EQ(10, ac.getOffTimer());

  ac.setOffTimer(12 * 60 + 15);  // we will round down to 10 min increments.
  EXPECT_EQ(12 * 60 + 10, ac.getOffTimer());

  ac.setOffTimer(23 * 60 + 50);
  EXPECT_EQ(23 * 60 + 50, ac.getOffTimer());
}

TEST(TestVestelACClass, Timer) {
  IRVestelAC ac(0);
  ac.begin();

  ac.setTimer(0);
  EXPECT_EQ(0, ac.getTimer());
  EXPECT_EQ(0, ac.getOnTimer());
  EXPECT_TRUE(ac.isTimeCommand());

  ac.setTimer(10);
  EXPECT_EQ(10, ac.getTimer());
  EXPECT_EQ(0, ac.getOnTimer());

  ac.setTimer(12 * 60 + 15);  // we will round down to 10 min increments.
  EXPECT_EQ(12 * 60 + 10, ac.getTimer());
  EXPECT_EQ(0, ac.getOnTimer());

  ac.setTimer(23 * 60 + 50);
  EXPECT_EQ(23 * 60 + 50, ac.getTimer());
  EXPECT_EQ(0, ac.getOnTimer());
}

TEST(TestVestelACClass, MessageConstuction) {
  IRVestelAC ac(0);

  EXPECT_EQ(
      "Power: On, Mode: 0 (AUTO), Temp: 25C, Fan: 13 (AUTO HOT), Sleep: Off, "
      "Turbo: Off, Ion: Off, Swing: Off",
      ac.toString());
  ac.setMode(kVestelACCool);
  ac.setTemp(21);
  ac.setFan(kVestelACFanHigh);
  EXPECT_FALSE(ac.isTimeCommand());
  EXPECT_EQ(
      "Power: On, Mode: 1 (COOL), Temp: 21C, Fan: 11 (HIGH), Sleep: Off, "
      "Turbo: Off, Ion: Off, Swing: Off",
      ac.toString());
  ac.setSwing(true);
  ac.setIon(true);
  ac.setTurbo(true);
  EXPECT_FALSE(ac.isTimeCommand());
  EXPECT_EQ(
      "Power: On, Mode: 1 (COOL), Temp: 21C, Fan: 11 (HIGH), Sleep: Off, "
      "Turbo: On, Ion: On, Swing: On",
      ac.toString());

  // Now change a few already set things.
  ac.setSleep(true);
  ac.setMode(kVestelACHeat);
  EXPECT_EQ(
      "Power: On, Mode: 4 (HEAT), Temp: 21C, Fan: 11 (HIGH), Sleep: On, "
      "Turbo: Off, Ion: On, Swing: On",
      ac.toString());
  EXPECT_FALSE(ac.isTimeCommand());

  ac.setTemp(25);
  ac.setPower(false);
  EXPECT_EQ(
      "Power: Off, Mode: 4 (HEAT), Temp: 25C, Fan: 11 (HIGH), Sleep: On, "
      "Turbo: Off, Ion: On, Swing: On",
      ac.toString());
  EXPECT_FALSE(ac.isTimeCommand());

  // Check that the checksum is valid.
  EXPECT_TRUE(IRVestelAC::validChecksum(ac.getRaw()));
  ac.setTime(23 * 60 + 59);
  EXPECT_TRUE(ac.isTimeCommand());
  EXPECT_EQ(
      "Time: 23:59, Timer: Off, On Timer: Off, Off Timer: Off",
      ac.toString());
  ac.setTimer(8 * 60 + 0);
  EXPECT_TRUE(ac.isTimeCommand());
  EXPECT_EQ(
      "Time: 23:59, Timer: 8:00, On Timer: Off, Off Timer: Off",
      ac.toString());
  ac.setOnTimer(7 * 60 + 40);
  EXPECT_EQ(
      "Time: 23:59, Timer: Off, On Timer: 7:40, Off Timer: Off",
      ac.toString());
  ac.setOffTimer(17 * 60 + 10);
  EXPECT_EQ(
      "Time: 23:59, Timer: Off, On Timer: 7:40, Off Timer: 17:10",
      ac.toString());
  ac.setTimer(8 * 60 + 0);
  EXPECT_EQ(
      "Time: 23:59, Timer: 8:00, On Timer: Off, Off Timer: Off",
      ac.toString());
  ac.setTimer(0);
  EXPECT_EQ(
      "Time: 23:59, Timer: Off, On Timer: Off, Off Timer: Off",
      ac.toString());
  ac.on();
  EXPECT_FALSE(ac.isTimeCommand());
  EXPECT_EQ(
      "Power: On, Mode: 4 (HEAT), Temp: 25C, Fan: 11 (HIGH), Sleep: On, "
      "Turbo: Off, Ion: On, Swing: On",
      ac.toString());
}

// Tests for decodeVestelAC().

// Decode normal "synthetic" messages.
TEST(TestDecodeVestelAC, NormalDecodeWithStrict) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  irsend.begin();

  // With the specific decoder.
  uint64_t expectedState = 0x0F00D9001FEF201ULL;
  irsend.reset();
  irsend.sendVestelAC(expectedState);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decodeVestelAC(&irsend.capture, kVestelACBits, true));
  EXPECT_EQ(VESTEL_AC, irsend.capture.decode_type);
  EXPECT_EQ(kVestelACBits, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_EQ(expectedState, irsend.capture.value);
  EXPECT_EQ(0, irsend.capture.address);
  EXPECT_EQ(0, irsend.capture.command);

  // With the all the decoders.
  irsend.reset();
  irsend.sendVestelAC(expectedState);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(VESTEL_AC, irsend.capture.decode_type);
  EXPECT_EQ(kVestelACBits, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_EQ(expectedState, irsend.capture.value);
  EXPECT_EQ(0, irsend.capture.address);
  EXPECT_EQ(0, irsend.capture.command);

  IRVestelAC ac(0);
  ac.begin();
  ac.setRaw(irsend.capture.value);
  EXPECT_EQ(
      "Power: On, Mode: 0 (AUTO), Temp: 25C, Fan: 13 (AUTO HOT), Sleep: Off, "
      "Turbo: Off, Ion: Off, Swing: Off",
      ac.toString());
}

// Decode a real message from Raw Data.
TEST(TestDecodeVestelAC, RealNormalExample) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  IRVestelAC ac(0);
  irsend.begin();

  uint16_t rawData[115] = {
      3098, 9080, 548, 1538, 526, 492,  526, 468,  524, 468,  526, 468,
      550,  466,  526, 466,  526, 504,  540, 466,  526, 1538, 526, 466,
      526,  466,  552, 1540, 522, 466,  526, 492,  526, 544,  526, 1536,
      526,  1536, 552, 1536, 526, 1536, 552, 1536, 552, 1536, 526, 1536,
      526,  1574, 542, 1536, 526, 492,  526, 466,  526, 494,  524, 468,
      524,  468,  526, 492,  526, 502,  540, 468,  524, 494,  524, 468,
      526,  468,  524, 468,  526, 492,  526, 468,  524, 520,  524, 1538,
      524,  468,  524, 468,  524, 468,  524, 468,  524, 468,  524, 1538,
      524,  506,  538, 468,  524, 468,  524, 1538, 524, 468,  550, 1538,
      550,  1538, 524, 1538, 534, 1528, 544};  // VESTEL_AC
  irsend.reset();
  irsend.sendRaw(rawData, 115, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(VESTEL_AC, irsend.capture.decode_type);
  EXPECT_EQ(kVestelACBits, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_EQ(0xF4410001FF1201ULL, irsend.capture.value);
  EXPECT_EQ(0, irsend.capture.address);
  EXPECT_EQ(0, irsend.capture.command);
  ac.begin();
  ac.setRaw(irsend.capture.value);
  EXPECT_EQ(
      "Power: On, Mode: 4 (HEAT), Temp: 16C, Fan: 1 (AUTO), Sleep: Off, "
      "Turbo: Off, Ion: On, Swing: Off",
      ac.toString());
}

TEST(TestDecodeVestelAC, RealTimerExample) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  IRVestelAC ac(0);
  irsend.begin();

  uint16_t rawData[115] = {
      3022, 9080, 546, 1536, 526, 466,  526, 492,  526, 468,  526, 492,
      524,  468,  524, 494,  524, 504,  540, 492,  524, 1538, 526, 468,
      524,  492,  526, 466,  552, 1536, 526, 1536, 526, 1570, 542, 492,
      524,  1538, 550, 1538, 524, 1536, 526, 494,  524, 466,  526, 468,
      524,  1574, 540, 1536, 550, 1536, 526, 468,  550, 1536, 526, 492,
      526,  468,  524, 492,  526, 518,  526, 1536, 552, 1536, 550, 1536,
      526,  494,  550, 1538, 526, 492,  524, 1538, 526, 504,  540, 466,
      526,  1536, 526, 1536, 526, 468,  550, 1538, 524, 468,  524, 1538,
      550,  1574, 540, 468,  550, 1538, 526, 492,  524, 468,  526, 466,
      526,  468,  524, 494,  524, 468,  546};  // VESTEL_AC 2D6570B8EE201
  irsend.reset();
  irsend.sendRaw(rawData, 115, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(VESTEL_AC, irsend.capture.decode_type);
  EXPECT_EQ(kVestelACBits, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_EQ(0x2D6570B8EE201ULL, irsend.capture.value);
  EXPECT_EQ(0, irsend.capture.address);
  EXPECT_EQ(0, irsend.capture.command);
  ac.begin();
  ac.setRaw(irsend.capture.value);
  EXPECT_EQ(
      "Time: 5:45, Timer: Off, On Timer: 14:00, Off Timer: 23:00",
      ac.toString());
}

// General housekeeping
TEST(TestDecodeVestelAC, Housekeeping) {
  ASSERT_EQ("VESTEL_AC", typeToString(VESTEL_AC));
  ASSERT_FALSE(hasACState(VESTEL_AC));  // Uses uint64_t, not uint8_t*.
}
