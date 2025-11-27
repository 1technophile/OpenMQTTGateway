#include <ArduinoJson.h>
#include <config_JSONMessages.h>
#include <core/Filter.h>
#include <gtest/gtest.h>

#include "../mocks/mock_IStorage.h"

using namespace ::testing;

class FilterTest : public ::testing::Test {
protected:
  void SetUp() override {
    filterUnderTest = new Filter(mockStorage);
  }

  void TearDown() override {
    delete filterUnderTest;
  }

  MockStorage mockStorage;
  Filter* filterUnderTest;
};

TEST_F(FilterTest, TestFilterisEmptyOnCreation) {
  EXPECT_TRUE(filterUnderTest->isEmptyThe(Filter::BLOCK));
  EXPECT_TRUE(filterUnderTest->isEmptyThe(Filter::PASS));
}

TEST_F(FilterTest, AddWhitelistValueMakesListNonEmpty) {
  bool result = filterUnderTest->add("id", "deviceA", Filter::PASS);

  EXPECT_TRUE(result);
  EXPECT_FALSE(filterUnderTest->isEmptyThe(Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "deviceA", Filter::PASS));
  EXPECT_EQ(1u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, AddFilterReturnsTrue) {
  EXPECT_TRUE(filterUnderTest->add("id", "device1", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->add("name", "sensor", Filter::BLOCK));
}

TEST_F(FilterTest, AddFilterReturnsFalseWhenLimitReached) {
  // Add 50 filters (the limit)
  for (size_t i = 0; i < MAX_TOTAL_FILTERS; ++i) {
    std::string value = "device" + std::to_string(i);
    EXPECT_TRUE(filterUnderTest->add("id", value.c_str(), Filter::PASS));
  }

  EXPECT_EQ(MAX_TOTAL_FILTERS, filterUnderTest->getTotalFilterCount());

  // 51st filter should fail
  EXPECT_FALSE(filterUnderTest->add("id", "overflow", Filter::PASS));
  EXPECT_EQ(MAX_TOTAL_FILTERS, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, HasCapacityReturnsFalseWhenFull) {
  EXPECT_TRUE(filterUnderTest->hasCapacity());

  for (size_t i = 0; i < MAX_TOTAL_FILTERS; ++i) {
    std::string value = "device" + std::to_string(i);
    filterUnderTest->add("id", value.c_str(), Filter::PASS);
  }

  EXPECT_FALSE(filterUnderTest->hasCapacity());
}

TEST_F(FilterTest, ClearRemovesAllFilters) {
  filterUnderTest->add("id", "device1", Filter::PASS);
  filterUnderTest->add("name", "sensor", Filter::BLOCK);

  EXPECT_EQ(2u, filterUnderTest->getTotalFilterCount());

  filterUnderTest->clear();

  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
  EXPECT_TRUE(filterUnderTest->isEmptyThe(Filter::PASS));
  EXPECT_TRUE(filterUnderTest->isEmptyThe(Filter::BLOCK));
}

TEST_F(FilterTest, AddBlacklistValueIsDetectable) {
  EXPECT_TRUE(filterUnderTest->add("id", "badDevice", Filter::BLOCK));
  EXPECT_FALSE(filterUnderTest->isEmptyThe(Filter::BLOCK));
  EXPECT_TRUE(filterUnderTest->contains("id", "badDevice", Filter::BLOCK));
}

TEST_F(FilterTest, RemoveValueClearsEntriesFromBothLists) {
  filterUnderTest->add("id", "shared", Filter::PASS);
  filterUnderTest->add("id", "shared", Filter::BLOCK);
  filterUnderTest->add("name", "shared", Filter::BLOCK);

  EXPECT_TRUE(filterUnderTest->remove("id", "shared"));

  EXPECT_TRUE(filterUnderTest->isEmptyThe(Filter::PASS));
  EXPECT_FALSE(filterUnderTest->isEmptyThe(Filter::BLOCK));
  EXPECT_FALSE(filterUnderTest->contains("id", "shared", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "shared", Filter::BLOCK));
  EXPECT_TRUE(filterUnderTest->contains("name", "shared", Filter::BLOCK));
}

TEST_F(FilterTest, RemoveReturnsFalseWhenNotFound) {
  EXPECT_FALSE(filterUnderTest->remove("id", "nonexistent"));
}

TEST_F(FilterTest, ContainsReturnsFalseWhenValueMissing) {
  filterUnderTest->add("id", "existing", Filter::PASS);

  EXPECT_FALSE(filterUnderTest->contains("id", "missing", Filter::PASS));
}

TEST_F(FilterTest, WildcardMatchingWithAsterisk) {
  filterUnderTest->add("id", "sensor_*", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "sensor_123", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "sensor_abc", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "sensor_", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "temp_123", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "sensor", Filter::PASS));
}

TEST_F(FilterTest, WildcardMatchingWithQuestionMark) {
  filterUnderTest->add("name", "dev??e", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("name", "device", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("name", "devABe", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("name", "dev1e", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("name", "device1", Filter::PASS));
}

TEST_F(FilterTest, WildcardMatchingComplex) {
  filterUnderTest->add("id", "*_sensor_*", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "temp_sensor_01", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "humidity_sensor_data", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "sensor_01", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "temp_data", Filter::PASS));
}

TEST_F(FilterTest, WildcardMatchingComplexInTheMiddle) {
  filterUnderTest->add("id", "in_*_the_*_middle", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "in_1_the_2_middle", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "in_One_the_Alex_middle", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "in_1_the_Ale", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "In__The_Ale_2", Filter::PASS));
}

TEST_F(FilterTest, ToSerializesPassAndBlockArrays) {
  filterUnderTest->add("id", "wl", Filter::PASS);
  filterUnderTest->add("name", "bl", Filter::BLOCK);

  StaticJsonDocument<JSON_MSG_BUFFER> outDoc;
  JsonObject serialized = outDoc.to<JsonObject>();
  filterUnderTest->to(serialized);

  ASSERT_TRUE(serialized.containsKey("pass"));
  ASSERT_TRUE(serialized.containsKey("block"));

  // Both pass and block filters should be serialized with their keys
  ASSERT_TRUE(serialized["pass"].containsKey("id"));
  ASSERT_TRUE(serialized["block"].containsKey("name"));
  // Verify the arrays contain expected values
  JsonArray idArray = serialized["pass"]["id"].as<JsonArray>();
  EXPECT_EQ(1u, idArray.size());
  EXPECT_STREQ("wl", idArray[0].as<const char*>());

  JsonArray nameArray = serialized["block"]["name"].as<JsonArray>();
  EXPECT_EQ(1u, nameArray.size());
  EXPECT_STREQ("bl", nameArray[0].as<const char*>());
}

// ============================================================================
// JSON Serialization & Deserialization Tests
// ============================================================================

TEST_F(FilterTest, ToSerializesMultipleValuesPerKey) {
  filterUnderTest->add("id", "device1", Filter::PASS);
  filterUnderTest->add("id", "device2", Filter::PASS);
  filterUnderTest->add("id", "device3", Filter::PASS);

  StaticJsonDocument<JSON_MSG_BUFFER> outDoc;
  JsonObject serialized = outDoc.to<JsonObject>();
  filterUnderTest->to(serialized);

  JsonObject passObj = serialized["pass"].as<JsonObject>();
  JsonObject blockObj = serialized["block"].as<JsonObject>();

  ASSERT_TRUE(passObj.containsKey("id"));
  JsonArray idArray = passObj["id"].as<JsonArray>();
  EXPECT_EQ(3u, idArray.size());
}

TEST_F(FilterTest, ToSerializesEmptyFilterAsEmptyObject) {
  StaticJsonDocument<JSON_MSG_BUFFER> outDoc;
  JsonObject serialized = outDoc.to<JsonObject>();
  filterUnderTest->to(serialized);
  JsonObject passObj = serialized["pass"].as<JsonObject>();
  JsonObject blockObj = serialized["block"].as<JsonObject>();

  EXPECT_EQ(0u, passObj.size());
  EXPECT_EQ(0u, blockObj.size());
}

TEST_F(FilterTest, FromLoadsPassFiltersFromJson) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray idArray = data.createNestedArray("id");
  idArray.add("sensor1");
  idArray.add("sensor2");

  filterUnderTest->from(data);

  // from() loads into both PASS and BLOCK lists
  EXPECT_TRUE(filterUnderTest->contains("id", "sensor1", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "sensor2", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "sensor1", Filter::BLOCK));
  EXPECT_TRUE(filterUnderTest->contains("id", "sensor2", Filter::BLOCK));
  EXPECT_EQ(4u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromLoadsMixedPassAndBlockFilters) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();

  JsonArray idArray = data.createNestedArray("id");
  idArray.add("allowed1");
  idArray.add("allowed2");

  JsonArray nameArray = data.createNestedArray("name");
  nameArray.add("blocked1");

  filterUnderTest->from(data);

  // from() loads into both lists: 3 values * 2 lists = 6 total
  EXPECT_EQ(6u, filterUnderTest->getTotalFilterCount());
  EXPECT_TRUE(filterUnderTest->contains("id", "allowed1", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "allowed1", Filter::BLOCK));
  EXPECT_TRUE(filterUnderTest->contains("name", "blocked1", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("name", "blocked1", Filter::BLOCK));
}

TEST_F(FilterTest, FromIgnoresInvalidNonArrayValues) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  data["id"] = "notAnArray"; // Should be ignored

  filterUnderTest->from(data);

  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromIgnoresInvalidNonStringValuesInArray) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray idArray = data.createNestedArray("id");
  idArray.add(123); // Integer, should be ignored
  idArray.add("valid"); // This should work

  filterUnderTest->from(data);

  // "valid" is loaded into both PASS and BLOCK
  EXPECT_EQ(2u, filterUnderTest->getTotalFilterCount());
  EXPECT_TRUE(filterUnderTest->contains("id", "valid", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "valid", Filter::BLOCK));
}

TEST_F(FilterTest, FromStopsLoadingWhenLimitReached) {
  // Pre-fill to near capacity
  for (size_t i = 0; i < MAX_TOTAL_FILTERS - 2; ++i) {
    std::string value = "device" + std::to_string(i);
    filterUnderTest->add("id", value.c_str(), Filter::PASS);
  }

  // Try to load 5 more filters via JSON
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray nameArray = data.createNestedArray("name");
  for (int i = 0; i < 5; ++i) {
    nameArray.add(("filter" + std::to_string(i)).c_str());
  }

  filterUnderTest->from(data);

  // Should have only added 2 filters (to reach limit)
  EXPECT_EQ(MAX_TOTAL_FILTERS, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, ToStringSerializesCorrectly) {
  filterUnderTest->add("id", "device1", Filter::PASS);
  filterUnderTest->add("name", "blocked", Filter::BLOCK);

  char buffer[JSON_MSG_BUFFER];
  filterUnderTest->to(buffer);

  // Parse the serialized string back
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  deserializeJson(doc, buffer);
  JsonObject obj = doc.as<JsonObject>();

  JsonObject passObj = obj["pass"].as<JsonObject>();
  JsonObject blockObj = obj["block"].as<JsonObject>();
  EXPECT_TRUE(passObj.containsKey("id"));
  EXPECT_TRUE(blockObj.containsKey("name"));
}

TEST_F(FilterTest, SerializeAndDeserializeWithWildcards) {
  // Create a new filter and add wildcard patterns via from()
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray idArray = data.createNestedArray("id");
  idArray.add("sensor*");
  idArray.add("device?");

  filterUnderTest->from(data);

  // Verify wildcards work after deserialization
  EXPECT_TRUE(filterUnderTest->contains("id", "sensor123", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "sensorABC", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "deviceA", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "device1", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "device12", Filter::PASS));
}

// ============================================================================
// fromRulesList and toRulesList Tests
// ============================================================================

TEST_F(FilterTest, FromRulesListLoadsPassRuleCorrectly) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");

  JsonObject rule = rulesArray.createNestedObject();
  rule["target"] = "value";
  rule["action"] = "pass";
  rule["key"] = "id";
  rule["value"] = "sensor1";

  filterUnderTest->fromRulesList(data);

  EXPECT_TRUE(filterUnderTest->contains("id", "sensor1", Filter::PASS));
  EXPECT_EQ(1u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromRulesListLoadsBlockRuleCorrectly) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");

  JsonObject rule = rulesArray.createNestedObject();
  rule["target"] = "value";
  rule["action"] = "block";
  rule["key"] = "id";
  rule["value"] = "badDevice";

  filterUnderTest->fromRulesList(data);

  EXPECT_TRUE(filterUnderTest->contains("id", "badDevice", Filter::BLOCK));
  EXPECT_EQ(1u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromRulesListLoadsMultipleRulesCorrectly) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");

  // Add pass rule
  JsonObject passRule = rulesArray.createNestedObject();
  passRule["target"] = "value";
  passRule["action"] = "pass";
  passRule["key"] = "id";
  passRule["value"] = "allowed";

  // Add block rule
  JsonObject blockRule = rulesArray.createNestedObject();
  blockRule["target"] = "value";
  blockRule["action"] = "block";
  blockRule["key"] = "name";
  blockRule["value"] = "blocked";

  filterUnderTest->fromRulesList(data);

  EXPECT_TRUE(filterUnderTest->contains("id", "allowed", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("name", "blocked", Filter::BLOCK));
  EXPECT_EQ(2u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromRulesListLoadsTopicFilterCorrectly) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");

  JsonObject rule = rulesArray.createNestedObject();
  rule["target"] = "topic";
  rule["action"] = "pass";
  rule["value"] = "home/sensors/+";

  filterUnderTest->fromRulesList(data);

  // Topic filters are added with a special key
  EXPECT_EQ(1u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromRulesListIgnoresInvalidAction) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");

  JsonObject rule = rulesArray.createNestedObject();
  rule["target"] = "value";
  rule["action"] = "invalid";
  rule["key"] = "id";
  rule["value"] = "test";

  filterUnderTest->fromRulesList(data);

  // Rule should be ignored
  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromRulesListIgnoresMissingKey) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");

  JsonObject rule = rulesArray.createNestedObject();
  rule["target"] = "value";
  rule["action"] = "pass";
  rule["value"] = "test";
  // Missing "key" field

  filterUnderTest->fromRulesList(data);

  // Rule should be ignored
  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromRulesListIgnoresMissingValue) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");

  JsonObject rule = rulesArray.createNestedObject();
  rule["target"] = "value";
  rule["action"] = "pass";
  rule["key"] = "id";
  // Missing "value" field

  filterUnderTest->fromRulesList(data);

  // Rule should be ignored
  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, FromRulesListIgnoresNonObjectElements) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");

  // Add invalid non-object element
  rulesArray.add("invalid");

  // Add valid rule after invalid
  JsonObject rule = rulesArray.createNestedObject();
  rule["target"] = "value";
  rule["action"] = "pass";
  rule["key"] = "id";
  rule["value"] = "valid";

  filterUnderTest->fromRulesList(data);

  // Only valid rule should be added
  EXPECT_EQ(1u, filterUnderTest->getTotalFilterCount());
  EXPECT_TRUE(filterUnderTest->contains("id", "valid", Filter::PASS));
}

