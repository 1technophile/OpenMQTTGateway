/*
	Copyright (C) 2015 CurlyMo and Meloen

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
#include "ev1527.h"

#define PULSE_MULTIPLIER	5
#define MIN_PULSE_LENGTH	251
#define MAX_PULSE_LENGTH	311
#define AVG_PULSE_LENGTH	256
#define RAW_LENGTH				50

static int validate(void) {
	if(ev1527->rawlen == RAW_LENGTH) {
		if(ev1527->raw[ev1527->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   ev1527->raw[ev1527->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int unitcode, int state) {
	ev1527->message = json_mkobject();
	json_append_member(ev1527->message, "unitcode", json_mknumber(unitcode, 0));
	if(state == 0) {
		json_append_member(ev1527->message, "state", json_mkstring("opened"));
	} else {
		json_append_member(ev1527->message, "state", json_mkstring("closed"));
	}
}

static void parseCode(void) {
	int binary[RAW_LENGTH/2], x = 0, i = 0;

	if(ev1527->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "ev1527: parsecode - invalid parameter passed %d", ev1527->rawlen);
		return;
	}

	for(x=0;x<ev1527->rawlen-2;x+=2) {
		if(ev1527->raw[x+3] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	int unitcode = binToDec(binary, 0, 19);
	int state = binary[20];
	createMessage(unitcode, state);
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void ev1527Init(void) {

	protocol_register(&ev1527);
	protocol_set_id(ev1527, "ev1527");
	protocol_device_add(ev1527, "ev1527", "ev1527 contact sensor");
	ev1527->devtype = CONTACT;
	ev1527->hwtype = RF433;
	ev1527->minrawlen = RAW_LENGTH;
	ev1527->maxrawlen = RAW_LENGTH;
	ev1527->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	ev1527->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&ev1527->options, "u", "unitcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(104857[0-5]|10485[0-6][0-9]|1048[0-4][0-9][0-9]|104[0-7][0-9]{3}|10[0-3][0-9]{4}|0?[0-9]{1,6})$");
	options_add(&ev1527->options, "t", "opened", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&ev1527->options, "f", "closed", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	ev1527->parseCode=&parseCode;
	ev1527->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "ev1527";
	module->version = "1.2";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	ev1527Init();
}
#endif
