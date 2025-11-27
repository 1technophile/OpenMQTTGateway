
#include <config_JSONMessages.h>
#include <core/MessageFilter.h>
#include <gtest/gtest.h>

#include "../mocks/mock_IStorage.h"

using namespace ::testing;

class MessageFilterTest : public Test {
protected:
  void SetUp() override {
    filter = new Filter(mockStorage);
    underTest = new MessageFilter(*filter);
  }

  void TearDown() override {
    delete underTest;
    delete filter;
  }

  JsonObject createMessage(const char* key, const char* value) {
    messageBuffer.clear(); // Reset del documento
    JsonObject obj = messageBuffer.to<JsonObject>();
    obj[key] = value;
    return obj;
  }

  StaticJsonDocument<JSON_MSG_BUFFER> messageBuffer; // Variabile membro
  MockStorage mockStorage;
  Filter* filter = nullptr;
  MessageFilter* underTest = nullptr;
};

TEST_F(MessageFilterTest, MessagePresentInBlackListReturnsTrue) {
  filter->add("name", "blocked", Filter::BLOCK);
  JsonObject payload = createMessage("name", "blocked");

  EXPECT_TRUE(underTest->inBlockList(payload));
}

TEST_F(MessageFilterTest, BlacklistIgnoreFlagSkipsMatching) {
  filter->add("name", "blocked", Filter::BLOCK);
  underTest->ignoreBlockList(true);
  JsonObject payload = createMessage("name", "blocked");

  EXPECT_FALSE(underTest->inBlockList(payload));
}

TEST_F(MessageFilterTest, MessageNotInBlackListReturnsFalse) {
  filter->add("name", "blocked", Filter::BLOCK);
  JsonObject payload = createMessage("name", "allowed");

  EXPECT_FALSE(underTest->inBlockList(payload));
}

TEST_F(MessageFilterTest, EmptyWhitelistAllowsAllMessages) {
  JsonObject payload = createMessage("id", "any");

  EXPECT_TRUE(underTest->inPassList(payload));
}

TEST_F(MessageFilterTest, WhitelistMatchReturnsTrue) {
  filter->add("id", "sensorA", Filter::PASS);
  JsonObject payload = createMessage("id", "sensorA");

  EXPECT_TRUE(underTest->inPassList(payload));
}

TEST_F(MessageFilterTest, WhitelistMissReturnsFalse) {
  filter->add("id", "sensorA", Filter::PASS);
  JsonObject payload = createMessage("id", "sensorB");

  EXPECT_FALSE(underTest->inPassList(payload));
}

TEST_F(MessageFilterTest, WhitelistIgnoreFlagAlwaysAllows) {
  filter->add("id", "sensorA", Filter::PASS);
  underTest->ignorePassList(true);
  JsonObject payload = createMessage("id", "sensorB");

  EXPECT_TRUE(underTest->inPassList(payload));
}

// ============================================================================
// Block List Edge Cases
// ============================================================================

TEST_F(MessageFilterTest, EmptyBlockListAllowsAllMessages) {
  JsonObject payload = createMessage("id", "any");

  EXPECT_FALSE(underTest->inBlockList(payload));
}

TEST_F(MessageFilterTest, MultipleKeysInMessageFirstMatches) {
  filter->add("id", "blocked", Filter::BLOCK);

  messageBuffer.clear();
  JsonObject payload = messageBuffer.to<JsonObject>();
  payload["id"] = "blocked";
  payload["name"] = "allowed";

  EXPECT_TRUE(underTest->inBlockList(payload));
}

TEST_F(MessageFilterTest, MultipleKeysInMessageSecondMatches) {
  filter->add("name", "blocked", Filter::BLOCK);

  messageBuffer.clear();
  JsonObject payload = messageBuffer.to<JsonObject>();
  payload["id"] = "allowed";
  payload["name"] = "blocked";

  EXPECT_TRUE(underTest->inBlockList(payload));
}

TEST_F(MessageFilterTest, BlockListWithNonStringValuesIgnored) {
  filter->add("temp", "25", Filter::BLOCK);

  messageBuffer.clear();
  JsonObject payload = messageBuffer.to<JsonObject>();
  payload["temp"] = 25; // Integer, not string

  EXPECT_FALSE(underTest->inBlockList(payload));
}

// ============================================================================
// Pass List Edge Cases
// ============================================================================

TEST_F(MessageFilterTest, PassListWithMultipleEntriesSameKey) {
  filter->add("id", "sensorA", Filter::PASS);
  filter->add("id", "sensorB", Filter::PASS);

  JsonObject payload1 = createMessage("id", "sensorA");
  JsonObject payload2 = createMessage("id", "sensorB");

  EXPECT_TRUE(underTest->inPassList(payload1));
  EXPECT_TRUE(underTest->inPassList(payload2));
}

TEST_F(MessageFilterTest, PassListWithDifferentKeys) {
  filter->add("id", "sensorA", Filter::PASS);
  filter->add("type", "temperature", Filter::PASS);

  JsonObject payload1 = createMessage("id", "sensorA");
  JsonObject payload2 = createMessage("type", "temperature");

  EXPECT_TRUE(underTest->inPassList(payload1));
  EXPECT_TRUE(underTest->inPassList(payload2));
}

TEST_F(MessageFilterTest, PassListWithNonStringValuesIgnored) {
  filter->add("temp", "25", Filter::PASS);

  messageBuffer.clear();
  JsonObject payload = messageBuffer.to<JsonObject>();
  payload["temp"] = 25; // Integer, not string

  // Since pass list exists but no match, should return false
  EXPECT_FALSE(underTest->inPassList(payload));
}

TEST_F(MessageFilterTest, PassListMatchesFirstKeyInMultiKeyMessage) {
  filter->add("id", "sensorA", Filter::PASS);

  messageBuffer.clear();
  JsonObject payload = messageBuffer.to<JsonObject>();
  payload["id"] = "sensorA";
  payload["name"] = "unknown";

  EXPECT_TRUE(underTest->inPassList(payload));
}

// ============================================================================
// Ignore Flags Tests
// ============================================================================

TEST_F(MessageFilterTest, IsBlockListIgnoredReturnsFalseByDefault) {
  EXPECT_FALSE(underTest->isBlockListIgnored());
}

TEST_F(MessageFilterTest, IsBlockListIgnoredReturnsTrueAfterSet) {
  underTest->ignoreBlockList(true);

  EXPECT_TRUE(underTest->isBlockListIgnored());
}

TEST_F(MessageFilterTest, IsBlockListIgnoredCanBeToggled) {
  underTest->ignoreBlockList(true);
  EXPECT_TRUE(underTest->isBlockListIgnored());

  underTest->ignoreBlockList(false);
  EXPECT_FALSE(underTest->isBlockListIgnored());
}

TEST_F(MessageFilterTest, IsPassListIgnoredReturnsFalseByDefault) {
  EXPECT_FALSE(underTest->isPassListIgnored());
}

TEST_F(MessageFilterTest, IsPassListIgnoredReturnsTrueAfterSet) {
  underTest->ignorePassList(true);

  EXPECT_TRUE(underTest->isPassListIgnored());
}

TEST_F(MessageFilterTest, IsPassListIgnoredCanBeToggled) {
  underTest->ignorePassList(true);
  EXPECT_TRUE(underTest->isPassListIgnored());

  underTest->ignorePassList(false);
  EXPECT_FALSE(underTest->isPassListIgnored());
}