TEST_F(FilterTest, FromRulesListStopsWhenLimitReached) {
  // Pre-fill to near capacity
  for (size_t i = 0; i < MAX_TOTAL_FILTERS - 2; ++i) {
    std::string value = "device" + std::to_string(i);
    filterUnderTest->add("id", value.c_str(), Filter::PASS);
  }

  // Try to load 5 more rules via JSON
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  JsonArray rulesArray = data.createNestedArray("rules");
  for (int i = 0; i < 5; ++i) {
    JsonObject rule = rulesArray.createNestedObject();
    rule["target"] = "value";
    rule["action"] = "pass";
    rule["key"] = "name";
    rule["value"] = ("filter" + std::to_string(i)).c_str();
  }

  filterUnderTest->fromRulesList(data);

  // Should have only added 2 filters (to reach limit)
  EXPECT_EQ(MAX_TOTAL_FILTERS, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, ToRulesListSerializesPassFiltersCorrectly) {
  filterUnderTest->add("id", "sensor1", Filter::PASS);
  filterUnderTest->add("id", "sensor2", Filter::PASS);

  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  filterUnderTest->toRulesList(data);

  EXPECT_TRUE(data.containsKey("rules"));
  JsonArray rulesArray = data["rules"].as<JsonArray>();
  EXPECT_EQ(2u, rulesArray.size());

  // Check first rule
  JsonObject rule1 = rulesArray[0].as<JsonObject>();
  EXPECT_STREQ("pass", rule1["action"].as<const char*>());
  EXPECT_STREQ("id", rule1["key"].as<const char*>());
  EXPECT_STREQ("sensor1", rule1["value"].as<const char*>());
}

TEST_F(FilterTest, ToRulesListSerializesBlockFiltersCorrectly) {
  filterUnderTest->add("name", "blocked1", Filter::BLOCK);
  filterUnderTest->add("name", "blocked2", Filter::BLOCK);

  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  filterUnderTest->toRulesList(data);

  EXPECT_TRUE(data.containsKey("rules"));
  JsonArray rulesArray = data["rules"].as<JsonArray>();
  EXPECT_EQ(2u, rulesArray.size());

  // Check first rule
  JsonObject rule1 = rulesArray[0].as<JsonObject>();
  EXPECT_STREQ("block", rule1["action"].as<const char*>());
  EXPECT_STREQ("name", rule1["key"].as<const char*>());
  EXPECT_STREQ("blocked1", rule1["value"].as<const char*>());
}

TEST_F(FilterTest, ToRulesListSerializesMixedFiltersCorrectly) {
  filterUnderTest->add("id", "allowed", Filter::PASS);
  filterUnderTest->add("name", "blocked", Filter::BLOCK);

  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  filterUnderTest->toRulesList(data);

  EXPECT_TRUE(data.containsKey("rules"));
  JsonArray rulesArray = data["rules"].as<JsonArray>();
  EXPECT_EQ(2u, rulesArray.size());

  // Count pass and block rules
  int passCount = 0, blockCount = 0;
  for (JsonVariant ruleVar : rulesArray) {
    JsonObject rule = ruleVar.as<JsonObject>();
    const char* action = rule["action"];
    if (strcmp(action, "pass") == 0) {
      passCount++;
    } else if (strcmp(action, "block") == 0) {
      blockCount++;
    }
  }

  EXPECT_EQ(1, passCount);
  EXPECT_EQ(1, blockCount);
}

TEST_F(FilterTest, ToRulesListEmptyFiltersList) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  filterUnderTest->toRulesList(data);

  EXPECT_TRUE(data.containsKey("rules"));
  JsonArray rulesArray = data["rules"].as<JsonArray>();
  EXPECT_EQ(0u, rulesArray.size());
}

