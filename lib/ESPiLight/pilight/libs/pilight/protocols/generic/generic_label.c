/*
	Copyright (C) 2015 CurlyMo

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
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "generic_label.h"

static void createMessage(int id, char *label, char *color) {
	generic_label->message = json_mkobject();
	json_append_member(generic_label->message, "id", json_mknumber(id, 0));
	json_append_member(generic_label->message, "label", json_mkstring(label));
	json_append_member(generic_label->message, "color", json_mkstring(color));
}

static int createCode(JsonNode *code) {
	int id = -1, free_label = 0;
	char *label = NULL;
	char *color = "black";
	double itmp = 0;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);

	json_find_string(code, "label", &label);
	if(json_find_number(code, "label", &itmp) == 0) {
		if((label = MALLOC(BUFFER_SIZE)) == NULL) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
		free_label = 1;
		snprintf(label, BUFFER_SIZE, "%d", (int)itmp);
	}
	json_find_string(code, "color", &color);

	if(id == -1 || label == NULL) {
		logprintf(LOG_ERR, "generic_label: insufficient number of arguments");
		return EXIT_FAILURE;
	} else {
		createMessage(id, label, color);
	}
	if(free_label) {
		FREE(label);
	}

	return EXIT_SUCCESS;

}

static void printHelp(void) {
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -l --label=label\t\tset this label\n");
	printf("\t -c --color=color\t\tset label color\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void genericLabelInit(void) {

	protocol_register(&generic_label);
	protocol_set_id(generic_label, "generic_label");
	protocol_device_add(generic_label, "generic_label", "Generic Label");
	generic_label->devtype = LABEL;

	options_add(&generic_label->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,})$");
	options_add(&generic_label->options, 'l', "label", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING | JSON_NUMBER, NULL, NULL);
	options_add(&generic_label->options, 'c', "color", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);

	generic_label->printHelp=&printHelp;
	generic_label->createCode=&createCode;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "generic_label";
	module->version = "1.2";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	genericLabelInit();
}
#endif
