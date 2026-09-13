/*
  Theengs OpenMQTTGateway - We Unite Sensors in One Open-Source Interface

   Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker
   Send and receiving command by MQTT

   Message handling helpers shared by the WebUI display pipeline.

    Copyright: (c)Florian ROBERT

    This file is part of OpenMQTTGateway.

    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef config_WebUIMessage_h
#define config_WebUIMessage_h

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

/*
Helpers used by webUIPubPrint() and handleRoot() to turn a module JSON message
into the text shown on the WebUI display.

The gateway does not control what ends up in those messages: a JSON payload
handed to jsonDispatch() carries whatever keys and value types the producer put
in it, and the SERIAL gateway forwards a wired peer's JSON verbatim. Everything
here is therefore written to survive a missing field, a field of the wrong type
and a message holding more fields than the display can show.

They are deliberately free of WebServer, FreeRTOS and ArduinoJson dependencies so
the host tests in test/host/webui_message can exercise the same code the firmware
runs.
*/

struct webUIQueueMessage; // full definition in config_WebUI.h

// Number of property slots making up lines 2 to 4 of a display message.
#define WEBUI_MAX_PROPERTIES 6

/*
Bounded accumulator for the property strings of a BLE display message.

The renderer walks a long list of optional JSON fields and moves to the next slot
for each field it finds. There are far more candidate fields than slots, so both
the advance and every write are clamped here: a message carrying more fields than
fit keeps the slots it filled first and the surplus is dropped, instead of the
writes running past the end of the array and assigning to String objects that do
not exist.
*/
class WebUIProperties {
public:
  WebUIProperties() : _slot(-1) {}

  // Move to the next slot, saturating one past the last slot.
  void next() {
    if (_slot < (int)WEBUI_MAX_PROPERTIES) {
      _slot++;
    }
  }

  // Write the current slot; ignored once the slots are exhausted.
  void set(const String& value) {
    setAt(_slot, value);
  }

  // Write an explicit slot; ignored when the index is out of range.
  void setAt(int slot, const String& value) {
    if (slot >= 0 && slot < (int)WEBUI_MAX_PROPERTIES) {
      _values[slot] = value;
    }
  }

  // Index of the slot a set() would write, -1 before the first next().
  int slot() const {
    return _slot;
  }

  // Lines 2, 3 and 4 each show a pair of slots: line(0) is slots 0 and 1.
  String line(int index) const {
    if (index < 0 || index * 2 + 1 >= (int)WEBUI_MAX_PROPERTIES) {
      return String();
    }
    return _values[index * 2] + _values[index * 2 + 1];
  }

  // True when no slot was filled, so there is nothing worth displaying.
  bool empty() const {
    for (int i = 0; i < (int)WEBUI_MAX_PROPERTIES; i++) {
      if (_values[i].length() != 0) {
        return false;
      }
    }
    return true;
  }

private:
  String _values[WEBUI_MAX_PROPERTIES];
  int _slot;
};

/*
Copy the first '/' separated token of a topic into `title`.

This replaces a strdup()/strtok() pair: there is no allocation left to fail, no
NULL return to hand to strlcpy(), and no dependence on strtok()'s shared parser
state, which several tasks publishing to the WebUI would otherwise share.

Returns false and leaves an empty string when the topic is NULL, empty, or made
of separators only.
*/
inline bool webUITopicTitle(const char* topicori, char* title, size_t size) {
  if (title == NULL || size == 0) {
    return false;
  }
  title[0] = '\0';
  if (topicori == NULL) {
    return false;
  }
  const char* start = topicori;
  while (*start == '/') {
    start++;
  }
  if (*start == '\0') {
    return false;
  }
  const char* end = strchr(start, '/');
  size_t length = (end != NULL) ? (size_t)(end - start) : strlen(start);
  if (length > size - 1) {
    length = size - 1;
  }
  memcpy(title, start, length);
  title[length] = '\0';
  return true;
}

/*
Read a JSON field that is expected to hold a string.

ArduinoJson yields a null pointer for a missing key, for an explicit null, and
for a value that is simply not a string. Passing that straight to strlcpy() or
strncmp() dereferences NULL, so every consumer of display text goes through here.
*/
template <typename TVariant>
inline const char* webUIStringField(const TVariant& value) {
  return value.template is<const char*>() ? value.template as<const char*>() : NULL;
}

// Same, but substitutes an empty string so the result is always safe to copy.
template <typename TVariant>
inline const char* webUIStringFieldOrEmpty(const TVariant& value) {
  const char* text = webUIStringField(value);
  return (text != NULL) ? text : "";
}

/*
Escape message text before it is placed in the "{t}{s}{m}{e}" mini template that
the root page hands to innerHTML.

Two classes of character have to go:
 - the HTML metacharacters, so the text cannot open a tag or close an attribute;
 - the braces of the template tokens, because root_script rewrites {t}, {s}, {m}
   and {e} into real table markup before the assignment, so text containing {e}
   would close the cell and let whatever follows be parsed as markup.
An escaped brace still shows up as a plain brace once the browser decodes the
entity, so legitimate text reads the same as before.
*/
inline String webUIEscapeDisplayText(const char* text, size_t maxLength) {
  String escaped;
  if (text == NULL) {
    return escaped;
  }
  size_t length = strnlen(text, maxLength);
  escaped.reserve(length + 8);
  for (size_t i = 0; i < length; i++) {
    switch (text[i]) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&#39;";
        break;
      case '{':
        escaped += "&#123;";
        break;
      case '}':
        escaped += "&#125;";
        break;
      default:
        escaped += text[i];
        break;
    }
  }
  return escaped;
}

/*
Build the table rows the root page serves for the message currently on display.

{t}, {s} and {e} are the page template's own tokens - root_script turns them into
real table markup and assigns the result to innerHTML. Everything coming from the
message is escaped first, so the only markup in the response is the markup this
function put there.
*/
inline String webUIRenderMessageRows(const char* title, const char* line1, const char* line2,
                                     const char* line3, const char* line4, size_t maxLength) {
  String rows = "{t}{s}<b>";
  rows += webUIEscapeDisplayText(title, maxLength);
  rows += "</b>{e}{s}";
  rows += webUIEscapeDisplayText(line1, maxLength);
  rows += "{e}{s}";
  rows += webUIEscapeDisplayText(line2, maxLength);
  rows += "{e}{s}";
  rows += webUIEscapeDisplayText(line3, maxLength);
  rows += "{e}{s}";
  rows += webUIEscapeDisplayText(line4, maxLength);
  rows += "{e}</table>";
  return rows;
}

// Callback handing a message to the display queue, true when the queue took it.
typedef bool (*webUIQueueSendFn)(webUIQueueMessage*);

/*
Hand a completed message over to the display queue.

When `send` accepts it the queue owns the allocation; on refusal - a full queue,
a queue that was never created, or no callback at all - the allocation is
released here. The caller's pointer is cleared either way, so the same message
can never be queued twice, freed twice, or leaked.
*/
inline bool webUIHandOffMessage(webUIQueueMessage*& message, webUIQueueSendFn send) {
  if (message == NULL) {
    return false;
  }
  bool queued = (send != NULL) && send(message);
  if (!queued) {
    free(message);
  }
  message = NULL;
  return queued;
}

#endif
