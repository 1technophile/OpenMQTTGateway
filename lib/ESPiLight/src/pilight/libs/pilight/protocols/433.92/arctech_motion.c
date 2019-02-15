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

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "arctech_motion.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	250
#define MAX_PULSE_LENGTH	284
#define AVG_PULSE_LENGTH	279
#define RAW_LENGTH				132

static int validate(void) {
	if(arctech_motion->rawlen == RAW_LENGTH) {
		if(arctech_motion->raw[arctech_motion->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   arctech_motion->raw[arctech_motion->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV) &&
			 arctech_motion->raw[1] >= AVG_PULSE_LENGTH*(PULSE_MULTIPLIER*3)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state, int all) {
	arctech_motion->message = json_mkobject();
	json_append_member(arctech_motion->message, "id", json_mknumber(id, 0));
	if(all == 1) {
		json_append_member(arctech_motion->message, "all", json_mknumber(all, 0));
	} else {
		json_append_member(arctech_motion->message, "unit", json_mknumber(unit, 0));
	}

	if(state == 1) {
		json_append_member(arctech_motion->message, "state", json_mkstring("on"));
	} else {
		json_append_member(arctech_motion->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int binary[RAW_LENGTH/4], x = 0, i = 0;

	if(arctech_motion->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "arctech_motion: parsecode - invalid parameter passed %d", arctech_motion->rawlen);
		return;
	}

	for(x=0;x<arctech_motion->rawlen;x+=4) {
		if(arctech_motion->raw[x+3] > AVG_PULSE_LENGTH*PULSE_MULTIPLIER) {
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
void arctechMotionInit(void) {

	protocol_register(&arctech_motion);
	protocol_set_id(arctech_motion, "arctech_motion");
	protocol_device_add(arctech_motion, "kaku_motion", "KlikAanKlikUit Motion Sensor");
	arctech_motion->devtype = MOTION;
	arctech_motion->hwtype = RF433;
	arctech_motion->minrawlen = RAW_LENGTH;
	arctech_motion->maxrawlen = RAW_LENGTH;
	arctech_motion->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	arctech_motion->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&arctech_motion->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1}|[1][0-5])$");
	options_add(&arctech_motion->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,7}|[1-5][0-9]{7}|6([0-6][0-9]{6}|7(0[0-9]{5}|10([0-7][0-9]{3}|8([0-7][0-9]{2}|8([0-5][0-9]|6[0-3]))))))$");
	options_add(&arctech_motion->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&arctech_motion->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	arctech_motion->parseCode=&parseCode;
	arctech_motion->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "arctech_motion";
	module->version = "2.2";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	arctechMotionInit();
}
#endif