TEST_F(FilterTest, RoundTripFromAndToRulesList) {
  // Create original filters
  filterUnderTest->add("id", "sensor1", Filter::PASS);
  filterUnderTest->add("id", "sensor2", Filter::PASS);
  filterUnderTest->add("name", "blocked", Filter::BLOCK);

  // Serialize to rules format
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  filterUnderTest->toRulesList(data);

  // Create new filter and deserialize
  Filter* newFilter = new Filter(mockStorage);
  newFilter->fromRulesList(data);

  // Verify new filter has same content
  EXPECT_TRUE(newFilter->contains("id", "sensor1", Filter::PASS));
  EXPECT_TRUE(newFilter->contains("id", "sensor2", Filter::PASS));
  EXPECT_TRUE(newFilter->contains("name", "blocked", Filter::BLOCK));
  EXPECT_EQ(filterUnderTest->getTotalFilterCount(), newFilter->getTotalFilterCount());

  delete newFilter;
}

TEST_F(FilterTest, ToRulesListWithWildcards) {
  filterUnderTest->add("id", "sensor*", Filter::PASS);
  filterUnderTest->add("name", "device?", Filter::BLOCK);

  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  JsonObject data = doc.to<JsonObject>();
  filterUnderTest->toRulesList(data);

  EXPECT_TRUE(data.containsKey("rules"));
  JsonArray rulesArray = data["rules"].as<JsonArray>();
  EXPECT_EQ(2u, rulesArray.size());

  // Verify wildcards are preserved
  bool foundPass = false, foundBlock = false;
  for (JsonVariant ruleVar : rulesArray) {
    JsonObject rule = ruleVar.as<JsonObject>();
    const char* value = rule["value"];
    if (strcmp(value, "sensor*") == 0) {
      foundPass = true;
    } else if (strcmp(value, "device?") == 0) {
      foundBlock = true;
    }
  }

  EXPECT_TRUE(foundPass);
  EXPECT_TRUE(foundBlock);
}

