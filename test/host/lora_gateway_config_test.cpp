#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#include <ArduinoJson.h>
#if defined(LORA_RADIO_SX1262)
#  include <RadioLib.h>
#else
#  include <LoRa.h>
#endif
#include "lora/LoRaRadio.h"
#define MQTT_SERVER  "localhost"
#define MQTT_PORT    "1883"
#define MQTT_USER    ""
#define Gateway_Name "test"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

#include "config_WebContent.h"

using String = std::string;
#define JSON_MSG_BUFFER    2048
#define Gateway_Short_Name "test"
#define CR                 ""
#define F(value)           value
template <typename... Args>
void testLog(Args&&...) {}
#define THEENGS_LOG_NOTICE(...)  testLog(__VA_ARGS__)
#define THEENGS_LOG_WARNING(...) testLog(__VA_ARGS__)
#define THEENGS_LOG_ERROR(...)   testLog(__VA_ARGS__)

struct TestPreferences {
  std::map<std::string, std::string> records;
  void begin(const char*, bool) {}
  void end() {}
  bool isKey(const char* key) { return records.count(key) != 0; }
  String getString(const char* key, const char* fallback) { return isKey(key) ? records[key] : fallback; }
  size_t putString(const char* key, const String& value) {
    records[key] = value;
    return value.size();
  }
  int remove(const char* key) { return static_cast<int>(records.erase(key)); }
} preferences;

String publishedState;
bool enqueueJsonObject(JsonObject& data) {
  publishedState.clear();
  serializeJson(data, publishedState);
  return true;
}

LORAConfig_s LORAConfig;
bool LORAConfig_fromJson(JsonObject&, LORAConfig_s&);
// Generated from the production functions, not a copy of their implementation.
#include "lora_gateway_config_under_test.h"

struct TestServer {
  std::map<std::string, std::string> arguments;
  std::string page;
  String uri() { return "/la"; }
  size_t args() { return arguments.size(); }
  int method() { return 1; }
  bool hasArg(const char* key) { return arguments.count(key) != 0; }
  String arg(const char* key) { return arguments.at(key); }
  String arg(size_t index) {
    auto it = arguments.begin();
    std::advance(it, index);
    return it->second;
  }
  String argName(size_t index) {
    auto it = arguments.begin();
    std::advance(it, index);
    return it->first;
  }
  void sendContent(const char* text) { page += text; }
} server;
StaticJsonDocument<64> modules;
const char* gateway_name = "test";
#define WEBUI_TRACE_LOG(...)   testLog(__VA_ARGS__)
#define THEENGS_LOG_TRACE(...) testLog(__VA_ARGS__)
#define WEBUI_SECURE
void beginChunkedResponse() { server.page.clear(); }
void sendHeaderChunk(const char*) {}
void sendFooterChunk() {}
void sendBodyChunk(const char* format, ...) {
  char chunk[3000];
  va_list args;
  va_start(args, format);
  const int length = vsnprintf(chunk, sizeof(chunk), format, args);
  va_end(args);
  assert(length >= 0 && static_cast<size_t>(length) < sizeof(chunk));
  server.sendContent(chunk);
}
#include "lora_web_handler_under_test.h"

bool command(const char* json) {
  StaticJsonDocument<JSON_MSG_BUFFER> doc;
  assert(!deserializeJson(doc, json));
  JsonObject data = doc.as<JsonObject>();
  assert(hasLORAConfig(data));
  return LORAConfig_update(data);
}

