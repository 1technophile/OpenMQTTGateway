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
#include "rc101.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	236
#define MAX_PULSE_LENGTH	246
#define AVG_PULSE_LENGTH	241
#define RAW_LENGTH				66

static int validate(void) {
	if(rc101->rawlen == RAW_LENGTH) {
		if(rc101->raw[rc101->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   rc101->raw[rc101->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int state, int unit, int all) {
	rc101->message = json_mkobject();
	json_append_member(rc101->message, "id", json_mknumber(id, 0));
	if(all == 1) {
		json_append_member(rc101->message, "all", json_mknumber(1, 0));
	} else {
		json_append_member(rc101->message, "unit", json_mknumber(unit, 0));
	}
	if(state == 1) {
		json_append_member(rc101->message, "state", json_mkstring("on"));
	} else {
		json_append_member(rc101->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int i = 0, x = 0, binary[RAW_LENGTH/2];

	if(rc101->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "rc101: parsecode - invalid parameter passed %d", rc101->rawlen);
		return;
	}

	for(i=0;i<rc101->rawlen; i+=2) {
		if(rc101->raw[i] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[x++] = 1;
		} else {
			binary[x++] = 0;
		}
	}

	int id = binToDec(binary, 0, 19);
	int state = binary[20];
	int unit = 7-binToDec(binary, 21, 23);
	int all = 0;
	if(unit == 7 && state == 1) {
		all = 1;
		state = 0;
	}
	if(unit == 6 && state == 0) {
		all = 1;
		state = 1;
	}
	createMessage(id, state, unit, all);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		rc101->raw[i]=AVG_PULSE_LENGTH;
		rc101->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		rc101->raw[i]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		rc101->raw[i+1]=AVG_PULSE_LENGTH;
	}
}

static void clearCode(void) {
	createLow(0, 63);
}

static void createId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(id, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*2;
			createHigh(x, x+1);
		}
	}
}

static void createUnit(int unit) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(7-unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*2;
			createHigh(42+x, 42+x+1);
		}
	}
}

static void createState(int state) {
	if(state == 1) {
		createHigh(40, 41);
	}
}

static void createFooter(void) {
	rc101->raw[64]=(AVG_PULSE_LENGTH);
	rc101->raw[65]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int state = -1;
	int unit = -1;
	int all = -1;
	double itmp = 0;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if(json_find_number(code, "all", &itmp) == 0)
		all = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(all == 1 && state == 1) {
		unit = 6;
		state = 0;
	}
	if(all == 1 && state == 0) {
		unit = 7;
		state = 1;
	}

	if(id == -1 || state == -1 || unit == -1) {
		logprintf(LOG_ERR, "rc101: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 1048575 || id < 0) {
		logprintf(LOG_ERR, "rc101: invalid id range");
		return EXIT_FAILURE;
	} else if(unit > 4 || unit < 0) {
		createMessage(id, state, unit, all);
		clearCode();
		createId(id);
		createState(state);
		if(unit > -1) {
			createUnit(unit);
		}
		createFooter();
		rc101->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -u --unit=unit\t\t\tcontrol a device with this unit\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -a --all\t\t\tsend command to all devices with this id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void rc101Init(void) {

	protocol_register(&rc101);
	protocol_set_id(rc101, "rc101");
	protocol_device_add(rc101, "rc101", "rc101 Switches");
	protocol_device_add(rc101, "rc102", "rc102 Switches");
	rc101->devtype = SWITCH;
	rc101->hwtype = RF433;
	rc101->minrawlen = RAW_LENGTH;
	rc101->maxrawlen = RAW_LENGTH;
	rc101->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	rc101->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&rc101->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-4])$");
	options_add(&rc101->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-4])$");
	options_add(&rc101->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&rc101->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&rc101->options, "a", "all", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);

	options_add(&rc101->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&rc101->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	rc101->parseCode=&parseCode;
	rc101->createCode=&createCode;
	rc101->printHelp=&printHelp;
	rc101->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "rc101";
	module->version = "1.1";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	rc101Init();
}
#endif
