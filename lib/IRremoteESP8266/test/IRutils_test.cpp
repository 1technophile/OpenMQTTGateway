// Copyright 2017 David Conran

#include "IRutils.h"
#include "gtest/gtest.h"

// Tests reverseBits().

// Tests reverseBits for typical use.
TEST(ReverseBitsTest, TypicalUse) {
  EXPECT_EQ(0xF, reverseBits(0xF0, 8));
  EXPECT_EQ(0xFFFF, reverseBits(0xFFFF0000, 32));
  EXPECT_EQ(0x555500005555FFFF, reverseBits(0xFFFFAAAA0000AAAA, 64));
  EXPECT_EQ(0, reverseBits(0, 64));
  EXPECT_EQ(0xFFFFFFFFFFFFFFFF, reverseBits(0xFFFFFFFFFFFFFFFF, 64));
}

// Tests reverseBits for bit size values <= 1
TEST(ReverseBitsTest, LessThanTwoBitsReversed) {
  EXPECT_EQ(0x12345678, reverseBits(0x12345678, 1));
  EXPECT_EQ(1234, reverseBits(1234, 0));
}

// Tests reverseBits for bit size larger than a uint64_t.
TEST(ReverseBitsTest, LargerThan64BitsReversed) {
  EXPECT_EQ(0, reverseBits(0, 65));
  EXPECT_EQ(0xFFFFFFFFFFFFFFFF, reverseBits(0xFFFFFFFFFFFFFFFF, 100));
  EXPECT_EQ(0x555500005555FFFF, reverseBits(0xFFFFAAAA0000AAAA, 3000));
}

// Tests reverseBits for bit sizes less than all the data stored.
TEST(ReverseBitsTest, LessBitsReversedThanInputHasSet) {
  EXPECT_EQ(0xF8, reverseBits(0xF1, 4));
  EXPECT_EQ(0xF5, reverseBits(0xFA, 4));
  EXPECT_EQ(0x12345678FFFF0000, reverseBits(0x123456780000FFFF, 32));
}
