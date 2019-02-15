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
#include "clarus.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	160
#define MAX_PULSE_LENGTH	200
#define AVG_PULSE_LENGTH	180
#define RAW_LENGTH				50

static int validate(void) {
	if(clarus_switch->rawlen == RAW_LENGTH) {
		if(clarus_switch->raw[clarus_switch->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   clarus_switch->raw[clarus_switch->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(const char *id, int unit, int state) {
	clarus_switch->message = json_mkobject();
	json_append_member(clarus_switch->message, "id", json_mkstring(id));
	json_append_member(clarus_switch->message, "unit", json_mknumber(unit, 0));
	if(state == 2)
		json_append_member(clarus_switch->message, "state", json_mkstring("on"));
	else
		json_append_member(clarus_switch->message, "state", json_mkstring("off"));
}

static void parseCode(void) {
	int x = 0, z = 65, binary[RAW_LENGTH/4];
	char id[3];

	if(clarus_switch->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "clarus_switch: parsecode - invalid parameter passed %d", clarus_switch->rawlen);
		return;
	}

	/* Convert the one's and zero's into binary */
	for(x=0;x<clarus_switch->rawlen-2;x+=4) {
		if(clarus_switch->raw[x+3] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[x/4]=1;
		} else if(clarus_switch->raw[x+0] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[x/4]=2;
		} else {
			binary[x/4]=0;
		}
	}

	for(x=9;x>=5;--x) {
		if(binary[x] == 2) {
			break;
		}
		z++;
	}

	int unit = binToDecRev(binary, 0, 5);
	int state = binary[11];
	int y = binToDecRev(binary, 6, 9);
	sprintf(&id[0], "%c%d", z, y);

	createMessage(id, unit, state);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		clarus_switch->raw[i]=(AVG_PULSE_LENGTH);
		clarus_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		clarus_switch->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		clarus_switch->raw[i+3]=(AVG_PULSE_LENGTH);
	}
}

static void createMed(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		clarus_switch->raw[i]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		clarus_switch->raw[i+1]=(AVG_PULSE_LENGTH);
		clarus_switch->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		clarus_switch->raw[i+3]=(AVG_PULSE_LENGTH);
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		clarus_switch->raw[i]=(AVG_PULSE_LENGTH);
		clarus_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		clarus_switch->raw[i+2]=(AVG_PULSE_LENGTH);
		clarus_switch->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}

static void clearCode(void) {
	createLow(0,47);
}

static void createUnit(int unit) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unit, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createHigh(23-(x+3), 23-x);
		}
	}
}

static void createId(const char *id) {
	int l = ((int)(id[0]))-65;
	int y = atoi(&id[1]);
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(y, binary);
	for(i=0;i<=length;i++) {
		x=i*4;
		if(binary[i]==1) {
			createHigh(39-(x+3), 39-x);
		}
	}
	x=(l*4);
	createMed(39-(x+3), 39-x);
}

static void createState(int state) {
	if(state == 0) {
		createMed(40,43);
		createHigh(44,47);
	} else {
		createHigh(40,43);
		createMed(44,47);
	}
}

static void createFooter(void) {
	clarus_switch->raw[48]=(AVG_PULSE_LENGTH);
	clarus_switch->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	const char *id = NULL;
	int unit = -1;
	int state = -1;
	double itmp;
	char *stmp;

	if(json_find_string(code, "id", &stmp) == 0)
		id = stmp;
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;
	if(json_find_number(code, "unit", &itmp) == 0)
		unit = (int)round(itmp);

	if(id == NULL || unit == -1 || state == -1) {
		logprintf(LOG_ERR, "clarus_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if((int)(id[0]) < 65 || (int)(id[0]) > 70) {
		logprintf(LOG_ERR, "clarus_switch: invalid id range");
		return EXIT_FAILURE;
	} else if(atoi(&id[1]) < 0 || atoi(&id[1]) > 31) {
		logprintf(LOG_ERR, "clarus_switch: invalid id range");
		return EXIT_FAILURE;
	} else if(unit > 63 || unit < 0) {
		logprintf(LOG_ERR, "clarus_switch: invalid unit range");
		return EXIT_FAILURE;
	} else {
		createMessage(id, unit, ((state == 2 || state == 1) ? 2 : 0));
		clearCode();
		createUnit(unit);
		createId(id);
		createState(state);
		createFooter();
		clarus_switch->rawlen = RAW_LENGTH;
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
void clarusSwitchInit(void) {

	protocol_register(&clarus_switch);
	protocol_set_id(clarus_switch, "clarus_switch");
	protocol_device_add(clarus_switch, "clarus_switch", "Clarus Switches");
	clarus_switch->devtype = SWITCH;
	clarus_switch->hwtype = RF433;
	clarus_switch->minrawlen = RAW_LENGTH;
	clarus_switch->maxrawlen = RAW_LENGTH;
	clarus_switch->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	clarus_switch->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&clarus_switch->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&clarus_switch->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&clarus_switch->options, "u", "unit", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^([0-9]|[1-5][0-9]|6[0-3])$");
	options_add(&clarus_switch->options, "i", "id", OPTION_HAS_VALUE, DEVICES_ID, JSON_STRING, NULL, "^[ABCDEF](3[012]?|[012][0-9]|[0-9]{1})$");

	options_add(&clarus_switch->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&clarus_switch->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	clarus_switch->parseCode=&parseCode;
	clarus_switch->createCode=&createCode;
	clarus_switch->printHelp=&printHelp;
	clarus_switch->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "clarus_switch";
	module->version = "2.4";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	clarusSwitchInit();
}
#endif
