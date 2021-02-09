// Copyright 2019 David Conran

#include "ir_Teco.h"
#include "IRrecv.h"
#include "IRrecv_test.h"
#include "IRsend.h"
#include "IRsend_test.h"
#include "gtest/gtest.h"

// General housekeeping
TEST(TestTeco, Housekeeping) {
  ASSERT_EQ("TECO", typeToString(TECO));
  ASSERT_FALSE(hasACState(TECO));  // Uses uint64_t, not uint8_t*.
}

// Tests for sendTeco()

// Test sending typical data only.
TEST(TestSendTeco, SendDataOnly) {
  IRsendTest irsend(0);
  irsend.begin();

  irsend.reset();
  irsend.sendTeco(0x250002BC9);
  EXPECT_EQ(
      "m9000s4440"
      "m620s1650m620s580m620s580m620s1650m620s580m620s580m620s1650m620s1650"
      "m620s1650m620s1650m620s580m620s1650m620s580m620s1650m620s580m620s580"
      "m620s580m620s580m620s580m620s580m620s580m620s580m620s580m620s580"
      "m620s580m620s580m620s580m620s580m620s1650m620s580m620s1650m620s580"
      "m620s580m620s1650m620s580"
      "m620s1000000",
      irsend.outputStr());
}

// Test sending typical data with repeats.
TEST(TestSendTeco, SendWithRepeats) {
  IRsendTest irsend(0);
  irsend.begin();

  irsend.reset();
  irsend.sendTeco(0x250002BC9, kTecoBits, 2);  // two repeats.
  EXPECT_EQ(
      "m9000s4440"
      "m620s1650m620s580m620s580m620s1650m620s580m620s580m620s1650m620s1650"
      "m620s1650m620s1650m620s580m620s1650m620s580m620s1650m620s580m620s580"
      "m620s580m620s580m620s580m620s580m620s580m620s580m620s580m620s580"
      "m620s580m620s580m620s580m620s580m620s1650m620s580m620s1650m620s580"
      "m620s580m620s1650m620s580"
      "m620s1000000"
      "m9000s4440"
      "m620s1650m620s580m620s580m620s1650m620s580m620s580m620s1650m620s1650"
      "m620s1650m620s1650m620s580m620s1650m620s580m620s1650m620s580m620s580"
      "m620s580m620s580m620s580m620s580m620s580m620s580m620s580m620s580"
      "m620s580m620s580m620s580m620s580m620s1650m620s580m620s1650m620s580"
      "m620s580m620s1650m620s580"
      "m620s1000000"
      "m9000s4440"
      "m620s1650m620s580m620s580m620s1650m620s580m620s580m620s1650m620s1650"
      "m620s1650m620s1650m620s580m620s1650m620s580m620s1650m620s580m620s580"
      "m620s580m620s580m620s580m620s580m620s580m620s580m620s580m620s580"
      "m620s580m620s580m620s580m620s580m620s1650m620s580m620s1650m620s580"
      "m620s580m620s1650m620s580"
      "m620s1000000",
      irsend.outputStr());
}


// Tests for IRTeco class.

TEST(TestTecoACClass, Power) {
  IRTecoAc ac(0);
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
}

TEST(TestTecoACClass, OperatingMode) {
  IRTecoAc ac(0);
  ac.begin();

  ac.setMode(kTecoAuto);
  EXPECT_EQ(kTecoAuto, ac.getMode());

  ac.setMode(kTecoCool);
  EXPECT_EQ(kTecoCool, ac.getMode());

  ac.setMode(kTecoHeat);
  EXPECT_EQ(kTecoHeat, ac.getMode());

  ac.setMode(kTecoFan);
  EXPECT_EQ(kTecoFan, ac.getMode());

  ac.setMode(kTecoDry);
  EXPECT_EQ(kTecoDry, ac.getMode());

  ac.setMode(kTecoAuto - 1);
  EXPECT_EQ(kTecoAuto, ac.getMode());

  ac.setMode(kTecoCool);
  EXPECT_EQ(kTecoCool, ac.getMode());

  ac.setMode(kTecoHeat + 1);
  EXPECT_EQ(kTecoAuto, ac.getMode());

  ac.setMode(255);
  EXPECT_EQ(kTecoAuto, ac.getMode());
}

