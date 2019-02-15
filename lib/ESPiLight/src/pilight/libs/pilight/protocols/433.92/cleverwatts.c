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
#include "cleverwatts.h"

#define PULSE_MULTIPLIER	4
#define MIN_PULSE_LENGTH	265
#define MAX_PULSE_LENGTH	274
#define AVG_PULSE_LENGTH	269
#define RAW_LENGTH				50

static int validate(void) {
	if(cleverwatts->rawlen == RAW_LENGTH) {
		if(cleverwatts->raw[cleverwatts->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   cleverwatts->raw[cleverwatts->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int id, int unit, int state, int all) {
	cleverwatts->message = json_mkobject();
	json_append_member(cleverwatts->message, "id", json_mknumber(id, 0));
	if(all == 0) {
		json_append_member(cleverwatts->message, "all", json_mknumber(1, 0));
	} else {
		json_append_member(cleverwatts->message, "unit", json_mknumber(unit, 0));
	}
	if(state == 0)
		json_append_member(cleverwatts->message, "state", json_mkstring("on"));
	else
		json_append_member(cleverwatts->message, "state", json_mkstring("off"));
}

static void parseCode(void) {
	int i = 0, x = 0, binary[RAW_LENGTH/2];
	int id = 0, state = 0, unit = 0, all = 0;

	if(cleverwatts->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "cleverwatts: parsecode - invalid parameter passed %d", cleverwatts->rawlen);
		return;
	}

	for(x=1;x<cleverwatts->rawlen-1;x+=2) {
		if(cleverwatts->raw[x] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 1;
		} else {
			binary[i++] = 0;
		}
	}

	id = binToDecRev(binary, 0, 19);
	state = binary[20];
	unit = binToDecRev(binary, 21, 22);
	all = binary[23];

	createMessage(id, unit, state, all);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		cleverwatts->raw[i]=(AVG_PULSE_LENGTH);
		cleverwatts->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=2) {
		cleverwatts->raw[i]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		cleverwatts->raw[i+1]=(AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createHigh(0,47);
}

static void createId(int id) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(id, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*2;
			createLow(39-(x+1), 39-x);
		}
	}
}

static void createAll(int all) {
	if(all == 0) {
		createLow(46, 47);
	}
}

static void createState(int state) {
	if(state == 1) {
		createLow(40, 41);
	}
}

static void createUnit(int unit) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*2;
			createLow(45-(x+1), 45-x);
		}
	}
}

static void createFooter(void) {
	cleverwatts->raw[48]=(AVG_PULSE_LENGTH);
	cleverwatts->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int id = -1;
	int unit = -1;
	int state = -1;
	int all = 0;
	double itmp = -1;

	if(json_find_number(code, "id", &itmp) == 0)
		id = (int)round(itmp);
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);
	if(json_find_number(code, "all", &itmp)	== 0)
		all = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=1;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=0;

	if(id == -1 || (unit == -1 && all == 0) || state == -1) {
		logprintf(LOG_ERR, "cleverwatts: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(id > 1048575 || id < 1) {
		logprintf(LOG_ERR, "cleverwatts: invalid id range");
		return EXIT_FAILURE;
	} else if((unit > 3 || unit < 0) && all == 0) {
		logprintf(LOG_ERR, "cleverwatts: invalid unit range");
		return EXIT_FAILURE;
	} else {
		if(unit == -1 && all == 1) {
			unit = 3;
		}
		createMessage(id, unit, state, all ^ 1);
		clearCode();
		createId(id);
		createState(state);
		createUnit(unit);
		createAll(all);
		createFooter();
		cleverwatts->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
	printf("\t -u --unit=unit\t\t\tcontrol a device with this unit code\n");
	printf("\t -i --id=id\t\t\tcontrol a device with this id\n");
	printf("\t -a --all\t\t\tsend command to all devices with this id\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void cleverwattsInit(void) {

	protocol_register(&cleverwatts);
	protocol_set_id(cleverwatts, "cleverwatts");
	protocol_device_add(cleverwatts, "cleverwatts", "Cleverwatts Switches");
	cleverwatts->devtype = SWITCH;
	cleverwatts->hwtype = RF433;
	cleverwatts->minrawlen = RAW_LENGTH;
	cleverwatts->maxrawlen = RAW_LENGTH;
	cleverwatts->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	cleverwatts->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&cleverwatts->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&cleverwatts->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&cleverwatts->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-3])$");
	options_add(&cleverwatts->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]{1,6}|10([0-3][0-9]{4}|4([0-7][0-9]{3}|8([0-4][0-9]{2}|5([0-6][0-9]|7[0-5])))))$");
	options_add(&cleverwatts->options, "a", "all", OPTION_OPT_VALUE, DEVICES_OPTIONAL, JSON_NUMBER, NULL, NULL);

	options_add(&cleverwatts->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&cleverwatts->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	cleverwatts->parseCode=&parseCode;
	cleverwatts->createCode=&createCode;
	cleverwatts->printHelp=&printHelp;
	cleverwatts->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "cleverwatts";
	module->version = "1.1";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	cleverwattsInit();
}
#endif