// ============================================================================
// MQTT Command Handling Tests
// ============================================================================

TEST_F(MessageFilterTest, HandleMQTTCommandWithNoFilterKeyDoesNothing) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  command["other"] = "value";

  underTest->handleMQTTCommand(command);

  // Verify no filters were added
  JsonObject testPayload = createMessage("id", "test");
  EXPECT_TRUE(underTest->inPassList(testPayload)); // Empty pass list allows all
}

TEST_F(MessageFilterTest, HandleMQTTCommandAddsPassFilters) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray idArray = passObj.createNestedArray("id");
  idArray.add("sensorA");
  idArray.add("sensorB");

  underTest->handleMQTTCommand(command);

  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject payload1 = doc1.to<JsonObject>();
  payload1["id"] = "sensorA";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject payload2 = doc2.to<JsonObject>();
  payload2["id"] = "sensorB";

  StaticJsonDocument<JSON_MSG_BUFFER> doc3;
  JsonObject payload3 = doc3.to<JsonObject>();
  payload3["id"] = "sensorC";

  EXPECT_TRUE(underTest->inPassList(payload1));
  EXPECT_TRUE(underTest->inPassList(payload2));
  EXPECT_FALSE(underTest->inPassList(payload3));
}

TEST_F(MessageFilterTest, HandleMQTTCommandAddsBlockFilters) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonObject blockObj = filterObj.createNestedObject("block");
  JsonArray nameArray = blockObj.createNestedArray("name");
  nameArray.add("blocked1");
  nameArray.add("blocked2");

  underTest->handleMQTTCommand(command);

  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject payload1 = doc1.to<JsonObject>();
  payload1["name"] = "blocked1";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject payload2 = doc2.to<JsonObject>();
  payload2["name"] = "blocked2";

  StaticJsonDocument<JSON_MSG_BUFFER> doc3;
  JsonObject payload3 = doc3.to<JsonObject>();
  payload3["name"] = "allowed";

  EXPECT_TRUE(underTest->inBlockList(payload1));
  EXPECT_TRUE(underTest->inBlockList(payload2));
  EXPECT_FALSE(underTest->inBlockList(payload3));
}

TEST_F(MessageFilterTest, HandleMQTTCommandSetsBothPassAndBlock) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");

  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray passArray = passObj.createNestedArray("id");
  passArray.add("allowed");

  JsonObject blockObj = filterObj.createNestedObject("block");
  JsonArray blockArray = blockObj.createNestedArray("id");
  blockArray.add("blocked");

  underTest->handleMQTTCommand(command);

  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject allowedPayload = doc1.to<JsonObject>();
  allowedPayload["id"] = "allowed";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject blockedPayload = doc2.to<JsonObject>();
  blockedPayload["id"] = "blocked";

  EXPECT_TRUE(underTest->inPassList(allowedPayload));
  EXPECT_TRUE(underTest->inBlockList(blockedPayload));
}

TEST_F(MessageFilterTest, HandleMQTTCommandSetsIgnorePassFlag) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["ignore_pass"] = true;

  underTest->handleMQTTCommand(command);

  EXPECT_TRUE(underTest->isPassListIgnored());
}

TEST_F(MessageFilterTest, HandleMQTTCommandSetsIgnoreBlockFlag) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["ignore_block"] = true;

  underTest->handleMQTTCommand(command);

  EXPECT_TRUE(underTest->isBlockListIgnored());
}

TEST_F(MessageFilterTest, HandleMQTTCommandSetsBothIgnoreFlags) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["ignore_pass"] = true;
  filterObj["ignore_block"] = true;

  underTest->handleMQTTCommand(command);

  EXPECT_TRUE(underTest->isPassListIgnored());
  EXPECT_TRUE(underTest->isBlockListIgnored());
}

// New tests for cmd handling

TEST_F(MessageFilterTest, HandleMQTTCommandActionResetClearsFiltersAndSetsIgnoreFlags) {
  // Pre-populate filters
  filter->add("id", "sensorX", Filter::PASS);
  filter->add("name", "blockedX", Filter::BLOCK);

  EXPECT_EQ(2u, filter->getTotalFilterCount());

  // Build command with cmd=reset and extra pass values (should be ignored because reset returns immediately)
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "reset";
  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray idArray = passObj.createNestedArray("id");
  idArray.add("newVal");

  underTest->handleMQTTCommand(command);

  // After reset: filters cleared and both ignore flags set, pass/block arrays NOT processed
  EXPECT_EQ(0u, filter->getTotalFilterCount());
  EXPECT_TRUE(underTest->isPassListIgnored());
  EXPECT_TRUE(underTest->isBlockListIgnored());

  // Pass ignored => always allowed
  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject payloadAllow = doc1.to<JsonObject>();
  payloadAllow["id"] = "anything";
  EXPECT_TRUE(underTest->inPassList(payloadAllow));

  // Block ignored => never blocks
  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject payloadBlock = doc2.to<JsonObject>();
  payloadBlock["name"] = "blockedX";
  EXPECT_FALSE(underTest->inBlockList(payloadBlock));
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionNewClearsThenLoadsFilters) {
  // Pre-populate filters and set ignore flags
  filter->add("id", "oldPass", Filter::PASS);
  filter->add("name", "oldBlock", Filter::BLOCK);
  underTest->ignorePassList(true);
  underTest->ignoreBlockList(true);

  EXPECT_EQ(2u, filter->getTotalFilterCount());

  // Build command with cmd=new and pass/block arrays
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "new";

  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray passId = passObj.createNestedArray("id");
  passId.add("allowed");

  JsonObject blockObj = filterObj.createNestedObject("block");
  JsonArray blockId = blockObj.createNestedArray("id");
  blockId.add("blocked");

  underTest->handleMQTTCommand(command);

  // Old filters cleared, new ones loaded
  EXPECT_EQ(2u, filter->getTotalFilterCount());
  EXPECT_TRUE(filter->contains("id", "allowed", Filter::PASS));
  EXPECT_TRUE(filter->contains("id", "blocked", Filter::BLOCK));

  // Ignore flags should remain unchanged (true) - 'new' cmd doesn't modify them
  EXPECT_TRUE(underTest->isPassListIgnored());
  EXPECT_TRUE(underTest->isBlockListIgnored());

  // Validate MessageFilter behavior
  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject allowedPayload = doc1.to<JsonObject>();
  allowedPayload["id"] = "allowed";
  EXPECT_TRUE(underTest->inPassList(allowedPayload));

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject blockedPayload = doc2.to<JsonObject>();
  blockedPayload["id"] = "blocked";
  EXPECT_FALSE(underTest->inBlockList(blockedPayload)); // Block list ignored, so returns false
}

TEST_F(MessageFilterTest, HandleMQTTCommandUnknownActionDoesNotClear) {
  // Pre-populate filters
  filter->add("id", "base", Filter::PASS);
  EXPECT_EQ(1u, filter->getTotalFilterCount());

  // Unknown cmd without pass/block -> no change
  messageBuffer.clear();
  JsonObject command1 = messageBuffer.to<JsonObject>();
  JsonObject filterObj1 = command1.createNestedObject("filter");
  filterObj1["cmd"] = "unknown";

  underTest->handleMQTTCommand(command1);
  EXPECT_EQ(1u, filter->getTotalFilterCount());
  EXPECT_TRUE(filter->contains("id", "base", Filter::PASS));

  // Unknown cmd with pass additions -> should add on top
  messageBuffer.clear();
  JsonObject command2 = messageBuffer.to<JsonObject>();
  JsonObject filterObj2 = command2.createNestedObject("filter");
  filterObj2["cmd"] = "unknown";
  JsonObject passObj2 = filterObj2.createNestedObject("pass");
  JsonArray idArray2 = passObj2.createNestedArray("id");
  idArray2.add("extra");

  underTest->handleMQTTCommand(command2);
  EXPECT_EQ(2u, filter->getTotalFilterCount());
  EXPECT_TRUE(filter->contains("id", "base", Filter::PASS));
  EXPECT_TRUE(filter->contains("id", "extra", Filter::PASS));
}

