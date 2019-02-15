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
#include "elro_400_switch.h"

#define PULSE_MULTIPLIER	3
#define MIN_PULSE_LENGTH	291
#define MAX_PULSE_LENGTH	301
#define AVG_PULSE_LENGTH	296
#define RAW_LENGTH				50

static int validate(void) {
	if(elro_400_switch->rawlen == RAW_LENGTH) {
		if(elro_400_switch->raw[elro_400_switch->rawlen-1] >= (MIN_PULSE_LENGTH*PULSE_DIV) &&
		   elro_400_switch->raw[elro_400_switch->rawlen-1] <= (MAX_PULSE_LENGTH*PULSE_DIV)) {
			return 0;
		}
	}

	return -1;
}

static void createMessage(int systemcode, int unitcode, int state) {
	elro_400_switch->message = json_mkobject();
	json_append_member(elro_400_switch->message, "systemcode", json_mknumber(systemcode, 0));
	json_append_member(elro_400_switch->message, "unitcode", json_mknumber(unitcode, 0));
	if(state == 1) {
		json_append_member(elro_400_switch->message, "state", json_mkstring("on"));
	} else {
		json_append_member(elro_400_switch->message, "state", json_mkstring("off"));
	}
}

static void parseCode(void) {
	int x = 0, i = 0, binary[RAW_LENGTH/4];

	if(elro_400_switch->rawlen>RAW_LENGTH) {
		logprintf(LOG_ERR, "elro_400_switch: parsecode - invalid parameter passed %d", elro_400_switch->rawlen);
		return;
	}

	for(x=0;x<elro_400_switch->rawlen-2;x+=4) {
		if(elro_400_switch->raw[x+3] > (int)((double)AVG_PULSE_LENGTH*((double)PULSE_MULTIPLIER/2))) {
			binary[i++] = 0;
		} else {
			binary[i++] = 1;
		}
	}

	int systemcode = binToDecRev(binary, 0, 4);
	int unitcode = binToDecRev(binary, 5, 9);
	int state = binary[11];
	createMessage(systemcode, unitcode, state);
}

static void createLow(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		elro_400_switch->raw[i]=(AVG_PULSE_LENGTH);
		elro_400_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		elro_400_switch->raw[i+2]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		elro_400_switch->raw[i+3]=(AVG_PULSE_LENGTH);
	}
}

static void createHigh(int s, int e) {
	int i;

	for(i=s;i<=e;i+=4) {
		elro_400_switch->raw[i]=(AVG_PULSE_LENGTH);
		elro_400_switch->raw[i+1]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
		elro_400_switch->raw[i+2]=(AVG_PULSE_LENGTH);
		elro_400_switch->raw[i+3]=(PULSE_MULTIPLIER*AVG_PULSE_LENGTH);
	}
}
static void clearCode(void) {
	createHigh(0,47);
}

static void createSystemCode(int systemcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(systemcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createLow(19-(x+3), 19-x);
		}
	}
}

static void createUnitCode(int unitcode) {
	int binary[255];
	int length = 0;
	int i=0, x=0;

	length = decToBinRev(unitcode, binary);
	for(i=0;i<=length;i++) {
		if(binary[i]==1) {
			x=i*4;
			createLow(39-(x+3), 39-x);
		}
	}
}

static void createState(int state) {
	if(state == 1) {
		createLow(44, 47);
		createLow(40, 43);
	} else {
		createLow(40, 43);
	}
}

static void createFooter(void) {
	elro_400_switch->raw[48]=(AVG_PULSE_LENGTH);
	elro_400_switch->raw[49]=(PULSE_DIV*AVG_PULSE_LENGTH);
}

static int createCode(struct JsonNode *code) {
	int systemcode = -1;
	int unitcode = -1;
	int state = -1;
	double itmp = 0;

	if(json_find_number(code, "systemcode", &itmp) == 0)
		systemcode = (int)round(itmp);
	if(json_find_number(code, "unitcode", &itmp) == 0)
		unitcode = (int)round(itmp);
	if(json_find_number(code, "off", &itmp) == 0)
		state=0;
	else if(json_find_number(code, "on", &itmp) == 0)
		state=1;

	if(systemcode == -1 || unitcode == -1 || state == -1) {
		logprintf(LOG_ERR, "elro_400_switch: insufficient number of arguments");
		return EXIT_FAILURE;
	} else if(systemcode > 31 || systemcode < 0) {
		logprintf(LOG_ERR, "elro_400_switch: invalid systemcode range");
		return EXIT_FAILURE;
	} else if(unitcode > 31 || unitcode < 0) {
		logprintf(LOG_ERR, "elro_400_switch: invalid unitcode range");
		return EXIT_FAILURE;
	} else {
		createMessage(systemcode, unitcode, state);
		clearCode();
		createSystemCode(systemcode);
		createUnitCode(unitcode);
		createState(state);
		createFooter();
		elro_400_switch->rawlen = RAW_LENGTH;
	}
	return EXIT_SUCCESS;
}

static void printHelp(void) {
	printf("\t -s --systemcode=systemcode\tcontrol a device with this systemcode\n");
	printf("\t -u --unitcode=unitcode\t\tcontrol a device with this unitcode\n");
	printf("\t -t --on\t\t\tsend an on signal\n");
	printf("\t -f --off\t\t\tsend an off signal\n");
}

#if !defined(MODULE) && !defined(_WIN32)
__attribute__((weak))
#endif
void elro400SwitchInit(void) {

	protocol_register(&elro_400_switch);
	protocol_set_id(elro_400_switch, "elro_400_switch");
	protocol_device_add(elro_400_switch, "elro_400_switch", "Elro 400 Series Switches");
	elro_400_switch->devtype = SWITCH;
	elro_400_switch->hwtype = RF433;
	elro_400_switch->minrawlen = RAW_LENGTH;
	elro_400_switch->maxrawlen = RAW_LENGTH;
	elro_400_switch->maxgaplen = MAX_PULSE_LENGTH*PULSE_DIV;
	elro_400_switch->mingaplen = MIN_PULSE_LENGTH*PULSE_DIV;

	options_add(&elro_400_switch->options, "s", "systemcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&elro_400_switch->options, "u", "unitcode", OPTION_HAS_VALUE, DEVICES_ID, JSON_NUMBER, NULL, "^(3[012]?|[012][0-9]|[0-9]{1})$");
	options_add(&elro_400_switch->options, "t", "on", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);
	options_add(&elro_400_switch->options, "f", "off", OPTION_NO_VALUE, DEVICES_STATE, JSON_STRING, NULL, NULL);

	options_add(&elro_400_switch->options, "0", "readonly", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");
	options_add(&elro_400_switch->options, "0", "confirm", OPTION_HAS_VALUE, GUI_SETTING, JSON_NUMBER, (void *)0, "^[10]{1}$");

	elro_400_switch->parseCode=&parseCode;
	elro_400_switch->createCode=&createCode;
	elro_400_switch->printHelp=&printHelp;
	elro_400_switch->validate=&validate;
}

#if defined(MODULE) && !defined(_WIN32)
void compatibility(struct module_t *module) {
	module->name = "elro_400_switch";
	module->version = "2.4";
	module->reqversion = "6.0";
	module->reqcommit = "84";
}

void init(void) {
	elro400SwitchInit();
}
#endif
