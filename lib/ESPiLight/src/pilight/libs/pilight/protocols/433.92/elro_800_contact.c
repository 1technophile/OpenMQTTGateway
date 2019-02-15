/*
	Copyright (C) 2014 CurlyMo

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
#include "elro_800_contact.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	283
#define MAX_PULSE_LENGTH	305
#define AVG_PULSE_LENGTH	300
#define RAW_LENGTH				50

static int validate(void) {
	if(elro_800_contact->rawlen == RAW_LENGTH) {
		if(elro_800_contact->raw[elro_800_contact->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   elro_800_contact->raw[elro_800_contact->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int systemcode, int unitcode, int state) {
	elro_800_contact->message = json_mkobject();
	json_append_member(elro_800_contact->message, "systemcode", json_mknumber(systemcode, 0));
	json_append_member(elro_800_contact->message, "unitcode", json_mknumber(unitcode, 0));
	if(state == 0) {
		json_append_member(elro_800_contact->message, "state", json_mkstring("opened"));
	} else {
		json_append_member(elro_800_contact->message, "state", json_mkstring("closed"));
	}
}

static void parseCode(void) {
	int binary[RAW_LENGTH/4], x = 0, i = 0;

	if(elro_800_contact->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "elro_800_contact: parsecode - invalid parameter passed %d", elro_800_contact->rawlen);
		return;
	}

	for(x=0;x<elro_800_contact->rawlen-2;x+=4) {
		if(elro_800_contact->raw[x+3] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	int systemcode = binToDec(binary, 0, 4);
	int unitcode = binToDec(binary, 5, 9);
	int state = binary[11];
	createMessage(systemcode, unitcode, state);
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void elro800ContactInit(void) {

	protocol_register(&elro_800_contact);
	protocol_set_id(elro_800_contact, "elro_800_contact");
	protocol_device_add(elro_800_contact, "elro_800_contact", "Elro Series 800 Contact");
	elro_800_contact->devtype = CONTACT;
	elro_800_contact->hwtype = RF433;
	elro_800_contact->minrawlen = RAW_LENGTH;
	elro_800_contact->maxrawlen = RAW_LENGTH;
	elro_800_contact->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	elro_800_contact->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&elro_800_contact->options, "s", "systemcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&elro_800_contact->options, "u", "unitcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&elro_800_contact->options, "t", "opened", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&elro_800_contact->options, "f", "closed", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	elro_800_contact->parseCode=&parseCode;
	elro_800_contact->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "elro_800_contact";
	module->version = "2.4";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	elro800ContactInit();
}
#endif