TEST_F(MessageFilterTest, HandleMQTTCommandWithNonArrayPassValueIsIgnored) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonObject passObj = filterObj.createNestedObject("pass");
  passObj["id"] = "notAnArray"; // Should be an array

  underTest->handleMQTTCommand(command);

  // Should still have empty pass list, allowing all
  JsonObject payload = createMessage("id", "test");
  EXPECT_TRUE(underTest->inPassList(payload));
}

TEST_F(MessageFilterTest, HandleMQTTCommandWithNonArrayBlockValueIsIgnored) {
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonObject blockObj = filterObj.createNestedObject("block");
  blockObj["name"] = "notAnArray"; // Should be an array

  underTest->handleMQTTCommand(command);

  // Should still have empty block list, allowing all
  JsonObject payload = createMessage("name", "test");
  EXPECT_FALSE(underTest->inBlockList(payload));
}

// ============================================================================
// Combined Pass and Block List Scenarios
// ============================================================================

TEST_F(MessageFilterTest, MessageInBothPassAndBlockLists) {
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("id", "sensor1", Filter::BLOCK);

  JsonObject payload = createMessage("id", "sensor1");

  // Should be in both lists
  EXPECT_TRUE(underTest->inPassList(payload));
  EXPECT_TRUE(underTest->inBlockList(payload));
}

TEST_F(MessageFilterTest, IgnoreFlagsPriorityOverFilters) {
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("name", "blocked", Filter::BLOCK);

  underTest->ignorePassList(true);
  underTest->ignoreBlockList(true);

  JsonObject passPayload = createMessage("id", "sensor1");
  JsonObject blockPayload = createMessage("name", "blocked");

  // With ignore flags, pass should allow all, block should block none
  EXPECT_TRUE(underTest->inPassList(passPayload));
  EXPECT_FALSE(underTest->inBlockList(blockPayload));
}

// ============================================================================
// Wildcard Pattern Tests (if supported by Filter)
// ============================================================================

TEST_F(MessageFilterTest, PassListWithWildcardPattern) {
  filter->add("id", "sensor*", Filter::PASS);

  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject payload1 = doc1.to<JsonObject>();
  payload1["id"] = "sensorA";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject payload2 = doc2.to<JsonObject>();
  payload2["id"] = "sensorB";

  StaticJsonDocument<JSON_MSG_BUFFER> doc3;
  JsonObject payload3 = doc3.to<JsonObject>();
  payload3["id"] = "device1";

  EXPECT_TRUE(underTest->inPassList(payload1));
  EXPECT_TRUE(underTest->inPassList(payload2));
  EXPECT_FALSE(underTest->inPassList(payload3));
}

TEST_F(MessageFilterTest, BlockListWithWildcardPattern) {
  filter->add("name", "temp_*", Filter::BLOCK);

  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject payload1 = doc1.to<JsonObject>();
  payload1["name"] = "temp_sensor1";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject payload2 = doc2.to<JsonObject>();
  payload2["name"] = "temp_sensor2";

  StaticJsonDocument<JSON_MSG_BUFFER> doc3;
  JsonObject payload3 = doc3.to<JsonObject>();
  payload3["name"] = "humidity";

  EXPECT_TRUE(underTest->inBlockList(payload1));
  EXPECT_TRUE(underTest->inBlockList(payload2));
  EXPECT_FALSE(underTest->inBlockList(payload3));
}

TEST_F(MessageFilterTest, WildcardQuestionMarkSingleChar) {
  filter->add("id", "sensor?", Filter::PASS);

  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject payload1 = doc1.to<JsonObject>();
  payload1["id"] = "sensorA";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject payload2 = doc2.to<JsonObject>();
  payload2["id"] = "sensor1";

  StaticJsonDocument<JSON_MSG_BUFFER> doc3;
  JsonObject payload3 = doc3.to<JsonObject>();
  payload3["id"] = "sensor12";

  EXPECT_TRUE(underTest->inPassList(payload1));
  EXPECT_TRUE(underTest->inPassList(payload2));
  EXPECT_FALSE(underTest->inPassList(payload3));
}

// ============================================================================
// Empty and Null Safety Tests
// ============================================================================

TEST_F(MessageFilterTest, EmptyMessageNotInBlockList) {
  filter->add("id", "test", Filter::BLOCK);

  messageBuffer.clear();
  JsonObject emptyPayload = messageBuffer.to<JsonObject>();

  EXPECT_FALSE(underTest->inBlockList(emptyPayload));
}

TEST_F(MessageFilterTest, EmptyMessageWithEmptyPassListAllowed) {
  messageBuffer.clear();
  JsonObject emptyPayload = messageBuffer.to<JsonObject>();

  EXPECT_TRUE(underTest->inPassList(emptyPayload));
}

TEST_F(MessageFilterTest, EmptyMessageWithNonEmptyPassListRejected) {
  filter->add("id", "test", Filter::PASS);

  messageBuffer.clear();
  JsonObject emptyPayload = messageBuffer.to<JsonObject>();

  EXPECT_FALSE(underTest->inPassList(emptyPayload));
}
// ============================================================================
// Clear Filter Tests
// ============================================================================

TEST_F(MessageFilterTest, HandleMQTTCommandActionClearRemovesAllFilters) {
  // Add some initial filters
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("name", "blocked", Filter::BLOCK);

  // Verify filters are active (use distinct documents to avoid reuse)
  StaticJsonDocument<JSON_MSG_BUFFER> verifyDoc1;
  JsonObject passPayload = verifyDoc1.to<JsonObject>();
  passPayload["id"] = "sensor1";
  StaticJsonDocument<JSON_MSG_BUFFER> verifyDoc2;
  JsonObject blockPayload = verifyDoc2.to<JsonObject>();
  blockPayload["name"] = "blocked";
  EXPECT_TRUE(underTest->inPassList(passPayload));
  EXPECT_TRUE(underTest->inBlockList(blockPayload));

  // Clear filters via MQTT command with cmd=clear
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "clear";

  underTest->handleMQTTCommand(command);

  // Clear should remove all filters
  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject newPassPayload = doc1.to<JsonObject>();
  newPassPayload["id"] = "sensor1";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject newBlockPayload = doc2.to<JsonObject>();
  newBlockPayload["name"] = "blocked";

  EXPECT_TRUE(underTest->inPassList(newPassPayload)); // Empty pass list allows all
  EXPECT_FALSE(underTest->inBlockList(newBlockPayload)); // Empty block list blocks none
}

