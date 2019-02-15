/*
	Copyright (C) 2014 CurlyMo & lvdp

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
#include "arctech_contact.h"

#define PULSE_MULTIPLIER	4
#define MIN_PULSE_LENGTH	250
#define MAX_PULSE_LENGTH	320
#define AVG_PULSE_LENGTH	300
#define MIN_RAW_LENGTH		132
#define MAX_RAW_LENGTH		148
#define RAW_LENGTH				148

static int validate(void) {
	if(arctech_contact->rawlen == MIN_RAW_LENGTH || arctech_contact->rawlen == MAX_RAW_LENGTH) {
		if(arctech_contact->raw[arctech_contact->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   arctech_contact->raw[arctech_contact->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV) &&
			 arctech_contact->raw[1] >= AVG_PULSE_LENGTH*(PULSE_MULTIPLIER*2)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state, int all) {
	arctech_contact->message = json_mkobject();
	json_append_member(arctech_contact->message, "id", json_mknumber(id, 0));
	if(all == 1) {
		json_append_member(arctech_contact->message, "all", json_mknumber(all, 0));
	} else {
		json_append_member(arctech_contact->message, "unit", json_mknumber(unit, 0));
	}

	if(state == 1) {
		json_append_member(arctech_contact->message, "state", json_mkstring("opened"));
	} else {
		json_append_member(arctech_contact->message, "state", json_mkstring("closed"));
	}
}

static void parseCode(void) {
	int binary[MAX_RAW_LENGTH/4], x = 0, i = 0;

	if(arctech_contact->rawlen>MAX_RAW_LENGTH) {
		logprintf(LOG_ERR, "arctech_contact: parsecode - invalid parameter passed %d", arctech_contact->rawlen);
		return;
	}

	for(x=0;x<arctech_contact->rawlen;x+=4) {
		if(arctech_contact->raw[x+3] > AVG_PULSE_LENGTH*PULSE_MULTIPLIER) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	int unit = binToDecRev(binary, 28, 31);
	int state = binary[27];
	int all = binary[26];
	int id = binToDecRev(binary, 0, 25);

	createMessage(id, unit, state, all);
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void arctechContactInit(void) {

	protocol_register(&arctech_contact);
	protocol_set_id(arctech_contact, "arctech_contact");
	protocol_device_add(arctech_contact, "kaku_contact", "KlikAanKlikUit Contact Sensor");
	protocol_device_add(arctech_contact, "dio_contact", "D-IO Contact Sensor");

	arctech_contact->devtype = CONTACT;
	arctech_contact->hwtype = RF433;
	arctech_contact->minrawlen = MIN_RAW_LENGTH;
	arctech_contact->maxrawlen = MAX_RAW_LENGTH;
	arctech_contact->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	arctech_contact->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&arctech_contact->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1}|[1][0-5])$");
	options_add(&arctech_contact->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,7}|[1-5][0-9]{7}|6([0-6][0-9]{6}|7(0[0-9]{5}|10([0-7][0-9]{3}|8([0-7][0-9]{2}|8([0-5][0-9]|6[0-3]))))))$");
	options_add(&arctech_contact->options, "t", "opened", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&arctech_contact->options, "f", "closed", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&arctech_contact->options, "a", "all", OPTION_HAS_VALUE, DEVICES_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	arctech_contact->parseCode=&parseCode;
	arctech_contact->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "arctech_contact";
	module->version = "2.2";
	module->reqversion = "6.0";
	module->reqcommit = "38";
}

void init(void) {
	arctechContactInit();
}
#endif
