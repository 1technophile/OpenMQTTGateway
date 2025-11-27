#include <config_RF.h>
#include <gtest/gtest.h>
#include <rf/RFConfiguration.h>

#include "../mocks/mock_IStorage.h"
#include "../mocks/mock_RFBaseGateway.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

class RFConfigurationTest : public ::testing::Test {
protected:
  void SetUp() override {
    config = new RFConfiguration(mockReceiver, mockStorage);
  }
  void TearDown() override {
    delete config;
  }
  MockStorage mockStorage;
  MockRFGateway mockReceiver;
  RFConfiguration* config;
};

TEST_F(RFConfigurationTest, ShouldInitializeWithDefaults) {
  // Assert
  ASSERT_NEAR(config->getFrequency(), RF_FREQUENCY, 0.01);
  ASSERT_EQ(config->getActiveReceiver(), ACTIVE_RECEIVER);
  ASSERT_EQ(config->getRssiThreshold(), 0);
  ASSERT_EQ(config->getNewOokThreshold(), 0);
}

TEST_F(RFConfigurationTest, ShouldSetAndGetFrequency) {
  config->setFrequency(433.92f);
  ASSERT_EQ(config->getFrequency(), 433.92f);
}

TEST_F(RFConfigurationTest, ShouldSetAndGetRssiThreshold) {
  config->setRssiThreshold(42);
  ASSERT_EQ(config->getRssiThreshold(), 42);
}

TEST_F(RFConfigurationTest, ShouldSetAndGetNewOokThreshold) {
  config->setNewOokThreshold(77);
  ASSERT_EQ(config->getNewOokThreshold(), 77);
}

TEST_F(RFConfigurationTest, ShouldSetAndGetActiveReceiver) {
  config->setActiveReceiver(2);
  ASSERT_EQ(config->getActiveReceiver(), 2);
}

TEST_F(RFConfigurationTest, ShouldReInitRestoreDefaults) {
  config->setFrequency(400.0f);
  config->setActiveReceiver(3);
  config->setRssiThreshold(99);
  config->setNewOokThreshold(88);

  config->reInit();

  ASSERT_NEAR(config->getFrequency(), RF_FREQUENCY, 0.01);
  ASSERT_EQ(config->getActiveReceiver(), ACTIVE_RECEIVER);
  ASSERT_EQ(config->getRssiThreshold(), 0);
  ASSERT_EQ(config->getNewOokThreshold(), 0);
}

TEST_F(RFConfigurationTest, ShouldValidateFrequencyRanges) {
  // Valid ranges
  ASSERT_TRUE(config->validFrequency(300.0f));
  ASSERT_TRUE(config->validFrequency(348.0f));
  ASSERT_TRUE(config->validFrequency(387.0f));
  ASSERT_TRUE(config->validFrequency(464.0f));
  ASSERT_TRUE(config->validFrequency(779.0f));
  ASSERT_TRUE(config->validFrequency(928.0f));

  // Invalid ranges
  ASSERT_FALSE(config->validFrequency(299.9f));
  ASSERT_FALSE(config->validFrequency(348.1f));
  ASSERT_FALSE(config->validFrequency(386.9f));
  ASSERT_FALSE(config->validFrequency(464.1f));
  ASSERT_FALSE(config->validFrequency(778.9f));
  ASSERT_FALSE(config->validFrequency(928.1f));
}

TEST_F(RFConfigurationTest, ShouldNotUpdateIfKeyMissing) {
  StaticJsonDocument<128> doc;
  JsonObject obj = doc.to<JsonObject>();
  float freq = 433.92f;
  config->setFrequency(freq);
  ASSERT_EQ(freq, 433.92f);
}

TEST_F(RFConfigurationTest, ShouldSerializeToJson) {
  config->setFrequency(433.92f);
  config->setRssiThreshold(12);
  config->setNewOokThreshold(34);
  config->setActiveReceiver(1);

  StaticJsonDocument<256> doc;
  JsonObject obj = doc.to<JsonObject>();
  config->to(obj);

  ASSERT_EQ(obj["frequency"].as<float>(), 433.92f);
  ASSERT_EQ(obj["rssithreshold"].as<int>(), 12);
  ASSERT_EQ(obj["ookthreshold"].as<int>(), 34);
  ASSERT_EQ(obj["active"].as<int>(), 1);
}

TEST_F(RFConfigurationTest, ShouldLoadFromJson) {
  StaticJsonDocument<256> doc;
  JsonObject obj = doc.to<JsonObject>();
  obj["frequency"] = 315.0f;
  obj["active"] = 2;

  config->from(obj);

  ASSERT_EQ(config->getFrequency(), 315.0f);
  ASSERT_EQ(config->getActiveReceiver(), 2);
}

TEST_F(RFConfigurationTest, ShouldNotUpdateFrequencyIfInvalid) {
  StaticJsonDocument<256> doc;
  JsonObject obj = doc.to<JsonObject>();
  obj["frequency"] = 100.0f; // Invalid

  float oldFreq = config->getFrequency();
  config->from(obj);

  ASSERT_EQ(config->getFrequency(), oldFreq);
}

TEST_F(RFConfigurationTest, ShouldCallenableOnLoadFromStorage) {
  std::string storedConfig = R"({"frequency":315.0,"active":2})";

  EXPECT_CALL(mockStorage, begin(true)).WillOnce(Return(true));
  EXPECT_CALL(mockStorage, isKey(::testing::StrEq("RFConfig"))).WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(::testing::StrEq("RFConfig"), ::testing::StrEq("{}"))).WillOnce(Return(storedConfig.c_str()));
  EXPECT_CALL(mockStorage, end()).Times(1);
  EXPECT_CALL(mockReceiver, enable()).Times(1);

  config->loadFromStorage();

  ASSERT_EQ(config->getFrequency(), 315.0f);
  ASSERT_EQ(config->getActiveReceiver(), 2);
}
