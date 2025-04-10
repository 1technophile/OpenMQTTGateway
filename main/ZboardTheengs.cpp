#if SELF_TEST
#  include <esp_system.h>

String getChipModel() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  switch (chip_info.model) {
    case CHIP_ESP32:
      return "ESP32";
    case CHIP_ESP32S2:
      return "ESP32-S2";
    case CHIP_ESP32S3:
      return "ESP32-S3";
    case CHIP_ESP32C3:
      return "ESP32-C3";
    case CHIP_ESP32H2:
      return "ESP32-H2";
    default:
      return "Unknown";
  }
}

// From https://github.com/ThingPulse/espgateway-ethernet-testbed

void addTestMessage(JsonArray& data, String name, String value, String result) {
  JsonObject object = data.createNestedObject();
  object["name"] = name;
  object["value"] = value;
  object["result"] = result;
}

void testDevice() {
  StaticJsonDocument<1280> doc;
  JsonArray data = doc.to<JsonArray>();

  addTestMessage(data, "Mac Address", String(WiFi.macAddress()), "OK");
  addTestMessage(data, "Chip Model", String(ESP.getChipModel()), "OK");
  addTestMessage(data, "Chip Revision", String(ESP.getChipRevision()), "OK");
  addTestMessage(data, "Available Cores", String(ESP.getChipCores()), ESP.getChipCores() == 2 ? "OK" : "NOK");
  addTestMessage(data, "Heap Size", String(ESP.getHeapSize() / 1024) + "kb", ESP.getHeapSize() > 100000 ? "OK" : "NOK");
  addTestMessage(data, "Free Heap", String(ESP.getFreeHeap() / 1024) + "kb", ESP.getFreeHeap() > 100000 ? "OK" : "NOK");
  addTestMessage(data, "ETH MAC Address", String(ETH.macAddress()), ETH.macAddress() ? "OK" : "NOK");
  addTestMessage(data, "ETH Local IP", ETH.localIP().toString(), ETH.localIP() ? "OK" : "NOK");
  addTestMessage(data, "ETH Full Duplex", ETH.fullDuplex() ? "true" : "false", ETH.fullDuplex() ? "OK" : "NOK");
  addTestMessage(data, "ETH Link Speed", String(ETH.linkSpeed()) + "Mbs", ETH.linkSpeed() ? "OK" : "NOK");
  addTestMessage(data, "Build Date", String(__DATE__), "OK");
  addTestMessage(data, "Build Time", String(__TIME__), "OK");
  Serial.println();
  serializeJson(doc, Serial);
  Serial.println();
}

void testLeds() {
  Log.notice(F("LED Test" CR));
  ledManager.setMode(0, 0, LEDManager::Mode::STATIC, LED_NETWORK_OK_COLOR, -1);
  delay(1000);
  ledManager.setMode(0, 1, LEDManager::Mode::STATIC, LED_NETWORK_OK_COLOR, -1);
  delay(1000);
  ledManager.setMode(0, 2, LEDManager::Mode::STATIC, LED_NETWORK_OK_COLOR, -1);
  delay(1000);
  ledManager.setMode(0, 3, LEDManager::Mode::STATIC, LED_NETWORK_OK_COLOR, -1);
  Log.notice(F("LED Test Finished" CR));
}

void checkSerial() {
  Log.notice(F("READY_FOR_SELFTEST" CR));
  unsigned long start = millis();
  while (millis() - start < 1000) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim(); // Remove any extra whitespace
      if (input == "SELFTEST") {
        Log.notice(F("SELFTEST Launched" CR));
        testLeds();
        // Print 20 times the test data
        for (int i = 0; i < 20; i++) {
          testDevice();
          delay(1000);
        }
        Log.notice(F("SELFTEST Finished" CR));
        // Restart
        ESPRestart(9);
      }
    }
  }
}
#endif