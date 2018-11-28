# ESPiLight - pilight 433.92 MHz protocols library for Arduino
# Copyright (c) 2016 Puuu.  All right reserved.
#
# Project home: https://github.com/puuu/espilight/
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with library. If not, see <http://www.gnu.org/licenses/>

SRC_DIR = pilight

DST_DIR = src/pilight
PROTOCOL_DIR = libs/pilight/protocols/433.92

PROTOCOLS = $(patsubst $(SRC_DIR)/$(PROTOCOL_DIR)/%.h,%,$(sort $(wildcard $(SRC_DIR)/$(PROTOCOL_DIR)/*.h)))

PILIGHT_FILES = libs/pilight/core/dso.h libs/pilight/core/mem.h	\
	libs/pilight/core/json.h libs/pilight/core/json.c	\
	libs/pilight/core/binary.h libs/pilight/core/binary.c	\
	libs/pilight/protocols/protocol_header.h		\
	libs/pilight/protocols/protocol_init.h
PROTOCOL_H_FILES = $(foreach protocol,$(PROTOCOLS),$(PROTOCOL_DIR)/$(protocol).h)
PROTOCOL_C_FILES = $(foreach protocol,$(PROTOCOLS),$(PROTOCOL_DIR)/$(protocol).c)
FILES = $(PILIGHT_FILES) $(PROTOCOL_H_FILES) $(PROTOCOL_C_FILES)

DST_FILES = $(foreach file,$(FILES),$(DST_DIR)/$(file))

.PHONY: all clean copy update release

all: $(SRC_DIR)/libs
	$(MAKE) -e copy

copy: $(DST_FILES)

$(DST_DIR)/%: $(SRC_DIR)/%
	@mkdir -p $(@D)
	cp $< $@

$(DST_DIR)/libs/pilight/core/json.c: $(SRC_DIR)/libs/pilight/core/json.c
	@mkdir -p $(@D)
	cp $< $@
#	ESP8266 Android, sprintf not working with float. Patch:
	sed 's/\(^[ \t]*\)\(sprintf(buf, "%.*f", decimals, num);\)/#ifdef ESP8266\n\1dtostrf(num, 0, decimals, buf);\n#else\n\1\2\n#endif/' -i $@
#	Arduino did not provide printf, fprintf
	sed 's!#include <stdio.h>!#include <stdio.h>\n#include "../../../../tools/aprintf.h"!' -i $@

$(DST_DIR)/libs/pilight/protocols/protocol_header.h:
	for protocol in $(PROTOCOLS); do\
	  echo "#include \"433.92/$${protocol}.h\""  >> $@;\
	done

$(DST_DIR)/libs/pilight/protocols/protocol_init.h: $(foreach file,$(PROTOCOL_C_FILES),$(DST_DIR)/$(file))
	for cfile in $^; do\
	  grep 'void .*Init(' $$cfile | sed 's/void \(.*Init\)(.*/\1();/'  >> $@;\
	done

pilight/libs:
	git submodule update --init pilight

update:
	$(MAKE) clean
	git submodule update pilight
	$(MAKE) copy

release: $(SRC_DIR)/libs
	git checkout release
	git merge master
	make update
	git add src/pilight
	git commit -m "integrate new pilight files"
	@echo "change log:"
	@git log --pretty="* %s" --no-merges HEAD...`git describe --abbrev=0 --tags`
	@git submodule summary `git describe --abbrev=0 --tags`
	@echo "run: git tag -a v"`grep version library.properties | sed 's/version=\(.*\)/\1/g'`

clean:
	-rm $(DST_FILES)

stylecheck:
	RESULT=0;\
	for file in src/*.h src/*.cpp src/tools/*.h src/tools/*.cpp tests/*/*.ino examples/*/*.ino; do\
	  clang-format -style=google "$$file" | diff -u "$$file" - || RESULT=$$?;\
	done;\
	exit $$RESULT
