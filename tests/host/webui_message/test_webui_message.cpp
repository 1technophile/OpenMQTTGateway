/*
  Host tests for main/webUIMessage.h - the helpers webUIPubPrint() and
  handleRoot() use to turn an untrusted module message into display text.

  These run on the build host with g++, no board involved. See README.md.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "Arduino.h"

/*------------------- Tiny assertion harness ----------------------*/

static int g_checks = 0;
static int g_failures = 0;
static const char* g_currentTest = "";

static void checkBool(bool condition, const char* expression, int line) {
  g_checks++;
  if (!condition) {
    g_failures++;
    printf("  FAIL %s:%d  %s\n", g_currentTest, line, expression);
  }
}

static void checkString(const char* actual, const char* expected, const char* expression, int line) {
  g_checks++;
  if (actual == NULL || expected == NULL || strcmp(actual, expected) != 0) {
    g_failures++;
    printf("  FAIL %s:%d  %s\n       expected \"%s\"\n       actual   \"%s\"\n",
           g_currentTest, line, expression, expected != NULL ? expected : "(null)",
           actual != NULL ? actual : "(null)");
  }
}

#define CHECK(cond)      checkBool((cond), #cond, __LINE__)
#define CHECK_STR(a, b)  checkString((a), (b), #a " == " #b, __LINE__)
#define RUN(fn)                 \
  do {                          \
    g_currentTest = #fn;        \
    int before = g_failures;    \
    fn();                       \
    if (g_failures == before) { \
      printf("  ok   %s\n", #fn); \
    }                           \
  } while (0)

/*------------------- Seams ----------------------*/

/*
webUIHandOffMessage() releases a message the queue refused. Route free() through
a counter so the tests can prove it happens exactly once, and never on the path
where the queue took ownership.
*/
static void* g_freed[8];
static int g_freeCount = 0;

static void testFree(void* pointer) {
  if (g_freeCount < (int)(sizeof(g_freed) / sizeof(g_freed[0]))) {
    g_freed[g_freeCount] = pointer;
  }
  g_freeCount++;
  free(pointer);
}

// The real webUIQueueMessage lives in config_WebUI.h, which pulls in the Arduino
// core. Only its size matters here, the helpers treat it as an opaque block.
struct webUIQueueMessage {
  char title[128];
  char line1[128];
  char line2[128];
  char line3[128];
  char line4[128];
};

#define free testFree
#include "webUIMessage.h"
#undef free

/*
Stand-in for an ArduinoJson variant, mirroring the three cases that matter:
a string value, a value of some other type, and a key that is not there. Both a
missing key and a wrong type make ArduinoJson's as<const char*>() return NULL,
which is exactly what webUIStringField() has to absorb.
*/
class TestVariant {
public:
  static TestVariant string(const char* text) {
    return TestVariant(true, text);
  }
  static TestVariant number() {
    return TestVariant(false, NULL);
  }
  static TestVariant object() {
    return TestVariant(false, NULL);
  }
  static TestVariant nullValue() {
    return TestVariant(false, NULL);
  }
  static TestVariant missing() {
    return TestVariant(false, NULL);
  }

  template <typename T>
  bool is() const {
    return _isString;
  }
  template <typename T>
  T as() const {
    return (T)_text;
  }

private:
  TestVariant(bool isString, const char* text) : _isString(isString), _text(text) {}
  bool _isString;
  const char* _text;
};

/*------------------- Property slots ----------------------*/

// A message with fewer fields than slots must land exactly where it used to.
static void properties_normal_message_layout() {
  WebUIProperties properties;
  properties.next();
  properties.set("temp: 20.1°C ");
  properties.next();
  properties.set("hum: 41.3% ");
  properties.next();
  properties.set("batt: 41% ");

  CHECK_STR(properties.line(0).c_str(), "temp: 20.1°C hum: 41.3% ");
  CHECK_STR(properties.line(1).c_str(), "batt: 41% ");
  CHECK_STR(properties.line(2).c_str(), "");
  CHECK(properties.slot() == 2);
  CHECK(!properties.empty());
}

// Exactly six fields is the boundary a real b-parasite (PLANT) message sits on.
static void properties_exactly_six_fields_all_shown() {
  WebUIProperties properties;
  const char* values[6] = {"a", "b", "c", "d", "e", "f"};
  for (int i = 0; i < 6; i++) {
    properties.next();
    properties.set(values[i]);
  }
  CHECK(properties.slot() == 5);
  CHECK_STR(properties.line(0).c_str(), "ab");
  CHECK_STR(properties.line(1).c_str(), "cd");
  CHECK_STR(properties.line(2).c_str(), "ef");
}

// The regression: more fields than slots used to write past properties[6].
static void properties_more_than_six_fields_are_dropped() {
  WebUIProperties properties;
  const char* values[12] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l"};
  for (int i = 0; i < 12; i++) {
    properties.next();
    properties.set(values[i]);
  }
  // The first six survive, the surplus is dropped rather than written anywhere.
  CHECK_STR(properties.line(0).c_str(), "ab");
  CHECK_STR(properties.line(1).c_str(), "cd");
  CHECK_STR(properties.line(2).c_str(), "ef");
  // The slot index saturates instead of running away.
  CHECK(properties.slot() == WEBUI_MAX_PROPERTIES);
}

// gravity advances twice to reach the next line; in range that is unchanged.
static void properties_gravity_skips_a_slot() {
  WebUIProperties properties;
  properties.next();
  properties.set("temp: 20.1°C ");
  properties.next();
  properties.next();
  properties.set("SG: 1.013 ");

  CHECK(properties.slot() == 2);
  CHECK_STR(properties.line(0).c_str(), "temp: 20.1°C ");
  CHECK_STR(properties.line(1).c_str(), "SG: 1.013 ");
}

// gravity alone lands in the second slot, as it did before.
static void properties_gravity_alone_lands_in_slot_one() {
  WebUIProperties properties;
  properties.next();
  properties.next();
  properties.set("SG: 1.013 ");
  CHECK(properties.slot() == 1);
  CHECK_STR(properties.line(0).c_str(), "SG: 1.013 ");
}

// gravity's second advance is what used to push the index past the array.
static void properties_gravity_at_the_boundary_is_dropped() {
  WebUIProperties properties;
  const char* values[5] = {"a", "b", "c", "d", "e"};
  for (int i = 0; i < 5; i++) {
    properties.next();
    properties.set(values[i]);
  }
  properties.next(); // -> slot 5
  properties.next(); // -> saturated at 6
  properties.set("SG: 1.013 ");

  CHECK(properties.slot() == WEBUI_MAX_PROPERTIES);
  CHECK_STR(properties.line(0).c_str(), "ab");
  CHECK_STR(properties.line(1).c_str(), "cd");
  CHECK_STR(properties.line(2).c_str(), "e"); // slot 5 stayed empty
}

// The BBQ branch addresses slots directly, one per probe.
static void properties_explicit_slots_are_bounded() {
  WebUIProperties properties;
  for (int i = 0; i < 6; i++) {
    properties.setAt(i, "p");
  }
  properties.setAt(6, "overflow");
  properties.setAt(99, "overflow");
  properties.setAt(-1, "underflow");

  CHECK_STR(properties.line(0).c_str(), "pp");
  CHECK_STR(properties.line(1).c_str(), "pp");
  CHECK_STR(properties.line(2).c_str(), "pp");
}

static void properties_empty_message_is_reported_empty() {
  WebUIProperties properties;
  CHECK(properties.empty());
  CHECK(properties.slot() == -1);
  // A write with no next() still cannot land anywhere.
  properties.set("ignored");
  CHECK(properties.empty());
  properties.next();
  properties.set("x");
  CHECK(!properties.empty());
}

static void properties_line_index_is_bounded() {
  WebUIProperties properties;
  properties.next();
  properties.set("a");
  CHECK_STR(properties.line(-1).c_str(), "");
  CHECK_STR(properties.line(3).c_str(), "");
  CHECK_STR(properties.line(99).c_str(), "");
}

/*------------------- Topic title ----------------------*/

static void topic_title_takes_the_first_token() {
  char title[128];
  CHECK(webUITopicTitle("SYStoMQTT", title, sizeof(title)));
  CHECK_STR(title, "SYStoMQTT");

  CHECK(webUITopicTitle("BTtoMQTT/AABBCCDDEEFF", title, sizeof(title)));
  CHECK_STR(title, "BTtoMQTT");

  // buildTopicFromId() produces a leading separator.
  CHECK(webUITopicTitle("/LORAtoMQTT/node1", title, sizeof(title)));
  CHECK_STR(title, "LORAtoMQTT");
}

static void topic_title_rejects_unusable_topics() {
  char title[128];

  // "origin" absent, or holding a non-string: ArduinoJson hands over NULL.
  strcpy(title, "sentinel");
  CHECK(!webUITopicTitle(NULL, title, sizeof(title)));
  CHECK_STR(title, "");

  // Empty string, which strtok() answered with NULL.
  strcpy(title, "sentinel");
  CHECK(!webUITopicTitle("", title, sizeof(title)));
  CHECK_STR(title, "");

  // Separators only, same strtok() NULL.
  strcpy(title, "sentinel");
  CHECK(!webUITopicTitle("/", title, sizeof(title)));
  CHECK_STR(title, "");

  strcpy(title, "sentinel");
  CHECK(!webUITopicTitle("////", title, sizeof(title)));
  CHECK_STR(title, "");
}

static void topic_title_truncates_and_guards_the_buffer() {
  char small[8];
  CHECK(webUITopicTitle("SYStoMQTT", small, sizeof(small)));
  CHECK_STR(small, "SYStoMQ");

  // A one byte buffer can only hold the terminator.
  char tiny[1];
  CHECK(webUITopicTitle("SYStoMQTT", tiny, sizeof(tiny)));
  CHECK_STR(tiny, "");

  CHECK(!webUITopicTitle("SYStoMQTT", NULL, 16));
  CHECK(!webUITopicTitle("SYStoMQTT", small, 0));
}

/*------------------- JSON string fields ----------------------*/

static void string_field_accepts_strings_only() {
  CHECK_STR(webUIStringField(TestVariant::string("v1.8.1")), "v1.8.1");
  CHECK_STR(webUIStringField(TestVariant::string("")), "");

  CHECK(webUIStringField(TestVariant::missing()) == NULL);
  CHECK(webUIStringField(TestVariant::nullValue()) == NULL);
  CHECK(webUIStringField(TestVariant::number()) == NULL);
  CHECK(webUIStringField(TestVariant::object()) == NULL);
}

static void string_field_or_empty_is_always_copyable() {
  CHECK_STR(webUIStringFieldOrEmpty(TestVariant::string("Acurite-Tower")), "Acurite-Tower");
  CHECK_STR(webUIStringFieldOrEmpty(TestVariant::missing()), "");
  CHECK_STR(webUIStringFieldOrEmpty(TestVariant::nullValue()), "");
  CHECK_STR(webUIStringFieldOrEmpty(TestVariant::number()), "");
  CHECK_STR(webUIStringFieldOrEmpty(TestVariant::object()), "");
}

/*------------------- Display text escaping ----------------------*/

static void escape_leaves_ordinary_text_alone() {
  CHECK_STR(webUIEscapeDisplayText("temp: 20.1°C hum: 41.3% ", 128).c_str(),
            "temp: 20.1°C hum: 41.3% ");
  CHECK_STR(webUIEscapeDisplayText("CH₂O: 5mg/m³ ", 128).c_str(), "CH₂O: 5mg/m³ ");
  CHECK_STR(webUIEscapeDisplayText("", 128).c_str(), "");
  CHECK_STR(webUIEscapeDisplayText(NULL, 128).c_str(), "");
}

static void escape_neutralises_html_metacharacters() {
  CHECK_STR(webUIEscapeDisplayText("<>&\"'", 128).c_str(), "&lt;&gt;&amp;&quot;&#39;");
  CHECK_STR(webUIEscapeDisplayText("a & b", 128).c_str(), "a &amp; b");
  // Already escaped input must show as text, not be decoded twice.
  CHECK_STR(webUIEscapeDisplayText("&lt;b&gt;", 128).c_str(), "&amp;lt;b&amp;gt;");
}

static void escape_defuses_a_script_payload() {
  String escaped = webUIEscapeDisplayText("<img src=x onerror=alert(1)>", 128);
  CHECK_STR(escaped.c_str(), "&lt;img src=x onerror=alert(1)&gt;");
  CHECK(strchr(escaped.c_str(), '<') == NULL);
  CHECK(strchr(escaped.c_str(), '>') == NULL);
}

// root_script rewrites {t}, {s}, {m} and {e} into table markup before the
// innerHTML assignment, so a message must not be able to spell them.
static void escape_breaks_the_template_tokens() {
  CHECK_STR(webUIEscapeDisplayText("{t}", 128).c_str(), "&#123;t&#125;");
  CHECK_STR(webUIEscapeDisplayText("{s}", 128).c_str(), "&#123;s&#125;");
  CHECK_STR(webUIEscapeDisplayText("{m}", 128).c_str(), "&#123;m&#125;");
  CHECK_STR(webUIEscapeDisplayText("{e}", 128).c_str(), "&#123;e&#125;");

  String escaped = webUIEscapeDisplayText("x{e}{s}<b>owned</b>{e}", 128);
  CHECK(strchr(escaped.c_str(), '{') == NULL);
  CHECK(strchr(escaped.c_str(), '}') == NULL);
  CHECK(strchr(escaped.c_str(), '<') == NULL);
}

static void escape_stops_at_the_field_length() {
  // A line is a fixed width char array; never read past it even unterminated.
  char line[8];
  memset(line, 'A', sizeof(line));
  CHECK_STR(webUIEscapeDisplayText(line, sizeof(line)).c_str(), "AAAAAAAA");

  CHECK_STR(webUIEscapeDisplayText("abcdef", 3).c_str(), "abc");
  CHECK_STR(webUIEscapeDisplayText("<<<<<<", 2).c_str(), "&lt;&lt;");
}

/*------------------- Rendered rows ----------------------*/

static void rendered_rows_keep_the_intended_table_markup() {
  String rows = webUIRenderMessageRows("BTtoMQTT  AABBCC", "Inkbird T(H) Sensor",
                                       "temp: 20.1°C hum: 41.3% ", "batt: 41% ", "", 128);
  CHECK_STR(rows.c_str(),
            "{t}{s}<b>BTtoMQTT  AABBCC</b>{e}{s}Inkbird T(H) Sensor{e}{s}"
            "temp: 20.1°C hum: 41.3% {e}{s}batt: 41% {e}{s}{e}</table>");
}

static void rendered_rows_escape_hostile_message_text() {
  String rows = webUIRenderMessageRows("<script>alert(1)</script>", "{e}{s}<b>x</b>",
                                       "a&b", "\"quoted\"", "'single'", 128);

  // The only markup left is the template's own.
  CHECK_STR(rows.c_str(),
            "{t}{s}<b>&lt;script&gt;alert(1)&lt;/script&gt;</b>{e}{s}"
            "&#123;e&#125;&#123;s&#125;&lt;b&gt;x&lt;/b&gt;{e}{s}"
            "a&amp;b{e}{s}"
            "&quot;quoted&quot;{e}{s}"
            "&#39;single&#39;{e}</table>");
}

static void rendered_rows_tolerate_missing_lines() {
  String rows = webUIRenderMessageRows(NULL, NULL, NULL, NULL, NULL, 128);
  CHECK_STR(rows.c_str(), "{t}{s}<b></b>{e}{s}{e}{s}{e}{s}{e}{s}{e}</table>");
}

/*------------------- Queue ownership ----------------------*/

static webUIQueueMessage* g_sent = NULL;
static bool g_sendResult = true;

static bool fakeSend(webUIQueueMessage* message) {
  g_sent = message;
  return g_sendResult;
}

static webUIQueueMessage* newMessage() {
  g_freeCount = 0;
  g_sent = NULL;
  return (webUIQueueMessage*)calloc(1, sizeof(webUIQueueMessage));
}

// The queue took it: it now owns the allocation, nothing is released here.
static void handoff_success_transfers_ownership() {
  webUIQueueMessage* message = newMessage();
  webUIQueueMessage* original = message;
  g_sendResult = true;

  CHECK(webUIHandOffMessage(message, fakeSend));
  CHECK(g_sent == original);
  CHECK(g_freeCount == 0);
  CHECK(message == NULL); // caller can no longer touch it

  free(original); // stands in for the queue consumer
}

// The queue refused it: released here, exactly once.
static void handoff_full_queue_frees_once() {
  webUIQueueMessage* message = newMessage();
  webUIQueueMessage* original = message;
  g_sendResult = false;

  CHECK(!webUIHandOffMessage(message, fakeSend));
  CHECK(g_sent == original);
  CHECK(g_freeCount == 1);
  CHECK(g_freed[0] == original);
  CHECK(message == NULL);
}

// No queue at all: the send callback reports failure and the message is released.
static void handoff_without_a_queue_frees_once() {
  webUIQueueMessage* message = newMessage();
  webUIQueueMessage* original = message;

  // webUIDiscardMessage() passes no callback at all.
  CHECK(!webUIHandOffMessage(message, NULL));
  CHECK(g_freeCount == 1);
  CHECK(g_freed[0] == original);
  CHECK(message == NULL);
}

// Allocation failed upstream: nothing to send, nothing to free.
static void handoff_of_a_failed_allocation_is_a_no_op() {
  webUIQueueMessage* message = NULL;
  g_freeCount = 0;
  g_sent = NULL;
  g_sendResult = true;

  CHECK(!webUIHandOffMessage(message, fakeSend));
  CHECK(g_sent == NULL);
  CHECK(g_freeCount == 0);
  CHECK(message == NULL);
}

// Clearing the caller's pointer is what makes a second hand off harmless.
static void handoff_twice_never_double_frees() {
  webUIQueueMessage* message = newMessage();
  g_sendResult = false;

  CHECK(!webUIHandOffMessage(message, fakeSend));
  CHECK(g_freeCount == 1);

  CHECK(!webUIHandOffMessage(message, fakeSend));
  CHECK(g_freeCount == 1); // still one, the pointer was already given up
}

/*------------------- Runner ----------------------*/

int main() {
  printf("webUIMessage host tests\n");

  RUN(properties_normal_message_layout);
  RUN(properties_exactly_six_fields_all_shown);
  RUN(properties_more_than_six_fields_are_dropped);
  RUN(properties_gravity_skips_a_slot);
  RUN(properties_gravity_alone_lands_in_slot_one);
  RUN(properties_gravity_at_the_boundary_is_dropped);
  RUN(properties_explicit_slots_are_bounded);
  RUN(properties_empty_message_is_reported_empty);
  RUN(properties_line_index_is_bounded);

  RUN(topic_title_takes_the_first_token);
  RUN(topic_title_rejects_unusable_topics);
  RUN(topic_title_truncates_and_guards_the_buffer);

  RUN(string_field_accepts_strings_only);
  RUN(string_field_or_empty_is_always_copyable);

  RUN(escape_leaves_ordinary_text_alone);
  RUN(escape_neutralises_html_metacharacters);
  RUN(escape_defuses_a_script_payload);
  RUN(escape_breaks_the_template_tokens);
  RUN(escape_stops_at_the_field_length);

  RUN(rendered_rows_keep_the_intended_table_markup);
  RUN(rendered_rows_escape_hostile_message_text);
  RUN(rendered_rows_tolerate_missing_lines);

  RUN(handoff_success_transfers_ownership);
  RUN(handoff_full_queue_frees_once);
  RUN(handoff_without_a_queue_frees_once);
  RUN(handoff_of_a_failed_allocation_is_a_no_op);
  RUN(handoff_twice_never_double_frees);

  printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
