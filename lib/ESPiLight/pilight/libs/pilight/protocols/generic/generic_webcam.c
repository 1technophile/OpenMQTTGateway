/*
	Copyright (C) 2014 CurlyMo & 1000io

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
#include <math.h>
#include <string.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "generic_webcam.h"

static int checkValues(JsonNode *code) {
	int height = 300;
	double itmp = -1;

	if(json_find_number(code, "gui-image-height", &itmp) == 0)
		height = (int)round(itmp);

	if(height <= 0) {
		logprintf(LOG_ERR, "Generic webcam gui-image-height cannot <= 0");
		return 1;
	}
	return 0;
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void genericWebcamInit(void) {

	protocol_register(&generic_webcam);
	protocol_set_id(generic_webcam, "generic_webcam");
	protocol_device_add(generic_webcam, "generic_webcam", "Generic webcam");
	generic_webcam->devtype = WEBCAM;
	generic_webcam->multipleId = 0;

	options_add(&generic_webcam->options, 'u', "url", OPTION_HAS_VALUE, DEVICES_VALUE, JSON_STRING, NULL, NULL);
	options_add(&generic_webcam->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,})$");

	options_add(&generic_webcam->options, 0, "image-width", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "[0-9]");
	options_add(&generic_webcam->options, 0, "image-height", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)300, "[0-9]");
	options_add(&generic_webcam->options, 0, "show-webcam", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)1, "^[10]{1}$");
	options_add(&generic_webcam->options, 0, "poll-interval", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)10, "^[10]{2}$");

	generic_webcam->checkValues=checkValues;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "generic_webcam";
	module->version = "1.3";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	genericWebcamInit();
}
#endif