TEST_F(MessageFilterTest, HandleMQTTCommandUnknownActionDoesNotRemoveFilters) {
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("name", "blocked", Filter::BLOCK);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "unknown_action";

  underTest->handleMQTTCommand(command);

  // Verify filters still exist
  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject passPayload = doc1.to<JsonObject>();
  passPayload["id"] = "sensor1";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject blockPayload = doc2.to<JsonObject>();
  blockPayload["name"] = "blocked";

  EXPECT_TRUE(underTest->inPassList(passPayload));
  EXPECT_TRUE(underTest->inBlockList(blockPayload));
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionClearReturnsImmediately) {
  filter->add("id", "oldSensor", Filter::PASS);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "clear";

  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray idArray = passObj.createNestedArray("id");
  idArray.add("newSensor");

  underTest->handleMQTTCommand(command);

  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject oldPayload = doc1.to<JsonObject>();
  oldPayload["id"] = "oldSensor";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject newPayload = doc2.to<JsonObject>();
  newPayload["id"] = "newSensor";

  EXPECT_TRUE(underTest->inPassList(oldPayload)); // Old filter cleared, empty pass allows all
  EXPECT_TRUE(underTest->inPassList(newPayload)); // New filter NOT added, empty pass allows all
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionNewWithoutFiltersJustClears) {
  filter->add("id", "sensor1", Filter::PASS);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "new";

  underTest->handleMQTTCommand(command);

  // Verify filter cleared (cmd=new clears but continues processing)
  JsonObject payload = createMessage("id", "sensor1");
  EXPECT_TRUE(underTest->inPassList(payload)); // Empty pass list allows all
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, HandleMQTTCommandNoActionAddsFiltersToExisting) {
  filter->add("id", "sensor1", Filter::PASS);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray idArray = passObj.createNestedArray("id");
  idArray.add("sensor2");

  underTest->handleMQTTCommand(command);

  // Verify both filters exist (additive when no cmd specified)
  EXPECT_EQ(2u, filter->getTotalFilterCount());
  EXPECT_TRUE(filter->contains("id", "sensor1", Filter::PASS));
  EXPECT_TRUE(filter->contains("id", "sensor2", Filter::PASS));
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionClearDoesNotResetIgnoreFlags) {
  filter->add("id", "sensor1", Filter::PASS);
  underTest->ignorePassList(true);
  underTest->ignoreBlockList(true);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "clear";

  underTest->handleMQTTCommand(command);

  // Verify cmd=clear only affects filters, not ignore flags
  EXPECT_EQ(0u, filter->getTotalFilterCount());
  EXPECT_TRUE(underTest->isPassListIgnored());
  EXPECT_TRUE(underTest->isBlockListIgnored());
}

TEST_F(MessageFilterTest, HandleMQTTCommandMultipleActionClearCallsAreIdempotent) {
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("name", "blocked", Filter::BLOCK);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "clear";

  // Clear twice
  underTest->handleMQTTCommand(command);
  underTest->handleMQTTCommand(command);

  EXPECT_EQ(0u, filter->getTotalFilterCount());
  JsonObject testPayload = createMessage("id", "anything");
  EXPECT_TRUE(underTest->inPassList(testPayload)); // Empty pass list allows all
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionClearDoesNotAffectSubsequentCommands) {
  messageBuffer.clear();
  JsonObject clearCommand = messageBuffer.to<JsonObject>();
  JsonObject clearFilter = clearCommand.createNestedObject("filter");
  clearFilter["cmd"] = "clear";

  underTest->handleMQTTCommand(clearCommand);
  EXPECT_EQ(0u, filter->getTotalFilterCount());

  // Add new filters after clear
  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject addCommand = doc2.to<JsonObject>();
  JsonObject addFilter = addCommand.createNestedObject("filter");
  JsonObject passObj = addFilter.createNestedObject("pass");
  JsonArray idArray = passObj.createNestedArray("id");
  idArray.add("newSensor");

  underTest->handleMQTTCommand(addCommand);

  EXPECT_EQ(1u, filter->getTotalFilterCount());
  StaticJsonDocument<JSON_MSG_BUFFER> doc3;
  JsonObject payload = doc3.to<JsonObject>();
  payload["id"] = "newSensor";

  EXPECT_TRUE(underTest->inPassList(payload));
}

TEST_F(MessageFilterTest, HandleMQTTCommandEmptyFilterObjectDoesNothing) {
  filter->add("id", "sensor1", Filter::PASS);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  command.createNestedObject("filter"); // Empty filter object

  underTest->handleMQTTCommand(command);

  // Verify existing filter unchanged
  JsonObject payload = createMessage("id", "sensor1");
  EXPECT_TRUE(underTest->inPassList(payload));
}

TEST_F(MessageFilterTest, HandleMQTTCommandWithOnlyActionClear) {
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("id", "sensor2", Filter::PASS);
  filter->add("name", "device1", Filter::BLOCK);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "clear";

  underTest->handleMQTTCommand(command);

  // Verify all filters cleared
  EXPECT_EQ(0u, filter->getTotalFilterCount());

  StaticJsonDocument<JSON_MSG_BUFFER> doc1;
  JsonObject payload1 = doc1.to<JsonObject>();
  payload1["id"] = "sensor1";

  StaticJsonDocument<JSON_MSG_BUFFER> doc2;
  JsonObject payload2 = doc2.to<JsonObject>();
  payload2["name"] = "device1";

  EXPECT_TRUE(underTest->inPassList(payload1)); // Empty pass allows all
  EXPECT_FALSE(underTest->inBlockList(payload2)); // Empty block blocks none
}

// ============================================================================
// Storage cmd Tests
// ============================================================================

