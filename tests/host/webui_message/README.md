# WebUI message handling - host tests

Tests for `main/webUIMessage.h`, the helpers `webUIPubPrint()` and `handleRoot()`
use to turn a module JSON message into WebUI display text.

They run on the build host with `g++`; no board and no PlatformIO toolchain is
involved, and nothing here is compiled into the firmware.

```sh
cd tests/host/webui_message
make
```

The suite builds with AddressSanitizer and UndefinedBehaviorSanitizer enabled, so
an out of bounds write or a double free fails the run rather than passing
quietly.

## What is covered

| Area | Cases |
| --- | --- |
| `WebUIProperties` | normal layout, exactly six fields, more than six, the `gravity` double advance in range / alone / at the boundary, explicit slots (BBQ probes), empty message, out of range line index |
| `webUITopicTitle()` | ordinary topic, leading separator, `NULL`, empty, separators only, truncation, zero sized and `NULL` buffer |
| `webUIStringField()` | string, empty string, missing key, explicit null, number, object |
| `webUIEscapeDisplayText()` | pass through incl. UTF-8, HTML metacharacters, a script payload, the `{t}` `{s}` `{m}` `{e}` template tokens, unterminated and over long fields |
| `webUIRenderMessageRows()` | intended table markup preserved, hostile text escaped, missing lines |
| `webUIHandOffMessage()` | queue accepts (ownership moves, no free), queue full (freed once), no queue, failed allocation, repeated hand off |

## ArduinoJson

The main suite models an ArduinoJson variant with a stand-in, so it needs no
third party code. `test_arduinojson_contract.cpp` runs the same expectations
against the real library and is opt in:

```sh
make ARDUINOJSON_DIR=/path/to/ArduinoJson/src check-arduinojson
```

With a PlatformIO checkout the sources sit under
`.pio/libdeps/<env>/ArduinoJson/src`.