TEST(TestTecoACClass, Temperature) {
  IRTecoAc ac(0);
  ac.begin();

  ac.setTemp(kTecoMinTemp);
  EXPECT_EQ(kTecoMinTemp, ac.getTemp());

  ac.setTemp(kTecoMinTemp + 1);
  EXPECT_EQ(kTecoMinTemp + 1, ac.getTemp());

  ac.setTemp(kTecoMaxTemp);
  EXPECT_EQ(kTecoMaxTemp, ac.getTemp());

  ac.setTemp(kTecoMinTemp - 1);
  EXPECT_EQ(kTecoMinTemp, ac.getTemp());

  ac.setTemp(kTecoMaxTemp + 1);
  EXPECT_EQ(kTecoMaxTemp, ac.getTemp());

  ac.setTemp(23);
  EXPECT_EQ(23, ac.getTemp());

  ac.setTemp(0);
  EXPECT_EQ(kTecoMinTemp, ac.getTemp());

  ac.setTemp(255);
  EXPECT_EQ(kTecoMaxTemp, ac.getTemp());
}

TEST(TestTecoACClass, FanSpeed) {
  IRTecoAc ac(0);
  ac.begin();
  ac.setFan(kTecoFanLow);

  ac.setFan(kTecoFanAuto);
  EXPECT_EQ(kTecoFanAuto, ac.getFan());

  ac.setFan(kTecoFanLow);
  EXPECT_EQ(kTecoFanLow, ac.getFan());
  ac.setFan(kTecoFanMed);
  EXPECT_EQ(kTecoFanMed, ac.getFan());
  ac.setFan(kTecoFanHigh);
  EXPECT_EQ(kTecoFanHigh, ac.getFan());

  ac.setFan(kTecoFanHigh);
  EXPECT_EQ(kTecoFanHigh, ac.getFan());
}

TEST(TestTecoACClass, Swing) {
  IRTecoAc ac(0);
  ac.begin();

  ac.setSwing(true);
  EXPECT_TRUE(ac.getSwing());

  ac.setSwing(false);
  EXPECT_EQ(false, ac.getSwing());

  ac.setSwing(true);
  EXPECT_TRUE(ac.getSwing());
}

TEST(TestTecoACClass, Sleep) {
  IRTecoAc ac(0);
  ac.begin();

  ac.setSleep(true);
  EXPECT_TRUE(ac.getSleep());

  ac.setSleep(false);
  EXPECT_EQ(false, ac.getSleep());

  ac.setSleep(true);
  EXPECT_TRUE(ac.getSleep());
}

TEST(TestTecoACClass, MessageConstuction) {
  IRTecoAc ac(0);

  EXPECT_EQ(
      "Power: Off, Mode: 0 (AUTO), Temp: 16C, Fan: 0 (Auto), Sleep: Off, "
      "Swing: Off",
      ac.toString());
  ac.setPower(true);
  ac.setMode(kTecoCool);
  ac.setTemp(21);
  ac.setFan(kTecoFanHigh);
  ac.setSwing(false);
  EXPECT_EQ(
      "Power: On, Mode: 1 (COOL), Temp: 21C, Fan: 3 (High), Sleep: Off, "
      "Swing: Off",
      ac.toString());
  ac.setSwing(true);
  EXPECT_EQ(
      "Power: On, Mode: 1 (COOL), Temp: 21C, Fan: 3 (High), Sleep: Off, "
      "Swing: On",
      ac.toString());
  ac.setSwing(false);
  ac.setFan(kTecoFanLow);
  ac.setSleep(true);
  ac.setMode(kTecoHeat);
  EXPECT_EQ(
      "Power: On, Mode: 4 (HEAT), Temp: 21C, Fan: 1 (Low), Sleep: On, "
      "Swing: Off",
      ac.toString());
  ac.setSleep(false);
  EXPECT_EQ(
      "Power: On, Mode: 4 (HEAT), Temp: 21C, Fan: 1 (Low), Sleep: Off, "
      "Swing: Off",
      ac.toString());
  ac.setTemp(25);
  EXPECT_EQ(
      "Power: On, Mode: 4 (HEAT), Temp: 25C, Fan: 1 (Low), Sleep: Off, "
      "Swing: Off",
      ac.toString());
}

TEST(TestTecoACClass, ReconstructKnownMessage) {
  IRTecoAc ac(0);

  const uint64_t expected = 0x250002BC9;
  ASSERT_FALSE(ac.getRaw() == expected);
  ac.setPower(true);
  ac.setMode(kTecoCool);
  ac.setTemp(27);
  ac.setFan(kTecoFanAuto);
  ac.setSleep(true);
  ac.setSwing(true);
  EXPECT_EQ(expected, ac.getRaw());
  EXPECT_EQ(
      "Power: On, Mode: 1 (COOL), Temp: 27C, Fan: 0 (Auto), Sleep: On, "
      "Swing: On",
      ac.toString());
}