// ============================================================================
// getFilterCount Tests
// ============================================================================

TEST_F(FilterTest, GetFilterCountReturnsZeroForEmptyLists) {
  EXPECT_EQ(0u, filterUnderTest->getFilterCount(Filter::PASS));
  EXPECT_EQ(0u, filterUnderTest->getFilterCount(Filter::BLOCK));
}

TEST_F(FilterTest, GetFilterCountReturnsCorrectPassCount) {
  filterUnderTest->add("id", "device1", Filter::PASS);
  filterUnderTest->add("id", "device2", Filter::PASS);
  filterUnderTest->add("name", "sensor", Filter::PASS);

  EXPECT_EQ(3u, filterUnderTest->getFilterCount(Filter::PASS));
  EXPECT_EQ(0u, filterUnderTest->getFilterCount(Filter::BLOCK));
}

TEST_F(FilterTest, GetFilterCountReturnsCorrectBlockCount) {
  filterUnderTest->add("id", "blocked1", Filter::BLOCK);
  filterUnderTest->add("id", "blocked2", Filter::BLOCK);

  EXPECT_EQ(0u, filterUnderTest->getFilterCount(Filter::PASS));
  EXPECT_EQ(2u, filterUnderTest->getFilterCount(Filter::BLOCK));
}

TEST_F(FilterTest, GetFilterCountWorksIndependentlyForBothLists) {
  filterUnderTest->add("id", "allowed1", Filter::PASS);
  filterUnderTest->add("id", "allowed2", Filter::PASS);
  filterUnderTest->add("name", "blocked1", Filter::BLOCK);

  EXPECT_EQ(2u, filterUnderTest->getFilterCount(Filter::PASS));
  EXPECT_EQ(1u, filterUnderTest->getFilterCount(Filter::BLOCK));
  EXPECT_EQ(3u, filterUnderTest->getTotalFilterCount());
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

TEST_F(FilterTest, AddWithNullKeyReturnsFalse) {
  EXPECT_FALSE(filterUnderTest->add(nullptr, "value", Filter::PASS));
  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, AddWithNullValueReturnsFalse) {
  EXPECT_FALSE(filterUnderTest->add("key", nullptr, Filter::PASS));
  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, RemoveWithNullKeyReturnsFalse) {
  EXPECT_FALSE(filterUnderTest->remove(nullptr, "value"));
}

TEST_F(FilterTest, RemoveWithNullValueReturnsFalse) {
  EXPECT_FALSE(filterUnderTest->remove("key", nullptr));
}

TEST_F(FilterTest, ContainsWithNullKeyReturnsFalse) {
  filterUnderTest->add("id", "device", Filter::PASS);
  EXPECT_FALSE(filterUnderTest->contains(nullptr, "device", Filter::PASS));
}

TEST_F(FilterTest, ContainsWithNullValueReturnsFalse) {
  filterUnderTest->add("id", "device", Filter::PASS);
  EXPECT_FALSE(filterUnderTest->contains("id", nullptr, Filter::PASS));
}

TEST_F(FilterTest, AddEmptyStringValueWorks) {
  EXPECT_TRUE(filterUnderTest->add("id", "", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "", Filter::PASS));
}

TEST_F(FilterTest, AddDuplicateValueCreatesMultipleEntries) {
  EXPECT_TRUE(filterUnderTest->add("id", "device1", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->add("id", "device1", Filter::PASS));

  EXPECT_EQ(2u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, RemoveOnlyRemovesMatchingValue) {
  filterUnderTest->add("id", "device1", Filter::PASS);
  filterUnderTest->add("id", "device2", Filter::PASS);
  filterUnderTest->add("name", "device1", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->remove("id", "device1"));

  EXPECT_FALSE(filterUnderTest->contains("id", "device1", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "device2", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("name", "device1", Filter::PASS));
}

TEST_F(FilterTest, RemoveRemovesKeyWhenLastValueRemoved) {
  filterUnderTest->add("id", "device1", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->remove("id", "device1"));

  // Key should be completely removed from internal map
  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, ContainsChecksCorrectList) {
  filterUnderTest->add("id", "device1", Filter::PASS);
  filterUnderTest->add("name", "blocked", Filter::BLOCK);

  // Check pass list
  EXPECT_TRUE(filterUnderTest->contains("id", "device1", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "device1", Filter::BLOCK));

  // Check block list
  EXPECT_TRUE(filterUnderTest->contains("name", "blocked", Filter::BLOCK));
  EXPECT_FALSE(filterUnderTest->contains("name", "blocked", Filter::PASS));
}

TEST_F(FilterTest, ClearAffectsBothLists) {
  filterUnderTest->add("id", "device1", Filter::PASS);
  filterUnderTest->add("id", "device2", Filter::PASS);
  filterUnderTest->add("name", "blocked1", Filter::BLOCK);
  filterUnderTest->add("name", "blocked2", Filter::BLOCK);

  EXPECT_EQ(4u, filterUnderTest->getTotalFilterCount());

  filterUnderTest->clear();

  EXPECT_EQ(0u, filterUnderTest->getTotalFilterCount());
  EXPECT_EQ(0u, filterUnderTest->getFilterCount(Filter::PASS));
  EXPECT_EQ(0u, filterUnderTest->getFilterCount(Filter::BLOCK));
}

TEST_F(FilterTest, HasCapacityReturnsTrueWhenNotFull) {
  EXPECT_TRUE(filterUnderTest->hasCapacity());

  filterUnderTest->add("id", "device1", Filter::PASS);
  EXPECT_TRUE(filterUnderTest->hasCapacity());
}

// ============================================================================
// Wildcard Edge Cases
// ============================================================================

TEST_F(FilterTest, WildcardOnlyAsteriskMatchesEverything) {
  filterUnderTest->add("id", "*", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "anything", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "123", Filter::PASS));
}

TEST_F(FilterTest, WildcardMultipleAsterisks) {
  filterUnderTest->add("id", "*_*_*", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "a_b_c", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "one_two_three", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "a_b", Filter::PASS));
}

TEST_F(FilterTest, WildcardQuestionMarkExactLength) {
  filterUnderTest->add("id", "???", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "abc", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "123", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "ab", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "abcd", Filter::PASS));
}

TEST_F(FilterTest, WildcardMixedAsteriskAndQuestionMark) {
  filterUnderTest->add("id", "dev?ce_*", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "device_123", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "devXce_abc", Filter::PASS));
  // "dev_ce_123" matches because ? can match underscore
  EXPECT_TRUE(filterUnderTest->contains("id", "dev_ce_123", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "device", Filter::PASS));
}