TEST_F(MessageFilterTest, HandleMQTTCommandActionPersistCallsSaveOnStorage) {
  // Add filters
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("name", "blocked", Filter::BLOCK);

  // Expect underlying storage methods to be called
  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(_, _)).Times(1).WillOnce(Return(100));
  EXPECT_CALL(mockStorage, end()).Times(1);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "persist";

  underTest->handleMQTTCommand(command);

  // Filters should remain unchanged
  EXPECT_EQ(2u, filter->getTotalFilterCount());
  EXPECT_TRUE(filter->contains("id", "sensor1", Filter::PASS));
  EXPECT_TRUE(filter->contains("name", "blocked", Filter::BLOCK));
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionPersistReturnsImmediately) {
  filter->add("id", "sensor1", Filter::PASS);

  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(_, _)).Times(1).WillOnce(Return(100));
  EXPECT_CALL(mockStorage, end()).Times(1);

  // Add persist cmd with additional pass filters (should be ignored)
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "persist";
  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray idArray = passObj.createNestedArray("id");
  idArray.add("sensor2");

  underTest->handleMQTTCommand(command);

  // Only original filter should exist (persist returns immediately)
  EXPECT_EQ(1u, filter->getTotalFilterCount());
  EXPECT_TRUE(filter->contains("id", "sensor1", Filter::PASS));
  EXPECT_FALSE(filter->contains("id", "sensor2", Filter::PASS));
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionReloadCallsLoadFromStorage) {
  // Add initial filters
  filter->add("id", "initial", Filter::PASS);
  EXPECT_EQ(1u, filter->getTotalFilterCount());

  // Mock underlying storage methods to simulate loading filters
  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(_, _))
      .Times(1)
      .WillOnce(Return("{\"id\":[\"loaded\"]}"));
  EXPECT_CALL(mockStorage, end()).Times(1);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "reload";

  underTest->handleMQTTCommand(command);

  // Original filter cleared, loaded filter should be present in BOTH lists
  // Note: Filter.from() adds to both PASS and BLOCK lists
  EXPECT_FALSE(filter->contains("id", "initial", Filter::PASS));
  EXPECT_TRUE(filter->contains("id", "loaded", Filter::PASS));
  EXPECT_TRUE(filter->contains("id", "loaded", Filter::BLOCK));
  EXPECT_EQ(2u, filter->getTotalFilterCount()); // One in PASS, one in BLOCK
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionReloadReturnsImmediately) {
  // Mock underlying storage methods
  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mockStorage, getString(_, _))
      .Times(1)
      .WillOnce(Return("{\"id\":[\"fromStorage\"]}"));
  EXPECT_CALL(mockStorage, end()).Times(1);

  // Add reload cmd with additional pass filters (should be ignored)
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "reload";
  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray idArray = passObj.createNestedArray("id");
  idArray.add("fromCommand");

  underTest->handleMQTTCommand(command);

  // Only storage filter should exist (reload returns immediately)
  // Filter.from() adds to both PASS and BLOCK lists
  EXPECT_TRUE(filter->contains("id", "fromStorage", Filter::PASS));
  EXPECT_TRUE(filter->contains("id", "fromStorage", Filter::BLOCK));
  EXPECT_FALSE(filter->contains("id", "fromCommand", Filter::PASS));
  EXPECT_EQ(2u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionPurgeCallsEraseStorage) {
  // Add filters
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("name", "blocked", Filter::BLOCK);
  EXPECT_EQ(2u, filter->getTotalFilterCount());

  // Expect underlying storage methods to be called
  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, isKey(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mockStorage, remove(_)).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockStorage, end()).Times(1);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "purge";

  underTest->handleMQTTCommand(command);

  // All filters should be cleared
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionPurgeReturnsImmediately) {
  filter->add("id", "sensor1", Filter::PASS);

  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, isKey(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mockStorage, remove(_)).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockStorage, end()).Times(1);

  // Add purge cmd with additional pass filters (should be ignored)
  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "purge";
  JsonObject passObj = filterObj.createNestedObject("pass");
  JsonArray idArray = passObj.createNestedArray("id");
  idArray.add("sensor2");

  underTest->handleMQTTCommand(command);

  // All filters cleared (purge returns immediately, pass filters not processed)
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, HandleMQTTCommandStorageActionsDoNotModifyIgnoreFlags) {
  // Set ignore flags
  underTest->ignorePassList(true);
  underTest->ignoreBlockList(true);

  // Test persist cmd
  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, putString(_, _)).Times(1).WillOnce(Return(100));
  EXPECT_CALL(mockStorage, end()).Times(1);
  messageBuffer.clear();
  JsonObject command1 = messageBuffer.to<JsonObject>();
  JsonObject filterObj1 = command1.createNestedObject("filter");
  filterObj1["cmd"] = "persist";
  underTest->handleMQTTCommand(command1);
  EXPECT_TRUE(underTest->isPassListIgnored());
  EXPECT_TRUE(underTest->isBlockListIgnored());

  // Test reload cmd (no key in storage)
  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(_)).Times(1).WillOnce(Return(false));
  EXPECT_CALL(mockStorage, end()).Times(1);
  messageBuffer.clear();
  JsonObject command2 = messageBuffer.to<JsonObject>();
  JsonObject filterObj2 = command2.createNestedObject("filter");
  filterObj2["cmd"] = "reload";
  underTest->handleMQTTCommand(command2);
  EXPECT_TRUE(underTest->isPassListIgnored());
  EXPECT_TRUE(underTest->isBlockListIgnored());

  // Test purge cmd
  EXPECT_CALL(mockStorage, begin(false)).Times(1);
  EXPECT_CALL(mockStorage, isKey(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(mockStorage, remove(_)).Times(1).WillOnce(Return(1));
  EXPECT_CALL(mockStorage, end()).Times(1);
  messageBuffer.clear();
  JsonObject command3 = messageBuffer.to<JsonObject>();
  JsonObject filterObj3 = command3.createNestedObject("filter");
  filterObj3["cmd"] = "purge";
  underTest->handleMQTTCommand(command3);
  EXPECT_TRUE(underTest->isPassListIgnored());
  EXPECT_TRUE(underTest->isBlockListIgnored());
}

TEST_F(MessageFilterTest, HandleMQTTCommandActionReloadWithEmptyStorage) {
  filter->add("id", "existing", Filter::PASS);
  EXPECT_EQ(1u, filter->getTotalFilterCount());

  // Mock storage returning no key
  EXPECT_CALL(mockStorage, begin(true)).Times(1);
  EXPECT_CALL(mockStorage, isKey(_)).Times(1).WillOnce(Return(false));
  EXPECT_CALL(mockStorage, end()).Times(1);

  messageBuffer.clear();
  JsonObject command = messageBuffer.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj["cmd"] = "reload";

  underTest->handleMQTTCommand(command);

  // Filters cleared, nothing loaded
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}
// ============================================================================
// Serialization Tests (MessageFilter::to)
// ============================================================================

TEST_F(MessageFilterTest, ToSerializesIgnoreFlagsDefaultValues) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();

  underTest->to(data);

  ASSERT_TRUE(data.containsKey("filter"));
  JsonObject filterObj = data["filter"];

  EXPECT_FALSE(filterObj["ignore_pass"].as<bool>());
  EXPECT_FALSE(filterObj["ignore_block"].as<bool>());
}

TEST_F(MessageFilterTest, ToSerializesIgnoreFlagsWhenSet) {
  underTest->ignorePassList(true);
  underTest->ignoreBlockList(true);

  StaticJsonDocument<512> doc;
  JsonObject data = doc.to<JsonObject>();

  underTest->to(data);

  ASSERT_TRUE(data.containsKey("filter"));
  JsonObject filterObj = data["filter"];

  EXPECT_TRUE(filterObj["ignore_pass"].as<bool>());
  EXPECT_TRUE(filterObj["ignore_block"].as<bool>());
}

TEST_F(MessageFilterTest, ToSerializesPassFilters) {
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("id", "sensor2", Filter::PASS);
  filter->add("name", "device1", Filter::PASS);

  StaticJsonDocument<1024> doc;
  JsonObject data = doc.to<JsonObject>();

  underTest->to(data);

  ASSERT_TRUE(data.containsKey("filter"));
  JsonObject filterObj = data["filter"];

  ASSERT_TRUE(filterObj.containsKey("rules"));
  JsonArray rules = filterObj["rules"].as<JsonArray>();

  // Should have 3 pass rules in the array
  ASSERT_EQ(3u, rules.size());

  // Count pass actions
  int passCount = 0;
  for (JsonVariant ruleVar : rules) {
    JsonObject rule = ruleVar.as<JsonObject>();
    const char* action = rule["action"];
    if (action && strcmp(action, "pass") == 0) {
      passCount++;
    }
  }
  ASSERT_EQ(3, passCount);
}

TEST_F(MessageFilterTest, ToSerializesBlockFilters) {
  filter->add("id", "blocked1", Filter::BLOCK);
  filter->add("name", "blocked2", Filter::BLOCK);

  StaticJsonDocument<1024> doc;
  JsonObject data = doc.to<JsonObject>();

  underTest->to(data);

  ASSERT_TRUE(data.containsKey("filter"));
  JsonObject filterObj = data["filter"];
  ASSERT_TRUE(filterObj.containsKey("rules"));
  JsonArray rules = filterObj["rules"].as<JsonArray>();

  // Should have 2 block rules
  ASSERT_EQ(2u, rules.size());

  // Count block actions and verify values are present
  int blockCount = 0;
  bool foundBlocked1 = false, foundBlocked2 = false;
  for (JsonVariant ruleVar : rules) {
    JsonObject rule = ruleVar.as<JsonObject>();
    const char* action = rule["action"];
    const char* value = rule["value"];
    if (action && strcmp(action, "block") == 0) {
      blockCount++;
      if (value && strcmp(value, "blocked1") == 0) foundBlocked1 = true;
      if (value && strcmp(value, "blocked2") == 0) foundBlocked2 = true;
    }
  }
  ASSERT_EQ(2, blockCount);
  ASSERT_TRUE(foundBlocked1);
  ASSERT_TRUE(foundBlocked2);
}

