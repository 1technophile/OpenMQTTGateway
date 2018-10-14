/*
	Copyright (C) 2014 CurlyMo & easy12

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
#include "rev_v3.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	253
#define MAX_PULSE_LENGTH	269
#define AVG_PULSE_LENGTH	264
#define RAW_LENGTH				50

static int validate(void) {
	if(rev3_switch->rawlen == RAW_LENGTH) {
		if(rev3_switch->raw[rev3_switch->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   rev3_switch->raw[rev3_switch->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state) {
	rev3_switch->message = json_mkobject();
	json_append_member(rev3_switch->message, "id", json_mknumber(id, 0));
	json_append_member(rev3_switch->message, "unit", json_mknumber(unit, 0));
	if(state == 1) {
		json_append_member(rev3_switch->message, "state", json_mkstring("on"));
	} else {
		json_append_member(rev3_switch->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int x = 0, i = 0, binary[RAW_LENGTH/4];

	if(rev3_switch->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "rev3_switch: parsecode - invalid parameter passed %d", rev3_switch->rawlen);
		return;
	}

	/* Convert the one's and zero's into binary */
	for(x=0;x<rev3_switch->rawlen-2;x+=4) {
		if(rev3_switch->raw[x+3] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	int unit = binToDec(binary, 6, 9);
	int state = binary[11];
	int id = binToDec(binary, 0, 5);

	createMessage(id, unit, state);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		rev3_switch->raw[i]=(AVG_PULSE_LENGTH);
		rev3_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		rev3_switch->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		rev3_switch->raw[i+3]=(AVG_PULSE_LENGTH);
	}
}
static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		rev3_switch->raw[i]=(AVG_PULSE_LENGTH);
		rev3_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		rev3_switch->raw[i+2]=(AVG_PULSE_LENGTH);
		rev3_switch->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createHigh(0,3);
	createLow(4,47);
}

static void createUnit(int unit) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createHigh(24+x, 24+x+3);
		}
	}
}

static void createId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(id, binary);
	for(i=0;i<=length;i++) {
		x=i*4;
		if(binary[i]==1) {
			createHigh(x, x+3);
		}
	}
}

static void createState(int state) {
	if(state == 0) {
		createHigh(40,43);
		createLow(44,47);
	} else {
		createLow(40,43);
		createHigh(44,47);
	}
}

static void createFooter(void) {
	rev3_switch->raw[48]=(AVG_PULSE_LENGTH);
	rev3_switch->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int unit = -1;
	int state = -1;
	double itmp = -1;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);

	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);

	if(id == -1 || unit == -1 || state == -1) {
		logprintf(LOG_ERR, "rev3_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 63 || id < 0) {
		logprintf(LOG_ERR, "rev3_switch: invalid id range");
		return EXIT_FAILURE;
	} else if(unit > 15 || unit < 0) {
		logprintf(LOG_ERR, "rev3_switch: invalid unit range");
		return EXIT_FAILURE;
	} else {
		createMessage(id, unit, state);
		clearCode();
		createUnit(unit);
		createId(id);
		createState(state);
		createFooter();
		rev3_switch->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
    printf("\t -t --on\t\t\tsend an on signal\n");
    printf("\t -f --off\t\t\tsend an off signal\n");
    printf("\t -u --unit=unit\t\t\tcontrol a device with this unit code\n");
    printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void rev3Init(void) {

    protocol_register(&rev3_switch);
    protocol_set_id(rev3_switch, "rev3_switch");
    protocol_device_add(rev3_switch, "rev3_switch", "Rev Switches v3");
    rev3_switch->devtype = SWITCH;
    rev3_switch->hwtype = RF433;
		rev3_switch->minrawlen = RAW_LENGTH;
		rev3_switch->maxrawlen = RAW_LENGTH;
		rev3_switch->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
		rev3_switch->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

    options_add(&rev3_switch->options, 't', "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
    options_add(&rev3_switch->options, 'f', "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
    options_add(&rev3_switch->options, 'u', "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]|[1][0-5])$");
    options_add(&rev3_switch->options, 'i', "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(6[0123]|[12345][0-9]|[0-9]{1})$");

    options_add(&rev3_switch->options, 0, "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
    options_add(&rev3_switch->options, 0, "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

    rev3_switch->parseCode=&parseCode;
    rev3_switch->createCode=&createCode;
    rev3_switch->printHelp=&printHelp;
    rev3_switch->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "rev3_switch";
	module->version = "0.14";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	rev3Init();
}
#endif
