/*
  webUIStringField() leans on one ArduinoJson behaviour: is<const char*>() is
  false unless the value really is a string, and as<const char*>() is NULL in
  every other case - missing key, explicit null, number, nested object.

  test_webui_message.cpp models that with a stand-in so the main suite needs no
  third party code. This optional check runs the same assertions against the real
  library, so the stand-in cannot quietly drift from it.

  Build with: make ARDUINOJSON_DIR=<ArduinoJson>/src check-arduinojson
*/
#include <stdio.h>
#include <string.h>

#include "Arduino.h"

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

#include "webUIMessage.h"

static int failures = 0;

static void expectNull(const char* label, const char* actual) {
  if (actual != NULL) {
    printf("  FAIL %s: expected NULL, got \"%s\"\n", label, actual);
    failures++;
  } else {
    printf("  ok   %s\n", label);
  }
}

static void expectText(const char* label, const char* actual, const char* expected) {
  if (actual == NULL || strcmp(actual, expected) != 0) {
    printf("  FAIL %s: expected \"%s\", got \"%s\"\n", label, expected,
           actual != NULL ? actual : "(null)");
    failures++;
  } else {
    printf("  ok   %s\n", label);
  }
}

int main() {
  printf("ArduinoJson contract for webUIStringField()\n");

  StaticJsonDocument<512> doc;
  deserializeJson(doc,
                  "{\"version\":\"v1.8.1\",\"empty\":\"\",\"number\":42,"
                  "\"real\":1.5,\"flag\":true,\"nothing\":null,"
                  "\"nested\":{\"a\":1},\"list\":[1,2]}");
  JsonObject data = doc.as<JsonObject>();

  expectText("string field", webUIStringField(data["version"]), "v1.8.1");
  expectText("empty string field", webUIStringField(data["empty"]), "");

  expectNull("missing key", webUIStringField(data["absent"]));
  expectNull("explicit null", webUIStringField(data["nothing"]));
  expectNull("integer", webUIStringField(data["number"]));
  expectNull("float", webUIStringField(data["real"]));
  expectNull("bool", webUIStringField(data["flag"]));
  expectNull("nested object", webUIStringField(data["nested"]));
  expectNull("array", webUIStringField(data["list"]));

  expectText("or-empty on string", webUIStringFieldOrEmpty(data["version"]), "v1.8.1");
  expectText("or-empty on missing", webUIStringFieldOrEmpty(data["absent"]), "");
  expectText("or-empty on integer", webUIStringFieldOrEmpty(data["number"]), "");

  printf("\n%d failure(s)\n", failures);
  return failures == 0 ? 0 : 1;
}