TEST_F(MessageFilterTest, ToSerializesBothPassAndBlockFilters) {
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("name", "blocked1", Filter::BLOCK);

  StaticJsonDocument<1024> doc;
  JsonObject data = doc.to<JsonObject>();

  underTest->to(data);

  ASSERT_TRUE(data.containsKey("filter"));
  JsonObject filterObj = data["filter"];
  ASSERT_TRUE(filterObj.containsKey("rules"));
  JsonArray rules = filterObj["rules"].as<JsonArray>();

  // Should have 2 rules (1 pass + 1 block)
  ASSERT_EQ(2u, rules.size());

  // Count pass and block actions
  int passCount = 0, blockCount = 0;
  for (JsonVariant ruleVar : rules) {
    JsonObject rule = ruleVar.as<JsonObject>();
    const char* action = rule["action"];
    if (action && strcmp(action, "pass") == 0) {
      passCount++;
    } else if (action && strcmp(action, "block") == 0) {
      blockCount++;
    }
  }
  ASSERT_EQ(1, passCount);
  ASSERT_EQ(1, blockCount);
}

TEST_F(MessageFilterTest, ToSerializesEmptyFilters) {
  StaticJsonDocument<512> doc;
  JsonObject data = doc.to<JsonObject>();

  underTest->to(data);

  ASSERT_TRUE(data.containsKey("filter"));
  JsonObject filterObj = data["filter"];
  ASSERT_TRUE(filterObj.containsKey("rules"));

  // Empty filters should produce empty rules array
  JsonArray rules = filterObj["rules"].as<JsonArray>();
  EXPECT_EQ(0u, rules.size());
}

TEST_F(MessageFilterTest, ToCreatesNestedFilterObject) {
  StaticJsonDocument<512> doc;
  JsonObject data = doc.to<JsonObject>();

  // Add some other data to verify nesting doesn't overwrite
  data["other_key"] = "other_value";

  underTest->to(data);

  EXPECT_TRUE(data.containsKey("other_key"));
  EXPECT_STREQ("other_value", data["other_key"].as<const char*>());
  EXPECT_TRUE(data.containsKey("filter"));
}

TEST_F(MessageFilterTest, ToSerializesCompleteConfiguration) {
  // Set ignore flags
  underTest->ignorePassList(true);
  underTest->ignoreBlockList(false);

  // Add various filters
  filter->add("id", "sensor1", Filter::PASS);
  filter->add("id", "sensor2", Filter::PASS);
  filter->add("type", "temp", Filter::PASS);
  filter->add("name", "blocked", Filter::BLOCK);

  StaticJsonDocument<1024> doc;
  JsonObject data = doc.to<JsonObject>();

  underTest->to(data);

  ASSERT_TRUE(data.containsKey("filter"));
  JsonObject filterObj = data["filter"];

  // Verify ignore flags
  EXPECT_TRUE(filterObj["ignore_pass"].as<bool>());
  EXPECT_FALSE(filterObj["ignore_block"].as<bool>());

  // Verify rules array contains both pass and block rules
  ASSERT_TRUE(filterObj.containsKey("rules"));
  JsonArray rules = filterObj["rules"].as<JsonArray>();
  ASSERT_EQ(4u, rules.size()); // 3 pass + 1 block

  // Count pass and block rules
  int passCount = 0, blockCount = 0;
  for (JsonVariant ruleVar : rules) {
    JsonObject rule = ruleVar.as<JsonObject>();
    const char* action = rule["action"];
    if (action && strcmp(action, "pass") == 0) {
      passCount++;
    } else if (action && strcmp(action, "block") == 0) {
      blockCount++;
    }
  }
  EXPECT_EQ(3, passCount);
  EXPECT_EQ(1, blockCount);
}

TEST_F(MessageFilterTest, ToWithMixedIgnoreFlags) {
  underTest->ignorePassList(false);
  underTest->ignoreBlockList(true);

  StaticJsonDocument<512> doc;
  JsonObject data = doc.to<JsonObject>();

  underTest->to(data);

  JsonObject filterObj = data["filter"];
  EXPECT_FALSE(filterObj["ignore_pass"].as<bool>());
  EXPECT_TRUE(filterObj["ignore_block"].as<bool>());
}

TEST_F(MessageFilterTest, ToMultipleCallsProduceSameResult) {
  filter->add("id", "test", Filter::PASS);
  underTest->ignorePassList(true);

  StaticJsonDocument<1024> doc1;
  JsonObject data1 = doc1.to<JsonObject>();
  underTest->to(data1);

  StaticJsonDocument<1024> doc2;
  JsonObject data2 = doc2.to<JsonObject>();
  underTest->to(data2);

  // Both should have same structure
  EXPECT_TRUE(data1.containsKey("filter"));
  EXPECT_TRUE(data2.containsKey("filter"));
  EXPECT_EQ(data1["filter"]["ignore_pass"].as<bool>(),
            data2["filter"]["ignore_pass"].as<bool>());
}

// ============================================================================
// Rules Array Parsing Tests - Type Safety
// ============================================================================

TEST_F(MessageFilterTest, RulesWithValidJsonObjectElements) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  JsonObject rule1 = rulesArray.createNestedObject();
  rule1["target"] = "topic";
  rule1["action"] = "pass";
  rule1["value"] = "home/sensor/temp";

  underTest->handleMQTTCommand(command);

  EXPECT_TRUE(filter->isTopicFilterPresent("home/sensor/temp", Filter::PASS));
}

