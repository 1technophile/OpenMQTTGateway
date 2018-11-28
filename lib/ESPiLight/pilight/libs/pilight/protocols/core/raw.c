/*
	Copyright (C) 2013 CurlyMo

	This file is part of pilight.

	pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "raw.h"

static int createCode(JsonNode *code) {
	char *rcode = NULL;
	double repeats = 10;
	char **array = NULL;
	unsigned int i = 0, n = 0;

	if(json_find_string(code, "code", &rcode) != 0) {
		logprintf(LOG_ERR, "raw: insufficient number of arguments");
		return EXIT_FAILURE;
	}

	json_find_number(code, "repeats", &repeats);

	n = explode(rcode, " ", &array);
	for(i=0;i<n;i++) {
		raw->raw[i]=atoi(array[i]);
	}
	array_free(&array, n);

	raw->rawlen=(int)i;
	raw->txrpt = repeats;

	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -c --code=\"raw\"\t\traw code devided by spaces\n\t\t\t\t\t(just like the output of debug)\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void rawInit(void) {

	protocol_register(&raw);
	protocol_set_id(raw, "raw");
	protocol_device_add(raw, "raw", "Raw Codes");
	raw->devtype = RAW;
	raw->hwtype = RF433;
	raw->config = 0;

	options_add(&raw->options, 'c', "code", OPTION_HAS_VALUE, 0, JSON_STRING, NULL, NULL);
	options_add(&raw->options, 'r', "repeats", OPTION_OPT_VALUE, 0, JSON_NUMBER, NULL, NULL);

	raw->createCode=&createCode;
	raw->printHelp=&printHelp;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "raw";
	module->version = "1.5";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	rawInit();
}
#endif