// Tests for decodeTeco().

// Decode normal "synthetic" messages.
TEST(TestDecodeTeco, NormalDecodeWithStrict) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  irsend.begin();

  // With the specific decoder.
  uint64_t expectedState = kTecoReset;
  irsend.reset();
  irsend.sendTeco(expectedState);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decodeTeco(&irsend.capture, kTecoBits, true));
  EXPECT_EQ(TECO, irsend.capture.decode_type);
  EXPECT_EQ(kTecoBits, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_EQ(expectedState, irsend.capture.value);
  EXPECT_EQ(0, irsend.capture.address);
  EXPECT_EQ(0, irsend.capture.command);

  // With the all the decoders.
  irsend.reset();
  irsend.sendTeco(expectedState);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(TECO, irsend.capture.decode_type);
  EXPECT_EQ(kTecoBits, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_EQ(expectedState, irsend.capture.value);
  EXPECT_EQ(0, irsend.capture.address);
  EXPECT_EQ(0, irsend.capture.command);

  IRTecoAc ac(0);
  ac.begin();
  ac.setRaw(irsend.capture.value);
  EXPECT_EQ(
      "Power: Off, Mode: 0 (AUTO), Temp: 16C, Fan: 0 (Auto), Sleep: Off, "
      "Swing: Off",
      ac.toString());
}

// Decode a real message from Raw Data.
TEST(TestDecodeTeco, RealNormalExample) {
  IRsendTest irsend(0);
  IRrecv irrecv(0);
  IRTecoAc ac(0);
  irsend.begin();

  uint16_t rawData1[73] = {
      9076, 4442,  670, 1620,  670, 516,  670, 516,  666, 1626,  670, 516,
      664, 520,  666, 1626,  666, 1626,  664, 1626,  666, 1626,  666, 520,
      666, 1626,  666, 520,  666, 1626,  666, 520,  666, 516,  670, 514,
      670, 516,  666, 520,  670, 516,  666, 520,  666, 516,  672, 514,  670,
      516,  666, 520,  666, 516,  672, 514,  670, 516,  666, 1624,  666, 520,
      666, 1626,  666, 520,  666, 516,  672, 1620,  670, 516,  670};
  uint64_t expected1 = 0b01001010000000000000010101111001001;  // 0x250002BC9
  irsend.reset();
  irsend.sendRaw(rawData1, 73, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(TECO, irsend.capture.decode_type);
  EXPECT_EQ(kTecoBits, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_EQ(expected1, irsend.capture.value);
  EXPECT_EQ(0, irsend.capture.address);
  EXPECT_EQ(0, irsend.capture.command);
  ac.begin();
  ac.setRaw(irsend.capture.value);
  EXPECT_EQ(
      "Power: On, Mode: 1 (COOL), Temp: 27C, Fan: 0 (Auto), Sleep: On, "
      "Swing: On",
      ac.toString());

  uint16_t rawData2[73] = {
    9048, 4472, 636, 548, 636, 1654, 638, 546, 642, 1650, 642, 546, 638,
    1654, 638, 1654, 638, 546, 638, 1654, 636, 546, 642, 1650, 640, 548,
    636, 548, 638, 546, 636, 546, 642, 542, 642, 546, 638, 546, 638, 546,
    636, 548, 642, 542, 642, 546, 636, 548, 636, 546, 642, 542, 642, 546,
    638, 546, 638, 546, 636, 1654, 642, 542, 642, 1650, 642, 546, 638, 546,
    638, 1654, 638, 546, 642};  // TECO 25000056A
  uint64_t expected2 = 0b01001010000000000000000010101101010;  // 0x25000056A
  irsend.reset();
  irsend.sendRaw(rawData2, 73, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(TECO, irsend.capture.decode_type);
  EXPECT_EQ(kTecoBits, irsend.capture.bits);
  EXPECT_FALSE(irsend.capture.repeat);
  EXPECT_EQ(expected2, irsend.capture.value);
  EXPECT_EQ(0, irsend.capture.address);
  EXPECT_EQ(0, irsend.capture.command);
  ac.begin();
  ac.setRaw(irsend.capture.value);
  EXPECT_EQ(
      "Power: On, Mode: 2 (DRY), Temp: 21C, Fan: 2 (Med), Sleep: Off, "
      "Swing: On",
      ac.toString());
}
