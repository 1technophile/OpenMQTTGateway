/*
	Copyright (C) 2013 CurlyMo & Bram1337

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
#include "conrad_rsl_contact.h"

#define PULSE_MULTIPLIER	5
#define MIN_PULSE_LENGTH	185
#define MAX_PULSE_LENGTH	195
#define AVG_PULSE_LENGTH	190
#define RAW_LENGTH				66

static int validate(void) {
	if(conrad_rsl_contact->rawlen == RAW_LENGTH) {
		if(conrad_rsl_contact->raw[conrad_rsl_contact->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   conrad_rsl_contact->raw[conrad_rsl_contact->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int state) {
	conrad_rsl_contact->message = json_mkobject();
	json_append_member(conrad_rsl_contact->message, "id", json_mknumber(id, 0));
	if(state == 1) {
		json_append_member(conrad_rsl_contact->message, "state", json_mkstring("opened"));
	} else {
		json_append_member(conrad_rsl_contact->message, "state", json_mkstring("closed"));
	}
}

static void parseCode(void) {
	int x = 0, binary[RAW_LENGTH/2];

	if(conrad_rsl_contact->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "conrad_rsl_contact: parsecode - invalid parameter passed %d", conrad_rsl_contact->rawlen);
		return;
	}

	/* Convert the one's and zero's into binary */
	for(x=0; x<conrad_rsl_contact->rawlen; x+=2) {
		if(conrad_rsl_contact->raw[x+1] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[x/2]=1;
		} else {
			binary[x/2]=0;
		}
	}

	int id = binToDecRev(binary, 6, 31);
	int check = binToDecRev(binary, 0, 3);
	int check1 = binary[32];
	int state = binary[4];

	if(check == 5 && check1 == 1) {
		createMessage(id, state);
	}
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void conradRSLContactInit(void) {

	protocol_register(&conrad_rsl_contact);
	protocol_set_id(conrad_rsl_contact, "conrad_rsl_contact");
	protocol_device_add(conrad_rsl_contact, "conrad_rsl_contact", "Conrad RSL Contact Sensor");
	conrad_rsl_contact->devtype = CONTACT;
	conrad_rsl_contact->hwtype = RF433;
	conrad_rsl_contact->minrawlen = RAW_LENGTH;
	conrad_rsl_contact->maxrawlen = RAW_LENGTH;
	conrad_rsl_contact->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	conrad_rsl_contact->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&conrad_rsl_contact->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(([0-9]|([1-9][0-9])|([1-9][0-9]{2})|([1-9][0-9]{3})|([1-9][0-9]{4})|([1-9][0-9]{5})|([1-9][0-9]{6})|((6710886[0-3])|(671088[0-5][0-9])|(67108[0-7][0-9]{2})|(6710[0-7][0-9]{3})|(671[0--1][0-9]{4})|(670[0-9]{5})|(6[0-6][0-9]{6})|(0[0-5][0-9]{7}))))$");
	options_add(&conrad_rsl_contact->options, "t", "opened", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&conrad_rsl_contact->options, "f", "closed", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	conrad_rsl_contact->parseCode=&parseCode;
	conrad_rsl_contact->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "conrad_rsl_contact";
	module->version = "2.3";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	conradRSLContactInit();
}
#endif