int main() {
#if defined(LORA_RADIO_SX1262)
  fake_radiolib::reset();
  LORAConfig_init();
  assert(!LORAConfig.rxBoostedGain);
  assert(OMGLoRaRadio.begin(LORAConfig));
  assert(command("{\"rxboostedgain\":true,\"save\":true}"));
  assert(LORAConfig.rxBoostedGain && fake_radiolib::boostedGain);
  StaticJsonDocument<JSON_MSG_BUFFER> stored;
  assert(!deserializeJson(stored, preferences.records.at("LORAConfig")));
  assert(stored["rxboostedgain"].is<bool>() && stored["rxboostedgain"].as<bool>());
  const auto saved = preferences.records.at("LORAConfig");
  assert(command("{\"txpower\":28}"));
  assert(LORAConfig.rxBoostedGain); // Partial updates preserve an existing gain setting.
  assert(!command("{\"rxboostedgain\":\"false\",\"save\":true}"));
  assert(!command("{\"frequency\":433000000,\"save\":true}"));
  assert(preferences.records.at("LORAConfig") == saved);
  assert(LORAConfig.rxBoostedGain && LORAConfig.frequency == 868000000);

  fake_radiolib::bandwidthFailureOnce = -66;
  assert(!command("{\"signalbandwidth\":250000,\"rxboostedgain\":false,\"save\":true}"));
  assert(LORAConfig.signalBandwidth == 125000 && LORAConfig.rxBoostedGain);
  assert(fake_radiolib::bandwidth == 125.0f && fake_radiolib::boostedGain);
  assert(preferences.records.at("LORAConfig") == saved);

  assert(command("{\"rxboostedgain\":false}"));
  assert(!LORAConfig.rxBoostedGain && !fake_radiolib::boostedGain);
  assert(command("{\"load\":true}"));
  assert(LORAConfig.rxBoostedGain && fake_radiolib::boostedGain);
  preferences.records["LORAConfig"] = "{\"frequency\":915000000}"; // Pre-feature NVS.
  assert(command("{\"load\":true}"));
  assert(!LORAConfig.rxBoostedGain && !fake_radiolib::boostedGain);
  assert(LORAConfig.frequency == 915000000);
  assert(command("{\"rxboostedgain\":true}"));
  assert(command("{\"init\":true}"));
  assert(!LORAConfig.rxBoostedGain && LORAConfig.frequency == 868000000);
  assert(command("{\"save\":true}"));
  assert(command("{\"erase\":true,\"save\":true}"));
  assert(!preferences.isKey("LORAConfig"));
  assert(command("{\"rxboostedgain\":true}"));
  const String state = stateLORAMeasures();
  assert(state == publishedState);
  assert(!deserializeJson(stored, state));
  assert(stored["rxboostedgain"].as<bool>());
  assert(stored["origin"] == "/LORAtoMQTT");
#else
  LoRa.reset();
  LORAConfig_init();
  assert(OMGLoRaRadio.begin(LORAConfig));
  assert(command("{\"frequency\":433000000,\"txpower\":14,\"save\":true}"));
  assert(LORAConfig.frequency == 433000000 && LoRa.frequency == 433000000);
  StaticJsonDocument<JSON_MSG_BUFFER> stored;
  assert(!deserializeJson(stored, preferences.records.at("LORAConfig")));
  assert(!stored.containsKey("rxboostedgain"));
  assert(!deserializeJson(stored, stateLORAMeasures()));
  assert(!stored.containsKey("rxboostedgain"));
#endif

  // Render the actual firmware handler and exercise checked/unchecked saves.
  handleLA();
  assert(server.page.find("name='save'") != std::string::npos);
  assert(server.page.find("</form></fieldset>") != std::string::npos);
#if defined(LORA_RADIO_SX1262)
  assert(server.page.find("433MHz") == std::string::npos);
  assert(server.page.find("value='5'>SF5") != std::string::npos);
  for (int power = 4; power <= 28; ++power) {
    assert(server.page.find(std::to_string(power) + " dBm</option>") != std::string::npos);
  }
  assert(server.page.find("value='0'>0 dBm") == std::string::npos);
  assert(server.page.find("name='bg' checked") != std::string::npos);
  server.arguments = {{"save", ""}, {"bg", "on"}, {"lt", "28"}, {"ls", "5"}};
  handleLA();
  assert(LORAConfig.rxBoostedGain && LORAConfig.txPower == 28 && LORAConfig.spreadingFactor == 5);
  assert(server.page.find("selected value='28'>28 dBm") != std::string::npos);
  assert(server.page.find("selected value='5'>SF5") != std::string::npos);
  server.arguments = {{"save", ""}};
  handleLA();
  assert(!LORAConfig.rxBoostedGain && !fake_radiolib::boostedGain);
  assert(!deserializeJson(stored, preferences.records.at("LORAConfig")));
  assert(stored["rxboostedgain"].is<bool>() && !stored["rxboostedgain"].as<bool>());
#else
  assert(server.page.find("433MHz") != std::string::npos);
  assert(server.page.find("value='5'>SF5") == std::string::npos);
  assert(server.page.find("name='bg'") == std::string::npos);
  for (int power = 0; power <= 14; ++power) {
    assert(server.page.find(std::to_string(power) + " dBm</option>") != std::string::npos);
  }
  assert(server.page.find("value='15'>15 dBm") == std::string::npos);
  server.arguments = {{"save", ""}, {"lt", "0"}};
  handleLA();
  assert(LORAConfig.txPower == 0 && LoRa.txPower == 0);
#endif
  std::cout << "LoRa production JSON/NVS/state tests passed\n";
  std::cout << "LoRa production WebUI handler/chunk tests passed (" << OMGLoRaRadio.family() << ")\n";
}