TEST_F(FilterTest, WildcardCaseSensitiveMatching) {
  filterUnderTest->add("id", "Sensor*", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "Sensor123", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "sensor123", Filter::PASS));
}

TEST_F(FilterTest, WildcardEmptyPatternMatchesEmpty) {
  filterUnderTest->add("id", "", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "", Filter::PASS));
  EXPECT_FALSE(filterUnderTest->contains("id", "anything", Filter::PASS));
}

// ============================================================================
// Multiple Keys Tests
// ============================================================================

TEST_F(FilterTest, MultipleKeysInSameList) {
  filterUnderTest->add("id", "device1", Filter::PASS);
  filterUnderTest->add("type", "sensor", Filter::PASS);
  filterUnderTest->add("location", "room1", Filter::PASS);

  EXPECT_TRUE(filterUnderTest->contains("id", "device1", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("type", "sensor", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("location", "room1", Filter::PASS));
  EXPECT_EQ(3u, filterUnderTest->getTotalFilterCount());
}

TEST_F(FilterTest, SameKeyInBothLists) {
  filterUnderTest->add("id", "allowed", Filter::PASS);
  filterUnderTest->add("id", "blocked", Filter::BLOCK);

  EXPECT_TRUE(filterUnderTest->contains("id", "allowed", Filter::PASS));
  EXPECT_TRUE(filterUnderTest->contains("id", "blocked", Filter::BLOCK));
  EXPECT_FALSE(filterUnderTest->contains("id", "allowed", Filter::BLOCK));
  EXPECT_FALSE(filterUnderTest->contains("id", "blocked", Filter::PASS));
}
