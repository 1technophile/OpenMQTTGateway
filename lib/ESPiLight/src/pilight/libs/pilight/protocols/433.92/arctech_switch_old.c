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
#include <math.h>

#include "../../core/pilight.h"
#include "../../core/common.h"
#include "../../core/dso.h"
#include "../../core/log.h"
#include "../protocol.h"
#include "../../core/binary.h"
#include "../../core/gc.h"
#include "arctech_switch_old.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	310
#define MAX_PULSE_LENGTH	405
#define AVG_PULSE_LENGTH	335
#define RAW_LENGTH				50

static int validate(void) {
	if(arctech_switch_old->rawlen == RAW_LENGTH) {
		if(arctech_switch_old->raw[arctech_switch_old->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   arctech_switch_old->raw[arctech_switch_old->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state) {
	arctech_switch_old->message = json_mkobject();
	json_append_member(arctech_switch_old->message, "id", json_mknumber(id, 0));
	json_append_member(arctech_switch_old->message, "unit", json_mknumber(unit, 0));
	if(state == 1)
		json_append_member(arctech_switch_old->message, "state", json_mkstring("on"));
	else
		json_append_member(arctech_switch_old->message, "state", json_mkstring("off"));
}

static void parseCode(void) {
	int binary[RAW_LENGTH/4], x = 0, i = 0;
	int len = (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2));

	if(arctech_switch_old->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "arctech_switch_old: parsecode - invalid parameter passed %d", arctech_switch_old->rawlen);
		return;
	}

	for(x=0;x<arctech_switch_old->rawlen-3;x+=4) {
		// valid telegrams must consist of 0110 and 1001 blocks
		int low_high = 0;
		if(arctech_switch_old->raw[x] > len) {
			low_high |= 1;
		}
		if(arctech_switch_old->raw[x+1] > len) {
			low_high |= 2;
		}
		if(arctech_switch_old->raw[x+2] > len) {
			low_high |= 4;
		}
		if(arctech_switch_old->raw[x+3] > len) {
			low_high |= 8;
		}
		switch(low_high) {
			case 6:
				binary[i++] = 1;
			break;
			case 10:
				binary[i++] = 0;
			break;
			default:
				return; // invalid telegram
		}
	}

	int unit = binToDec(binary, 0, 3);
	int state = binary[11];
	int id = binToDec(binary, 4, 8);
	createMessage(id, unit, state);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		arctech_switch_old->raw[i]=(AVG_PULSE_LENGTH);
		arctech_switch_old->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		arctech_switch_old->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		arctech_switch_old->raw[i+3]=(AVG_PULSE_LENGTH);
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		arctech_switch_old->raw[i]=(AVG_PULSE_LENGTH);
		arctech_switch_old->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		arctech_switch_old->raw[i+2]=(AVG_PULSE_LENGTH);
		arctech_switch_old->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createHigh(0,35);
	createLow(36,47);
}

static void createUnit(int unit) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createLow(x, x+3);
		}
	}
}

static void createId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(id, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createLow(16+x, 16+x+3);
		}
	}
}

static void createState(int state) {
	if(state == 0) {
		createHigh(44,47);
	}
}

static void createFooter(void) {
	arctech_switch_old->raw[48]=(AVG_PULSE_LENGTH);
	arctech_switch_old->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int unit = -1;
	int state = -1;
	double itmp = -1;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(id == -1 || unit == -1 || state == -1) {
		logprintf(LOG_ERR, "arctech_switch_old: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 31 || id < 0) {
		logprintf(LOG_ERR, "arctech_switch_old: invalid id range");
		return EXIT_FAILURE;
	} else if(unit > 15 || unit < 0) {
		logprintf(LOG_ERR, "arctech_switch_old: invalid unit range");
		return EXIT_FAILURE;
	} else {
		createMessage(id, unit, state);
		clearCode();
		createUnit(unit);
		createId(id);
		createState(state);
		createFooter();
		arctech_switch_old->rawlen = RAW_LENGTH;
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
void arctechSwitchOldInit(void) {

	protocol_register(&arctech_switch_old);
	protocol_set_id(arctech_switch_old, "arctech_switch_old");
	protocol_device_add(arctech_switch_old, "kaku_switch_old", "Old KlikAanKlikUit Switches");
	protocol_device_add(arctech_switch_old, "cogex", "Cogex Switches");
	protocol_device_add(arctech_switch_old, "intertechno_old", "Old Intertechno Switches");
	protocol_device_add(arctech_switch_old, "byebyestandby", "Bye Bye Standby Switches");
	protocol_device_add(arctech_switch_old, "duwi", "Düwi Terminal Switches");
	arctech_switch_old->devtype = SWITCH;
	arctech_switch_old->hwtype = RF433;
	arctech_switch_old->minrawlen = RAW_LENGTH;
	arctech_switch_old->maxrawlen = RAW_LENGTH;
	arctech_switch_old->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	arctech_switch_old->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&arctech_switch_old->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&arctech_switch_old->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&arctech_switch_old->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1}|[1][0-5])$");
	options_add(&arctech_switch_old->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");

	options_add(&arctech_switch_old->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&arctech_switch_old->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	arctech_switch_old->parseCode=&parseCode;
	arctech_switch_old->createCode=&createCode;
	arctech_switch_old->printHelp=&printHelp;
	arctech_switch_old->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "arctech_switch_old";
	module->version = "2.5";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	arctechSwitchOldInit();
}
#endif
