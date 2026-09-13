# Host tests for main/webUIMessage.h - see README.md
#
#   make            build and run
#   make ARDUINOJSON_DIR=/path/to/ArduinoJson/src check-arduinojson
#                   additionally verify the ArduinoJson assumptions

CXX      ?= g++
CXXFLAGS ?= -std=c++11 -O1 -g -Wall -Wextra -Wno-unused-parameter \
            -fsanitize=address,undefined -fno-omit-frame-pointer
ROOT     := ../../..
INCLUDES := -I shim -I $(ROOT)/main
BUILD    := build

.PHONY: all run check-arduinojson clean

all: run

$(BUILD)/test_webui_message: test_webui_message.cpp $(ROOT)/main/webUIMessage.h | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

run: $(BUILD)/test_webui_message
	./$(BUILD)/test_webui_message

# webUIStringField() assumes ArduinoJson answers is<const char*>() with false for
# a missing key, an explicit null and a non string value. This target checks that
# against the real library instead of the test stand-in.
check-arduinojson: $(BUILD)
ifndef ARDUINOJSON_DIR
	@echo "set ARDUINOJSON_DIR to the ArduinoJson src directory, e.g."
	@echo "  make ARDUINOJSON_DIR=~/.platformio/lib/ArduinoJson/src check-arduinojson"
	@exit 1
endif
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I $(ARDUINOJSON_DIR) \
	    -o $(BUILD)/test_arduinojson_contract test_arduinojson_contract.cpp
	./$(BUILD)/test_arduinojson_contract

clean:
	rm -rf $(BUILD)