TEST_F(MessageFilterTest, RulesWithMultipleValidObjects) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // First rule - topic filter
  JsonObject rule1 = rulesArray.createNestedObject();
  rule1["target"] = "topic";
  rule1["action"] = "pass";
  rule1["value"] = "home/sensor/temp";

  // Second rule - key-value filter
  JsonObject rule2 = rulesArray.createNestedObject();
  rule2["target"] = "message";
  rule2["action"] = "block";
  rule2["key"] = "id";
  rule2["value"] = "badDevice";

  underTest->handleMQTTCommand(command);

  EXPECT_TRUE(filter->isTopicFilterPresent("home/sensor/temp", Filter::PASS));
  EXPECT_TRUE(filter->contains("id", "badDevice", Filter::BLOCK));
  EXPECT_EQ(2u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayWithInvalidNonObjectElements) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Add a non-object element (string) - should be skipped
  rulesArray.add("invalid_string");

  // Add a valid object after the invalid element
  JsonObject validRule = rulesArray.createNestedObject();
  validRule["target"] = "topic";
  validRule["action"] = "pass";
  validRule["value"] = "home/sensor/temp";

  underTest->handleMQTTCommand(command);

  // Only the valid rule should be processed
  EXPECT_TRUE(filter->isTopicFilterPresent("home/sensor/temp", Filter::PASS));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayWithNumberElement) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Add a number element (invalid) - should be skipped
  rulesArray.add(42);

  // Add valid rule after
  JsonObject validRule = rulesArray.createNestedObject();
  validRule["target"] = "message";
  validRule["action"] = "pass";
  validRule["key"] = "id";
  validRule["value"] = "sensor1";

  underTest->handleMQTTCommand(command);

  // Only valid rule should be added
  EXPECT_TRUE(filter->contains("id", "sensor1", Filter::PASS));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayWithNestedArrayElement) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Add a nested array element (invalid) - should be skipped
  JsonArray invalidArray = rulesArray.createNestedArray();
  invalidArray.add("test");

  // Add valid rule
  JsonObject validRule = rulesArray.createNestedObject();
  validRule["target"] = "topic";
  validRule["action"] = "block";
  validRule["value"] = "home/blocked/topic";

  underTest->handleMQTTCommand(command);

  // Only valid rule should be processed
  EXPECT_TRUE(filter->isTopicFilterPresent("home/blocked/topic", Filter::BLOCK));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayMixedValidAndInvalidElements) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Valid object
  JsonObject rule1 = rulesArray.createNestedObject();
  rule1["target"] = "topic";
  rule1["action"] = "pass";
  rule1["value"] = "home/sensor/temp";

  // Invalid: string
  rulesArray.add("invalid");

  // Valid object
  JsonObject rule2 = rulesArray.createNestedObject();
  rule2["target"] = "topic";
  rule2["action"] = "block";
  rule2["value"] = "home/blocked/topic";

  // Invalid: number
  rulesArray.add(99);

  // Valid object
  JsonObject rule3 = rulesArray.createNestedObject();
  rule3["target"] = "message";
  rule3["action"] = "pass";
  rule3["key"] = "name";
  rule3["value"] = "sensor";

  underTest->handleMQTTCommand(command);

  // All three valid rules should be processed
  EXPECT_TRUE(filter->isTopicFilterPresent("home/sensor/temp", Filter::PASS));
  EXPECT_TRUE(filter->isTopicFilterPresent("home/blocked/topic", Filter::BLOCK));
  EXPECT_TRUE(filter->contains("name", "sensor", Filter::PASS));
  EXPECT_EQ(3u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayEmptyArray) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  filterObj.createNestedArray("rules"); // Empty array

  underTest->handleMQTTCommand(command);

  // No filters should be added
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayOnlyInvalidElements) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Add only invalid elements
  rulesArray.add("string1");
  rulesArray.add(123);
  rulesArray.add("string2");

  underTest->handleMQTTCommand(command);

  // No valid rules processed
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesObjectSkipsInvalidAndProcessesValidFields) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Valid rule with all required fields
  JsonObject validRule = rulesArray.createNestedObject();
  validRule["target"] = "message";
  validRule["action"] = "pass";
  validRule["key"] = "id";
  validRule["value"] = "device1";

  // Invalid rule - missing required field "value"
  JsonObject invalidRule = rulesArray.createNestedObject();
  invalidRule["target"] = "message";
  invalidRule["action"] = "pass";
  invalidRule["key"] = "id";
  // No "value" field

  underTest->handleMQTTCommand(command);

  // Only valid rule should be added
  EXPECT_TRUE(filter->contains("id", "device1", Filter::PASS));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesTopicFilterWithValidObject) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  JsonObject topicRule = rulesArray.createNestedObject();
  topicRule["target"] = "topic";
  topicRule["action"] = "pass";
  topicRule["value"] = "home/kitchen/temp";

  underTest->handleMQTTCommand(command);

  EXPECT_TRUE(underTest->allowedTopic("home/kitchen/temp"));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesMessageFilterWithValidObject) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  JsonObject msgRule = rulesArray.createNestedObject();
  msgRule["target"] = "message";
  msgRule["action"] = "block";
  msgRule["key"] = "id";
  msgRule["value"] = "blocked_id";

  JsonObject payload = createMessage("id", "blocked_id");
  underTest->handleMQTTCommand(command);

  EXPECT_TRUE(underTest->inBlockList(payload));
}

// ============================================================================
// allowedTopic Tests
// ============================================================================

TEST_F(MessageFilterTest, AllowedTopicReturnsTrueWhenNoFilters) {
  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
  EXPECT_TRUE(underTest->allowedTopic("any/topic"));
}

TEST_F(MessageFilterTest, AllowedTopicReturnsFalseWhenInBlockList) {
  filter->addTopicFilters("home/blocked/topic", Filter::BLOCK);

  EXPECT_FALSE(underTest->allowedTopic("home/blocked/topic"));
}

TEST_F(MessageFilterTest, AllowedTopicReturnsTrueWhenNotInBlockList) {
  filter->addTopicFilters("home/blocked/topic", Filter::BLOCK);

  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
}

TEST_F(MessageFilterTest, AllowedTopicReturnsTrueWhenInPassList) {
  filter->addTopicFilters("home/sensor/temp", Filter::PASS);

  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
}

TEST_F(MessageFilterTest, AllowedTopicReturnsFalseWhenNotInPassList) {
  filter->addTopicFilters("home/sensor/temp", Filter::PASS);

  EXPECT_FALSE(underTest->allowedTopic("home/sensor/humidity"));
}

TEST_F(MessageFilterTest, AllowedTopicBlockListTakesPrecedence) {
  filter->addTopicFilters("home/sensor/temp", Filter::PASS);
  filter->addTopicFilters("home/sensor/temp", Filter::BLOCK);

  EXPECT_FALSE(underTest->allowedTopic("home/sensor/temp"));
}

TEST_F(MessageFilterTest, AllowedTopicWithWildcardInPassList) {
  filter->addTopicFilters("home/sensor/*", Filter::PASS);

  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
  EXPECT_TRUE(underTest->allowedTopic("home/sensor/humidity"));
  EXPECT_FALSE(underTest->allowedTopic("home/actuator/light"));
}

TEST_F(MessageFilterTest, AllowedTopicWithWildcardInBlockList) {
  filter->addTopicFilters("home/blocked/*", Filter::BLOCK);

  EXPECT_FALSE(underTest->allowedTopic("home/blocked/topic1"));
  EXPECT_FALSE(underTest->allowedTopic("home/blocked/topic2"));
  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
}

TEST_F(MessageFilterTest, AllowedTopicIgnoresBlockListWhenFlagSet) {
  filter->addTopicFilters("home/blocked/topic", Filter::BLOCK);
  underTest->ignoreBlockList(true);

  EXPECT_TRUE(underTest->allowedTopic("home/blocked/topic"));
}

TEST_F(MessageFilterTest, AllowedTopicIgnoresPassListWhenFlagSet) {
  filter->addTopicFilters("home/sensor/temp", Filter::PASS);
  underTest->ignorePassList(true);

  EXPECT_TRUE(underTest->allowedTopic("home/sensor/humidity"));
  EXPECT_TRUE(underTest->allowedTopic("any/topic"));
}

TEST_F(MessageFilterTest, AllowedTopicWithNullPointerReturnsFalse) {
  // Defensive test - depends on implementation handling of null
  EXPECT_FALSE(underTest->allowedTopic(nullptr));
}

TEST_F(MessageFilterTest, AllowedTopicWithEmptyString) {
  filter->addTopicFilters("", Filter::PASS);

  EXPECT_TRUE(underTest->allowedTopic(""));
  EXPECT_FALSE(underTest->allowedTopic("home/sensor/temp"));
}

TEST_F(MessageFilterTest, AllowedTopicWithComplexWildcardPattern) {
  filter->addTopicFilters("home/*/temp", Filter::PASS);

  EXPECT_TRUE(underTest->allowedTopic("home/kitchen/temp"));
  EXPECT_TRUE(underTest->allowedTopic("home/bedroom/temp"));
  EXPECT_FALSE(underTest->allowedTopic("home/kitchen/humidity"));
}

TEST_F(MessageFilterTest, AllowedTopicMultipleTopicsInPassList) {
  filter->addTopicFilters("home/sensor/temp", Filter::PASS);
  filter->addTopicFilters("home/sensor/humidity", Filter::PASS);
  filter->addTopicFilters("home/actuator/light", Filter::PASS);

  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
  EXPECT_TRUE(underTest->allowedTopic("home/sensor/humidity"));
  EXPECT_TRUE(underTest->allowedTopic("home/actuator/light"));
  EXPECT_FALSE(underTest->allowedTopic("home/sensor/pressure"));
}

TEST_F(MessageFilterTest, AllowedTopicMultipleTopicsInBlockList) {
  filter->addTopicFilters("home/blocked/topic1", Filter::BLOCK);
  filter->addTopicFilters("home/blocked/topic2", Filter::BLOCK);

  EXPECT_FALSE(underTest->allowedTopic("home/blocked/topic1"));
  EXPECT_FALSE(underTest->allowedTopic("home/blocked/topic2"));
  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
}

TEST_F(MessageFilterTest, AllowedTopicEmptyPassListAllowsAll) {
  // When pass list is empty, all topics should be allowed (except blocked ones)
  EXPECT_TRUE(underTest->allowedTopic("any/topic"));
  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
}

TEST_F(MessageFilterTest, AllowedTopicEmptyBlockListAllowsAll) {
  // When block list is empty and pass list matches, topic is allowed
  filter->addTopicFilters("home/sensor/*", Filter::PASS);

  EXPECT_TRUE(underTest->allowedTopic("home/sensor/temp"));
}

// ============================================================================
// Rules Array - Action Validation Tests
// ============================================================================

TEST_F(MessageFilterTest, RulesArrayRejectInvalidActionValue) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Invalid action - not "pass" or "block"
  JsonObject invalidRule = rulesArray.createNestedObject();
  invalidRule["target"] = "message";
  invalidRule["action"] = "allow"; // Invalid!
  invalidRule["key"] = "id";
  invalidRule["value"] = "device1";

  // Valid rule after invalid one
  JsonObject validRule = rulesArray.createNestedObject();
  validRule["target"] = "message";
  validRule["action"] = "pass";
  validRule["key"] = "name";
  validRule["value"] = "sensor";

  underTest->handleMQTTCommand(command);

  // Only valid rule should be added
  EXPECT_TRUE(filter->contains("name", "sensor", Filter::PASS));
  EXPECT_FALSE(filter->contains("id", "device1", Filter::PASS));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayOnlyAcceptsPassOrBlock) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Try various invalid actions
  const char* invalidActions[] = {"allow", "deny", "whitelist", "blacklist", "include", "exclude"};

  for (const char* action : invalidActions) {
    JsonObject rule = rulesArray.createNestedObject();
    rule["action"] = action;
    rule["key"] = "id";
    rule["value"] = "test";
  }

  // Add one valid rule
  JsonObject validRule = rulesArray.createNestedObject();
  validRule["action"] = "pass";
  validRule["key"] = "id";
  validRule["value"] = "valid";

  underTest->handleMQTTCommand(command);

  // Only the valid pass rule should be added
  EXPECT_TRUE(filter->contains("id", "valid", Filter::PASS));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayTargetIsOptional) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Rule WITHOUT target field - should default to "message"
  JsonObject rule = rulesArray.createNestedObject();
  // NO target field
  rule["action"] = "pass";
  rule["key"] = "id";
  rule["value"] = "device1";

  underTest->handleMQTTCommand(command);

  // Rule should be processed as message filter (default target)
  EXPECT_TRUE(filter->contains("id", "device1", Filter::PASS));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayTopicRuleDoesNotRequireKey) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Topic rule - no key needed
  JsonObject topicRule = rulesArray.createNestedObject();
  topicRule["target"] = "topic";
  topicRule["action"] = "pass";
  topicRule["value"] = "home/sensor/*";

  underTest->handleMQTTCommand(command);

  EXPECT_TRUE(filter->isTopicFilterPresent("home/sensor/temp", Filter::PASS));
  EXPECT_EQ(1u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayMessageRuleRequiresKey) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Message rule WITHOUT key - should fail
  JsonObject messageRule = rulesArray.createNestedObject();
  messageRule["action"] = "pass";
  messageRule["value"] = "device1";
  // NO key field and NO target (defaults to message)

  underTest->handleMQTTCommand(command);

  // Rule should NOT be added
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayMissingActionField) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Rule missing action field
  JsonObject rule = rulesArray.createNestedObject();
  rule["key"] = "id";
  rule["value"] = "device1";
  // NO action field

  underTest->handleMQTTCommand(command);

  // Rule should NOT be added
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayMissingValueField) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Rule missing value field
  JsonObject rule = rulesArray.createNestedObject();
  rule["action"] = "pass";
  rule["key"] = "id";
  // NO value field

  underTest->handleMQTTCommand(command);

  // Rule should NOT be added
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayPassAndBlockActions) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Pass rule
  JsonObject passRule = rulesArray.createNestedObject();
  passRule["action"] = "pass";
  passRule["key"] = "id";
  passRule["value"] = "allowed";

  // Block rule
  JsonObject blockRule = rulesArray.createNestedObject();
  blockRule["action"] = "block";
  blockRule["key"] = "id";
  blockRule["value"] = "blocked";

  underTest->handleMQTTCommand(command);

  // Both should be added
  EXPECT_TRUE(filter->contains("id", "allowed", Filter::PASS));
  EXPECT_TRUE(filter->contains("id", "blocked", Filter::BLOCK));
  EXPECT_EQ(2u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayCaseSensitiveAction) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Wrong case - "Pass" instead of "pass"
  JsonObject rule = rulesArray.createNestedObject();
  rule["action"] = "Pass"; // Wrong case
  rule["key"] = "id";
  rule["value"] = "device1";

  underTest->handleMQTTCommand(command);

  // Rule should NOT be added (case sensitive)
  EXPECT_EQ(0u, filter->getTotalFilterCount());
}

TEST_F(MessageFilterTest, RulesArrayMixedTargetOptionalAndRequired) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject command = doc.to<JsonObject>();
  JsonObject filterObj = command.createNestedObject("filter");
  JsonArray rulesArray = filterObj.createNestedArray("rules");

  // Topic rule (target specified, no key)
  JsonObject topicRule = rulesArray.createNestedObject();
  topicRule["target"] = "topic";
  topicRule["action"] = "pass";
  topicRule["value"] = "home/sensor/*";

  // Message rule (target omitted, defaults to message, requires key)
  JsonObject messageRule = rulesArray.createNestedObject();
  messageRule["action"] = "block";
  messageRule["key"] = "name";
  messageRule["value"] = "BRO*";

  underTest->handleMQTTCommand(command);

  // Both should be added
  EXPECT_TRUE(filter->isTopicFilterPresent("home/sensor/temp", Filter::PASS));
  EXPECT_TRUE(filter->contains("name", "BRO*", Filter::BLOCK));
  EXPECT_EQ(2u, filter->getTotalFilterCount());
}